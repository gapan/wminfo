//
// binary clock by H_TeXMeX_H from http://www.linuxquestions.org
//
// this is wminfo plugin, see: http://dockapps.windowmaker.org/file.php/id/367
//

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

void print_help();

int main (int argc, char **argv)
{
    char *Label = "HH MM SS";
    char *Separator ="|";
    int label = 1;
    int separator = 1;
    int c;

    while ((c = getopt (argc, argv, "lsh")) != -1)
    switch (c)
        {
            case 'l':
                label = 0;
                break;
            case 's':
                separator = 0;
                break;
            case 'h':
                print_help();
                return 0;
            default:
                return 1;
       }

    if (label == 0) {
        Label = "         ";
    }

    if (separator == 0) {
        Separator = " ";
    }

    time_t rawtime;
    struct tm * timeinfo;

        time ( &rawtime );
        timeinfo = localtime ( &rawtime );

        printf("%s\n %c%s %c%s %c%s\n %c%s%c%c%s%c%c%s\n%c%c%s%c%c%s%c%c%s\n%c%c%s%c%c%s%c%c%s\n", 

        Label, 
        ((timeinfo->tm_hour % 10) & 8) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_min  % 10) & 8) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_sec  % 10) & 8) ? '*' : ' ', 
        Separator, 

        ((timeinfo->tm_hour % 10) & 4) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_min  / 10) & 4) ? '*' : ' ', 
        ((timeinfo->tm_min  % 10) & 4) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_sec  / 10) & 4) ? '*' : ' ', 
        ((timeinfo->tm_sec  % 10) & 4) ? '*' : ' ', 
        Separator, 

        ((timeinfo->tm_hour / 10) & 2) ? '*' : ' ', 
        ((timeinfo->tm_hour % 10) & 2) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_min  / 10) & 2) ? '*' : ' ', 
        ((timeinfo->tm_min  % 10) & 2) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_sec  / 10) & 2) ? '*' : ' ', 
        ((timeinfo->tm_sec  % 10) & 2) ? '*' : ' ', 
        Separator, 

        ((timeinfo->tm_hour / 10) & 1) ? '*' : ' ', 
        ((timeinfo->tm_hour % 10) & 1) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_min  / 10) & 1) ? '*' : ' ', 
        ((timeinfo->tm_min  % 10) & 1) ? '*' : ' ', 
        Separator, 
        ((timeinfo->tm_sec  / 10) & 1) ? '*' : ' ', 
        ((timeinfo->tm_sec  % 10) & 1) ? '*' : ' ', 
        Separator);

    return 0;
} 

void print_help() {
        printf("\nbinclock-vertical (C) 2012 H_TeXMeX_H (http://www.linuxquestions.org)\n\n");
        printf("  -l : disable label (default: enabled)\n");
        printf("  -s : disable separator (default: enabled)\n");
        printf("  -h : print this help.\n\n");
} 

