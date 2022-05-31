/*-------------------------------------------------------------------------
 *
 * nodeMaterialAnalyze.h
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/executor/nodeMaterialAnalyze.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEMATERIALANALYZE_H
#define NODEMATERIALANALYZE_H

#include "nodes/execnodes.h"

extern MaterialAnalyzeState * ExecInitMaterialAnalyze(MaterialAnalyze * node, EState *estate, int eflags);
extern void ExecEndMaterialAnalyze(MaterialAnalyzeState * node);
extern void ExecMaterialAnalyzeMarkPos(MaterialAnalyzeState * node);
extern void ExecMaterialAnalyzeRestrPos(MaterialAnalyzeState * node);
extern void ExecReScanMaterialAnalyze(MaterialAnalyzeState * node);

#endif							/* NODEMATERIALANALYZE_H */
