/*

 wminfo.c

 Version 4.1.2 (2014-01-02)

 - Cezary M. Kruk
   <c.kruk@bigfoot.com>
   http://linux-bsd-unix.strefa.pl/

 - Peter Trenholme

 - Noam Postavsky

 Version 1.51  (2000-08-01)

 - Robert Kling

 This software is licensed through the GNU General Public License Version 3.

 For more Window Maker dockapps see: http://dockapps.windowmaker.org/
 and http://web.cs.mun.ca/~gstarkes/wmaker/dockapps/

*/

#define WMINFO_VERSION_CK "4.1.2"
#define WMINFO_REVYEAR_CK "2011-2014"
#define WMINFO_REVDATE_CK "2014-01-02"

#define WMINFO_VERSION_PT "4.0.0"
#define WMINFO_REVYEAR_PT "     2012"

#define WMINFO_VERSION_NP "4.0.0"
#define WMINFO_REVYEAR_NP "     2012"

#define WMINFO_VERSION_RK "1.51"
#define WMINFO_REVYEAR_RK "     2000"
#define WMINFO_REVDATE_RK "2000-08-01"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <utmp.h>
#include <dirent.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include <errno.h>

#include "../wmgeneral/wmgeneral.h"
#include "../wmgeneral/misc.h"

#include <regex.h>

#include "wminfo.xpm"

// -----------------------------------------------------------------------------
// the options controlled by the switches begin here

int scroll_speed = 1;          // default: 1    allowed: from 0 upwards
int rewind_speed = 18;         // default: 18   allowed: from 0 upwards
int iso_encoding = 0;          // default: 0    allowed: 1, 2, 5,
                               //                        and 0 for none
char *upd_int = {"180"};       // default: 180  allowed: from 1 upwards

XpmColorSymbol colors[3] = {
	{"BackgroundColor", "#000000", 0},   // default: #000000
	{"ForegroundColor", "#c0c0c0", 0},   // default: #c0c0c0
	{NULL, NULL, 0}};                    // allowed: from #000000 to #ffffff

int timing_coefficient = 4;    // default: 4    allowed: 1, 2, 4, 8, or 16
int idle_time_base = 31250;    // the idle time base for the timing coefficient

int commove_changed_lines = 0; // default: 0    allowed: 0 or 1
int keep_empty_lines = 0;      // default: 0    allowed: 0 or 1
int verbose = 0;               // default: 0    allowed: 0 or 1

int encodings_number = 5;      // default: 5    the number of the encodings

char *week_starts = {"Mon"};   // "Mon" or "Sun"

// the options controlled by the switches end here
// -----------------------------------------------------------------------------

char wminfo_mask_bits[64*64];
int  wminfo_mask_width = 64;
int  wminfo_mask_height = 64;

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define MAXNOF_LINES 1024

void BlitString(char *name, int x, int y);
void BlitNum(int num, int x, int y);
void getlines(FILE *fp);
void print_help();
void startup_seq();
void set_temp_flag();
void put_temp_flag();
void cut_temp_flag();
void make_home_dir();

char *lines[MAXNOF_LINES];
int  scroll[MAXNOF_LINES];
int  noflines;
int  visiblelines;
int  longestline;

int  scrolling_step[MAXNOF_LINES];
int  program_passed = 0;
int  scrolling_lines = 0;
char *plugin = {"default.wmi"};

char temp_flag[1024];
char * basename (const char *fname);

char home[1024];
DIR  *mydir;
FILE *myfile;

int  idle_time_coefficient = 0;
int  idle_time = 0;

// seconds between plugin executions
long update_interval;

char timestring [35];
char *pattern = {""};
char *string = "";

void check_the_time();
void find_pattern();

int found_number = 0;
int found_code = 0;
int valid_code = 0;
int found_regex = 0;
int first_occurence = 0;
int reread_plugin = 0;

void define_regex();
void print_regex();

int predefined_regex = 33;
char *regex[33][2];

