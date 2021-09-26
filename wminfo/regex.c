/*

 regex.c

 Version 1.0.0 (2013-03-04)

 Cezary M. Kruk
 <c.kruk@bigfoot.com>
 http://linux-bsd-unix.strefa.pl/

 The program based on the code from http://www.peope.net/old/regex.html.

 This software is licensed through the GNU General Public License Version 3.

*/

#define REGEX_VERSION "1.0.0"
#define REGEX_REVDATE "2013-03-04"

#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <regex.h>

char *week_starts = {"Mon"};	// "Mon" or "Sun"

char *upd_int = {"180"};
void print_help();
void print_regex();

char timestring [35];
char *pattern = {""};
char *string = "";

void check_the_time();
void find_pattern();
void define_regex();

int found_regex = 0;
int found_code = 0;
int valid_code = 0;
int first_occurence = 0;

int predefined_regex = 33;
char *regex[33][2];

int main(int argc, char **argv)
{

	int  c = 0;
	int  z = 0;

	if (argc < 2) {
		print_help();
		return 1;
	}

	while ((c = getopt (argc, argv, "u:h")) != -1) {
		switch (c)
		{

			case 'u':
				upd_int = optarg;
				break;
			case 'h':
				print_help();
				return 1;
			case '?':
				print_help();
				return 1;
			default:
				abort();
		}
	}

if (strcmp(upd_int, "?") == 0) {
	print_regex();
	return 1;
}

if (strcmp(upd_int, "1s") == 0) {
	upd_int = "1";
	printf("That code matches every second.\n");
	return 1;
}

	while (1)
	{
		if (found_code == 0 && found_regex == 0) {
			pattern = "[dhms]";
			string = upd_int;
			find_pattern();
			if (found_code == 1) {
				define_regex();
				for ( z = 1; z <= predefined_regex; z++ ) {
					if (strcmp(regex[z][0], upd_int) == 0) {
						upd_int = regex[z][1];
						valid_code = 1;
					}
				}
				if (valid_code == 0) {
					printf("The code \"%s\" is not valid.\n", upd_int);
					exit(1);
				}
			}
		}

		if (found_regex == 0) {
			pattern = "[SMTWF: -]";
			string = upd_int;
			find_pattern();
		printf("Seeking regex \"%s\"\n", upd_int);
		}

		if (found_regex == 1 || found_code == 1) {
			check_the_time();
			pattern = upd_int;
			string = timestring;
			printf("%s\n", timestring);
			find_pattern();
		} else {
			printf("It is not accepted regular expression (use \": -\" or weekday name).\n");
			exit(1);
		}
		usleep(1000000);
	}
	return 0;
}

void check_the_time()
{
	time_t rawtime;
	struct tm * timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	if (strcmp(week_starts, "Mon") == 0) {
		strftime (timestring,35,"%a %m-%d %H:%M:%S %Y %W %j",timeinfo);
	} else if (strcmp(week_starts, "Sun") == 0) {
		strftime (timestring,35,"%a %m-%d %H:%M:%S %Y %U %j",timeinfo);
	} else {
		printf("Invalid setting for the first day of the week: \"%s\".\n", week_starts);
		exit(1);
	}
}

