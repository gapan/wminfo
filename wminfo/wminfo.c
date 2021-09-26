/*

 wminfo.c

 Version 2.5.2 (2012-01-14)
 Cezary M. Kruk <c.kruk@bigfoot.com>
 http://linux-bsd-unix.strefa.pl/

 Version 1.51 (2000-08-01)
 Robert Kling <robkli-8@student.luth.se>
 http://boombox.campus.luth.se/dockapps/

 This software is licensed through the GNU General Public License Version 3.

 For more Window Maker dockapps see: http://dockapps.org/
 and http://web.cs.mun.ca/~gstarkes/wmaker/dockapps/

*/

#define WMINFO_VERSION_NEW "2.5.2"
#define WMINFO_REVYEAR_NEW "2012"
#define WMINFO_REVDATE_NEW "2012-01-14"
#define WMINFO_VERSION_OLD "1.51"
#define WMINFO_REVYEAR_OLD "2000"
#define WMINFO_REVDATE_OLD "2000-08-01"

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

#include "../wmgeneral/wmgeneral.h"
#include "../wmgeneral/misc.h"

#include "wminfo.xpm"

// ----------------------------------------------------------------------------
// the options controlled by the switches begin here

int scroll_speed = 1;      // default: 1    allowed: from 0 upwards
int rewind_speed = 18;     // default: 18   allowed: from 0 upwards
int iso_encoding = 0;      // default: 0    allowed: 1, 2, 5, and 0 for none
char *upd_int = {"180"};   // default: 180  allowed: from 1 upwards

XpmColorSymbol colors[3] = {
	{"BackgroundColor", "#000000", 0},  // default: #000000
	{"ForegroundColor", "#bbbbbb", 0},  // default: #bbbbbb
	{NULL, NULL, 0}};                   // allowed: from #000000 to #ffffff

int scroll_ifchanged = 0;  // default: 0    allowed: 0 or 1
int keep_empty_lines = 0;  // default: 0    allowed: 0 or 1

// the options controlled by the switches end here
// ----------------------------------------------------------------------------

char wminfo_mask_bits[64*64];
int  wminfo_mask_width = 64;
int  wminfo_mask_height = 64;

char template[] = "/tmp/.wminfo.XXXXXX";
int fd;
char *temp_file = "";

#define CHAR_WIDTH 5
#define CHAR_HEIGHT 7
#define MAXNOF_LINES 1024

void BlitString(char *name, int x, int y);
void BlitNum(int num, int x, int y);
void getlines(char *plug);
void print_help();
void startup_seq();

char *lines[MAXNOF_LINES];
int  scroll[MAXNOF_LINES];
int noflines;
int visiblelines;
int longestline;

int scrolling_step[MAXNOF_LINES];
int program_passed = 0;
int something_changed = 0;
int mouse_was_here[MAXNOF_LINES];
int wait_a_minute = 0;
char *string = "";

// seconds between plugin executions
long update_interval;