int main(int argc, char *argv[])
{
	int  i, m = 0;
	int  k[MAXNOF_LINES];
	int  j[MAXNOF_LINES];
	int  len[MAXNOF_LINES];
	int  offset[MAXNOF_LINES];
	int  lineoffs = 0;
	int  scrollall = 0;
	int  c = 0;
	int  z = 0;

	FILE *plugin_out;

	long update = 0;

	XEvent  Event;
	int but_stat = -1;

	opterr = 0;

	if (argc < 2) {
		print_help();
		return 1;
	}

	while ((c = getopt (argc, argv, "p:s:r:i:u:b:f:t:ckvh")) != -1) {
		switch (c)
		{

			case 'p':
				plugin = optarg;
				break;
			case 's':
				scroll_speed = atoi(optarg);
				break;
			case 'r':
				rewind_speed = atoi(optarg);
				break;
			case 'i':
				iso_encoding = atoi(optarg);
				break;
			case 'u':
				upd_int = optarg;
				break;
			case 'b':
				colors[0].value = optarg;
				break;
			case 'f':
				colors[1].value = optarg;
				break;
			case 't':
				idle_time_coefficient = atoi(optarg);
				break;
			case 'c':
				if (commove_changed_lines == 0) { commove_changed_lines = 1; }
				else if (commove_changed_lines == 1) { commove_changed_lines = 0; }
				break;
			case 'k':
				if (keep_empty_lines == 0) { keep_empty_lines = 1; }
				else if (keep_empty_lines == 1) { keep_empty_lines = 0; }
				break;
			case 'v':
				if (verbose == 0) { verbose = 1; }
				else if (verbose == 1) { verbose = 0; }
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
	}

	pattern = "^[0-9]+$";
	string = upd_int;
	find_pattern();

	if (found_number == 0) {
		pattern = "[0-9]+[dhms]";
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
				printf("wminfo: The code \"%s\" is not valid.\n", upd_int);
				exit(1);
			}
		}
	}

	if (found_number == 0) {
		pattern = "[SMTWF: -]";
		string = upd_int;
		find_pattern();
//		printf("Seeking regex \"%s\"\n", upd_int);
	}

	if (iso_encoding > encodings_number) {
		iso_encoding = 0;
	}

	if (idle_time_coefficient == 1) {
		timing_coefficient = 1;
	} else
	if (idle_time_coefficient == 2) {
		timing_coefficient = 2;
	} else
	if (idle_time_coefficient == 4) {
		timing_coefficient = 4;
	} else
	if (idle_time_coefficient == 8) {
		timing_coefficient = 8;
	} else
	if (idle_time_coefficient == 16) {
		timing_coefficient = 16;
	}

	idle_time = timing_coefficient * idle_time_base;

	if (found_number == 0) {
		update_interval = 1;
	} else {
		update_interval = atoi(upd_int);
	}

	for (i = 0; i < MAXNOF_LINES; i++) { k[i] = 5; j[i] = 0; scroll[i] = 0; }

	createXBMfromXPM(wminfo_mask_bits, wminfo_xpm, wminfo_mask_width, wminfo_mask_height);
	openXwindow(argc, argv, wminfo_xpm, wminfo_mask_bits, wminfo_mask_width, wminfo_mask_height, colors);

	AddMouseRegion(1, 20,  6, 50, 15);  // index, left, top, right, bottom
	AddMouseRegion(2,  5, 16, 58, 25);
	AddMouseRegion(3,  5, 26, 58, 36);
	AddMouseRegion(4,  5, 37, 58, 47);
	AddMouseRegion(5, 20, 48, 50, 58);

	AddMouseRegion(0,  5,  0, 19, 15);  // scroll down
	AddMouseRegion(6,  5, 48, 19, 63);  // scroll up
	AddMouseRegion(7,  50, 0, 63, 15);  // scroll all lines
	AddMouseRegion(8,  50,48, 63, 63);  // rerun plugin

	make_home_dir();

	startup_seq();

	RedrawWindow();

	while (1)
	{

		if ((time(NULL) - update) >= update_interval)
		{

			if (program_passed == 0) {
				put_temp_flag();
			}

			if (found_regex == 1) {
				check_the_time();
				pattern = upd_int;
				string = timestring;
//				printf("%s\n", timestring);
				find_pattern();
			} else
			if (found_number == 0) {
				printf("wminfo: It is not accepted regular expression (use \": -\" or weekday name).\n");
				exit(1);
			}

			if (found_number == 1 || reread_plugin == 1 || program_passed == 0) {
				if ( !(plugin_out = (FILE*)popen(plugin, "r")) ) {
					// if plugin_out is NULL
					perror("wminfo: Problems with popen");
					exit(1);
				}
				if (plugin_out) {
					getlines(plugin_out);
					m = pclose(plugin_out);
				}
			}

			if (verbose == 1) {
				printf("wminfo: The \"%s\" command returned %d.\n", plugin, WEXITSTATUS(m));
			}

			if (!plugin_out || (WIFEXITED(m) && WEXITSTATUS(m) == 127)) {
				printf("wminfo: The \"%s\" plugin is invalid.\n", plugin);
				exit(1);
			}

			update = time(NULL);
			for ( i = 1; i < noflines; i++ ) len[i] = 58 - strlen(lines[i]) * 6;
		}

		for ( i = 1; i < noflines; i++ )
		{
			if (scroll[i] == 1)
			{
				scrolling_lines = 1;
				if ((k[i] > len[i] - 58) && (j[i] == 0)) {
					if (rewind_speed > 0) {
						offset[i] = (len[i] - 58 - rewind_speed - 5) % rewind_speed + 1;
					} else {
						offset[i] = len[i];
					}
					if (scrolling_step[i] == 0) usleep(20000);
					k[i] -= scroll_speed;
					scrolling_step[i]++;
				} else
				if ((k[i] < 5 ) && (j[i] == 1)) {
					if (offset[i] != 0) {
						k[i] = k[i] - offset[i];
						offset[i] = 0;
					}
					k[i] += rewind_speed;
					scrolling_step[i] = 0;
				} else
				if (k[i] <= len[i]) {
					j[i] = 1;
				} else
				if (k[i] >= 5 ) {
				j[i] = 0;
				k[i] = 5;
				}
			}
			else k[i] = 5;

			if ( i >= lineoffs + 1 && i < lineoffs+visiblelines)
				BlitString(lines[i], k[i], 11*(i-lineoffs-1) + 5);
		}

		if (reread_plugin == 1 || scrolling_lines == 1) {
			RedrawWindow();
		}

		if (scrolling_lines == 0) {
			RedrawWindow();
			cut_temp_flag();
			usleep(idle_time);
		}

		// X Events
		while (XPending(display))
		{
			XNextEvent(display, &Event);
			switch (Event.type)
			{
				case Expose:
					RedrawWindow();
					break;
				case DestroyNotify:
					XCloseDisplay(display);
					exit(0);
					break;
				case ButtonPress:
					i = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y);
					but_stat = i;
					break;
				case ButtonRelease:
					i = CheckMouseRegion(Event.xbutton.x, Event.xbutton.y);

					if (but_stat == i && but_stat > 0 && but_stat < 6)
					{
						if (scroll[but_stat+lineoffs] == 1)
							scroll[but_stat+lineoffs] = 0;
						else
							scroll[but_stat+lineoffs] = 1;
					} else
					if ((i == 0) && (lineoffs != 0)) lineoffs--;
					else
					if ((i == 6) && (lineoffs < noflines-6)) lineoffs++;
					else
					if (i == 7) {
						if (scrollall == 0) {
							for ( i = 1; i < noflines; i++ ) {
								len[i] = 58 - longestline * 6;
								scroll[i] = 1;
							}
							scrollall = 1;
						} else {
							for ( i = 1; i < noflines; i++ ) {
								len[i] = 58 - strlen(lines[i]) * 6;
								scroll[i] = 0;
							}
							scrollall = 0;
						}
					}
					else
					if (i == 8) {
						update = 0 - update_interval;
						put_temp_flag();
					}

					but_stat = -1;
					break;
			}
		}

		if (scrolling_lines == 1) {
			usleep(20000);
			scrolling_lines = 0;
		}
	}
	return 0;

}

