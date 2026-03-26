/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/
#define COMPILETIME   BUILDTIME
#define LASTCOMMITMSG COMMITMSG
#define LASTCOMMITID  COMMITID
#ifndef COMMITID_SHORT
#define COMMITID_SHORT 0
#endif
#define LASTCOMMITID_SHORT COMMITID_SHORT
#define LASTCOMMITTIME     LASTCOMMIT_DATE
#define CURRENT_BRANCH     BRANCH
extern char const diffinfo[];
