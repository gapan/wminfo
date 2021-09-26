/*

 wminfo.c 
 Version 1.51 (2000-08-01)
 
 Robert Kling (robkli-8@student.luth.se)
 http://boombox.campus.luth.se/dockapps/

 This software is licensed through the GNU General Public Lisence. 
 see http://www.BenSinclair.com/dockapp/ for more wm dock apps.

*/

#define WMINFO_VERSION "1.51"
#define WMINFO_REVDATE "2000-08-01"

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

char wminfo_mask_bits[64*64];
int  wminfo_mask_width = 64;
int  wminfo_mask_height = 64;

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

int html_parsing = 1;
int keep_spaces = 0;
int scroll_ifchanged = 0;

int main(int argc, char **argv)
{
	int	 i, m;
	int  k[MAXNOF_LINES];
	int  j[MAXNOF_LINES];
	int  len[MAXNOF_LINES];
	int	 lineoffs = 0;
	int	 scrollall = 0;
	int	 c = 0;
	
	char plugin_exec[128];
	char plugin_out[128];
	
	char *plugin = {"default"};
	char *scr_spd = {"1"};
	char *rew_spd = {"2"};
	char *upd_int = {"180"};
	long update = 0;
	long scroll_speed = 1;
	long rewind_speed = 2;    
	long update_interval;   // seconds between plugin executes	

	//FILE	*fp;
	XEvent  Event;
	int	 	but_stat = -1;
	
	opterr = 0;

	if (argc < 2) { 
		print_help();
		return 1;
	}
	
	while ((c = getopt (argc, argv, "kohs:u:p:nr:")) != -1) {
		switch (c)
		{
		
			case 'o':
				html_parsing = 0;
				break;
			case 'k':
				keep_spaces = 1;
				break;
			case 'n':
				scroll_ifchanged = 1;
				break;
			case 's':
				scroll_speed = atoi(optarg);
				break;
			case 'r':
				rewind_speed = atoi(optarg);
				break;
			case 'p':
				plugin	= optarg;
				break;
			case 'h':
				print_help();
				return 1;
			case 'u':
				upd_int = optarg;
				break;
			case '?':
				print_help();
				return 1;
			default:
				abort();
		}
	}
	
	//scroll_speed = 20 * 1000L;
	update_interval = atoi(upd_int);

	for (i = 0; i < MAXNOF_LINES; i++) { k[i] = 5; j[i] = 0; scroll[i] = 0; }
	
	strcpy(plugin_exec,"sh ");
	strcat(plugin_exec,plugin);
	strcat(plugin_exec," > wmiout.tmp");
	strcpy(plugin_out,getenv("PWD"));
	strcat(plugin_out,"/wmiout.tmp");
	
	createXBMfromXPM(wminfo_mask_bits, wminfo_xpm, wminfo_mask_width, wminfo_mask_height);
	openXwindow(argc, argv, wminfo_xpm, wminfo_mask_bits, wminfo_mask_width, wminfo_mask_height);
	
	AddMouseRegion(1, 20,  6, 50, 15);  // index, left, top, right, bottom
	AddMouseRegion(2,  5, 16, 58, 25);
	AddMouseRegion(3,  5, 26, 58, 36);
	AddMouseRegion(4,  5, 37, 58, 47);
	AddMouseRegion(5, 20, 48, 58, 58);
	
	AddMouseRegion(0,  5,  0, 19, 15);  // Scroll up
	AddMouseRegion(6,  5, 48, 19, 63);  // Scroll down
	AddMouseRegion(7,  50, 0, 63, 15);  // Scroll all lines
	
	
	startup_seq();
	RedrawWindow();
	usleep(3000000);
	
	while (1)
	{
		
		if ((time(NULL) - update) >= update_interval) 
		{  
			/*
			if ((fp = fopen(plugin_exec, "r")) == NULL)
			{ 
				printf("\nERROR: plugin-file (%s) not found.\n",plugin_exec);
				exit(0);
			}
			*/
			m = system(plugin_exec);
			//printf("\nUsing plugin: %s returned %d.\n",plugin_exec, m);
			wait(NULL);
			
			getlines(plugin_out);
			update = time(NULL); 
			for ( i = 1; i < noflines; i++ ) len[i] = 58 - strlen(lines[i]) * 6;			
		}
		
		for ( i = 1; i < noflines; i++ )
		{
			if (scroll[i] == 1)
			{
				if ((k[i] > len[i]) && (j[i] == 0)) k[i] -= scroll_speed; else 
				if ((k[i] < 5 ) && (j[i] == 1)) k[i] += rewind_speed; else
				if (k[i] <= len[i]) { j[i] = 1; } else
				if (k[i] >= 5 ) { j[i] = 0; k[i]--; }
			}
			else k[i] = 5;
			
			if ( i >= lineoffs + 1 && i < lineoffs+visiblelines )				
				BlitString(lines[i], k[i], 11*(i-lineoffs-1) + 5);
		}
		
		RedrawWindow();
		
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
							scroll[but_stat+lineoffs] = 0; else 
							scroll[but_stat+lineoffs] = 1;
					} else
						
					if ((i == 0) && (lineoffs != 0)) lineoffs--; else
					if ((i == 6) && (lineoffs < noflines-6)) lineoffs++; else
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
	printf("\nwmInfo %s (C) 2000 Robert Kling (%s)\n\n",WMINFO_VERSION,WMINFO_REVDATE);
	printf("  Usage: wminfo -p <plugin> [-suoknh]\n\n");
	printf("  -s x        : text scroll-speed (default 1).\n");
	printf("  -r x        : text \"rewind\"-speed (default 2).\n");
	printf("  -u x        : run the plugin every x seconds (default 180).\n");
	printf("  -o          : override (disable) the automatic html-tag-removal.\n");
	printf("  -k          : Keep all consequtive spaces between words, not just one.\n");
	printf("  -n          : Start scrolling lines that have changed.\n");
	printf("  -h          : display this helptext.\n\n");
}