void startup_seq()
{
	BlitString("",5,5);
	BlitString("",5,48);
	BlitString("",40,5);
	BlitString("",16,16);
}

// blits a string at given co-ordinates
void BlitString(char *name, int x, int y)
{
	int i;
	int k;
        int j;

	k = x;

	copyXPMArea(0, 64, 1, 9, k-1, y);

	for (i=0; name[i]; i++, k+=6)
	{
		j = name[i];
		if (k>-2) {
			if (j<0) {
				copyXPMArea((j + 96) * 6, 64 + iso_encoding * 10, 6, 9, k, y);
			} else {
				copyXPMArea((j - 32) * 6, 64, 6, 9, k, y);
			}
		}
		copyXPMArea(0, 64, 1, 9, k, y);
	}
}

// blits number to give coordinates.. two 0's, right justified
void BlitNum(int num, int x, int y)
{
	char buf[1024];
	int newx=x;

	if (num > 99 ) newx -= CHAR_WIDTH;

	if (num > 999) newx -= CHAR_WIDTH;

	sprintf(buf, "%02i", num);
	BlitString(buf, newx, y);
}

void chomp(char *s)
{
	while(*s && *s != '\n' && *s != '\r') s++;
	*s = 0;
}

void getlines(FILE *fp)
{
	int i = 0;
	int k = 1;
	char achar;
	char temp[1024] = {""};
	int offset = 0;

	size_t result;
	int plugin_test = 0;

	longestline = 0;

	while (!feof(fp) && (k < MAXNOF_LINES))
	{
		do {
			result = fread(&achar,1,1,fp);

			if (result == 1) {
				plugin_test = 1;
			}

			if (plugin_test == 0) {
				cut_temp_flag();
				printf("wminfo: There is no \"%s\" plugin or it is invalid.\n",plugin);
				exit(1);
			}

			if (result < 0) {
				printf("wminfo: Something bad happened here.\n");
				exit(1);
			}
			temp[i++] = achar;

			if (i == 1023) {
				break;
			}

		} while (achar != '\n');

		offset = 0;

		temp[i] = 0;

		if ((strlen(temp) > 1) && (strlen(temp) < 10)) { // 10
			chomp(temp);
			offset = 10 - strlen(temp); // 10
			for (i = 0; i <= offset; i++) {
				strcat(temp, " ");
			}
			strcat(temp, "\n");
		} else {
			offset = 0;
		}

		lines[k] = (char *)realloc(lines[k],strlen(temp)+1);

		if (program_passed == 1)
			if (commove_changed_lines && (strcmp(lines[k],temp) != 0)) scroll[k] = 1;

		if ( lines[k] == NULL ) {
			printf("wminfo: Error allocating memory for strings.\n");
			exit(1);
		}

		if (strlen(temp) > longestline) longestline = strlen(temp);

		strcpy(lines[k],"");
		strcpy(lines[k],temp);

		if (keep_empty_lines)
		    k++;
		else if (i > 1)
		    k++;
		strcpy(temp,"");
		i = 0;
	}
	program_passed = 1;
	noflines = k;
	if (noflines > 5) visiblelines = 6; else visiblelines = noflines;
}

