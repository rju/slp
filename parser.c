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
#include <string.h>

/* list of token names and descriptions */
char *token_names[] = {
	"title command","author command","date command","section command",
	"image command","embedded listing",
	"ID","separator","label","listing line","value",
	"end of listing",
	"start of parameter sequence",
	"end of parameter sequence",
	"frame","structure","item",
	"NEWLINE","INDENT","space","UNCOVER",
	"OVERLAY","ONLY","URL","column","column separator","end block","overlay range",
	"property", "comma", "part", "end of listing head", "description separater",
	"curly open brace", "curly close brace", "lst frames" };

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
int parse_copy();
int parse_column();
int parse_column_sep();
int parse_end();
int parse_part();
int parse_subtitle();

/* ------------------------------------ */
/* global properties                    */

int token;		   // token id of the last fetched token
int structure_count = 0;
FILE *ofile;		   // the handle for the output file
char* string = NULL;       // data contains the result string from the scanner
int int_val = 0;

/* Contains the name and type of the active item list:
 * itemize, enumeration, description
 */
char *item_type;

/* ------------------------------------ */
/* internal helper functions            */

char *strduplicate(const char *src) {
	int length = strlen(src)+1;
	char *buffer = malloc(length*sizeof(char));
	return strcpy(buffer,src);
}

char *strappend(char *dest, const char *src) {
	if (src == NULL)
		return dest;
	else if (dest == NULL) {
		return strduplicate(src);
	} else {
		char *result = malloc(strlen(dest)+strlen(src)+1);
		strcpy(result,dest);
		// free(dest); /* this should work */
		return strcat(result,src);
	}
}

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
		fprintf(ofile,"\n\n%%%%%%\n\\part{%s}\n\n", string);
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
		fprintf(ofile,"\n%%%%\n\\section{%s}\n\n", string);
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
		fprintf(ofile,"\\frametitle{%s}\n", string);
		token = yylex();
		while (token != EOF) {
			switch (token) {
			case SUBTITLE:
				if (!parse_subtitle()) return 0;
				break;
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
				if (!parse_item(1,item_type)) return 0;
				break;
			case URL:
				if (!parse_url()) return 0;
				break;
			case COPY:
				if (!parse_copy()) return 0;
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
			case VALUE:
				// mirror value to output
				fprintf(ofile,"%s",string);
				token = yylex();
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
				fprintf(stderr, "<%s:%d> [%d] Frame mode: illegal token %s found.\n", 
					__FILE__,__LINE__,yylineno, get_token_name(token));
				fprintf(stderr, "Expected commands: uncover (+), overlay (#), only (~), structure ('), image, listing, item or URL\n");
				fprintf(stderr, "\tor end frame mode with: New frame, section, title, author, date or an end of file\n");
				return 0;
			}
		}
		return 1;
	} else {
		fprintf(stderr, "<%s:%d> [%d] Frame mode: expected value, but %s found.\n", __FILE__,__LINE__, yylineno, get_token_name(token));
		return 0;
	}
}

/* parser and renderer for nested frames
 */
int parse_frame_body(const char *command, const int subslide) {
	int result = 0;
	token = yylex();
	if (token == OVERLAY_CODE) {
		fprintf(ofile,"\\%s%s{",command,string);
		token = yylex();
	} else {		
		fprintf(ofile,"\\%s<%d->{",command,subslide);
	}
	if ( token == CURLY_O_BRACE ) {
		token = yylex();
		result = parse_frame_body_loop (subslide);
	} else {
		result = parse_frame_body_single (subslide);
	}

	fprintf(ofile,"}\n");
	if ( token == CURLY_C_BRACE )
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
			if (!parse_item(1,item_type)) return 0;
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
		case CURLY_C_BRACE:
			return 1;
		case VALUE:
			// mirror value to output
			fprintf(ofile,"%s",string);
			token = yylex();
			break;
		case NEWLINE: // ignore empty newline
			token = yylex();
			break;
		default:
			fprintf(stderr, "<%s:%d> [%d] Frame mode: illegal token %s found.\n", 
				__FILE__,__LINE__,yylineno, get_token_name(token));
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
	case COPY:
		return parse_copy();
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
	token = yylex();
	if (token == NUMBER) {
		token = yylex();
	} else {
		string = "0.5";
	}
	fprintf(ofile,"\\begin{columns}[t]\n\\begin{column}{%s\\textwidth}\n",strtrim(string));
	structure_count = 0;
	return 0;
}

