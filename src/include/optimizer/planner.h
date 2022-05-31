/*-------------------------------------------------------------------------
 *
 * planner.h
 *	  prototypes for planner.c.
 *
 * Note that the primary entry points for planner.c are declared in
 * optimizer/optimizer.h, because they're intended to be called from
 * non-planner code.  Declarations here are meant for use by other
 * planner modules.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/optimizer/planner.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PLANNER_H
#define PLANNER_H

#include "nodes/pathnodes.h"
#include "nodes/plannodes.h"
/*
 * AQP
 */
#include "nodes/aqpstate.h"


/* Hook for plugins to get control in planner() */
/*
 * AQP
 */
typedef PlannedStmt *(*planner_hook_type) (Query *parse,
										   const char *query_string,
										   int cursorOptions,
										   ParamListInfo boundParams,
										   AQPState * sstate);
extern PGDLLIMPORT planner_hook_type planner_hook;

/* Hook for plugins to get control when grouping_planner() plans upper rels */
typedef void (*create_upper_paths_hook_type) (PlannerInfo *root,
											  UpperRelationKind stage,
											  RelOptInfo *input_rel,
											  RelOptInfo *output_rel,
											  void *extra);
extern PGDLLIMPORT create_upper_paths_hook_type create_upper_paths_hook;

/*
 * AQP
 */
extern PlannedStmt *standard_planner(Query *parse, const char *query_string,
									 int cursorOptions,
									 ParamListInfo boundParams, AQPState * aqp_state);

extern PlannerInfo *subquery_planner(PlannerGlobal *glob, Query *parse,
									 PlannerInfo *parent_root,
									 bool hasRecursion, double tuple_fraction);

extern RowMarkType select_rowmark_type(RangeTblEntry *rte,
									   LockClauseStrength strength);

extern bool limit_needed(Query *parse);

extern void mark_partial_aggref(Aggref *agg, AggSplit aggsplit);

extern Path *get_cheapest_fractional_path(RelOptInfo *rel,
										  double tuple_fraction);

extern Expr *preprocess_phv_expression(PlannerInfo *root, Expr *expr);

/*
 * AQP
 */
extern PlannedStmt *AQP_planner(Query *parse, const char *query_string, int cursorOptions,
								ParamListInfo boundParams, AQPState * aqp_state);

#endif							/* PLANNER_H */
