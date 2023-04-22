#include "dependencies.h"

FILE *fp = NULL;
int ticks = 0;
int shellargs = 2;

void writeLogs(char *logs){
    if (shellargs==2){
        FILE *fp = fopen("log", "a");
        fprintf(fp, "%s",logs);
        fclose(fp);
    }
    else if (shellargs==3){
        FILE *fp = fopen("schedlog", "a");
        fprintf(fp, "%s",logs);
        fclose(fp);
    }
    
    return;
}