/* parser and renderer for seperator of two columns
 */
int parse_column_sep() {
	token = yylex();
	if (token == NUMBER) {
		token = yylex();
	} else {
		string = "0.5";
	}
	fprintf(ofile,"\\end{column}\n\\begin{column}{%s\\textwidth}\n",strtrim(string));
	structure_count = 0;
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
		fprintf(ofile,"\\url{%s}\n", string);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Title value expected, but %s found.\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

int parse_copy() {
	while ((token = yylex()) != END) {
		fprintf(ofile,"%s", string);
		token = yylex();
	}
	token = yylex();
	return 1;
}

/*
 * Parse frame subtitle
 */
int parse_subtitle() {
	if ((token = yylex()) == VALUE) {
		fprintf(ofile,"\\framesubtitle{%s}\n", string);
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Title value expected, but %s found.\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

/*
 *
 */
int parse_title() {
	if ((token = yylex()) == VALUE) {
		fprintf(stderr, "Title: %s\n", string); 
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Title value expected, but %s found.\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

/*
 *
 */
int parse_author() {
	if ((token = yylex()) == VALUE) {
		fprintf(stderr, "Author: %s\n", string); 
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Author value expected, but %s found.\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

/*
 *
 */
int parse_date() {
	if ((token = yylex()) == VALUE) {
		fprintf(stderr, "Date: %s\n", string); 
		token = yylex();
		return 1;
	} else {
		fprintf(stderr, "[%d] Date value expected, but %s found.\n",
			yylineno, get_token_name(token));
		return 0;
	}
}

/* ------------------------------ */
/* highlighting lines             */

char* parse_hl_lines() {
	char* lines = NULL;
	if ((token = yylex()) == NORMAL_O_BRACE) {
		while (token != NORMAL_C_BRACE) {
			if ((token = yylex()) == NUMBER) {
				lines = strappend(lines,string);
				if ((token = yylex()) == COMMA) {
					lines = strappend(lines,",");
				}
			} else {
				fprintf(stderr,
					"[%d] Listing mode: Number expected, but %s \"%s\" found\n",
					yylineno, get_token_name(token), yytext);
				return NULL;
			}
		}
	}

	return lines;
}

char* parse_highlighting() {
	char* highlighting = strduplicate("linebackgroundcolor={");
	while ((token = yylex()) != SQUARE_C_BRACE) {
		if (token == NUMBER) {
			int number = int_val;
			#define MAX_NUM_LENGTH 4
			char* lines = parse_hl_lines();
			if (lines != NULL) {
				char *frame = malloc(MAX_NUM_LENGTH+strlen(lines)+11+1);
				sprintf(frame,"\\btLstHL<%d>{%s}",number,lines);
				highlighting = strappend(highlighting,frame);
				// free(frame); /* should work */
				free(lines);
			}
		} else {
			fprintf(stderr,
				"[%d] Listing mode: Number expected, but %s \"%s\" found\n",
				yylineno, get_token_name(token), yytext);
			return NULL;
		}
	}
	return strappend(highlighting,"},");
}

/*
 * process listings
 */
int parse_listing_text(const char* language, const char* caption, const char* highlighting) {
	char* buffer = strduplicate("");
	while ((token = yylex()) != LST_END) {
		if (token == LST_LINE) {
			buffer = strappend(buffer,string);
		} else {
			free(buffer);
			fprintf(stderr,
				"[%d] Listing mode: End of listing or listing expected, but %s \"%s\" found\n",
				yylineno, get_token_name(token), yytext);
			return 0;
		}
	}
	
	fprintf(ofile,"\\begin{lstlisting}[%sescapechar=\\%%,",highlighting);
	if (caption == NULL)
		fprintf(ofile, "language=%s]\n", language);
	else
		fprintf(ofile, "language=%s,caption=%s]\n", language, caption);
	fprintf(ofile,"%s\\end{lstlisting}\n", buffer);
	// free(buffer); /* should work */
	token = yylex();
	return 1;
}


char* parse_label() {
	if ((token = yylex()) == LABEL) {
		return string;	
	} else {
		fprintf(stderr, 
			"[%d] Listing mode: missing label for listing, found %s \"%s\" instead\n", 
			yylineno, get_token_name(token), yytext);
		return NULL;
	}
}

int parse_listing() {
	int result = 0;
	if ((token = yylex()) == ID) {
		char* language = strduplicate(string);
		char* highlighting;
		char* caption;

		if ((token = yylex()) == LST_FRAME) { /* highlight rows in frame */
			highlighting = parse_highlighting();
			token = yylex();
		} else 
			highlighting = strduplicate("");
		switch (token) {
		case LST_SEPARATOR:
			caption = strduplicate(parse_label());
			result = parse_listing_text(language,caption,highlighting);
			free(caption);
			break;
		case NEWLINE:
			result = parse_listing_text(language,NULL,highlighting);
			break;
		default:
			fprintf(stderr, 
				"[%d] Listing mode: Separator '-' expected, found %s \"%s\" instead\n", 
				yylineno, get_token_name(token), yytext);
			result = 0;
			break;
		}
		free(highlighting);
		free(language);
	} else {
		fprintf(stderr, "[%d] Listing mode: Language name expected, but %s \"%s\"\n",
			yylineno, get_token_name(token), yytext);
		result = 0;
	}
	return result;
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
	char *overlay_code;

	token = yylex();
	if (token == OVERLAY_CODE) {
		overlay_code = strduplicate(string);
		token = yylex();
	} else
		overlay_code = strduplicate("");
	if (token == SQUARE_O_BRACE) {
		char *properties;
		if ((properties = parse_properties()) != NULL)  {
			if ((token = yylex()) == VALUE) {
				fprintf(ofile,"\\includegraphics%s[%s]{%s}\n",
						overlay_code, properties, string);
				free(overlay_code);
				free(properties);
				token = yylex();
				if (token == IMAGE) // is an image group
					parse_figure_inner(index+1);
				return 1;
			} else {
				free(properties);
				free(overlay_code);
				fprintf(stderr, 
					"[%d] Figure mode: Missing image, found %s instead\n",
					yylineno, get_token_name(token));
				return 0;
			}
		} else {
			free(overlay_code);
			return 0;
		}
	} else {
		free(overlay_code);
		fprintf(stderr, "[%d] Figure mode: Missing [, found %s instead\n", 
			yylineno, get_token_name(token));
		return 0;
	}
}

/*
 * process animations
 */
int parse_animation() {
	int numbers[1000]; 
	int numbers_max = 0;
	char *overlay_code = NULL;

	token = yylex();
	if (token == OVERLAY_CODE) {
		overlay_code = strduplicate(string);
		token = yylex();
	}
	while (token == NUMBER) {
		numbers[numbers_max++] = int_val;
		token = yylex();
		if (token == COMMA) 
			token = yylex();
	}
	if (token == SQUARE_O_BRACE) {
		char *properties;
		if ((properties = parse_properties()) != NULL)  {
			if ((token = yylex()) == VALUE) {
				if (overlay_code != NULL)
					fprintf(ofile,"\\only%s{\\begin{figure}\n",overlay_code);
				else
					fprintf(ofile,"\\begin{figure}\n");
				for (int i=0;i<numbers_max;i++) {
					fprintf(ofile,"\\includegraphics<%d>[%s,page=%d]{%s}\n",
						i+1, properties, numbers[i], string);
				}
				if (overlay_code != NULL)
					fprintf(ofile,"\\end{figure}}\n");
				else
					fprintf(ofile,"\\end{figure}\n");

				free(properties);

				token = yylex();
				return 1;
			} else {
				free(properties);
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
	char *result = NULL;
	while ((token = yylex()) != SQUARE_C_BRACE) {
		switch (token) {
		case PROPERTY:
			result = strappend(result,string); 
			break;
		case COMMA:
			result = strappend(result,",");
			break;
		case SQUARE_C_BRACE:
			break;
		default:
			fprintf(stderr, 
				"[%d] Figure mode, property sequence: Missing ]. Found '%s'\n", 
				yylineno,get_token_name(token));
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
		for (sym=string; (*sym != 0) && (*sym != ':');sym++);
		if (*sym == ':') {
			*sym = 0;
			sym++;
			fprintf(ofile,"\\structure{%s} %s\n", string, sym);
		} else {
			fprintf(ofile,"\\structure{%s}\n", string);
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
			fprintf(ofile,"\\item[%s] ",string); 
			if ((token = yylex()) == DESC_SEP) {
				if ((token = yylex()) == VALUE) {
					fprintf(ofile,"%s\n",string); 
					token = yylex();
					return 1;
				} else {
					fprintf(stderr, 
						"<%s:%d> [%d] Item mode: Missing value for item, got %s \"%s\" instead.\n",
						__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
					return 0;
				}
			} else {
				fprintf(stderr, 
					"<%s:%d> [%d] Description item mode: Label-value-separator = expected, but %s \"%s\" found instead.\n",
					__FILE__,__LINE__, 
					yylineno,get_token_name(token),yytext);
				return 0;
			}
		} else {
			fprintf(stderr, 
				"<%s:%d> [%d] Description item mode: Missing label for item, got %s \"%s\" instead.\n",
				__FILE__,__LINE__,
				yylineno,get_token_name(token),yytext);
			return 0;
		}
	} else {
		if ((token = yylex()) == VALUE) {
			fprintf(ofile,"\\item %s\n",string);
			token = yylex();
			return 1;
		} else {
			fprintf(stderr, 
				"<%s:%d> [%d] Item mode: Missing value for item, got %s \"%s\" instead.\n",
				__FILE__,__LINE__, yylineno,get_token_name(token),yytext);
			return 0;
		}
	}
}


/*
 *
 */
int parse_item(int present_indent, const char *mode) {
	fprintf(ofile,"\\begin{%s}\n",mode);
	if (!parse_item_value(mode))
		return 0;

	while (token != EOF) {
		switch (token) {
		case INDENT: 
			if (present_indent==int_val) { // same indent
				if ((token = yylex()) == ITEM) {
					if (!parse_item_value(mode))
						return 0;
				} else {
					fprintf(stderr,
						"<%s:%d> [%d] Item expected, but %s \"%s\" found.\n",
						__FILE__,__LINE__,yylineno,get_token_name(token),yytext);
					return 0;
				}
			} else if (present_indent>int_val) { // less indent go back
				fprintf(ofile,"\\end{%s}\n",mode);
				return 1;
			} else if (present_indent<int_val) { // new indent go sub
				if ((token = yylex()) == ITEM) {
					if (!parse_item(int_val,item_type))
						return 0;
				} else {
					fprintf(stderr,
						"<%s:%d> [%d] Item expected, but %s \"%s\" found.\n",
						__FILE__,__LINE__,yylineno,get_token_name(token),yytext);
					return 0;
				}
			}
			break;
		case ITEM: // this means zero new_indent	
			if (present_indent>1) {	
				fprintf(ofile,"\\end{%s}\n",mode);
				return 1;
			} else 
				if (!parse_item_value(mode))
					return 0;
			break;
		case VALUE:
		case SECTION:
		case FRAME:
		case IMAGE:
		case STRUCT:
		case NEWLINE:
		case EOF:
		case COLUMN:
		case COLUMN_SEP:
		case END:
		case CURLY_C_BRACE:
		case ZERO: // leave frame => close all item list
			fprintf(ofile,"\\end{%s}\n",mode);
			return 1;
		default:
			fprintf(stderr, 
				"<%s:%d> [%d] Top mode: %s \"%s\" not allowed in top mode.\n",
				__FILE__,__LINE__,
				yylineno, get_token_name(token), yytext);
			fprintf(stderr,
				"<%s:%d> Expected: item, indent, section, frame, image, structure ('), newline, or end of file\n",
				__FILE__,__LINE__);
			return 0;
		}
	}
	fprintf(stderr, "<%s:%d> [%d] Item mode: Unhandled EOF\n",__FILE__,__LINE__, yylineno);
	return 0;
}


