/*-------------------------------------------------------------------------
 *
 * aqpstate.h
 *	  definitions for aqp state nodes
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/nodes/aqpstate.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef AQPSTATE_H
#define AQPSTATE_H

#include "nodes/execnodes.h"
#include "nodes/pathnodes.h"

typedef struct AQPState
{
	PlannerInfo *root;
	bool		need_break;
	int			version;
	/* stage version */
	EState	   *estate;
}			AQPState;

extern AQPState * CreateAQPState(void);
extern void SetPlannerHook(bool open);
extern void SetJoinSearchHook(bool open);
extern void SetExecutorStartHook(bool open);
extern void SetAQPHook(bool open);
extern bool JudgeQuery(Query *query, AQPState * aqp_state);
extern bool AQPNeedBreak(List *querytree_list, AQPState * aqp_state);

#endif							/* AQPSTATE_H */