void find_pattern()
{
	regex_t regex;
	int reti;
	char msgbuf[100];

	reti = regcomp(&regex, pattern, REG_EXTENDED);
	if( reti ){ fprintf(stderr, "Could not compile the regex.\n"); exit(1); }

	// execute regular expression
	reti = regexec(&regex, string, 0, NULL, 0);
	if( !reti ){
		if ((found_regex == 1 || found_code == 1) && string == timestring) {
			if (first_occurence == 0) {
				printf("Found regex \"%s\" in the string \"%s\"\n", upd_int, string);
				printf ("REGEX MATCHED -- WAITING UNITL IT STOPS TO MATCH.\n");
			}
			first_occurence = 1;
		}
		if (found_regex == 0 && strcmp(pattern, "[SMTWF: -]") == 0) {
			found_regex = 1;
			printf("Recognized regex \"%s\"\n", string);
		}
		if (found_code == 0 && strcmp(pattern, "[dhms]") == 0) {
			found_code = 1;
			printf("Recognized code \"%s\"\n", string);
		}
	}
	else if( reti == REG_NOMATCH ) {
				if (first_occurence == 1) {
				printf ("REGEX STOPPED TO MATCH -- SEEKING THE REGEX.\n");
				printf("Seeking regex \"%s\"\n", upd_int);
			}
			first_occurence = 0;
	}
	else{
		regerror(reti, &regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "Regex match failed: \"%s\".\n", msgbuf);
		exit(1);
	}

	// free compiled regular expression if you want to use the regex_t again
	regfree(&regex);
}

void print_help()
{
	check_the_time();
	printf("\nregex %s  (C) Cezary M. Kruk  (%s)\n\n",REGEX_VERSION,REGEX_REVDATE);
	printf("  Usage:\n");
	printf("      regex -u [<regular_expression>|<code>]\n\n");
	printf("  Display codes:\n");
	printf("      regex -u \"?\"\n\n");
	printf("  Time string format:\n");
	if (strcmp(week_starts, "Mon") == 0) {
		printf("      %%a %%m-%%d %%H:%%M:%%S %%Y %%W %%j (for example: %s)\n\n", timestring);
	} else if (strcmp(week_starts, "Sun") == 0) {
		printf("      %%a %%m-%%d %%H:%%M:%%S %%Y %%U %%j (for example: %s)\n\n", timestring);
	} else {
		printf("Invalid setting for the first day of the week: \"%s\".\n", week_starts);
		exit(1);
	}
	printf("  Sample regular expressions:\n");
	printf("      :..:.[05]             every 5 seconds\n");
	printf("      :..:.0                every 10 seconds\n");
	printf("      \":..:([03]0|[14]5)\"   every 15 seconds\n");
	printf("      :..:[03]0             every 30 seconds\n");
	printf("      :..:00                every minute\n");
	printf("      :.[05]:00             every 5 minutes\n");
	printf("      :.0:00                every 10 minutes\n");
	printf("      \":([03]0|[14]5):00\"   every 15 minutes\n");
	printf("      :[03]0:00             every 30 minutes\n");
	printf("      :00:00                every hour\n");
	printf("      .[02468]:00:00        every 2 hours\n");
	printf("      00:00:00              everyday\n\n");
	printf("  Sample commands using timestring script (see: timestring.wmi plugin):\n");
	printf("    every weekday (from Monday to Friday):\n");
	printf("      wminfo -p 'timestring.wmi \"[MTWF]\"' -u 24h\n");
	printf("    two times during the weekend (Saturday and Sunday):\n");
	printf("      wminfo -p 'timestring.wmi \"S\"' -u 24h\n");
	printf("    every two days:\n");
	printf("      wminfo -p 'timestring.wmi \" [0-3][0-9][02468]$\"' -u 24h\n");
	printf("      wminfo -p 'timestring.wmi \" [0-3][0-9][13579]$\"' -u 24h\n");
	printf("    every week:\n");
	printf("      wminfo -p 'timestring.wmi \"%s\"' -u 24h\n", week_starts);
	printf("    every two weeks:\n");
	printf("      wminfo -p 'timestring.wmi \"%s.* [0-5][02468] \"' -u 24h\n", week_starts);
	printf("      wminfo -p 'timestring.wmi \"%s.* [0-5][13579] \"' -u 24h\n", week_starts);
	printf("    twice a month:\n");
	printf("      wminfo -p 'timestring.wmi \"(-01|-15)\"' -u 24h\n");
	printf("    every four weeks:\n");
	printf("      wminfo -p 'timestring.wmi \"%s.* ([024][048]|[135][26]) \"' -u 24h\n", week_starts);
	printf("    once a month:\n");
	printf("      wminfo -p 'timestring.wmi \"\\-01\"' -u 24h\n");
	printf("    every two months:\n");
	printf("      wminfo -p 'timestring.wmi \"[01][02468]-01\"' -u 24h\n");
	printf("      wminfo -p 'timestring.wmi \"[01][13579]-01\"' -u 24h\n");
	printf("    every Friday the 13th:\n");
	printf("      wminfo -p 'timestring.wmi \"Fri ..-13\"' -u 24h\n\n");
}

