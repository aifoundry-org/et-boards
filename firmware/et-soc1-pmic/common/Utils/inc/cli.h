/***********************************************************************
*
* Copyright (c) 2025 Ainekko, Co.
* SPDX-License-Identifier: Apache-2.0
*
************************************************************************/

/***********************************************************************/
/*! \file cli.h
    \brief Command line interface
    simple interative command line parser
    this is a collection of simple calls which assist in parsing a command line
    for test software.
*/
/***********************************************************************/

#ifndef __CLI_H__
#define __CLI_H__

#include "i2cm2.h"
#include "CLITask.h"

// Public (External) Procedures
/**
 * @brief    searchList - search through an string array for an exact match but case insensitive
 * @param    s    : point to the string
 * @param    list    : pointer to array of strings, terminated with null string
 * @return    -1 if not found, index of the found string otherwise
 */
int searchList(char *s, const char **list);
/**
 * @brief    searchCommand - search through command tree for exact match but case insensitive
 * @param    cmdstr : pointer to string holding command
 * @param    list    : pointer to array of command structure with id, name and help info
 * @param    len    : number of commands usually sizeof(list)/sizeof(commandList_t)
 * @return    -1 if error, or id number
 */
int searchCommand(char *cmdstr, const commandList_t *list, int len);
/**
 * @param[in] char *s pointer to string with number
 * @param[in] int32_t value if valid number, then this points to the value to be updated
 * @return  -2 if  no number, -1 if invalid number, 0 if valid number
 */
int parseNumber(char *s, int32_t *value);
/**
 * @brief    nextToken - scans for white space
 * @param[in] char **str : copy of pointer to the command line,
 * this is updated every time this command is called
 * @return    pointer to current token,
 * null terminated and stripped of white space
 */
char *nextToken(char **str);
/**
 * @brief    printHelp - print command help message to default UART
 * @param    list - command list same as searchCommand uses
 * @param    len - number of entries in command list
 */
void printHelp(const commandList_t *list, int len);
/**
 * @brief    dump_mem - dump memory to console in hexadecimal and ascii
 * @param    mem - pointer to byte memory
 * @param    count - number of bytes to print
 */
void dump_mem(uint8_t *mem, uint16_t count);

void i2cPErrorOL( i2cmErr_t rv, char const * msg );
void i2cPError( i2cmErr_t rv, char const * msg );
void i2cPError2OL( i2cmErr_t rv, char const * fmt, char const * msg, uint32_t v );
char toupper1( char c );
void toupperstr( char * p );
uint32_t atodec( char const * p, bool * pok );
uint32_t atohex( char const * p, bool * pok );

#endif // __CLI_H__
