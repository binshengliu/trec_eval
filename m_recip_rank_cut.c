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

static int 
te_calc_recip_rank_cut (const EPI *epi, const REL_INFO *rel_info,
		    const RESULTS *results, const TREC_MEAS *tm,
		    TREC_EVAL *eval);
static long long_cutoff_array[] = {5, 10, 15, 20, 30, 100, 200, 500, 1000};
static PARAMS default_recip_rank_cutoffs = {
    NULL, sizeof (long_cutoff_array) / sizeof (long_cutoff_array[0]),
    &long_cutoff_array[0]};

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_recip_rank_cut =
    {"recip_rank_cut",
     "    Reciprocal Rank at cutoffs\n\
    Measure is most useful for tasks in which there is only one relevant\n\
    doc, or the user only wants one relevant doc.\n\
    Cutoffs must be positive without duplicates\n\
    Default param: -m recip_rank_cut.5,10,15,20,30,100,200,500,1000\n",
     te_init_meas_a_float_cut_long,
     te_calc_recip_rank_cut,
     te_acc_meas_a_cut,
     te_calc_avg_meas_a_cut,
     te_print_single_meas_a_cut,
     te_print_final_meas_a_cut,
     (void *)&default_recip_rank_cutoffs, -1};

static int
te_calc_recip_rank_cut (const EPI *epi, const REL_INFO *rel_info,
                        const RESULTS *results, const TREC_MEAS *tm,
                        TREC_EVAL *eval)
{
    long *cutoffs = (long *) tm->meas_params->param_values;
    long cutoff_index = 0;
    RES_RELS res_rels;
    long i;
    double value = 0.0;

    if (UNDEF == te_form_res_rels (epi, rel_info, results, &res_rels))
	return (UNDEF);

    if (res_rels.num_rel == 0)
	return (0);

    for (i = 0; i < res_rels.num_ret; i++) {
	if (i == cutoffs[cutoff_index]) {
	    /* Calculate previous cutoff threshold.
	       Note all guaranteed to be positive by init_meas */
            eval->values[tm->eval_index + cutoff_index].value = value;
            if (++cutoff_index == tm->meas_params->num_params)
		break;
	}

	if (value == 0.0 && res_rels.results_rel_list[i] >= epi->relevance_level)
            value = (double) 1.0 / (double) (i+1);
    }
    /* calculate values for those cutoffs not achieved */
    while (cutoff_index < tm->meas_params->num_params) {
        eval->values[tm->eval_index+cutoff_index].value = value;
	cutoff_index++;
    }

    return (1);
}
