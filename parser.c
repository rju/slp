/*
 * parser.c
 *
 *  Created on: 18 Apr 2012
 *      Author: rju
 */

#include "parser.h"
#include "scanner.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* list of token names and descriptions */
char *token_names[] = {
	"title command","author command","date command","section command",
	"image command","embedded listing",
	"ID","separator","label","LINE","value",
	"end of listing",
	"start of parameter sequence",
	"end of parameter sequence",
	"frame","structure","item",
	"NEWLINE","INDENT","space","UNCOVER",
	"OVERLAY","ONLY","URL","column","column separator","end block","overlay range",
	"property", "comma", "part", "end of listing head", "description separater",
	"open brace", "close brace" };

/* ------------------------------------ *
 * local function declaration for       *
 * parsing and output generation        */
int parse_section();
int parse_frame();
int parse_title();
int parse_author();
int parse_date();
int parse_listing();
int parse_figure();
int parse_figure_inner(int index);
int parse_animation();
int parse_structure();
int parse_item();
int parse_enumerate();
char* parse_properties();
int parse_frame_body(const char *commanf, const int subslide);
int parse_frame_body_loop (const int subslide);
int parse_frame_body_single (const int subslide);
int parse_url();
int parse_column();
int parse_column_sep();
int parse_end();
int parse_part();

/* ------------------------------------ */
/* global properties                    */

char *title, *author, *date; // properties for the whole document, could be used in future to generate the titlepage
int token;		// token id of the last fetched token
int structure_count = 0;
FILE *ofile;		   // the handle for the output file
char* data = NULL;        // data contains the result string from the scanner

/* Contains the name and type of the active item list:
 * itemize, enumeration, description
 */
char *item_type;

/* ------------------------------------ */
/* internal helper functions            */

/* Removes leading and trailing whitespaces.
 * The resulting string gets a new string object
 *
 * string = input
 * 
 * returns a string without trailing/leading whitespaces
 */
char* strtrim(const char *string) {
	while (isspace(*string))
		string++;
	char *result = strcpy((char*) malloc(strlen(string) + 1), string);
	char *last = result + strlen(result) - 1;
	while (isspace(*last))
		last--;
	last++;
	*last = 0;
	return result;
}

/* Displays the token name or token description for a
 * given token from the symbol table.
 *
 * token_id = numerical value representing the token
 *
 * returns a string
 */
const char* get_token_name(int token_id) {
	return token_names[token_id-101];
}

/* ------------------------------------ */
/* parser functions                     */

/* Main parser function
 */
int parse() {
	token = yylex();
	do {
		switch (token) {
		case TITLE:
			if (!parse_title()) return 0;
			break;
		case AUTHOR:
			if (!parse_author()) return 0;
			break;
		case DATE:
			if (!parse_date()) return 0;
			break;
		case SECTION:
			if (!parse_section()) return 0;
			break;
		case FRAME:
			if (!parse_frame()) return 0;
			break;
		case PART:
			if (!parse_part()) return 0;
			break;
		case EOF:
		case ZERO:
			return 1;
		case NEWLINE:
			token = yylex();
			break;
		case IMAGE:
			fprintf(stderr, "[%d] Top mode: figure not allowed in top mode.\n",yylineno);
			return 0;
		case ANIMATION:
			fprintf(stderr, "[%d] Top mode: figure not allowed in top mode.\n",yylineno);
			return 0;
		case LISTING:
			fprintf(stderr, "[%d] Top mode: listing not allowed in top mode.\n", yylineno);
			return 0;
		case STRUCT:
			fprintf(stderr, "[%d] Top mode: structure not allowed in top mode.\n", yylineno);
			return 0;
		case ITEM:
			fprintf(stderr, "[%d] Top mode: item not allowed in top mode.\n", yylineno);
			return 0;
		default:
			fprintf(stderr, "[%d] Top mode: %s \"%s\" not allowed in top mode.\n", yylineno,
					get_token_name(token), yytext);
			return 0;
		}
	} while (token != EOF);

	return 1;
}


/*
 * parse part rule
 */
int parse_part() {
	if ((token = yylex()) == VALUE) {
		fprintf(ofile,"\n\n%%%%%%\n\\part{%s}\n\n", data);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Part value expected, but %s \"%s\" found\n", yylineno, get_token_name(token),yytext);
		return 0;
	}
}

