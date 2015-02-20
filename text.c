/* object for dealing with text files */

#include <stdio.h>
#include <malloc.h>
#include <ctype.h>
#include "text.h"

#define PRINTF printf


#define TEXT_FILE_MAGIC (0x432656)
#define LINE_MAGIC (0x678543)

struct line
{
	int magic;
	struct line *next;
	struct line *prev;
	int lineno;
	char *data;
};

struct text_file
{
	int magic;
	char filename[1024];
	int size;
	int lines;
	struct line *current_line;
	char * current_inputstring;	
	char *buffer;
	char buffer_checksum;
	struct line *firstline;
};



/***********************************************************
 *    FUNCTION: gntoken
 *
 *  PARAMETERS: pointer to char *inputstring,
 *		char *token,
 *		int length_of_token_buffer;
 *		
 *     RETURNS: int end_of_line_flag
 *
 * DESCRIPTION: gntoken reads through inputstring and returns
 * the next token string in 'token'.  input_string is changed
 * to point to the current place within the string.
 *
 ***********************************************************/

#define g_isalpha(x) (isalpha(x)||(x=='_'))
#define g_isalnum(x) (isalnum(x)||(x=='_'))

#define g_iswhite(x) ((x==' ')||(x=='\t'))	
#define g_iseol(x) ((x==0x0d)||(x==0))

#define add_to_token(x) *token++ = x
#define retn(x) {*token = 0; return(x);}
#define get_next(x) x = **string 
#define inc (*string = *string + 1)

char *hextodec(char *buffer)
{
	return(buffer);
}

int gntoken(char **string, char *token, int token_length)
{
	int chars = 0;
	char got;

point1:	

	get_next(got);

	if (g_iseol(got))
	{
		add_to_token(';');
		retn(1);
	}

	if (g_iswhite(got))
	{
		inc;
		goto point1;
	}

	if (isdigit(got))	goto point3;
	
	if (g_isalpha(got)) 	goto point2;
	/* AAA */
	if (got=='&')	goto point4;
	/* BBB */

	/* otherwise a punct */
	add_to_token(got);
	inc;
	retn(0);
	
point2:

	add_to_token(got);
	inc;
	get_next(got);

	if (g_isalnum(got))	goto point2;

	/* otherwise its a white or a punct */
	retn(0);

point3:

	add_to_token(got);
	inc;
	get_next(got);

	if (isdigit(got))	goto point3;

	/* otherwise, we don't want it */
	retn(0);

point4:
	inc;
	get_next(got);
	
	if (!isxdigit(got))
	{
		add_to_token('&');
		retn(0);
	}

point5:
	
	add_to_token(got);
	inc;
	get_next(got);
	
	if (isxdigit(got))	goto point5;

	token = hextodec(token);
	retn(0);
}	
	

static char
calc_checksum(char *buffer, int length)
{	
	int i;
	char sum = 0;

	while (length-- > 0)
	{
		sum += buffer[length];
	}

	return sum;
}	

/***************************************************************************
 *    FUNCTION: 
 *
 *  PARAMETERS: 
 *		
 *     RETURNS: text file ptr or NULL on failure
 *
 * DESCRIPTION: reads into memory the file 'filename' and stores
 * pointers to the beginning of each line in a list.  It then
 * closes the file.
 *
 * The following functions can be used to use this data:
 * text_get_current_line - copies the current line into a string
 * text_move_to_next_line - moves onto the next line in the file
 * text_get_lineno - return the number of the current line
 * text_get_next_token - get next token from the current line
 * text_set_line - set the line position within the file XXX NYI
 * text_reset_file - set the line position to the top of the file
 * text_set_offset - set the offset within the line
 * text_get_filename - get the filename
 **************************************************************************/

