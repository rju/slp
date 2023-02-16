/* scanner for SLP */

%{
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
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
EXTVALUECHAR [A-Za-z0-9\_\\\.\"\{\}:\[\]=,\-/ ]
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
%x copy

%%

"title:" {push(value); return TITLE;}
"author:" {push(value); return AUTHOR;}
"date:" {push(value); return DATE;}
"sec:" {push(value); return SECTION;}
"img:" {push(value); push(property); return IMAGE;}
"anim:" {push(value); push(property); push(number); return ANIMATION;}
"url:" {push(value); return URL;}
"part:" { push(value); return PART; }
"copy:" { push(copy); return COPY; }

"- " {push(value); item_type = "itemize"; return ITEM;}
"* " {push(label); item_type = "description"; return ITEM;}
"--" {push(lsthead); return LISTING;}
"> " {push(value); return FRAME;}
">> " {push(value); return SUBTITLE;}
"'" {push(value); return STRUCT;}
"$+" {return UNCOVER;}
"# " {push(value); item_type = "enumerate"; return ITEM;}
"$#" {return OVERLAY;}
"$~" {return ONLY;}

"col:" { return COLUMN; }
"sep:" { return COLUMN_SEP; }
"end:" { return END; }
{DIGIT}+"."{DIGIT}+ { string = yytext; return NUMBER; }

"<"{OVERLAYCHAR}+">" { 
	string = yytext;
	return OVERLAY_CODE; 
}
"{" {
	return CURLY_O_BRACE;
}

"}" {
	return CURLY_C_BRACE;
}

"\n" { return NEWLINE; } /* eat up whitespace */
"\t"+ {
	int_val = strlen(yytext) + 1;
	return INDENT;
}
"  "+ { 
	int_val = strlen(yytext)/2 + 1;
	return INDENT;
}
" " { return SPACE; }
"\\"{EXTVALUECHAR}+"\n" {
	string = yytext;
	return VALUE;
}

<copy>"end:" {
	pop();
	return END;
}
<copy>.*"\n" {
	if (strcmp(yytext,"end:\n") == 0) {
		pop();
		return END;
	} else {
		string = yytext;
		return LST_LINE;
	}
}

<property,number>"<"{OVERLAYCHAR}+">" { 
	string = yytext;
	return OVERLAY_CODE; 
}

<label>{LABELCHAR}* {
	string = yytext;
	return LABEL;
}
<label>"=" {
	pop();
	push(value);
	return DESC_SEP;
}

<lsthead>" "{ID}+ {
	string = yytext+1;
	return ID;
}
<lsthead>" - " {
	push(lstlabel);
	return LST_SEPARATOR;
}
<lsthead>" - [" {
	push(lstframe);
	return LST_FRAME;
}	
<lsthead>"\n" {
	pop(); /* leave listing head */
	push(lst); /* goto listing body mode */
	return NEWLINE;
}
<lstlabel>.*" --\n" {
	string = yytext;
	pop(); pop(); // label and head removed
	push(lst);
	return LABEL;
}
<lst>.*"\n" {
	if (strcmp(yytext,"--\n") == 0) {
		pop();
		return LST_END;
	} else {
		string = yytext;
		return LST_LINE;
	}
}

<lstframe>"]" {
	pop(); /* got back to lsthead */
	return SQUARE_C_BRACE;
}
<lstframe>"(" {
	return NORMAL_O_BRACE;
}
<lstframe>")" {
	return NORMAL_C_BRACE;
}
<lstframe>[ \t]+  /* ignore whitespace */

<property>"[" {
	return SQUARE_O_BRACE;
}
<property>"]" {
	pop(); /* end of properties */
	return SQUARE_C_BRACE;
}
<property>{ID}+"="{VALUECHAR}+ {
	string = yytext;
	return PROPERTY;
}
<property>{ID}+ {
    string = yytext;
    return VALUE;
}
<property,number,lstframe>"," {
	return COMMA;
}

<number,lstframe>{DIGIT}+ {
	int_val = atoi(yytext);
	string = yytext;
	return NUMBER;
}
<number>"[" {
	pop(); /* no more numbers, got to property sequence. Used by anim: */
	return SQUARE_O_BRACE;
}

<value>.*"\n" { /* single prefix command value */
	string = yytext;
	while (isspace(*string))
		string++;
	char *last = string + strlen(string) -1;
	while (isspace(*last)) {
		*last = 0;
		last--;
	}
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
