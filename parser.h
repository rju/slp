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

#define TITLE        101
#define AUTHOR       102
#define DATE         103
#define SECTION      104
#define IMAGE        105
#define LISTING      106
#define ID           107
#define SEP          108
#define LABEL        109
#define LINE         110
#define VALUE        111
#define LSTEND       112
#define PSEQ_START   113
#define PSEQ_END     114
#define FRAME        115
#define STRUCT       116
#define ITEM         117
#define NEWLINE      118
#define INDENT       119
#define SPACE        120
#define UNCOVER      121
#define OVERLAY      122
#define ONLY         123
#define URL          124
#define COLUMN       125
#define COLUMN_SEP   126
#define END          127
#define OVERLAY_CODE 128
#define PROPERTY     129
#define PROP_SEP     130
#define PART         131
#define HEAD_END     132
#define DESC_SEP     133

#define ZERO         0

extern char* data;

extern char *item_type;
extern FILE *ofile;

int parse();
char* strtrim(const char *string);

#endif /* PARSER_H_ */
