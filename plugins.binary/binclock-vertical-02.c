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
  register short int i;
  char sym[]={' ','*'};

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  printf("%s\n", Label);

// Since an hour is <25, a minute is <60, and second is <61, Hl[0]=Ml[0]=Sl[0]=' ', so don't look up (or compute) those
  i=(timeinfo->tm_hour)/10;
//bin[0][0]=(i>>3)&(short int) 1;
  bin[1][0]=(i>>2)&(short int) 1;
  bin[2][0]=(i>>1)&(short int) 1;
  bin[3][0]=i&(short int) 1;
  i=(timeinfo->tm_hour)%10;
  bin[0][1]=(i>>3)&(short int) 1;
  bin[1][1]=(i>>2)&(short int) 1;
  bin[2][1]=(i>>1)&(short int) 1;
  bin[3][1]=i&(short int) 1;
  i=(timeinfo->tm_min)/10;
//bin[0][2]=(i>>3)&(short int) 1;
  bin[1][2]=(i>>2)&(short int) 1;
  bin[2][2]=(i>>1)&(short int) 1;
  bin[3][2]=i&(short int) 1;
  i=(timeinfo->tm_min)%10;
  bin[0][3]=(i>>3)&(short int) 1;
  bin[1][3]=(i>>2)&(short int) 1;
  bin[2][3]=(i>>1)&(short int) 1;
  bin[3][3]=i&(short int) 1;
  i=(timeinfo->tm_sec)/10;
//bin[0][4]=(i>>3)&(short int) 1;
  bin[1][4]=(i>>2)&(short int) 1;
  bin[2][4]=(i>>1)&(short int) 1;
  bin[3][4]=i&(short int) 1;
  i=(timeinfo->tm_sec)%10;
  bin[0][5]=(i>>3)&(short int) 1;
  bin[1][5]=(i>>2)&(short int) 1;
  bin[2][5]=(i>>1)&(short int) 1;
  bin[3][5]=i&(short int) 1;
// And, because those fields are always blank, use a format just prints that.
  printf(" %c%s %c%s %c%s\n",                  sym[bin[0][1]],Separator,               sym[bin[0][3]],Separator,               sym[bin[0][5]],Separator);
  printf("%c%c%s%c%c%s%c%c%s\n",sym[bin[1][0]],sym[bin[1][1]],Separator,sym[bin[1][2]],sym[bin[1][3]],Separator,sym[bin[1][4]],sym[bin[1][5]],Separator);
  printf("%c%c%s%c%c%s%c%c%s\n",sym[bin[2][0]],sym[bin[2][1]],Separator,sym[bin[2][2]],sym[bin[2][3]],Separator,sym[bin[2][4]],sym[bin[2][5]],Separator);
  printf("%c%c%s%c%c%s%c%c%s\n",sym[bin[3][0]],sym[bin[3][1]],Separator,sym[bin[3][2]],sym[bin[3][3]],Separator,sym[bin[3][4]],sym[bin[3][5]],Separator);
  return 0;
}

void print_help() {
        printf("\nbinclock-vertical (C) 2012 PTrenholme (http://www.linuxquestions.org)\n\n");
        printf("  -l : disable label (default: enabled)\n");
        printf("  -s : disable separator (default: enabled)\n");
        printf("  -h : print this help.\n\n");
}

