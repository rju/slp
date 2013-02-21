/* scanner for SLP */

%{
#include <stdlib.h>
#include "parser.h"

// start condition stack
void push(int new_start_condition);
int pop();

#define START_CONDITION_STACK_SIZE 10
int start_condition_stack[START_CONDITION_STACK_SIZE];
int start_condition_top = 0; // start a lowest element

%}

DIGIT [0-9]
ID [A-Za-z0-9]
OVERLAYCHAR [0-9\-,]
VALUECHAR [A-Za-z0-9\_\\\.\"\{\}]
LABELCHAR [^=\n\t]

%option header-file="scanner.h"
%option yylineno

%x value
%x lsthead
%x lst
%x lstlabel
%x property
%x prop_value
%x label
%x number
%x lstframe

%%

"title:" {push(value); return TITLE;}
"author:" {push(value); return AUTHOR;}
"date:" {push(value); return DATE;}
"sec:" {push(value); return SECTION;}
"img:" {push(value); push(property); return IMAGE;}
"anim:" {push(value); push(property); push(number); return ANIMATION;}
"url:" {push(value); return URL;}
"part:" { push(value); return PART; }

"- " {push(value); item_type = "itemize"; return ITEM;}
"* " {push(label); item_type = "description"; return ITEM;}
"--" {push(lsthead); return LISTING;}
">" {push(value); return FRAME;}
"'" {push(value); return STRUCT;}
"$+" {return UNCOVER;}
"# " {push(value); item_type = "enumerate"; return ITEM;}
"$#" {return OVERLAY;}
"$~" {return ONLY;}

"col:" { return COLUMN; }
"sep:" { return COLUMN_SEP; }
"end:" { return END; }

"<"{OVERLAYCHAR}+">" { 
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	return OVERLAY_CODE; 
}
"{" {
	return O_BRACE;
}

"}" {
	return C_BRACE;
}

"\n" { return NEWLINE; } /* eat up whitespace */
"\t"+ {
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	return INDENT;
}
"  "+ {
	data = (char*)malloc(strlen(yytext)/2+1);
	int i;
	char *ch = data;
	for (i=0;i<strlen(yytext)/2;i++,ch++)
		*ch='\t';
	*ch=0;
	return INDENT;
}
" " { return SPACE; }

<property,number>"<"{OVERLAYCHAR}+">" { 
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	return OVERLAY_CODE; 
}

<label>{LABELCHAR}* {
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	return LABEL;
}
<label>"=" {
	pop();
	push(value);
	return DESC_SEP;
}

<lsthead>" "{ID}+ {
	data = strcpy((char*)malloc(strlen(yytext)),yytext+1);
	return ID;
}
<lsthead>" - " {
	push(lstlabel);
	return SEP;
}
<lsthead>"\n" {
	pop();
	push(lst);
	return HEAD_END;
}
<lstlabel>.*" --\n" {
	data = strncpy((char*)malloc(strlen(yytext)-2),yytext,strlen(yytext)-3);
	pop(); pop(); // label and head removed
	push(lst);
	return LABEL;
}
<lst>(.*"\n")+"--\n" {
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	pop();
	return LSTEND;
}
<lst>(.*"\n")+"--[" {
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	pop();
	push(lstframe);
	return LSTFRAME;
}
<lstframe>"]\n" {
	pop();
	return LSTEND;
}
<lstframe>"(" {
	return OR_BRACE;
}
<lstframe>")" {
	return CR_BRACE;
}

<property>"[" {
	return PSEQ_START;
}
<property>"]" {
	pop(); 
	return PSEQ_END;
}
<property>{ID}+"="{VALUECHAR}+ {
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	return PROPERTY;
}
<property,number,lstframe>"," {
	return COMMA;
}

<number,lstframe>{DIGIT}+ {
	data = strcpy((char*)malloc(strlen(yytext)+1),yytext);
	return NUMBER;
}
<number>"[" {
	pop();
	return PSEQ_START;
}

<value>.*"\n" {
	data = strtrim(yytext);
	pop();
	return VALUE;
}

%%

/*
 * set a new start condition for the scanner and push the
 * old one on a stack
 */
void push(int new_start_condition) {
	if (start_condition_top<START_CONDITION_STACK_SIZE) {
		start_condition_stack[start_condition_top++]=YY_START;
		BEGIN(new_start_condition);
	} else {
		fprintf(stderr,"Too many nested start conditions.\n");
		exit(2);
	}
}

int pop() {
	return BEGIN(start_condition_stack[--start_condition_top]);
}

int yywrap(void) {
	return (1);
}
