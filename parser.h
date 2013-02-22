/*
 * parser.h
 *
 *  Created on: 18 Apr 2012
 *      Author: rju
 */

#ifndef PARSER_H_
#define PARSER_H_

#include <stdio.h>
#include <stdlib.h> 
#include <config.h>

#define TITLE          101
#define AUTHOR         102
#define DATE           103
#define SECTION        104
#define IMAGE          105
#define LISTING        106
#define ID             107
#define LST_SEPARATOR  108
#define LABEL          109
#define LST_LINE       110
#define VALUE          111
#define LST_END        112
#define SQUARE_O_BRACE 113
#define SQUARE_C_BRACE 114
#define FRAME          115
#define STRUCT         116
#define ITEM           117
#define NEWLINE        118
#define INDENT         119
#define SPACE          120
#define UNCOVER        121
#define OVERLAY        122
#define ONLY           123
#define URL            124
#define COLUMN         125
#define COLUMN_SEP     126
#define END            127
#define OVERLAY_CODE   128
#define PROPERTY       129
#define COMMA          130
#define PART           131

#define DESC_SEP       133
#define NORMAL_O_BRACE 134
#define NORMAL_C_BRACE 135
#define ANIMATION      136
#define NUMBER         137
#define CURLY_O_BRACE  138
#define CURLY_C_BRACE  139
#define LST_FRAME      140

#define ZERO             0

extern char* string;
extern int int_val;

extern char *item_type;
extern FILE *ofile;

int parse();
char* strtrim(const char *string);

#endif /* PARSER_H_ */