/*
 * parse section rule
 */
int parse_section() {
	if ((token = yylex()) == VALUE) {
		fprintf(ofile,"\n%%%%\n\\section{%s}\n\n", data);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Section value expected, but %s found\n", yylineno, get_token_name(token));
		return 0;
	}
}

/*
 * parse the content of a frame
 */
int parse_frame() {
	int subslide = 1;

	structure_count = 0;

	fprintf(ofile,"%% FRAME\n");
	fprintf(ofile,"\\begin{frame}[fragile]\n");

	if ((token = yylex()) == VALUE) {
		fprintf(ofile,"\\frametitle{%s}\n", data);
		token = yylex();
		while (token != EOF) {
			switch (token) {
			case UNCOVER:
				if (!parse_frame_body("uncover",subslide++)) return 0;
				break;
			case OVERLAY:
				if (!parse_frame_body("overlay",subslide++)) return 0;
				break;
			case ONLY:
				if (!parse_frame_body("only",subslide++)) return 0;
				break;
			case STRUCT:
				if (!parse_structure()) return 0;
				break;
			case IMAGE:
				if (!parse_figure()) return 0;
				break;
			case ANIMATION:
				if (!parse_animation()) return 0;
				break;
			case LISTING:
				if (!parse_listing()) return 0;
				break;
			case ITEM:
				if (parse_item(1,item_type) == -1) return 0;
				break;
			case URL:
				if (!parse_url()) return 0;
				break;
			case COLUMN:
				if (parse_column() == -1) return 0;
				break;
			case COLUMN_SEP:
				if (parse_column_sep() == -1) return 0;
				break;
			case END:
				if (parse_end() == -1) return 0;
				break;
			case FRAME:
			case SECTION:
			case TITLE:
			case AUTHOR:
			case DATE:
			case ZERO:
			case PART:
				fprintf(ofile,"\\end{frame}\n\n");
				return 1;
			case NEWLINE: // ignore empty newline
				token = yylex();
				break;
			default:
				fprintf(stderr, "[%d] Frame mode: illegal token %s found.\n", yylineno, get_token_name(token));
				fprintf(stderr, "Expected commands: uncover (+), overlay (#), only (~), structure ('), image, listing, item or URL\n");
				fprintf(stderr, "\tor end frame mode with: New frame, section, title, author, date or an end of file\n");
				return 0;
			}
		}
		return 1;
	} else {
		fprintf(stderr, "[%d] Frame mode: expected value, but %s found.\n", yylineno, get_token_name(token));
		return 0;
	}
}

/* parser and renderer for nested frames
 */
int parse_frame_body(const char *command, const int subslide) {
	int result = 0;
	token = yylex();
	if (token == OVERLAY_CODE) {
		fprintf(ofile,"\\%s%s{",command,data);
		token = yylex();
	} else {		
		fprintf(ofile,"\\%s<%d->{",command,subslide);
	}
	if ( token == O_BRACE ) {
		token = yylex();
		result = parse_frame_body_loop (subslide);
	} else {
		result = parse_frame_body_single (subslide);
	}

	fprintf(ofile,"}\n");
	if ( token == C_BRACE )
		token = yylex();
	return result;
}

/* parser and renderer for nested frames which use grouping
 */
int parse_frame_body_loop (const int subslide) {
	while (token != EOF) {
		switch (token) {
		case STRUCT:
			if (!parse_structure()) return 0;
			break;
		case IMAGE:
			if (!parse_figure()) return 0;
			break;
		case ANIMATION:
			if (!parse_animation()) return 0;
			break;
		case LISTING:
			if (!parse_listing()) return 0;
			break;
		case ITEM:
			if (parse_item(1,item_type) == -1) return 0;
			break;
		case URL:
			if (!parse_url()) return 0;
			break;
		case COLUMN:
			if (parse_column() == -1) return 0;
			break;
		case COLUMN_SEP:
			if (parse_column_sep() == -1) return 0;
			break;
		case END:
			if (parse_end() == -1) return 0;
			break;
		case C_BRACE:
			return 1;
		case NEWLINE: // ignore empty newline
			token = yylex();
			break;
		default:
			fprintf(stderr, "[%d] Frame mode: illegal token %s found.\n", yylineno, get_token_name(token));
			fprintf(stderr, "Expected commands: uncover (+), overlay (#), only (~), structure ('), image, listing, item or URL\n");
			fprintf(stderr, "\tor end frame mode with: New frame, section, title, author, date or an end of file\n");
			return 0;
		}
	}
	return 1;
}

