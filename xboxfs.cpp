#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <endian.h>

class XBoxFATX {
 public: // methods
     std::string datafilename(int which);
     void showinfo();
 public: // -structors
     XBoxFATX(char* path);
     ~XBoxFATX();
 private: // methods
     unsigned long  int getlongBE( FILE* fp,long int pos);
     unsigned       int getintBE(  FILE* fp,long int pos);
     unsigned short int getshortBE(FILE* fp,long int pos);
 private: // variables
     int bytesPerSector;        // usually 512
     int sectorsPerCluster;     // from device: 32    (0x0020)
     int bytesPerCluster;       // from device: 16384 (0x4000)
     int totalClusters;         // derived
     int usedClusters;          // derived
     int rootDirCluster;        // usually 1
     int partitionID;           // from device
     int lastfile;              // derived from device
     long int bytesPerDevice;   // huge! (256M/512M/768M...)
     std::vector<unsigned int>clustermap; // from device
     std::string deviceName;    // from device
     std::string databasename;  // "Data"
     std::string dirpath;       // derived
};
XBoxFATX::XBoxFATX(char* path)
{
    // set locale so we get thousands grouping
    setlocale(LC_ALL,"");
    // useful defaults
    bytesPerSector=512;
    databasename="Data";
    //
    // make sure dir path ends with '/'
    dirpath=std::string(path);
    if (dirpath.at(dirpath.length()-1)!='/') {
        dirpath+="/";
    }
    // check for existence and readability of Data0/1/2...
    lastfile=2;
    int fnum=0;
    while (fnum<=lastfile) {
        std::string fname=datafilename(fnum);
        FILE* fp=fopen(fname.c_str(),"rb");
        // if not found, fail
        if (fp==NULL) {
            perror(fname.c_str());
            exit(1);
        }
        // grab info while files are open
        if (fnum==0) {
            bytesPerDevice=getlongBE(fp,0x240);
            lastfile=(bytesPerDevice/(1024*1024*1024))+2;
        }
        else if (fnum==1) {
            unsigned int magic=getintBE(fp,0x0);
            // make sure it has the magic header (XTAF=FATX)
            if (magic!=0x58544146) {
                fprintf(stderr,"Magic 'XTAF' header not found\n");
            }
            partitionID=getintBE(fp,0x04);
            sectorsPerCluster=getintBE(fp,0x08);
            rootDirCluster=getintBE(fp,0x0c);
            bytesPerCluster=sectorsPerCluster*bytesPerSector;
            totalClusters=(bytesPerDevice/bytesPerCluster);
            // read in the cluster map
            usedClusters=0;
            for (int num=0; num<totalClusters; num++) {
                clustermap.push_back(getintBE(fp,(num*(sizeof(unsigned int)))+0x1000));
                if (clustermap[num]!=0) {
                    usedClusters++;
                }
            }
            // remove count for the '0'th entry, not a real cluster
            usedClusters--;
        }
        else if (fnum==2) {
            // lookup 'name.txt' to find volume name
            // char* nametxt=readfile("name.txt");
        }
        // we're not concerned with other files at the moment,
        // just that they exist
        fclose(fp);
        // increment to check next file
        fnum++;
    }
}
XBoxFATX::~XBoxFATX()
{
    // placeholder, nothing to (un)do yet
}
void XBoxFATX::showinfo()
{
    fprintf(stderr,"Partition ID: %08X\n",partitionID);
    fprintf(stderr," Device Name: '%s'\n",deviceName.c_str());
    fprintf(stderr,"  Total Size: %'luM\n",bytesPerDevice/(1024*1024));
    fprintf(stderr,"   Num files: %d (0000-%04d)\n",lastfile+1,lastfile);
    fprintf(stderr,"Cluster Size: %'uK bytes\n",bytesPerCluster/1024);
    fprintf(stderr,"  Root Dir @: %'u\n",rootDirCluster);
    fprintf(stderr,"Num Clusters: %'u\n",totalClusters);
    fprintf(stderr,"UsedClusters: %'u\n",usedClusters);
    // fprintf(stderr,"   getlongBE: %16lx\n",getlongBE(NULL,0));
    // fprintf(stderr,"    getintBE: %8x\n",getintBE(NULL,0));
    // fprintf(stderr,"  getshortBE: %4x\n",getshortBE(NULL,0));
}
unsigned long int XBoxFATX::getlongBE(FILE* fp,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned long int valueBE=0xfe0fdcbaefbeadde; // 16,045,690,984,232,390,654
    // just to make sure, we need 64 bit length here
    assert(sizeof(unsigned long int)==8);
    if ((fp)&&(!fseek(fp,pos,SEEK_SET))) {
        fread(&valueBE,sizeof(valueBE),1,fp);
    }
    return be64toh(valueBE);
}
unsigned int XBoxFATX::getintBE(FILE* fp,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned int valueBE=0xefbeadde; // 3,735,928,559
    // just to make sure, we need 32 bit length here
    assert(sizeof(unsigned int)==4);
    if ((fp)&&(!fseek(fp,pos,SEEK_SET))) {
        fread(&valueBE,sizeof(valueBE),1,fp);
    }
    return be32toh(valueBE);
}
unsigned short int XBoxFATX::getshortBE(FILE* fp,long int pos)
{
    // FIXME: dummy value, shouldn't show up
    unsigned short int valueBE=0xadde; // 57,005
    // just to make sure, we need 16 bit length here
    assert(sizeof(unsigned short int)==2);
    if ((fp)&&(!fseek(fp,pos,SEEK_SET))) {
        fread(&valueBE,sizeof(valueBE),1,fp);
    }
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
void usage()
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
int main(int argc,char* argv[])
{
    // if no options, nor a directory are provided, show help and exit
    if (argc<2) {
        // does not return
        usage();
    }
    // perform setup and initial testing
    XBoxFATX xbox=XBoxFATX(argv[1]);
    xbox.showinfo();

    return 0;
}
