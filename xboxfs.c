#include <stdio.h>
#include <stdlib.h>

void usage()
{
    fprintf(stderr,"%s: v%s, Compiled: %s %s\n",PROGNAME,VERSION,__DATE__,__TIME__);
    fprintf(stderr,"\n");
    fprintf(stderr,"%s: DIR [-l]\n",PROGNAME);
    fprintf(stderr,"\n");
    fprintf(stderr,"\tDIR\tDirectory containing XBox 'Data...' files (required) (try ./)\n");
    fprintf(stderr,"\t-l\tList directory tree\n");
    exit(1);
}

int main()
{
    usage();
    return 0;
}

