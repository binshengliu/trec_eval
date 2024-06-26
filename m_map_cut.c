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
te_calc_map_cut (const EPI *epi, const REL_INFO *rel_info,
		 const RESULTS *results, const TREC_MEAS *tm, TREC_EVAL *eval);
static long long_cutoff_array[] = {5, 10, 15, 20, 30, 100, 200, 500, 1000};
static PARAMS default_map_cutoffs = {
    NULL, sizeof (long_cutoff_array) / sizeof (long_cutoff_array[0]),
    &long_cutoff_array[0]};

/* See trec_eval.h for definition of TREC_MEAS */
TREC_MEAS te_meas_map_cut =
    {"map_cut",
     "    Mean Average Precision at cutoffs\n\
    Map measured at various doc level cutoffs in the ranking.\n\
    If the cutoff is larger than the number of docs retrieved, then\n\
    it is assumed nonrelevant docs fill in the rest.\n\
    Map itself is precision measured after each relevant doc is retrieved,\n\
    averaged over all relevant docs for the topic.\n\
    Cutoffs must be positive without duplicates\n\
    Default param: -m map_cut.5,10,15,20,30,100,200,500,1000\n",
    te_init_meas_a_double_cut_long,
    te_calc_map_cut,
    te_acc_meas_a_cut,
    te_calc_avg_meas_a_cut,
    te_print_single_meas_a_cut,
    te_print_final_meas_a_cut,
    (void *) &default_map_cutoffs, -1
};

static int 
te_calc_map_cut (const EPI *epi, const REL_INFO *rel_info,
		const RESULTS *results,	const TREC_MEAS *tm, TREC_EVAL *eval)
{
    long *cutoffs = (long *) tm->meas_params->param_values;
    long cutoff_index = 0;
    long i;
    RES_RELS res_rels;
    long rel_so_far = 0;
    double sum = 0.0;

    if (UNDEF == te_form_res_rels (epi, rel_info, results, &res_rels))
	return (UNDEF);

    if (res_rels.num_rel == 0)
	return (0);

    for (i = 0; i < res_rels.num_ret; i++) {
	if (i == cutoffs[cutoff_index]) {
	    /* Calculate previous cutoff threshold.
	       Note all guaranteed to be positive by init_meas */
	    eval->values[tm->eval_index + cutoff_index].value =
		sum / (double) res_rels.num_rel;
	    if (++cutoff_index == tm->meas_params->num_params)
		break;
	}
	if (res_rels.results_rel_list[i] >= epi->relevance_level) {
	    rel_so_far++;
	    sum += (double) rel_so_far / (double) (i+1);
	}
    }
    /* calculate values for those cutoffs not achieved */
    while (cutoff_index < tm->meas_params->num_params) {
	eval->values[tm->eval_index+cutoff_index].value =
	    sum / (double) res_rels.num_rel;
	cutoff_index++;
    }
    return (1);
}
