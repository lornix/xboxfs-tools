#include "xboxfs.h"

XBoxFATX::~XBoxFATX()
{
    // all done! close fpread
    closeAllFiles();
}
void XBoxFATX::setDefaults()
{
    // useful defaults
    // default device name
    deviceName=DEFAULTDEVICENAME;
    deviceNameSet=false;
    // set to 'no current file'
    fp=NULL;
    currentfnum=999; // waaaay out of possible values
    //
    verbose=false;
    //
    // set locale so we get thousands grouping
    setlocale(LC_ALL,"");
}
void XBoxFATX::selectfile(unsigned int fnum,filepos_t pos)
{
    if ((fnum>=FIRSTDATAFILE)&&(pos>=ONEGIG)) {
        // xbox puts 1Gig per file
        // if position > 1GIG, fix fnum
        // integer division, no fractional part
        fnum=(unsigned int)(pos/ONEGIG)+FIRSTDATAFILE;
        // Make the position represent WITHIN the respective file
        pos=pos%ONEGIG;
    }
    if ((!fp)||(fnum!=currentfnum)) {
        // haven't accessed this file before, set up fp
        // close currently open file
        if (fp) {
            fclose(fp);
        }
        std::string fname=datafilename(fnum);
        // open for binary read/write (binary for OTHER OS's)
        fp=fopen(fname.c_str(),"rb+");
        // if not found, fail
        if (!fp) {
            perror(fname.c_str());
            exit(1);
        }
        currentfnum=fnum;
    }
    if ((fp)&&(fseek(fp,pos,SEEK_SET))) {
        perror("selectfile-fseek");
        exit(1);
    }
}
void XBoxFATX::readdata(unsigned int fnum,filepos_t pos,filepos_t len,void* buf)
{
    selectfile(fnum,pos);
    if (fread(buf,len,1,fp)!=1) {
        perror("readdata");
        exit(1);
        // if nothing read, zero out the buffer
        // memset(buf,0,len);
    }
}
void XBoxFATX::writedata(unsigned int fnum,filepos_t pos,filepos_t len,void* buf)
{
    selectfile(fnum,pos);
    if (fwrite(buf,len,1,fp)!=1) {
        perror("writedata");
        exit(1);
    }
}
void XBoxFATX::closeAllFiles()
{
    // close any open files
    if (fp) {
        fclose(fp);
    }
    // indicate 'no current open file'
    fp=NULL;
    currentfnum=999; // again, out of valid range
}
unsigned long int XBoxFATX::getlongBE(unsigned int fnum,filepos_t pos)
{
    // dummy value, shouldn't show up
    unsigned long int valueBE=0xee0fdcbaefbeadde; // 0xDEADBEEFBADC0FEE in BE order
    // just to make sure, we need 64 bit length here
    assert(sizeof(valueBE)==8);
    readdata(fnum,pos,sizeof(valueBE),&valueBE);
    return be64toh(valueBE);
}
unsigned int XBoxFATX::getintBE(unsigned int fnum,filepos_t pos)
{
    // dummy value, shouldn't show up
    unsigned int valueBE=0xefbeadde; // 0xDEADBEEF
    // just to make sure, we need 32 bit length here
    assert(sizeof(valueBE)==4);
    readdata(fnum,pos,sizeof(valueBE),&valueBE);
    return be32toh(valueBE);
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
void version()
{
    fprintf(stderr,"%s: v%s, Compiled: %s %s",PROGNAME,VERSION,__DATE__,__TIME__);
    fprintf(stderr," <%s>\n",AUTHOREMAIL);
}
void usage()
{
    version();
    fprintf(stderr,"\n");
    fprintf(stderr,"%s {DIR} [[--list|--tree|--dir] [{PATH}]] [--zero|--info] \\\n",PROGNAME);
    fprintf(stderr,"\t[--extract|--store {FNAME}] [--help|--verbose|--version]\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"  {DIR}\tDirectory containing XBox360 Data files (required)\n");
    fprintf(stderr,"\t======> and any one of the following: (*) <=======\n");
    fprintf(stderr,"   --list|l    *!List files [from {PATH}]\n");
    fprintf(stderr,"   --tree|t    *!List directory tree with files [from {PATH}]\n");
    fprintf(stderr,"   --dir|d     *!List directory tree without files [from {PATH}]\n");
    fprintf(stderr,"   --extract|x *!Extract {FNAME} from device\n");
    fprintf(stderr,"   --store|s   *!Store {FNAME} in device\n");
    fprintf(stderr,"   --info|i    * Show information about device\n");
    fprintf(stderr,"   --zero      * zero unused space (improves compression of files)\n");
    fprintf(stderr,"   * should be last option on line, cannot be used with other '*' options\n");
    fprintf(stderr,"   ! may not be working yet\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"   --help|h    This output\n");
    fprintf(stderr,"   --verbose|v Be verbose\n");
    fprintf(stderr,"   --version|V Version information\n");
    fprintf(stderr,"\n");
    exit(1);
}
