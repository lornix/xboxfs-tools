#include "xboxfs.h"

XBoxFATX::~XBoxFATX()
{
    // all done! close fp
    closeAllFiles();
}
void XBoxFATX::setDefaults()
{
    // useful defaults
    // default device name
    deviceName=DEFAULTDEVICENAME;
    deviceNameSet=false;
    // internal values
    lastfnum=-1;
    fp=NULL;
    //
    // set locale so we get thousands grouping
    setlocale(LC_ALL,"");
}
void XBoxFATX::readdata(int fnum,long int pos,int len,void* buf)
{
    if ((fnum>=2)&&(pos>=ONEGIG)) {
        // xbox puts 1Gig per file
        // if position > 1GIG, fix fnum
        // integer division, no fractional part
        fnum=(pos/ONEGIG)+2;
        // Make the position represent WITHIN the respective file
        pos=pos%ONEGIG;
    }
    if ((!fp)||(fnum!=lastfnum)) {
        // haven't read this file before, set up fp
        // close currently open file
        if (fp) {
            fclose(fp);
        }
        std::string fname=datafilename(fnum);
        fp=fopen(fname.c_str(),"rb");
        // if not found, fail
        if (fp==NULL) {
            perror(fname.c_str());
            exit(1);
        }
        lastfnum=fnum;
    }
    // zero out the buffer, prevents data leakage
    memset(buf,0,len);
    if ((fp)&&(!fseek(fp,pos,SEEK_SET))) {
        fread(buf,len,1,fp);
    }
}
void XBoxFATX::closeAllFiles()
{
    // close any open files
    if (fp) {
        fclose(fp);
    }
    fp=NULL;
    lastfnum=-1;
}
unsigned long int XBoxFATX::getlongBE(int fnum,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned long int valueBE=0xee0fdcbaefbeadde; // 0xDEADBEEFBADC0FEE in BE order
    // just to make sure, we need 64 bit length here
    assert(sizeof(unsigned long int)==8);
    readdata(fnum,pos,sizeof(unsigned long int),&valueBE);
    return be64toh(valueBE);
}
unsigned int XBoxFATX::getintBE(int fnum,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned int valueBE=0xefbeadde; // 0xDEADBEEF
    // just to make sure, we need 32 bit length here
    assert(sizeof(unsigned int)==4);
    readdata(fnum,pos,sizeof(unsigned int),&valueBE);
    return be32toh(valueBE);
}
unsigned short int XBoxFATX::getshortBE(int fnum,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned short int valueBE=0xadde; // 0xDEAD
    // just to make sure, we need 16 bit length here
    assert(sizeof(unsigned short int)==2);
    readdata(fnum,pos,sizeof(unsigned short int),&valueBE);
    return be16toh(valueBE);
}
std::string XBoxFATX::datafilename(int which)
{
    std::stringstream fname;
    fname << dirpath << DEFAULTBASENAME;
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