int main(int argc, char **argv)
{
	int  i, m;
	int  k[MAXNOF_LINES];
	int  j[MAXNOF_LINES];
	int  len[MAXNOF_LINES];
	int  offset[MAXNOF_LINES];
	int  lineoffs = 0;
	int  scrollall = 0;
	int  c = 0;
	
	char plugin_exec[256];
	char plugin_out[256];
	
	char *plugin = {"default.wmi"};
	long update = 0;
	
	XEvent  Event;
	int but_stat = -1;
	
	opterr = 0;
	
	if (argc < 2) {
		print_help();
		return 1;
	}
	
	while ((c = getopt (argc, argv, "p:s:r:i:u:b:f:ckh")) != -1) {
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
			case 'c':
				if (scroll_ifchanged == 0) { scroll_ifchanged = 1; }
				else if (scroll_ifchanged == 1) { scroll_ifchanged = 0; }
				break;
			case 'k':
				if (keep_empty_lines == 0) { keep_empty_lines = 1; }
				else if (keep_empty_lines == 1) { keep_empty_lines = 0; }
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
	
	// scroll_speed = 20 * 1000L (will be set later);
	update_interval = 1;
	
	for (i = 0; i < MAXNOF_LINES; i++) { k[i] = 5; j[i] = 0; scroll[i] = 0; }
	
	fd = mkstemp(template);
	strcpy(plugin_exec,"sh ");
	strcat(plugin_exec,plugin);
	strcat(plugin_exec," > ");
	strcat(plugin_exec,template);
	strcpy(plugin_out,template);
	
	createXBMfromXPM(wminfo_mask_bits, wminfo_xpm, wminfo_mask_width, wminfo_mask_height);
	openXwindow(argc, argv, wminfo_xpm, wminfo_mask_bits, wminfo_mask_width, wminfo_mask_height, colors);
	
	AddMouseRegion(1, 20,  6, 50, 15);  // index, left, top, right, bottom
	AddMouseRegion(2,  5, 16, 58, 25);
	AddMouseRegion(3,  5, 26, 58, 36);
	AddMouseRegion(4,  5, 37, 58, 47);
	AddMouseRegion(5, 20, 48, 58, 58);
	
	AddMouseRegion(0,  5,  0, 19, 15);  // scroll up
	AddMouseRegion(6,  5, 48, 19, 63);  // scroll down
	AddMouseRegion(7,  50, 0, 63, 15);  // scroll all lines
	
	
	startup_seq();
	RedrawWindow();
	
	while (1)
	{
		
		if ((time(NULL) - update) >= update_interval)
		{
		
			m = system(plugin_exec);
			printf("\nCommand: \"%s\" returned %d.\n", plugin_exec, m);
			wait(NULL);
			
			if (m > 256) {
				printf("Error: plugin file \"%s\" not found.\n", plugin);
				exit(0);
			}
			
			if (program_passed == 1) {
				something_changed = 0;
				wait_a_minute = 1;
			}
			getlines(plugin_out);
			update = time(NULL); 
			for ( i = 1; i < noflines; i++ ) len[i] = 58 - strlen(lines[i]) * 6;
		}
		
		for ( i = 1; i < noflines; i++ )
		{
			if (scroll[i] == 1)
			{
				wait_a_minute = 0;
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
			
			if (program_passed == 1 && mouse_was_here[i] == 1) {
				wait_a_minute = 0;
			} else
			if (something_changed == 1) {
				wait_a_minute = 0;
			}
			
			if ( i >= lineoffs + 1 && i < lineoffs+visiblelines && wait_a_minute == 0)
				BlitString(lines[i], k[i], 11*(i-lineoffs-1) + 5);
		}
		
		if (wait_a_minute == 0) {
			RedrawWindow();
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
					
					if (mouse_was_here[i] == 0) {
						mouse_was_here[i] = 1;
					} else
					if (mouse_was_here[i] == 1) {
						mouse_was_here[i] = 0;
					}
					
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
					
					but_stat = -1;
					break;
			}
		}
		usleep(20000L);
	}
	return 0;
	
}

void print_help() {
	printf("\nwminfo %s (C) %s Cezary M. Kruk (%s)\n",WMINFO_VERSION_NEW,WMINFO_REVYEAR_NEW,WMINFO_REVDATE_NEW);
	printf("wmInfo %s  (C) %s Robert Kling   (%s)\n\n",WMINFO_VERSION_OLD,WMINFO_REVYEAR_OLD,WMINFO_REVDATE_OLD);
	printf("  Usage: wminfo -p <plugin.wmi> [-sriubfckh]\n\n");
	printf("  -s x      : scrolling speed (default: %d).\n", scroll_speed);
	printf("  -r x      : rewinding speed (default: %d).\n", rewind_speed);
	printf("  -i x      : ISO encoding: 1 for ISO-8859-1,\n");
	printf("                            2 for ISO-8859-2,\n");
	printf("                            5 for ISO-8859-5 (default: %d).\n", iso_encoding);
	printf("  -u x      : update the information every x seconds (default: %s).\n", upd_int);
	printf("  -b#rrggbb : background color (default: %s).\n", colors[0].value);
	printf("  -f#rrggbb : foreground color (default: %s).\n", colors[1].value);
	printf("  -c        : scroll changed lines (default: %d).\n", scroll_ifchanged);
	printf("  -k        : keep empty lines (default: %d).\n", keep_empty_lines);
	printf("  -h        : this help.\n\n");
}

void startup_seq() {
	BlitString("",5,5);
	BlitString("",5,48);
	BlitString("",40,5);
	BlitString("",16,16);
}

// Blits a string at given co-ordinates
void BlitString(char *name, int x, int y)
{
	int		i;
	int		c;
	int		k;
	
	k = x;
	
	copyXPMArea(60, 64, 1, 9, k-1, y);
	
	for (i=0; name[i]; i++)
	{
		c = toupper(name[i]);
		
	/* letters */
		if (c >= 'A' && c <= 'Z')
		{   // it's a letter
			c -= 'A';
			if ( k > -2) copyXPMArea(c * 6, 74, 6, 9, k, y);
			k += 6;
		} else
	/* numbers */
		if (c >= '0' && c <= '9')
		{   // it's a number
			c -= '0';
			if ( k > -2) copyXPMArea(c * 6, 64, 6, 9, k, y);
			k += 6;
		} else
	/* punctuation marks */
		if ( (c >= ' ' && c <= '/') || (c >= ':' && c <= '@') ||
		   (c >= '[' && c <= '`') || (c >= '{' && c <= '~') )
		{
		if (c == ' ') {
			if ( k > -2) copyXPMArea(60, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '!') {
			if ( k > -2) copyXPMArea(66, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '"') {
			if ( k > -2) copyXPMArea(72, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '#') {
			if ( k > -2) copyXPMArea(78, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '$') {
			if ( k > -2) copyXPMArea(84, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '%') {
			if ( k > -2) copyXPMArea(90, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '&') {
			if ( k > -2) copyXPMArea(96, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '\'') {
			if ( k > -2) copyXPMArea(102, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '(') {
			if ( k > -2) copyXPMArea(108, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == ')') {
			if ( k > -2) copyXPMArea(114, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '*') {
			if ( k > -2) copyXPMArea(120, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '+') {
			if ( k > -2) copyXPMArea(126, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == ',') {
			if ( k > -2) copyXPMArea(132, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '-') {
			if ( k > -2) copyXPMArea(138, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '.') {
			if ( k > -2) copyXPMArea(144, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '/') {
			if ( k > -2) copyXPMArea(150, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == ':') {
			if ( k > -2) copyXPMArea(156, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == ';') {
			if ( k > -2) copyXPMArea(162, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '<') {
			if ( k > -2) copyXPMArea(168, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '=') {
			if ( k > -2) copyXPMArea(174, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '>') {
			if ( k > -2) copyXPMArea(180, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '?') {
			if ( k > -2) copyXPMArea(186, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '@') {
			if ( k > -2) copyXPMArea(192, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '[') {
			if ( k > -2) copyXPMArea(198, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '\\') {
			if ( k > -2) copyXPMArea(204, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == ']') {
			if ( k > -2) copyXPMArea(210, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '^') {
			if ( k > -2) copyXPMArea(216, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '_') {
			if ( k > -2) copyXPMArea(222, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '`') {
			if ( k > -2) copyXPMArea(228, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '{') {
			if ( k > -2) copyXPMArea(234, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '|') {
			if ( k > -2) copyXPMArea(240, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '}') {
			if ( k > -2) copyXPMArea(246, 64, 6, 9, k, y);
			k += 6;
		} else
		if (c == '~') {
			if ( k > -2) copyXPMArea(252, 64, 6, 9, k, y);
			k += 6;
		}
		} else
	/* ISO-8859-1 */
		if ( iso_encoding == 1)
		{
		if (c == 228) {
			if ( k > -2) copyXPMArea(0, 84, 6, 9, k, y);	// a diaeresis
			k += 6;
		} else
		if (c == 246) {
			if ( k > -2) copyXPMArea(6, 84, 6, 9, k, y);	// o diaeresis
			k += 6;
		} else
		if (c == 252) {
			if ( k > -2) copyXPMArea(12, 84, 6, 9, k, y);	// u diaeresis
			k += 6;
		} else
		if (c == 223) {
			if ( k > -2) copyXPMArea(18, 84, 6, 9, k, y);	// sharp s
			k += 6;
		} else
		if (c == 224) {
			if ( k > -2) copyXPMArea(24, 84, 6, 9, k, y);	// a grave
			k += 6;
		} else
		if (c == 226) {
			if ( k > -2) copyXPMArea(30, 84, 6, 9, k, y);	// a circumflex
			k += 6;
		} else
		if (c == 231) {
			if ( k > -2) copyXPMArea(36, 84, 6, 9, k, y);	// c cedilla
			k += 6;
		} else
		if (c == 232) {
			if ( k > -2) copyXPMArea(42, 84, 6, 9, k, y);	// e grave
			k += 6;
		} else
		if (c == 233) {
			if ( k > -2) copyXPMArea(48, 84, 6, 9, k, y);	// e acute
			k += 6;
		} else
		if (c == 234) {
			if ( k > -2) copyXPMArea(54, 84, 6, 9, k, y);	// e circumflex
			k += 6;
		} else
		if (c == 235) {
			if ( k > -2) copyXPMArea(60, 84, 6, 9, k, y);	// e diaeresis
			k += 6;
		} else
		if (c == 238) {
			if ( k > -2) copyXPMArea(66, 84, 6, 9, k, y);	// i circumflex
			k += 6;
		} else
		if (c == 239) {
			if ( k > -2) copyXPMArea(72, 84, 6, 9, k, y);	// i diaeresis
			k += 6;
		} else
		if (c == 244) {
			if ( k > -2) copyXPMArea(78, 84, 6, 9, k, y);	// o circumflex
			k += 6;
		} else
		if (c == 249) {
			if ( k > -2) copyXPMArea(84, 84, 6, 9, k, y);	// u grave
			k += 6;
		} else
		if (c == 251) {
			if ( k > -2) copyXPMArea(90, 84, 6, 9, k, y);	// u circumflex
			k += 6;
		} else
		if (c == 255) {
			if ( k > -2) copyXPMArea(96, 84, 6, 9, k, y);	// y diaeresis
			k += 6;
		} else
		if (c == 225) {
			if ( k > -2) copyXPMArea(102, 84, 6, 9, k, y);	// a acute
			k += 6;
		} else
		if (c == 237) {
			if ( k > -2) copyXPMArea(108, 84, 6, 9, k, y);	// i acute
			k += 6;
		} else
		if (c == 243) {
			if ( k > -2) copyXPMArea(114, 84, 6, 9, k, y);	// o acute
			k += 6;
		} else
		if (c == 241) {
			if ( k > -2) copyXPMArea(120, 84, 6, 9, k, y);	// n tilde
			k += 6;
		}
		} else
	/* ISO-8859-2 */
		if ( iso_encoding == 2)
		{
		if (c == 177) {
			if ( k > -2) copyXPMArea(0, 94, 6, 9, k, y);	// a ogonek
			k += 6;
		} else
		if (c == 230) {
			if ( k > -2) copyXPMArea(6, 94, 6, 9, k, y);	// c acute
			k += 6;
		} else
		if (c == 234) {
			if ( k > -2) copyXPMArea(12, 94, 6, 9, k, y);	// e ogonek
			k += 6;
		} else
		if (c == 179) {
			if ( k > -2) copyXPMArea(18, 94, 6, 9, k, y);	// l stroke
			k += 6;
		} else
		if (c == 241) {
			if ( k > -2) copyXPMArea(24, 94, 6, 9, k, y);	// n acute
			k += 6;
		} else
		if (c == 243) {
			if ( k > -2) copyXPMArea(30, 94, 6, 9, k, y);	// o acute
			k += 6;
		} else
		if (c == 182) {
			if ( k > -2) copyXPMArea(36, 94, 6, 9, k, y);	// s acute
			k += 6;
		} else
		if (c == 188) {
			if ( k > -2) copyXPMArea(42, 94, 6, 9, k, y);	// z acute
			k += 6;
		} else
		if (c == 191) {
			if ( k > -2) copyXPMArea(48, 94, 6, 9, k, y);	// z dot above
			k += 6;
		}
		} else
	/* ISO-8859-5 */
		if ( iso_encoding == 5)
		{
		if (c >= 208 && c <= 239)
		{   // it's a Cyrillic letter
			if ( k > -2) copyXPMArea((c - 208) * 6, 104, 6, 9, k, y);
			k += 6;
		}
		} else
		{   // it's a blank or something else
			if ( k > -2) copyXPMArea(60, 64, 6, 9, k, y);
			k += 6;
		}
		
		if (k >= 58) break;
	}
	copyXPMArea(60, 64, 1, 9, k, y);
	
}

// Blits number to give coordinates.. two 0's, right justified
void BlitNum(int num, int x, int y)
{
	char buf[1024];
	int newx=x;
	
	if (num > 99 ) newx -= CHAR_WIDTH;
	
	if (num > 999) newx -= CHAR_WIDTH;
	
	sprintf(buf, "%02i", num);
	BlitString(buf, newx, y);
}

void chomp(char *s) {
	while(*s && *s != '\n' && *s != '\r') s++;
	*s = 0;
}

void getlines(char *plug)
{
	FILE *fp;
	int i = 0;
	int k = 1;
	char achar;
	char temp[1024] = {""};
	int offset = 0;
	
	longestline = 0;
	
	if ((fp = fopen(plug, "r")) == NULL)
	{
		printf("Error: plugin out-file \"%s\" not found.\n",plug);
		exit(0);
	}
	
	while (!feof(fp) && (k < 128))
	{
		do {
			fread(&achar,1,1,fp);
			temp[i++] = achar;
		} while (achar != '\n');
		
		offset = 0;
		
		temp[i] = 0;
		
		if ((strlen(temp) > 1) && (strlen(temp) < 9)) { // 9
			chomp(temp);
			offset = 8 - strlen(temp); // 8
			for (i = 0; i <= offset; i++) {
				strcat(temp, " ");
			}
			strcat(temp, "\n");
		} else {
			offset = 0;
		}
		
		lines[k] = (char *)realloc(lines[k],strlen(temp)+1);
		
		if (program_passed == 1)
			update_interval = atoi(upd_int);
		
		if (program_passed == 1 && strcmp(lines[k],temp) != 0) {
			something_changed = 1;
			wait_a_minute = 0;
		}
		
		if (program_passed == 1)
			if (scroll_ifchanged && (strcmp(lines[k],temp) != 0)) scroll[k] = 1;
		
		if ( lines[k] == NULL ) {
			printf("Error: error allocating memory for strings.\n");
			exit(0);
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
	
	fclose(fp);
	remove(plug);
}