void startup_seq() {
	BlitString("up",5,5);
	BlitString("down",5,48);
	BlitString("all",40,5);
	BlitString("",16,16);
}

// Blits a string at given co-ordinates
void BlitString(char *name, int x, int y)
{
	int		i;
	int		c;
	int		k;
	
	k = x;
	
	copyXPMArea(73,64,1,8,k-1,y);
	
	for (i=0; name[i]; i++)
	{  
		c = toupper(name[i]); 
		
		if (c >= 'A' && c <= 'Z')
		{   // its a letter
			c -= 'A';
			if ( k > -2) copyXPMArea(c * 6, 74, 6, 8, k, y);
			k += 6;
		} else
		if (c >= '0' && c<= ':')
		{   // its a number or symbol
			c -= '0';
			if ( k > -2) copyXPMArea(c * 6, 64, 6, 8, k, y);
			k += 6;
		} else
		if (c == 246) {
			if ( k > -2) copyXPMArea(0, 84, 6, 9, k, y);
			k += 6;
		} else
		if (c == 228) {
			if ( k > -2) copyXPMArea(6, 84, 6, 9, k, y);
			k += 6;
		} else
		if (c == 229) {
			if ( k > -2) copyXPMArea(12, 84, 6, 9, k, y);
			k += 6;
		} else
		if (c == '.') {
			if ( k > -2) copyXPMArea(64, 64, 6, 8, k, y);
			k += 6;
		} else
		if (c == '/') {
			if ( k > -2) copyXPMArea(68, 64, 6, 8, k, y);
			k += 6;
		} else
		if (c == '-') {
			if ( k > -2) copyXPMArea(74, 64, 6, 8, k, y);
			k += 6;
		} else
		{   // its a blank or something else
			if ( k > -2) copyXPMArea(83,64,6,8,k,y);
			k += 6;
		}
		
		if (k >= 58) break;
	}
	copyXPMArea(73,64,1,8,k,y);
	
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

void getlines(char *plug)
{
	FILE *fp;
	int i = 0, j;
	int k = 1;
	int htmlflag = 0;
	char achar;
	char *cp;
	char temp[256] = {""};
	
	longestline = 0;
	
	if ((fp = fopen(plug, "r")) == NULL)
	{ 
		printf("\nERROR: plugin out-file (%s) not found.\n",plug);
		exit(0);
	}
	
	while (!feof(fp) && ( k < 128)) 
	{
		do {
			fread(&achar,1,1,fp);
			if (html_parsing == 1) {
				if (achar == '<')  htmlflag = 1; else
				if (achar == '>')  htmlflag = 0; else
				if ((achar == ' ') && (temp[i-1] == ' ') && (keep_spaces == 0)) ; else 
				if (htmlflag == 0) temp[i++] = achar;
			} else
			if ((achar == ' ') && (temp[i-1] == ' ') && (keep_spaces == 0)) ; else temp[i++] = achar;
			
		} while (achar != '\n');
		
		temp[i] = 0;
		
		if (html_parsing == 1) 
		{
			if ((cp = strstr(temp,"&aring;")) != NULL) {
				j = 7;
				cp[0] = 'å';
				while (cp[j] != 0) cp[j-6] = cp[j++];
				cp[j-6] = 0;
			}
			
			if ((cp = strstr(temp,"&auml;")) != NULL) {
				j = 6;
				cp[0] = 'ä';
				while (cp[j] != 0) cp[j-5] = cp[j++];
				cp[j-5] = 0;
			}
			
			if ((cp = strstr(temp,"&ouml;")) != NULL) {
				j = 6;
				cp[0] = 'ö';
				while (cp[j] != 0) cp[j-5] = cp[j++];
				cp[j-5] = 0;
			}
		}

		
		
		lines[k] = (char *)realloc(lines[k],strlen(temp)+1);

		if (scroll_ifchanged && (strcmp(lines[k],temp) != 0)) scroll[k] = 1;
		
		if ( lines[k] == NULL ) {
			printf("\nERROR: error allocating memory for strings.\n");
			exit(0);
		}
		
		if (strlen(temp) > longestline) longestline = strlen(temp);
		
		strcpy(lines[k],"");
		strcpy(lines[k],temp);
		if (i > 1) k++;
		strcpy(temp,"");
		i = 0;
	}
	noflines = k;
	if (noflines > 5) visiblelines = 6; else visiblelines = noflines;
	
	fclose(fp);
	remove(plug);
}
