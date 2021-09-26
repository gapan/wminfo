//
// binary clock by PTrenholme from http://www.linuxquestions.org
//
// this is wminfo plugin, see: http://dockapps.windowmaker.org/file.php/id/367
//

#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

void print_help();

static char sym[]={' ','*'};

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
    short int bin[4][6];
    register short int i,j,k;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    printf("%s\n", Label);

    for (k=0;k<6;++k) {
        switch (k) {
            case 0: i=(timeinfo->tm_hour)/10;break;
            case 1: i=(timeinfo->tm_hour)%10;break;
            case 2: i=(timeinfo->tm_min)/10;break;
            case 3: i=(timeinfo->tm_min)%10;break;
            case 4: i=(timeinfo->tm_sec)/10;break;
            case 5: i=(timeinfo->tm_sec)%10;break;
        }
        for (j=0;j<4;++j) {
            bin[j][k]=(i>>(3-j))&1;
        }
    }
    for (i=0;i<4;++i) {
        for (j=0;j<6;++j) {
            printf("%c", sym[bin[i][j]]);
            if (j==1 || j==3 || j==5) printf("%s", Separator);
        }
        printf("\n");
    }
    return 0;
}

void print_help() {
        printf("\nbinclock-vertical (C) 2012 PTrenholme (http://www.linuxquestions.org)\n\n");
        printf("  -l : disable label (default: enabled)\n");
        printf("  -s : disable separator (default: enabled)\n");
        printf("  -h : print this help.\n\n");
}

