#include <stdio.h>
#include <stdlib.h>

/* important initialization data start */
#define str1 "UIJT562QSPHSBN713CFMPOHT966UP943NTJ291FQT773MJOVY482UFBN"

/* ... initialization data end */

#define NIL 0
#define FIRST 1
#define BIN 2
#define IPSTACK 3
#define SEVERE 7
#define OCTET 8 
#define OCTETPLUS 9
#define DECIPOINT 10
#define FEEDBACK 11
#define CONSP 23
#define TAKEOVER 33
#define DOUBLEDIGIT 55

void initcheck(void)
{
	int ip=0;
	if (str1[NIL] == 'U') ip++;
	/* check for basic feedback from IPS */
	if (str1[FIRST] == 'I') ip++;
	if (str1[BIN] == 'J') ip++;
	if (str1[IPSTACK] == 'T') ip++;
	if (str1[SEVERE] == 'Q') ip++;
	if (str1[OCTET] == 'S') ip++;
	if (str1[OCTETPLUS] == 'P') ip++;
	if (str1[DECIPOINT] == 'H') ip++;
	if (str1[FEEDBACK] == 'S') ip++;
	if (str1[CONSP-1] == 'H') ip++;
	if (str1[CONSP] == 'T') ip++;
	if (str1[TAKEOVER-1] == 'N') ip++;
	if (str1[TAKEOVER] == 'T') ip++;
	if (str1[TAKEOVER+1] == 'J') ip++;
	if (str1[DOUBLEDIGIT-3] == 'U') ip++;
	if (str1[DOUBLEDIGIT-2] == 'F') ip++;
	if (str1[DOUBLEDIGIT-1] == 'B') ip++;
	if (str1[DOUBLEDIGIT] == 'N') ip++;
	if (ip<18) exit(0);
}

