#include <stdio.h>
#include "text.h"
#include <malloc.h>

#define MAX_ADDRESS (1024)
#define OUTFILE "out.hex"

#define PRINTF 

#define PASS_0 	(0xaaaa0001)
#define PASS_1	(0xaaaa0002)

struct Instruction
{
	char name[16];
	int data;
	char *(*function)(char *, int);
};

void report_fatal_error(char *message);

char *byte_ins_f_d(char *name, int opcode);
char *byte_ins_f(char *name, int opcode);
char *bit_ins_f_b(char *name, int opcode);
char *no_param_ins(char *name, int opcode);
char *byte_literal_ins(char *name, int opcode);
char *address_ins(char *name, int opcode);
char *tris_ins(char *name, int opcode);

#define NUM_INSTRUCTIONS (37)
struct Instruction instructions[NUM_INSTRUCTIONS] =
{
	{"addwf", 0x0700, byte_ins_f_d },
	{"andwf", 0x0500, byte_ins_f_d },
	{"comf", 0x0900, byte_ins_f_d },
	{"decf", 0x0300, byte_ins_f_d },
	{"decfsz", 0x0b00, byte_ins_f_d },
	{"incf", 0x0a00, byte_ins_f_d },
	{"incfsz", 0x0f00, byte_ins_f_d },
	{"iorwf", 0x0400, byte_ins_f_d },
	{"movf", 0x0800, byte_ins_f_d },
	{"rlf", 0x0d00, byte_ins_f_d },
	{"rrf", 0x0c00, byte_ins_f_d },
	{"subwf", 0x0200, byte_ins_f_d },
	{"swapf", 0x0e00, byte_ins_f_d },
	{"xorwf", 0x0600, byte_ins_f_d },
	{"bcf", 0x1000, bit_ins_f_b},
	{"bsf", 0x1400, bit_ins_f_b},
	{"btfsc", 0x1800, bit_ins_f_b},
	{"btfss", 0x1c00, bit_ins_f_b},
	{"clrw", 0x0100, no_param_ins},
	{"nop", 0x0000, no_param_ins},
	{"clrwdt", 0x0064, no_param_ins},
	{"retfie", 0x0009, no_param_ins},
	{"return", 0x0008, no_param_ins},
	{"sleep", 0x0063, no_param_ins},
	{"option", 0x0062, no_param_ins},
	{"addlw", 0x3e00, byte_literal_ins},
	{"andlw", 0x3900, byte_literal_ins},
	{"iorlw", 0x3800, byte_literal_ins},
	{"movlw", 0x3000, byte_literal_ins},
	{"retlw", 0x3400, byte_literal_ins},
	{"sublw", 0x3c00, byte_literal_ins},
	{"xorlw", 0x3a00, byte_literal_ins},
	{"call", 0x2000, address_ins},
	{"goto", 0x2800, address_ins},
	{"clrf", 0x0100, byte_ins_f},
	{"movwf", 0x0000, byte_ins_f},
	{"tris", 0x0060, tris_ins}
};


struct labels
{
	struct label *firstlabel;
	int count;
};

struct label
{
	struct label *next;
	char title[64];
	int address;
};

struct Asm_Global
{
	int pass;
	char filename[1024];
	struct text_file *src;
	int current_address;
	struct labels *labels;
	unsigned short output[MAX_ADDRESS];
}asm_global;

int 
search_table(char *token)
{
	int i;
	char * ret;

	if (token[0] == 0)
	{
		/* it was blank, dont complain */
		return 0;
	}

	for (i = 0 ; i < NUM_INSTRUCTIONS ; i++)
	{
		if (strcmp(token, instructions[i].name) == 0)
		{
			/* weve got it */
			ret = instructions[i].function(token, 
				instructions[i].data);
			if (ret != NULL)
			{	
				report_fatal_error(ret);
			}
			return 0;
		}
	}
	return -1;
}

void
output(unsigned short data)
{
	if (asm_global.current_address > MAX_ADDRESS)
	{
		report_fatal_error("code past maximum address");
	}
	asm_global.output[asm_global.current_address] = data;

	asm_global.current_address ++;
}

