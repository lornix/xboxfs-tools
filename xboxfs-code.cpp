#include "xboxfs.h"

XBoxFATX::~XBoxFATX()
{
    // placeholder, nothing to (un)do yet
}
void XBoxFATX::readdata(int fnum,long int pos,int len,void* buf)
{
    if ((fnum>=2)&&(pos>=(1024*1024*1024))) {
        // if position > 1GIG, make sure fnum is right
        // xbox puts 1Gig per file
        fnum=(pos/(1024*1024*1024))+2;
    }
    if (fnum!=lastfnum) {
        // haven't read this file before, set up fp
        // close currently open file
        if (fp) {
            fclose(fp);
        }
        std::string fname=datafilename(fnum);
        fp=fopen(fname.c_str(),"rb");
        lastfnum=fnum;
    }
    fprintf(stderr,"readdata: %d: (%d) %04lx\n",fnum,len,pos);
    if ((fp)&&(!fseek(fp,pos,SEEK_SET))) {
        int retval=fread(buf,len,1,fp);
        fprintf(stderr,"fread result: %d\n",retval);
    }
}
unsigned long int XBoxFATX::getlongBE(int fnum,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned long int valueBE=0xfe0fdcbaefbeadde; // 16,045,690,984,232,390,654
    // just to make sure, we need 64 bit length here
    assert(sizeof(unsigned long int)==8);
    readdata(fnum,pos,sizeof(unsigned long int),&valueBE);
    return be64toh(valueBE);
}
unsigned int XBoxFATX::getintBE(int fnum,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned int valueBE=0xefbeadde; // 3,735,928,559
    // just to make sure, we need 32 bit length here
    assert(sizeof(unsigned int)==4);
    readdata(fnum,pos,sizeof(unsigned int),&valueBE);
    return be32toh(valueBE);
}
unsigned short int XBoxFATX::getshortBE(int fnum,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned short int valueBE=0xadde; // 57,005
    // just to make sure, we need 16 bit length here
    assert(sizeof(unsigned short int)==2);
    readdata(fnum,pos,sizeof(unsigned short int),&valueBE);
    return be16toh(valueBE);
}
std::string XBoxFATX::datafilename(int which)
{
    std::stringstream fname;
    fname << dirpath << databasename;
    fname.width(4);
    fname.fill('0');
    fname << which;
    return fname.str();
}
void XBoxFATX::usage()
{
    fprintf(stderr,"%s: v%s, Compiled: %s %s\n",PROGNAME,VERSION,__DATE__,__TIME__);
    fprintf(stderr,"\n");
    fprintf(stderr,"usage: DIR [-l]\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"\tDIR\tDirectory containing XBox360 'Data\?\?\?\?' files (required) (try ./)\n");
    fprintf(stderr,"\t -l\tList directory tree (default if no options present)\n");
    fprintf(stderr,"\n");
    exit(1);
}