void make_home_dir()
{
	sprintf(home,"%s/.wminfo/",getenv("HOME"));
	mydir = opendir(home);
	if (mydir == NULL) {
		if (errno != ENOENT) {
			perror("wminfo: Problems with opendir");
		}
			printf("wminfo: Creating the \"%s\" directory.\n", home);
			if (mkdir(home, 0755) < 0) {
				perror("wminfo: Problems with mkdir");
				fprintf(stderr, "wminfo: Can not create the \"%s\" directory.\n", home);
				exit(1);
			}
	}
	closedir(mydir);
}

void set_temp_flag()
{
	sprintf(home,"%s/.wminfo/",getenv("HOME"));
	strcpy(temp_flag,home);
	strcat(temp_flag,".");
	strcat(temp_flag,basename(plugin));
}

void put_temp_flag()
{
	set_temp_flag();

	myfile = fopen(temp_flag, "w");
	if (myfile == NULL) {
		perror("wminfo: Problems with fopen");
		fprintf(stderr, "wminfo: Can not write the temporary \"%s\" file.\n", temp_flag);
		exit(1);
	}
	fclose(myfile);
}

void cut_temp_flag()
{
	set_temp_flag();

	myfile = fopen(temp_flag, "r");
	if (myfile == NULL) {
		return;
	}
	else {
		fclose(myfile);
		unlink(temp_flag);
	}
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
	if( reti ){ fprintf(stderr, "wminfo: Could not compile regex.\n"); exit(1); }

	// execute regular expression
	reti = regexec(&regex, string, 0, NULL, 0);
	if( !reti ){
		if (strcmp(pattern, "^[0-9]+$") == 0) {
			found_number = 1;
		}
		if (found_regex == 1 && string == timestring) {
			if (first_occurence == 0) {
//				printf("Found regex \"%s\" in the string \"%s\"\n", upd_int, string);
//				printf ("REGEX MATCHED -- WAITING UNITL IT STOPS TO MATCH.\n");
			}
			first_occurence = first_occurence + 1;
			reread_plugin = reread_plugin + 1;
		}
		if (found_regex == 0 && strcmp(pattern, "[SMTWF: -]") == 0) {
			found_regex = 1;
//			printf("Recognized regex \"%s\"\n", string);
		}
		if (found_code == 0 && strcmp(pattern, "[0-9]+[dhms]") == 0) {
			found_code = 1;
//			printf("Recognized code \"%s\"\n", string);
		}
	}
	else if( reti == REG_NOMATCH ) {
		if (first_occurence > 0) {
//			printf ("REGEX STOPPED TO MATCH -- REREADING THE REGEX.\n");
//			printf("Seeking regex \"%s\"\n", upd_int);
			first_occurence = 0;
			reread_plugin = 0;
		}
	}
	else{
		regerror(reti, &regex, msgbuf, sizeof(msgbuf));
		fprintf(stderr, "wminfo: Regex match failed: \"%s\".\n", msgbuf);
		exit(1);
	}

	// free compiled regular expression if you want to use the regex_t again
	regfree(&regex);
}