void define_regex()
{
	char *regular_expressions[] = {
	  "1s", "it is an alias for the update interval equal 1 second",
	  "2s", ":..:.[02468]",
	  "3s", ":..:([03][0369]|[14][258]|[25][147])",
	  "4s", ":..:([024][048]|[135][26])",
	  "5s", ":..:.[05]",
	  "6s", ":..:([03][06]|[14][28]|[25]4)",
	  "8s", ":.([02468]:([04][08]|[15]6|24|32)|[13579]:([04]4|[15]2|2[08]|36))",
	 "10s", ":..:.0",
	 "12s", ":..:(00|12|24|36|48)",
	 "15s", ":..:([03]0|[14]5)",
	 "20s", ":..:[024]0",
	 "30s", ":..:[03]0",
	 "60s", ":..:00",
	  "1m", ":..:00",
	  "2m", ":.[02468]:00",
	  "3m", ":([03][0369]|[14][258]|[25][147]):00",
	  "4m", ":([024][048]|[135][26]):00",
	  "5m", ":.[05]:00",
	  "6m", ":([03][06]|[14][28]|[25]4):00",
	 "10m", ":.0:00",
	 "12m", ":(00|12|24|36|48):00",
	 "15m", ":([03]0|[14]5):00",
	 "20m", ":[024]0:00",
	 "30m", ":[03]0:00",
	 "60m", ":00:00",
	  "1h", ":00:00",
	  "2h", ".[02468]:00:00",
	  "3h", "(0[0369]|1[258]|21):00:00",
	  "4h", "(0[048]|1[26]|20):00:00",
	  "6h", "(0[06]|1[28]):00:00",
	  "8h", "(0[08]|16):00:00",
	 "12h", "(00|12):00:00",
	 "24h", "00:00:00",
	  "1d", "00:00:00"
	};

	int r = 0;

	for ( r = 0; r <= predefined_regex; r++ ) {
		regex[r][0] = regular_expressions[r * 2];
		regex[r][1] = regular_expressions[r * 2 + 1];
	}
}

void print_regex()
{
	check_the_time();
	define_regex();
	printf("\nregex %s  (C) Cezary M. Kruk  (%s)\n\n",REGEX_VERSION,REGEX_REVDATE);
	printf("  Time string format:\n");
	if (strcmp(week_starts, "Mon") == 0) {
		printf("      %%a %%m-%%d %%H:%%M:%%S %%Y %%W %%j (for example: %s)\n\n", timestring);
	} else if (strcmp(week_starts, "Sun") == 0) {
		printf("      %%a %%m-%%d %%H:%%M:%%S %%Y %%U %%j (for example: %s)\n\n", timestring);
	} else {
		printf("Invalid setting for the first day of the week: \"%s\".\n", week_starts);
		exit(1);
	}
	printf("  Predefined regular expressions:\n");
	printf("    code | regex\n");
	printf("    -----+--------------------------------------------------------------------\n");

	int r = 0;
	char *space = "";

	for ( r = 0; r <= predefined_regex; r++ ) {
		if (strlen(regex[r][0]) == 2) {
			space = " ";
		} else {
			space = "";
		}
		printf("     %s%s | \"%s\"\n", space, regex[r][0], regex[r][1]);
	}
	printf("\n");
}