int 
get_numeric(char *buffer)
{
	/* XXX fix! */
	if (isalpha(buffer[0]))
	{
		return -1;
	}

	if (strlen(buffer) == 0) return -1;	

	/* see if its a hex number */
	if (strncmp(buffer, "0x",2) == 0)
	{
		/* its a hex number */
		report_fatal_error("Hex number not yet supposed!");
	}
	/* otherwise presume its a dec number */
	return atoi(buffer);
}

char *
address_ins(char *name, int opcode)
{
	int k;
	unsigned short value = 0;
	char buffer[1024];

	text_get_next_token(asm_global.src, buffer, 1024);
	
	k = label_resolve(asm_global.labels, buffer);

	if ((k == -1) && (asm_global.pass == PASS_1))
	{
		/* unknown label */
		return "label not found";
	}

	value = opcode;
	value |= k;

	PRINTF("%s: output is %04x\n",
		name, value );

	output(value);

	return NULL;
}

char *
byte_literal_ins(char *name, int opcode)
{
	int k;
	unsigned short value = 0;
	char buffer[1024];

	text_get_next_token(asm_global.src, buffer, 1024);
	if ((k = get_numeric(buffer)) == -1)
	{
		/* I want a numeric value here */
		return "syntax error: param 1 expected numeric";
	}

	if (k > 0xff)
	{
		/* This is too long */
		return "erorr: param 1 numeric too large";
	}

	value = opcode;
	value |= k;

	PRINTF("%s: data is %04x, k is %d, output is %04x\n",
		name, opcode, k, value );
	output(value);

	return NULL;
}

char *
bit_ins_f_b(char *name, int opcode)
{
	int f;
	int b;
	unsigned short value = 0;
	char buffer[1024];

	text_get_next_token(asm_global.src, buffer, 1024);
	if ((f = get_numeric(buffer)) == -1)
	{
		/* I want a numeric value here */
		return "syntax error: param 1 expected numeric";
	}

	if (f > 0x7f)
	{
		/* This is too long */
		return "erorr: param 1 numeric too large";
	}

	text_get_next_token(asm_global.src, buffer, 1024);

	if (strcmp(buffer, ",") != 0)
	{
		/* I want a comma here */
		return "syntax error: expected comma";
	}
	text_get_next_token(asm_global.src, buffer, 1024);

	if ((b = get_numeric(buffer)) == -1)
	{
		/* I want a numeric value here */
		return "syntax error: param 2 expected numeric";
	}

	if (b > 7)
	{
		/* number too big for a bit number */
		return "error: param 2 too large for bit address";
	}
	value = opcode;
	value |= (b << 7);
	value |= (f & 0x7f);
	PRINTF("%s: data is %04x, b is %d, f is %04x, output is %04x\n",
		name, opcode, b, f, value );
	output(value);

	return NULL;
}

char * 
no_param_ins(char *name, int opcode)
{
	PRINTF("%s: output is %04x\n",
		name, opcode);
	output((unsigned short)opcode);
	return NULL;
}
char *
tris_ins(char *name, int opcode)
{
	int f;
	char buffer[1024];
	unsigned short value = 0;

	text_get_next_token(asm_global.src, buffer, 1024);
	if ((f = get_numeric(buffer)) == -1)
	{
		/* I want a numeric value here */
		return "syntax error: param 1 expected numeric";
	}

	if (f > 7)
	{
		/* This is too long */
		return "erorr: param 1 numeric too large";
	}

	value = opcode;
	value |= f;
	PRINTF("%s: output is %04x\n",
		name, value );
	output(value);

	return NULL;
}
	
char *
byte_ins_f(char *name, int opcode)
{
	int f;
	int d = 1;
	char buffer[1024];
	unsigned short value = 0;

	text_get_next_token(asm_global.src, buffer, 1024);
	if ((f = get_numeric(buffer)) == -1)
	{
		/* I want a numeric value here */
		return "syntax error: param 1 expected numeric";
	}

	if (f > 0x7f)
	{
		/* This is too long */
		return "erorr: param 1 numeric too large";
	}

	value = opcode;
	value |= (d << 7);
	value |= (f & 0x7f);
	PRINTF("%s: data is %04x, d is %d, f is %04x, output is %04x\n",
		name, opcode, d, f, value );
	output(value);

	return NULL;
}