/* parser and renderer for nested frames, single line
 */
int parse_frame_body_single (const int subslide) {
	switch (token) {
	case STRUCT:
		return parse_structure();
		break;
	case IMAGE:
		return parse_figure();
		break;
	case ANIMATION:
		return parse_animation();
		break;
	case LISTING:
		return parse_listing();
		break;
	case ITEM:
		return parse_item(1,item_type);
		break;
	case URL:
		return parse_url();
		break;
	default:
		fprintf(stderr,"[%d] Overlay mode: Illegal token %s\n", yylineno, get_token_name(token));
		fprintf(stderr,"Expected commands: structure ('), image, listing, item and URL\n");
		return 0;
	}
}

/* parser and renderer for start of two columns
 */
int parse_column() {
	fprintf(ofile,"\\begin{columns}[t]\n\\begin{column}{0.5\\textwidth}\n");
	token = yylex();
	return 0;
}

/* parser and renderer for seperator of two columns
 */
int parse_column_sep() {
	fprintf(ofile,"\\end{column}\n\\begin{column}{0.5\\textwidth}\n");
	token = yylex();
	return 0;
}

/* parser and renderer for end of two columns
 */
int parse_end() {
	fprintf(ofile,"\\end{column}\n\\end{columns}\n");
	token = yylex();
	return 0;
}


int parse_url() {
	if ((token = yylex()) == VALUE) {
		fprintf(ofile,"\\url{%s}\n",data);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Title value expected, but %s found.\n", yylineno, get_token_name(token));
		return 0;
	}
}

/*
 *
 */
int parse_title() {
	if ((token = yylex()) == VALUE) {
		title = data;
		fprintf(stderr, "Title: %s\n", title);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Title value expected, but %s found.\n", yylineno, get_token_name(token));
		return 0;
	}
}

/*
 *
 */
int parse_author() {
	if ((token = yylex()) == VALUE) {
		author = data;
		fprintf(stderr, "Author: %s\n", author);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Author value expected, but %s found.\n", yylineno, get_token_name(token));
		return 0;
	}
}

/*
 *
 */
int parse_date() {
	if ((token = yylex()) == VALUE) {
		date = data;
		fprintf(stderr, "Date: %s\n", date);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Date value expected, but %s found.\n", yylineno, get_token_name(token));
		return 0;
	}
}

/*
 * process listings
 */
int parse_listing_text() {
	while ((token = yylex()) == LINE) {
		fprintf(ofile,"%s", data);
	}
	if (token != LSTEND) {
		fprintf(stderr,
				"[%d] Listing mode: End of listing or listing expected, but %s \"%s\" found\n",
				yylineno, get_token_name(token), yytext);
		return 0;
	} else {
		fprintf(ofile,"\\end{lstlisting}\n");
		token = yylex();
		return 1;
	}
}


int parse_listing() {
	fprintf(ofile,"\\begin{lstlisting}");
	if ((token = yylex()) == ID) {
		fprintf(ofile,"[escapechar=@,language=%s", data);
		if ((token = yylex()) == SEP) {
			if ((token = yylex()) == LABEL) {
				fprintf(ofile,",caption=%s]\n", data);
				return parse_listing_text();
			} else {
				fprintf(stderr, "[%d] Listing mode: missing label for listing, found %s \"%s\" instead\n", yylineno, get_token_name(token), yytext);
				return 0;
			}
		} else if (token == HEAD_END) {
			fprintf(ofile,"]\n");
			return parse_listing_text();
		} else {
			fprintf(stderr, "[%d] Listing mode: Separator '-' expected\n", yylineno);
			return 0;
		}
	} else {
		fprintf(stderr, "[%d] Listing mode: Language name expected, but %s \"%s\"\n", yylineno, get_token_name(token), yytext);
		return 0;
	}
}

/*
 * process figures
 */
int parse_figure() {
	fprintf(ofile,"\\begin{figure}\n");
	if (parse_figure_inner(0)) {
		fprintf(ofile,"\\end{figure}\n");
		return 1;
	} else
		return 0;
}

