/*
 *	@(#) string.h 1.5 88/10/26 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 *
 */

/*
 **	string.h -- string(3) routines
 */

#if MSDOS == 0
// extern char	*sys_errlist[];
# define strerror(err)	( sys_errlist[err] );
#endif


#ifdef	M_V7
#define	strchr	index
#define	strrchr	rindex
#endif

typedef unsigned int size_t;

#ifndef NO_PROTOTYPE
extern	char
	*strcpy( char *, char * ),
	*strncpy( char *, char *, int ),
	*strcat( char *, char * ),
	*strncat( char *, char *, int ),
	*strchr( char *, int ),
	*strrchr( char *, int ),
	*strpbrk( char *, char * ),
	*strtok( char *, char * );

extern	int
	strcmp( char *, char * ),
	strncmp( char *, char *, int ),
	strlen( char * ),
	strspn( char *, char * ),
	strcspn( char *, char * );

extern	char
	*strdup( char * );
#else
extern	char
	*strcpy(),
	*strncpy(),
	*strcat(),
	*strncat(),
	*strchr(),
	*strrchr(),
	*strpbrk(),
	*strtok();

extern	int
	strcmp(),
	strncmp(),
	strlen(),
	strspn(),
	strcspn();

extern	char
	*strdup();
#endif