char * 
byte_ins_f_d(char *name, int opcode)
{	
	int f;
	int d;
	int ok = 0;
	char buffer[1024];
	unsigned short value = 0;

	text_get_next_token(asm_global.src, buffer, 1024);
	if ((f = get_numeric(buffer)) == -1)
	{
		/* I want a numeric value here */
		return "syntax error: param 1 expected numeric";
	}

	if (f > 0x7f)
	{
		/* This is too long */
		return "erorr: param 1 numeric too large";
	}

	text_get_next_token(asm_global.src, buffer, 1024);

	if (strcmp(buffer, ",") != 0)
	{
		/* I want a comma here */
		return "syntax error: expected comma";
	}
	text_get_next_token(asm_global.src, buffer, 1024);

	if (strcmp(buffer, "f") == 0)
	{
		ok = 1;
		d = 1;
	}
	if (strcmp(buffer, "w") == 0)
	{
		ok = 1;
		d = 0;
	}
	if (ok == 0)
	{
		/* wanted an f or a w */
		return "syntax error: param 2 expected 'w' or 'f'";
	}
	value = opcode;
	value |= (d << 7);
	value |= (f & 0x7f);
	PRINTF("%s: data is %04x, d is %d, f is %04x, output is %04x\n",
		name, opcode, d, f, value );
	output(value);
	
	return NULL;
}

struct labels *
label_init()
{
	struct labels *new;

	new = calloc(sizeof (*new), 1);

	if (new == NULL)
	{
		printf("label_init: cant allocate labels structure\n");
		exit(1);
	}
	return new;
}

void
label_dump(struct labels *me)
{
	struct label *current;

	printf("There are %d labels\n", me->count);
	current = me->firstlabel;

	while (current != NULL)
	{
		printf("label \"%s\" at address %d\n", 
			current->title, current->address);
		current = current->next;
	}
	printf("dump complete\n");
}

void
report_fatal_error(char *message)
{
	char buffer[1024];
	char fname[1024];

	text_get_filename(asm_global.src, fname, 1024);

	sprintf(buffer, "asm: %s: line %d: %s\n",	
		fname, text_get_lineno(asm_global.src), message);
	printf("%s", buffer);
	
	exit(1);
}


int
label_resolve(struct labels *me, char* title)
{
	struct label *current;

	if (me == NULL)
	{
		printf("label_resolve: passed NULL labels structure\n");	
		exit(1);
	}
	if (title == NULL)
	{
		printf("label_resolve: passed NULL labels structure\n");
		exit(1);
	}	
	current = me->firstlabel;

	while (current != NULL)
	{
		if (strcmp(title, current->title) == 0)
		{
			/* found the label */
			return current->address;
		}
		current = current->next;
	}

	/* otherwise, it wasnt found */
	return -1;
}

int 
label_define(struct labels *me, char* title, int address)
{
	struct label *new;

	if (me == NULL)
	{
		printf("label_define: passed NULL labels structure\n");	
		exit(1);
	}

	if (title == NULL)
	{
		printf("label_define: passed NULL labels structure\n");
		exit(1);
	}	
	
	/* see if its already defined */
	if (label_resolve(me, title) != -1)	
	{
		/* its been done before */
		return -1;
	}

	new = malloc(sizeof(*new));
	if (new == NULL)
	{
		printf("label_define: malloc failed for new label\n");
		exit(1);
	}	

	new->next = me->firstlabel;
	strcpy(new->title, title);	
	new->address = address;

	/* if (me->firstlabel != NULL) me->firstlabel->next = new;	*/
	me->firstlabel = new;
	me->count ++;

	return 0;
}