/*
 * process figures
 */
int parse_figure_inner(int index) {
	char *overlay_code = "";

	token = yylex();
	if (token == OVERLAY_CODE) {
		overlay_code = data;
		token = yylex();
	}
	if (token == PSEQ_START) {
		char *properties;
		if ((properties = parse_properties()) != NULL)  {
			if ((token = yylex()) == VALUE) {
				fprintf(ofile,"\\includegraphics%s[%s]{%s}\n",
						overlay_code, properties, data);
				token = yylex();
				if (token == IMAGE) // is an image group
					parse_figure_inner(index+1);
				return 1;
			} else {
				fprintf(stderr, 
					"[%d] Figure mode: Missing image, found %s instead\n",
					yylineno, get_token_name(token));
				return 0;
			}
		} else
			return 0;
	} else {
		fprintf(stderr, "[%d] Figure mode: Missing [, found %s instead\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

/*
 * process animations
 */
int parse_animation() {
	char **numbers = (char**)malloc(1000); // this is dirty in many ways.
	int numbers_max = 0;
	char *overlay_code = NULL;

	token = yylex();
	if (token == OVERLAY_CODE) {
		overlay_code = data;
		token = yylex();
	}
	while (token == NUMBER) {
		numbers[numbers_max++] = data;
		token = yylex();
		if (token == PROP_SEP) 
			token = yylex();
	}
	if (token == PSEQ_START) {
		char *properties;
		if ((properties = parse_properties()) != NULL)  {
			if ((token = yylex()) == VALUE) {
				if (overlay_code != NULL)
					fprintf(ofile,"\\only%s{\\begin{figure}\n",overlay_code);
				else
					fprintf(ofile,"\\begin{figure}\n");
				for (int i=0;i<numbers_max;i++) {
					fprintf(ofile,"\\includegraphics<%s>[%s,page=%s]{%s}\n",
						numbers[i], properties, numbers[i], data);
				}
				if (overlay_code != NULL)
					fprintf(ofile,"\\end{figure}}\n");
				else
					fprintf(ofile,"\\end{figure}\n");
				token = yylex();
				return 1;
			} else {
				fprintf(stderr, 
					"[%d] Figure mode: Missing image, found %s instead\n",
					yylineno, get_token_name(token));
				return 0;
			}
		} else {
			return 0;
		}
	} else {
		fprintf(stderr, "[%d] Figure mode: Missing [, found %s instead\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

/*
 * parse properties
 */
char* parse_properties() {
	char *result = strdup("");
	while ((token = yylex()) != PSEQ_END) {
		switch (token) {
		case PROPERTY:
			result = strcat(realloc(result,
				(result!=NULL?strlen(result):0)+strlen(data)+1),data);
			break;
		case PROP_SEP:
			result = strcat(realloc(result,
				(result!=NULL?strlen(result):0)+2),",");
			break;
		case PSEQ_END:
			break;
		default:
			fprintf(stderr, "[%d] Figure mode, property sequence: Missing ]. Found %s\n", yylineno,get_token_name(token));
			return NULL;
		}
	}
	return result;
}

/*
 *
 */
int parse_structure() {
	if ((token = yylex()) == VALUE) {
		if (structure_count > 0)
			fprintf(ofile,"\\vskip1em\n");
		structure_count++;
		char *sym;
		for (sym=data; (*sym != 0) && (*sym != ':');sym++);
		if (*sym == ':') {
			*sym = 0;
			sym++;
			fprintf(ofile,"\\structure{%s} %s\n", data, sym);
		} else {
			fprintf(ofile,"\\structure{%s}\n", data);
		}
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Figure mode: Missing ]\n", yylineno);
		return 0;
	}
}


int parse_item_value(const char *mode) {
	if (strcmp("description",mode) == 0) {
		if ((token = yylex()) == LABEL) {
			fprintf(ofile,"\\item[%s] ",data);
			if ((token = yylex()) != DESC_SEP) {
				fprintf(stderr, "<%s:%d> [%d] Description item mode: Label-value-separator = expected, but %s \"%s\" found instead.\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
				return 0;
			} else if ((token = yylex()) == VALUE) {
				fprintf(ofile,"%s\n",data);
				token = yylex();
				return 1;
			} else {
				fprintf(stderr, "<%s:%d> [%d] Item mode: Missing value for item, got %s \"%s\" instead.\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
				return 0;
			}
		} else {
			fprintf(stderr, "<%s:%d> [%d] Description item mode: Missing label for item, got %s \"%s\" instead.\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
			return 0;
		}
	} else if ((token = yylex()) == VALUE) {
		fprintf(ofile,"\\item %s\n",data);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "<%s:%d> [%d] Item mode: Missing value for item, got %s \"%s\" instead.\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
		return 0;
	}
}

/*
 *
 */
int parse_item(int indent, char *mode) {
	if (strcmp("description",mode) == 0) {
		if ((token = yylex()) == LABEL) {
			fprintf(ofile,"\\begin{%s}\n",mode);
			fprintf(ofile,"\\item[%s] ",data);
			if ((token = yylex()) != DESC_SEP) {
				fprintf(stderr, "<%s:%d> [%d] Description item mode: Label-value-separator = expected, but %s \"%s\" found instead.\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
				return -1;
			} else if ((token = yylex()) == VALUE) {
				fprintf(ofile,"%s\n",data);
			}
		} else {
			fprintf(stderr, "<%s:%d> [%d] Description item mode: Missing label for item, got %s \"%s\" instead.\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
			return -1;
		}
	} else if ((token = yylex()) == VALUE) { // first value
		fprintf(ofile,"\\begin{%s}\n",mode);
		fprintf(ofile,"\\item %s\n",data);
	} else {
		fprintf(stderr, "<%s:%d> [%d] Item mode: Missing value for item, got %s \"%s\" instead\n",__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
		return -1;
	}

	token = yylex();
	do {
		if (token == INDENT) {
			int indent_length = strlen(data)+1;
			if (indent==indent_length) { // same indent
				if ((token = yylex()) == ITEM) {
					if (!parse_item_value(mode)) return -1;
				} else {
					fprintf(stderr,"<%s:%d> [%d] Item expected, but %s \"%s\" found.\n",__FILE__,__LINE__,yylineno,get_token_name(token),yytext);
					return -1;
				}
			} else if (indent>indent_length) { // less indent go back
				fprintf(ofile,"\\end{%s}\n",mode);
				return indent_length;
			} else if (indent<indent_length) { // new indent go sub
				if ((token = yylex()) == ITEM) {
					int result = parse_item(indent+1,item_type);
					if (result > 0) { // not an error
						if (result < indent) {
							fprintf(ofile,"\\end{%s}\n",mode);
							return result;
						} else	if (result == indent) {
							if (!parse_item_value(mode)) return -1;
						}
					} else if (result == -1) {
						return -1;
					} else {
						if (result < indent) 
							fprintf(ofile,"\\end{%s}\n",mode);
						return 0;
					}
				} else {
					fprintf(stderr,"<%s:%d> [%d] Item expected, but %s \"%s\" found.\n",__FILE__,__LINE__,yylineno,get_token_name(token),yytext);
					return -1;
				}
			}
		} else {
			switch (token) {
			case ITEM: // go back all indent, base item list	
				if (indent>1) {	
					fprintf(ofile,"\\end{%s}\n",mode);
					return 1;
				} else if (!parse_item_value(mode)) 
					return -1;
				break;
			case SECTION:
			case FRAME:
			case IMAGE:
			case STRUCT:
			case NEWLINE:
			case EOF:
			case COLUMN:
			case COLUMN_SEP:
			case END:
			case C_BRACE:
			case ZERO: // leave frame => close all item list
				fprintf(ofile,"\\end{%s}\n",mode);
				return 0;
			default:
				fprintf(stderr, "<%s:%d> [%d] Top mode: %s \"%s\" not allowed in top mode.\n",__FILE__,__LINE__,
										yylineno, get_token_name(token), yytext);
				fprintf(stderr, "<%s:%d> Expected: item, indent, section, frame, image, structure ('), newline, or end of file\n",__FILE__,__LINE__);
				return -1;
			}
		}
	} while (token != EOF);
	fprintf(stderr, "<%s:%d> [%d] Item mode: Unhandled EOF\n",__FILE__,__LINE__, yylineno);
	return -1;
}


