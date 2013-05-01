/*
 * xstrtok.c
 * Written by D'Arcy J.M. Cain
 * based on code by Henry Spencer
 * 
 * This routine may be freely distributed as long as credit is given to D'Arcy
 * J.M. Cain, the source is included and this notice remains intact.  There is
 * specifically no restrictions on use of the program including personal or
 * commercial.  You may even charge others for this routine as long as the above
 * conditions are met.
 * 
 * This is not shareware and no registration fee is expected.  If you like the
 * program and want to support this method of distribution, write a program or
 * routine and distribute it the same way and I will feel I have been paid.
 * 
 * As for documentation, you're looking at it.
 * 
 * Because strtok is a little brain dead about multiple calls
 * this is a modified version which carries the pertinent information
 * in a structure.  This allows more than one string to be parsed
 * at the same time (the code is re-entrant.)
 *
 * Get next token from string x->s (NULL on 2nd, 3rd, etc. calls),
 * where tokens are strings separated by runs of chars from delim.
 * Writes NULs into s to end tokens.  delim need not remain constant
 * from call to call.   If quote is set then quotes (single * or double)
 * are respected.
 *
 * Note that unlike strtok(), this function returns a field for every
 * token even if the field is empty.
 *
 */

#include	<ctype.h>
#include	<string.h>
#include	<stdio.h>
#include  "xstrtok.h"

extern	char *xgetline(FILE *fp, char *buf, size_t *linenum);
char *xstrtok(XSTRTOK *xinfo);

char * xstrtok(XSTRTOK *xinfo)
{
	char		*scan;
	char		*tok;
#ifdef XSTR_QUOTES
	char		qchar = 0;
#endif
	const char	*dscan;

	/* figure out what stage we are at */
	if (xinfo->str2parse == NULL && xinfo->scanpoint == NULL)
		return(NULL);			/* last one went last time */

	if (xinfo->str2parse != NULL)
	{
		scan = xinfo->str2parse;	/* this is the first time */
		xinfo->str2parse = NULL;
	}
	else
		scan = xinfo->scanpoint;	/* we have already started */

	/* special case for space delimiter */
	if (*xinfo->delim == ' ')
		while (isspace((int) *scan))
			scan++;

	/* are we finished with the line? */
	if (*scan == '\0')
	{
		xinfo->scanpoint = NULL;
		return(*xinfo->delim == ' ' ? NULL : scan);
	}

	/* otherwise we now point to the next token */
	tok = scan;

	/* find the end of the token */
	/* three versions for speedup rather than testing within loop */

#ifdef XSTR_QUOTES
	/* first find empty string such as "" */
	if (xinfo->quote && (*tok == '\"' || *tok == '\'') && tok[0] == tok[1])
	{
		*tok = 0;
		xinfo->scanpoint = tok + 2;
		return(tok);
	}

	/* non-empty string respecting quotes */
	if (xinfo->quote) for (; *scan != '\0'; scan++)
	{
		for (dscan = xinfo->delim; *dscan != '\0';)	/* increment is in body */
		{
			/* have we found a delimiter? */
			if ((*xinfo->delim == ' ' && isspace((int) *scan)) || *scan == *dscan++)
			{
				xinfo->scanpoint = scan + 1;	/* point to next character */
				*scan = '\0';				/* terminate the token */
				return(tok);				/* and return it */
			}

			/* is it a quote character */
			if (*scan == '\'' || *scan == '\"')
			{
				qchar = *scan;				/* search for same close quote */
				strcpy(scan, scan + 1);		/* don't return quotes */

				while (*scan && *scan != qchar)
					scan++;

				strcpy(scan, scan + 1);		/* don't return quotes */
				scan--;
			}
		}
	}
	/* normal strtok semantics */
	else
#endif
	for (; *scan != '\0'; scan++)
	{
		for (dscan = xinfo->delim; *dscan != '\0';)	/* increment is in body */
		{
			/* have we found a delimiter? */
			if ((*xinfo->delim == ' ' && isspace((int) *scan)) || *scan == *dscan++)
			{
				xinfo->scanpoint = scan + 1;	/* point to next character */
				*scan = '\0';					/* terminate the token */
				return(tok);					/* and return it */
			}
		}
	}

	/* Reached end of string.  */
	xinfo->scanpoint = NULL;
	return(tok);
}