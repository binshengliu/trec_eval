/* 
   Copyright (c) 2008 - Chris Buckley. 

   Permission is granted for use and modification of this file for
   research, non-commercial purposes. 
*/

#include "common.h"
#include "sysfunc.h"
#include "trec_eval.h"
#include "functions.h"
#include "trec_format.h"
double log2(double x);

static int 
te_calc_ndcg_rel (const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static PARAMS default_ndcg_gains = { NULL, 0, NULL};

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_ndcg_rel =
    {"ndcg_rel",
     "    Normalized Discounted Cumulative Gain averaged over rel docs\n\
    Experimental measure\n\
    Compute a traditional nDCG measure according to Jarvelin and\n\
    Kekalainen (ACM ToIS v. 20, pp. 422-446, 2002), averaged at rel docs.\n\
    Idea behind ndcg_rel, is that the expected value of ndcg is a smoothly\n\
    decreasing function, with discontinuities upward at each transistion\n\
    between positive gain levels in the ideal ndcg.  Once the gain level \n\
    becomes 0, the expected value of ndcg then increases until all rel docs are\n\
    retrieved. Thus averaging ndcg is problematic, because these transistions\n\
    occur at different points for each topic.  Since it is not unusual for\n\
    ndcg to start off near 1.0, decrease to 0.25, and then increase to 0.75\n\
    at various cutoffs, the points at which ndcg is measured are important.\n\
    This version averages ndcg over each relevant doc, where relevant is\n\
    defined as expected gain > 0.  If a rel doc is not retrieved, then\n\
    ndcg for the doc is the dcg at the end of the retrieval / ideal dcg. \n\
    \n\
    Gain values are set to the appropriate relevance level by default.  \n\
    The default gain can be overridden on the command line by having \n\
    comma separated parameters 'rel_level=gain'.\n\
    Eg, 'trec_eval -m ndcg_rel.1=3.5,2=9.0,4=7.0 ...'\n\
    will give gains 3.5, 9.0, 3.0, 7.0 for relevance levels 1,2,3,4\n\
    respectively (level 3 remains at the default).\n\
    Gains are allowed to be 0 or negative, and relevance level 0\n\
    can be given a gain.\n",
    te_init_meas_s_double_p_pair,
    te_calc_ndcg_rel,
    te_acc_meas_s,
    te_calc_avg_meas_s,
    te_print_single_meas_s_double,
    te_print_final_meas_s_double_p,
    &default_ndcg_gains, -1
};

/* Keep track of valid rel_levels and associated gains */
/* Initialized in setup_gains */
typedef struct {
    long rel_level;
    long num_at_level;
    double gain;
} REL_GAIN;

typedef struct {
    REL_GAIN *rel_gains;
    long num_gains;
    long total_num_at_levels;
} GAINS;

static int setup_gains (const TREC_MEAS *tm, const RES_RELS *res_rels,
			GAINS *gains);
static double get_gain (const long rel_level, const GAINS *gains);
static int comp_rel_gain ();

static int 
te_calc_ndcg_rel (const EPI *epi, const REL_INFO *rel_info,
	       const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval)
{
    RES_RELS res_rels;
    double results_gain, results_dcg;
    double ideal_gain, ideal_dcg;
    double sum = 0.0;
    long num_rel_ret = 0;
    long num_rel = 0;
    long cur_level, num_at_level;
    long i;
    GAINS gains;
   
    if (UNDEF == te_form_res_rels (epi, rel_info, results, &res_rels))
	return (UNDEF);

    if (UNDEF == setup_gains (tm, &res_rels, &gains))
	return (UNDEF);

    results_dcg = 0.0;
    ideal_dcg = 0.0;
    cur_level = gains.num_gains - 1;
    ideal_gain = (cur_level >= 0) ? gains.rel_gains[cur_level].gain : 0.0;
    num_at_level = 0;
    
    for (i = 0; i < res_rels.num_ret && ideal_gain > 0.0; i++) {
	/* Calculate change in results dcg */
	results_gain = get_gain (res_rels.results_rel_list[i], &gains);
	if (results_gain != 0)
	    /* Note: i+2 since doc i has rank i+1 */
	    results_dcg += results_gain / log2((double) (i+2));
	/* Calculate change in ideal dcg */
	num_at_level++;
	while (cur_level >= 0 &&
	       num_at_level > gains.rel_gains[cur_level].num_at_level) {
	    num_at_level = 1;
	    cur_level--;
	    ideal_gain = (cur_level >= 0) ? gains.rel_gains[cur_level].gain:0.0;
	}
	if (ideal_gain > 0.0) {
	    num_rel++;
	    ideal_dcg += ideal_gain / log2((double)(i + 2));
	}
	/* Average will include this point if rel */
	if (results_gain > 0) {
	    sum += results_dcg / ideal_dcg;
	    num_rel_ret++;
	}
	if (epi->debug_level > 0) 
	    printf("ndcg_rel: %ld %ld %3.1f %6.4f %3.1f %6.4f %6.4f\n",
		   i, cur_level, results_gain, results_dcg,
		   ideal_gain, ideal_dcg, sum);
    }
    if (i < res_rels.num_ret) {
	while (i < res_rels.num_ret) {
	    /* Calculate change in results dcg */
	    results_gain = get_gain (res_rels.results_rel_list[i], &gains);
	    if (results_gain != 0)
		results_dcg += results_gain / log2((double) (i+2));
	    /* Average will include this point if rel */
	    if (results_gain > 0) {
		sum += results_dcg / ideal_dcg;
		num_rel_ret++;
	    }
	    if (epi->debug_level > 0) 
		printf("ndcg_rel: %ld %ld %3.1f %6.4f %3.1f %6.4f %6.4f\n",
		       i, cur_level, results_gain, results_dcg,
		       0.0, ideal_dcg, sum);
	    i++;
	}
    }
    while (ideal_gain > 0.0) {
	/* Calculate change in ideal dcg */
	num_at_level++;
	while (cur_level >= 0 &&
	       num_at_level > gains.rel_gains[cur_level].num_at_level) {
	    num_at_level = 1;
	    cur_level--;
	    ideal_gain = (cur_level >= 0) ? gains.rel_gains[cur_level].gain:0.0;
	}
	if (ideal_gain > 0.0) {
	    num_rel++;
	    ideal_dcg += ideal_gain / log2((double)(i + 2));
	}
	if (epi->debug_level > 0) 
	    printf("ndcg_rel: %ld %ld %3.1f %6.4f %3.1f %6.4f\n",
		   i, cur_level, 0.0, results_dcg,
		   ideal_gain, ideal_dcg);
	i++;
    }

    sum += ((double) (num_rel - num_rel_ret)) * results_dcg / ideal_dcg;
    if (epi->debug_level > 0) 
	printf("ndcg_rel: %ld %ld %6.4f %6.4f %6.4f\n",
	       i, cur_level, results_dcg, ideal_dcg, sum);
    if (sum > 0.0)
	eval->values[tm->eval_index].value = sum / num_rel;

    Free (gains.rel_gains);
    return (1);
}

static int
setup_gains (const TREC_MEAS *tm, const RES_RELS *res_rels, GAINS *gains)
{
    DOUBLE_PARAM_PAIR *pairs = NULL;
    long num_pairs = 0;
    long i,j;
    long num_gains;

    if (tm->meas_params) {
	pairs = (DOUBLE_PARAM_PAIR *) tm->meas_params->param_values;
	num_pairs = tm->meas_params->num_params;
    }

    if (NULL == (gains->rel_gains = Malloc(res_rels->num_rel_levels + num_pairs,
					   REL_GAIN)))
	return (UNDEF);
    num_gains = 0;
    for (i = 0; i < num_pairs; i++) {
	gains->rel_gains[num_gains].rel_level = atol (pairs[i].name);
	gains->rel_gains[num_gains].gain = (double) pairs[i].value;
	gains->rel_gains[num_gains].num_at_level = 0;
	num_gains++;
    }

    for (i = 0; i < res_rels->num_rel_levels; i++) {
	for (j = 0; j < num_gains && gains->rel_gains[j].rel_level != i; j++)
	    ;
	if (j < num_gains)
	    /* Was included in list of parameters. Update occurrence info */
	    gains->rel_gains[j].num_at_level = res_rels->rel_levels[i];
	else {
	    /* Not included in list of parameters. New gain level */
	    gains->rel_gains[num_gains].rel_level = i;
	    gains->rel_gains[num_gains].gain = (double) i;
	    gains->rel_gains[num_gains].num_at_level = res_rels->rel_levels[i];
	    num_gains++;
	}
    }

    /* Sort gains by increasing gain value */
    qsort ((char *) gains->rel_gains,
           (int) num_gains,
           sizeof (REL_GAIN),
           comp_rel_gain);

    gains->total_num_at_levels = 0;
    for (i = 0; i < num_gains; i++)
	gains->total_num_at_levels += gains->rel_gains[i].num_at_level;	

    gains->num_gains = num_gains;
    return (1);
}

static int comp_rel_gain (REL_GAIN *ptr1, REL_GAIN *ptr2)
{
    return (ptr1->gain - ptr2->gain);
}

static double
get_gain (const long rel_level, const GAINS *gains)
{
    long i;
    for (i = 0; i < gains->num_gains; i++)
	if (rel_level == gains->rel_gains[i].rel_level)
	    return (gains->rel_gains[i].gain);
    return (0.0);   /* Print Error ?? */
}

