/**--------------------------------------------------------------------------------
 *
 * pgreshape -- Embed the new column according to the desired position in any table
 * in the postgresql database.
 *
 * Copyright (c) 2019, Rafael Garcia Sagastume
 *
 *---------------------------------------------------------------------------------
 */

#include "common.h"

#if PG_VERSION_NUM >= 90600
#include "common/keywords.h"
#else
#include "parser/keywords.h"
#endif

#if PG_VERSION_NUM < 90600
#define	PG_KEYWORD(a,b,c) {a,0,c},

const ScanKeyword PQLScanKeywords[] =
{
#include "parser/kwlist.h"
};

const int NumPQLScanKeywords = lengthof(PQLScanKeywords);
#endif


/*credits to @euler on quarrel for generating the function*/
char * formatObjectIdentifier(char *s) {
	bool	need_quotes = false;

	char	*p;
	char	*ret;
	int		i = 0;

	/* different rule for first character */
	if (!((s[0] >= 'a' && s[0] <= 'z') || s[0] == '_'))
		need_quotes = true;
	else
	{
		/* otherwise check the entire string */
		for (p = s; *p; p++)
		{
			if (!((*p >= 'a' && *p <= 'z')
					|| (*p >= '0' && *p <= '9')
					|| (*p == '_')))
			{
				need_quotes = true;
				break;
			}
		}
	}

	if (!need_quotes)
	{
#if PG_VERSION_NUM >= 120000
		int kwnum = ScanKeywordLookup(s, &ScanKeywords);

		if (kwnum >= 0 && ScanKeywordCategories[kwnum] != UNRESERVED_KEYWORD)
			need_quotes = true;
#elif PG_VERSION_NUM >= 90600
		const ScanKeyword *keyword = ScanKeywordLookup(s,
														 ScanKeywords,
														 NumScanKeywords);

		if (keyword != NULL && keyword->category != UNRESERVED_KEYWORD)
			need_quotes = true;
#else
		const ScanKeyword *keyword = ScanKeywordLookup(s,
														 PQLScanKeywords,
														 NumPQLScanKeywords);

		if (keyword != NULL && keyword->category != UNRESERVED_KEYWORD)
			need_quotes = true;
#endif
	}

	if (!need_quotes)
	{
		/* no quotes needed */
		ret = strdup(s);
	}
	else
	{
		/*
		 * Maximum length for identifiers is NAMEDATALEN (64 bytes by default
		 * including trailing zero byte).
		 * */
		ret = malloc(NAMEDATALEN * sizeof(char));
		if (ret == NULL)
		{
			printf("could not allocate memory");
			exit(EXIT_FAILURE);
		}

		ret[i++] = '\"';
		for (p = s; *p; p++)
		{
			/* per SQL99, if quote is found, add another quote */
			if (*p == '\"')
				ret[i++] = '\"';
			ret[i++] = *p;
		}
		ret[i++] = '\"';
		ret[i] = '\0';
	}

	return ret;
}




