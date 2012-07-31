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
    // internal values
    fp=NULL;
    currentfnum=-1;
    //
    verbose=false;
    //
    // set locale so we get thousands grouping
    setlocale(LC_ALL,"");
}
void XBoxFATX::selectfile(int fnum,filepos_t pos)
{
    if ((fnum>=FIRSTDATAFILE)&&(pos>=ONEGIG)) {
        // xbox puts 1Gig per file
        // if position > 1GIG, fix fnum
        // integer division, no fractional part
        fnum=(pos/ONEGIG)+FIRSTDATAFILE;
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
void XBoxFATX::readdata(int fnum,filepos_t pos,filepos_t len,void* buf)
{
    selectfile(fnum,pos);
    if (fread(buf,len,1,fp)!=1) {
        perror("readdata");
        exit(1);
        // if nothing read, zero out the buffer
        // memset(buf,0,len);
    }
}
void XBoxFATX::writedata(int fnum,filepos_t pos,filepos_t len,void* buf)
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
    fp=NULL;
    currentfnum=-1;
}
unsigned long int XBoxFATX::getlongBE(int fnum,filepos_t pos)
{
    // dummy value, shouldn't show up
    unsigned long int valueBE=0xee0fdcbaefbeadde; // 0xDEADBEEFBADC0FEE in BE order
    // just to make sure, we need 64 bit length here
    assert(sizeof(valueBE)==8);
    readdata(fnum,pos,sizeof(valueBE),&valueBE);
    return be64toh(valueBE);
}
unsigned int XBoxFATX::getintBE(int fnum,filepos_t pos)
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
void XBoxFATX::version()
{
    fprintf(stderr,"%s: v%s, Compiled: %s %s",PROGNAME,VERSION,__DATE__,__TIME__);
    fprintf(stderr," <%s>\n",AUTHOREMAIL);
}
void XBoxFATX::usage()
{
    version();
    fprintf(stderr,"\n");
    fprintf(stderr,"%s: v%s, Compiled: %s %s",PROGNAME,VERSION,__DATE__,__TIME__);
    fprintf(stderr," <%s>\n",AUTHOREMAIL);
    fprintf(stderr,"\n");
    fprintf(stderr,"usage: DIR [[--list|--tree|--dir] [path]]\n");
    fprintf(stderr,"           [--extract|--store FNAME]\n");
    fprintf(stderr,"           [--verbose] [--help] [--version]\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"  DIR          Directory containing XBox360 Data files (required)\n");
    fprintf(stderr,"   --list|l    *List files [from path]\n");
    fprintf(stderr,"   --tree|t    *List directory tree with files [from path]\n");
    fprintf(stderr,"   --dir|d     *List directory tree without files [from path]\n");
    fprintf(stderr,"   --zero      zero unused space (improves compression of files)\n");
    fprintf(stderr,"   --extract|x *Extract FNAME from device\n");
    fprintf(stderr,"   --store|s   *Store FNAME in device\n");
    fprintf(stderr,"   --verbose|v Be verbose\n");
    fprintf(stderr,"   --help|h    This output\n");
    fprintf(stderr,"   --version|V Version information\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"         * possibly not working yet\n");
    fprintf(stderr,"\n");
    exit(1);
}
