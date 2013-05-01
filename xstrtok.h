#ifndef XSTRTOK_H_
#define XSTRTOK_H_

//#define XSTR_QUOTES

typedef struct {
	char		*scanpoint;		/* filled in by xstrtok */
	char		*str2parse;		/* string to parse - set for first call */
	const char	*delim;			/* string of delimiters */
#ifdef XSTR_QUOTES
	int		quote;			/* respect quoting if set */
#endif
} XSTRTOK;

char * xstrtok(XSTRTOK *xinfo);

#endif