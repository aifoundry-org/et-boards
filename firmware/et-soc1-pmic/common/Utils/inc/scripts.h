/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file scripts.h
    \brief Scripts related.
*/

#ifndef SCRIPTS_H
#define SCRIPTS_H


typedef struct
{
    char const * name;
    char const * const * pScript;
} scriptStruct_t;

extern scriptStruct_t scriptList [];
extern int const MAX_SCRIPTLIST;

#endif /* SCRIPTS_H */