int
asm_init()
{	
	memset(&asm_global, 0, sizeof(asm_global));
	asm_global.pass = PASS_0;

	asm_global.labels = label_init();

	if (asm_global.labels == NULL)
	{
		return -1;
	}

}

	
void
pass(int passno)
{
	int ret = 0;
	char buffer[1024];
	char first[1024];

	asm_global.pass = passno;

	/* start at the top of the file */
	text_reset_file(asm_global.src);

	/* start at the beginning of our address space */	
	asm_global.current_address = 0;

	for (;;)
	{
		if (text_get_next_token(asm_global.src, buffer, 1024) <= 0)
		{
			/* end of line */	
			ret = text_move_to_next_line(asm_global.src);

			if (ret < 0) 
			{
				break;
			}
			continue;
		}

		if (strcmp(buffer, "end") == 0)
		{
			/* dont do any more */
			break;
		}
		
		/* otherwise... */

		/* see if its a label, by seeing if the second token
		 * in the line is ':'  */
		strcpy(first, buffer);	
		text_get_next_token(asm_global.src, buffer, 1024);
		if (strcmp(buffer, ":") == 0)
		{
			/* it was a label */
			if (asm_global.pass == PASS_0)
			{
				/* dont define labels in pass 1 */
				ret = label_define(asm_global.labels, 
					first, asm_global.current_address);
				PRINTF("adding label %s at address %d\n", 
					first, asm_global.current_address);	
				if (ret == -1)
				{
					/* label defined already */
					
					report_fatal_error(
						"duplicate label");
				}
			}
		}
		else
		{
			/* we need to reset the line position */
			text_set_offset(asm_global.src, 0);
		}
		/* now read in buffer again */	
		text_get_next_token(asm_global.src, buffer, 1024);
		if (strcmp(buffer, ";") == 0)
		{
			/* weve got an end of line */	
			ret = text_move_to_next_line(asm_global.src);

			if (ret < 0) break;
			continue;
		}
			

	
		if (search_table(buffer) == -1)
		{
			/* it wasnt a bad instruction */
			report_fatal_error("invalid instruction");
		}

		/* now check to see if there crap at the end of the line */
		if (text_get_next_token(asm_global.src, buffer, 1024) > 0)
		{
			/* the only acceptable thing is a ; */
			if (strcmp(buffer, ";") != 0)
			{
				/* this is an error */
				report_fatal_error("garbage at end of line\n");
			}
			ret = text_move_to_next_line(asm_global.src);
			if (ret < 0) break;
		}
	}	
}	

void
write_data_irec(unsigned short *data, int length ,int address, FILE *outptr)
{
	int i;
	int total = 0;

	fprintf(outptr, ":%02X", (length * 2) & 0xff);
 	total += ((length * 2) & 0xff);
	/* total += ((length * 2) >> 8); */

	address *= 2;

	fprintf(outptr, "%04X", address);
 	total += (address & 0xff);
	total += (address >> 8);
	/* write the type */
	fprintf(outptr, "00");

	for (i = 0 ; i < length ; i ++)
	{
		fprintf(outptr, "%02X%02X",
			data[i] & 0xff, data[i] >> 8);
		total += (data[i] & 0xff);
		total += (data[i] >> 8);
	}
	printf("total is %04x\n", total	);
	fprintf(outptr, "%02X\n", (256 - (total & 0xff) & 0xff));
}
void
write_end_irec(FILE *outptr)
{
	fprintf(outptr, ":00000001FF\n");
}
void
dump_output()
{	
	FILE *outptr;
	int addr = 0;

	outptr = fopen(OUTFILE, "w");
	if (outptr == NULL)
	{
		report_fatal_error("Cannot open out.hex for writing");
	}
	
	for (;;)
	{
		write_data_irec(&asm_global.output[addr], 8, addr, outptr);
		addr += 8;
		if(addr > asm_global.current_address) break;
	}
		
	write_end_irec(outptr);
}

main(int argc, char **argv)
{
	int ret = 0;
	char buffer[1024];

	asm_init();
	asm_global.src = text_init(argv[1]);
	if (asm_global.src == NULL)
	{
		printf("Couldnt load file\n");
		exit(1);
	}

	pass(PASS_0);
	printf("asm: pass 0 complete\n");
	pass(PASS_1);
	dump_output();
	printf("asm: pass 1 complete\n");
}
	
		

