/*-------------------------------------------------------------------------
 *
 * aqpstate.c
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/nodes/aqpstate.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "commands/explain.h"
#include "nodes/aqpstate.h"
#include "optimizer/planner.h"
#include "optimizer/paths.h"
#include "executor/executor.h"
#include "commands/defrem.h"

AQPState *
CreateAQPState(void)
{
    AQPState *aqp_state = makeNode(AQPState);
    aqp_state->root = NULL;
    aqp_state->need_break = false;
    aqp_state->estate = NULL;
    aqp_state->version = 1;
    return aqp_state;
}

bool
JudgeQuery(Query *query, AQPState *aqp_state)
{
    /* Can't open when 'select pg_backend_pid()' */
    if (query->utilityStmt != NULL ) {
        /* Open when explain */
        if (query->utilityStmt->type == T_ExplainStmt) {
            ExplainStmt *stmt = (ExplainStmt *)query->utilityStmt;
            ListCell   *lc;
            bool analyze;
            /* Parse options list. */
            foreach(lc, stmt->options)
            {
                DefElem    *opt = (DefElem *) lfirst(lc);

                if (strcmp(opt->defname, "analyze") == 0)
                    analyze = defGetBoolean(opt);
            }
            if (analyze) {
                ExplainOneQuery_hook = AQP_ExplainOnePlan;
            }
        }
        return true;
    }
    else if (query->jointree->fromlist == NULL || query->commandType != CMD_SELECT) {
        return true;
    }
    else {
        return false;
    }
}

bool
AQPNeedBreak(List *querytree_list, AQPState *aqp_state)
{
    /* TODO: (zackery) support multiple querytree */
    if(list_length(querytree_list) != 1) {
        return true;
    }
    else {
        Query* query = linitial_node(Query, querytree_list);
        return JudgeQuery(query, aqp_state);
    }
}

void
SetPlannerHook(bool open) {
    if (open) {
        planner_hook = AQP_planner;
    }
    else {
        planner_hook = NULL;
    }
}

void
SetJoinSearchHook(bool open) {
    if (open) {
        join_search_hook = AQP_standard_join_search;
    }
    else {
        join_search_hook = NULL;
    }
}

void
SetExecutorStartHook(bool open) {
    if (open) {
        ExecutorStart_hook = AQP_standard_ExecutorStart;
    }
    else {
        ExecutorStart_hook = NULL;
    }
}

/* TODO: (Zackery) Maybe can simplify */
void
SetAQPHook(bool open) {
    if (open) {
        join_search_hook = AQP_standard_join_search;
        ExecutorStart_hook = AQP_standard_ExecutorStart;
    }
    else {
        ExplainOneQuery_hook = NULL;
    }
}