struct text_file *
text_init(char *filename)
{	
	FILE *inptr;
	struct text_file *new;
	int red;
	int chars_left;
	struct line *current_line;
	char *bp;
	struct line *new_line;

	if (filename == NULL)
	{
		PRINTF("text_init: NULL filename\n");
		return NULL;
	}

	if (strlen(filename) > (1024 - 1))
	{
		PRINTF("text_init: filename > 1024 chars\n");
		return NULL;
	}
			
	new = calloc(sizeof(*new), 1);
	if (new == NULL)
	{
		PRINTF("text_init: Failed to alloc text_file structure\n");
		return NULL;
	}

	strcpy(new->filename, filename);
	new->lines = 1;

	inptr = fopen(filename, "rb");

	if (inptr == NULL)
	{
		PRINTF("text_init: Failed to open file %s\n", filename);	
		new->magic = 0;
		free(new);
		return NULL;
	}

	fseek(inptr, 0, SEEK_END);
	new->size = ftell(inptr); /* record length */
	fseek(inptr, 0, SEEK_SET);

	new->buffer = malloc(new->size);
	if (new->buffer == NULL)
	{
		PRINTF("text_init: %s: Failed to alloc %d bytes to load file\n", 
			new->filename, new->size);
		new->magic = 0;
		free(new);
		return NULL;
	}	


	red = fread(new->buffer, 1, new->size, inptr);
	if (red != new->size)
	{
		PRINTF("text_init: %s: Couldnt read whole file\n", 
			new->filename);

		new->magic = 0;
		free(new->buffer);
		free(new);
		return NULL;
	}	
	fclose(inptr);	
	/* calculate a checksum for the file */
	new->buffer_checksum = calc_checksum(new->buffer, new->size);

	/* go through the buffer, reading one line at a time, creating
	 * a new structure to hold the data and adding it to the set of
	 * lines */

 	chars_left = new->size;

	current_line = malloc(sizeof(*current_line));
	if (current_line == NULL)
	{
		PRINTF("text_init: %s: failed to allocate first line\n", 
			new->filename);
		new->magic = 0;
		free(new->buffer);
		free(new);
		return NULL;
	}

	current_line->prev = NULL;
	current_line->next = NULL;
	current_line->data = new->buffer;
	current_line->lineno = 1;
	current_line->magic = LINE_MAGIC;

	new->firstline = current_line;

	bp = new->buffer;

	while (chars_left-- > 0)
	{
		// replace LFs with spaces.
		if( *bp == 0x0A ) // LF
		{
			*bp = ' ';
		}

		if (*bp == 0x0d)
		{
			/* replace the \n with a zero */
			*bp = 0;

			/* we have a new line */
			new_line = malloc(sizeof(*new_line));
			if (new_line == NULL)
			{
				PRINTF("text_init: %s: failed to allocate mem for line %d\n", 
					new->filename, new->lines);
				/* tidy up - free off the chain */
				current_line = new->firstline;
				while (current_line != NULL)
				{
					new_line = current_line->next;
					current_line->magic = 0;
					free(current_line);
					current_line = new_line;
				}
				free(new->buffer);	
				new->magic = 0;
				free(new);
				return NULL;
			}

			current_line->next = new_line;
			new_line->prev = current_line;
			current_line = new_line;

			current_line->next = NULL;
			current_line->data = bp + 1;
			current_line->lineno = ++new->lines;
			current_line->magic = LINE_MAGIC;
		}	

		bp ++;
	}

	new->current_line = new->firstline;
	new->current_inputstring = new->current_line->data;
	new->magic = TEXT_FILE_MAGIC;
}

int 
text_get_next_token(struct text_file *me, char *token, int length)
{
	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_get_lineno: invalid text_file ptr\n");
		return -1;
	}
	if (me->current_line == NULL)
	{
		PRINTF("text_get_next_token: %s: passed end of file\n",
			me->filename);
		return -2;
	}

	if (gntoken(&me->current_inputstring, token, length) != 0)
	{
		/* it was the end of the line */
		token[0] = 0;
	}

	return (strlen(token));
}	
int 
text_get_lineno(struct text_file *me)
{
	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_get_lineno: invalid text_file ptr\n");
		return -1;
	}

	return me->current_line->lineno;
}

int
text_reset_file(struct text_file *me)
{
	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_reset_file: invalid text_file ptr\n");
		exit(1);
	}	

	me->current_line = me->firstline;
	me->current_inputstring = me->current_line->data;
	return 0;
}

void
text_get_filename(struct text_file *me, char *buffer, int length)
{
	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_get_filename: invalid text_file ptr\n");
		exit(1);
	}	
	if (buffer == NULL)
	{
		PRINTF("text_get_filename: passed null pointer\n");
		exit(1);
	}

	if (strlen(me->filename) > length)
	{	
		PRINTF("text_get_filename: buffer too short for filename\n");
		return;
	}

	strcpy(buffer, me->filename);
}

int
text_set_offset(struct text_file *me, int offset)
{
	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_set_offset: invalid text_file ptr\n");
		return -1;
	}	

	if (me->current_line == NULL)
	{
		PRINTF("text_set_offset: %s: passed end of file\n",
			me->filename);
		return -2;
	}

	if (offset > strlen(me->current_line->data))
	{
		PRINTF("text_set_offset: %s: offset is longer than string\n",
			me->filename);
		return -3;
	}
		
	me->current_inputstring = me->current_line->data + offset;

	return 0;
}

int
text_get_current_line(struct text_file *me, char *buffer, int bufflen)
{

	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_get_current_line: invalid text_file ptr\n");
		return -1;
	}	

	if (me->current_line == NULL)
	{
		PRINTF("text_get_current_line: %s: passed end of file\n",
			me->filename);
		return -2;
	}

	strcpy(buffer, me->current_line->data);

	return 0;
}

int
text_move_to_next_line(struct text_file *me)
{
	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_get_current_line: invalid text_file ptr\n");
		return -1;
	}	

	me->current_line = me->current_line->next;

	if (me->current_line == NULL)
	{
		return -1;	
	}
	me->current_inputstring = me->current_line->data;
	return 0;
}	
int
text_set_line(struct text_file *me, int lineno)
{

	if ((me == NULL) || (me->magic != TEXT_FILE_MAGIC))
	{
		PRINTF("text_set_line: invalid text_file ptr\n");
		return -1;
	}	
	
	if (lineno > me->lines)
	{
		PRINTF("text_set_line: %s: line %d greater than total lines\n",
			me->filename,lineno);
		return -3;
	}

	/* XXX NYI */
}
#ifdef TESTING_TEST_C
main(int argc, char **argv)
{
	struct text_file *txt;
	char token[1024];
	char buffer[1024];

	txt = text_init(argv[1]);
	if (txt == NULL)
	{
		printf("text_init returned NULL\n");
		exit(1);
	}

	for (;;)
	{
		text_get_current_line(txt, buffer, 1024);

		printf("line %d is \"%s\"\n", text_get_lineno(txt),
			buffer);

		for (;;)
		{
			if (text_get_next_token(txt, token, 1024) <= 0) break;
			printf("token is \"%s\"\n", token);
		}
				
		if (text_move_to_next_line(txt) != 0) break;
	}
}
#endif	