void print_help()
{
	printf("\nwminfo %s  (C) %s Cezary M. Kruk  (%s)\n",WMINFO_VERSION_CK,WMINFO_REVYEAR_CK,WMINFO_REVDATE_CK);
	printf("              (C) %s Peter Trenholme\n",WMINFO_REVYEAR_PT);
	printf("              (C) %s Noam Postavsky\n",WMINFO_REVYEAR_NP);
	printf("wmInfo %s   (C) %s Robert Kling    (%s)\n\n",WMINFO_VERSION_RK,WMINFO_REVYEAR_RK,WMINFO_REVDATE_RK);
	printf("  Usage: wminfo -p <plugin> [-sriubftckvh] [-- [-screen x] [-geometry +x+y]]\n\n");
	printf("  -p plugin : plugin to run.\n\n");
	printf("  -s x      : scrolling speed: from 0 upwards (default: %d).\n", scroll_speed);
	printf("  -r x      : rewinding speed: from 0 upwards (default: %d).\n", rewind_speed);
	printf("  -i x      : ISO encoding: 0 for none,\n");
	printf("                            1 for ISO-8859-1,\n");
	printf("                            2 for ISO-8859-2,\n");
	printf("                            5 for ISO-8859-5\n");
	printf("                            (default: %d; encodings number: %d).\n", iso_encoding, encodings_number);
	printf("  -u xxx    : updates frequency in seconds or time related regex (default: %s).\n", upd_int);
	printf("  -b#rrggbb : background color: from #000000 to #ffffff (default: %s).\n", colors[0].value);
	printf("  -f#rrggbb : foreground color: from #000000 to #ffffff (default: %s).\n", colors[1].value);
	printf("  -t        : timing coefficient: 1, 2, 4, 8, or 16 (default: %d; idle: %d).\n", timing_coefficient, idle_time_base);
	printf("  -c        : commove changed lines -- a switch (default: %d).\n", commove_changed_lines);
	printf("  -k        : keep empty lines -- a switch (default: %d).\n", keep_empty_lines);
	printf("  -v        : verbose mode -- a switch (default: %d).\n", verbose);
	printf("  -h        : help -- a trigger.\n\n");
	printf("  --             : separates options from arguments supplied to the shell:\n");
	printf("  -screen x      : x display screen number to draw the wminfo window.\n");
	printf("  -geometry +x+y : left top corner of wminfo window initial position.\n\n");
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
	printf("\nwminfo %s  (C) %s Cezary M. Kruk  (%s)\n\n",WMINFO_VERSION_CK,WMINFO_REVYEAR_CK,WMINFO_REVDATE_CK);
	printf("  Time string format:\n");
	if (strcmp(week_starts, "Mon") == 0) {
		printf("      %%a %%m-%%d %%H:%%M:%%S %%Y %%W %%j (for example: %s)\n\n", timestring);
	} else if (strcmp(week_starts, "Sun") == 0) {
		printf("      %%a %%m-%%d %%H:%%M:%%S %%Y %%U %%j (for example: %s)\n\n", timestring);
	} else {
		printf("wminfo: Invalid setting for the first day of the week: \"%s\".\n", week_starts);
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

