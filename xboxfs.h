#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <cmath>
#include <unistd.h>
#include <endian.h>
#include <string.h>

// my info
#define AUTHORNAME  "L. R. Nix"
#define AUTHOREMAIL "lornix@lornix.com"
// clustermap code values
#define CLUSTEREND (0xFFFFFFFFu)
#define CLUSTERID  (0xFFFFFFF8u)
// max length of directory name entry
#define DIRNAMELEN (0x2a)
// useful constants, easier to visualize what's going on
#define ONEKAY (1024lu)
#define ONEMEG (1024*1024lu)
#define ONEGIG (1024*1024*1024lu)
//
#define FIRSTDATAFILE     (2)
#define DEFAULTBASENAME   "Data"
#define DEFAULTFILESIZE   (ONEGIG)
#define DEFAULTDEVICENAME "Memory Unit"
#define BYTESPERSECTOR    (512)
#define OFFBYTESPERDEVICE (0x240)
#define FATXMAGIC_LE      (0x46415458lu) // "FATX"
#define FATXMAGIC_BE      (0x58544146lu) // "XTAF"

typedef unsigned long int filepos_t;

struct direntries {
    int attributes;
    std::string name;
    unsigned int startCluster;
    filepos_t filesize;
    unsigned short int createDate;
    unsigned short int createTime;
    unsigned short int lwriteDate;
    unsigned short int lwriteTime;
    unsigned short int accessDate;
    unsigned short int accessTime;
    int nestlevel;
};

class XBoxFATX {
 public: // variables
     bool verbose;              // verbose output?
 public: // methods
     std::string datafilename(int which);
     void showinfo();
     void usage();
     void readDirectoryTree(unsigned int startCluster);
     void readClusters(unsigned int startCluster,unsigned char** dirbuf,filepos_t* buflen);
     void convertUTF16(char* outstr,wchar_t* instr,unsigned int len);
     void zeroClusters();
     filepos_t getfilesize(std::string filename);
     unsigned char* readfilecontents(std::string filename);
 public: // -structors
     XBoxFATX(char* path);
     ~XBoxFATX();
 private: // methods
     unsigned long  int getlongBE( int fnum,filepos_t pos);
     unsigned       int getintBE(  int fnum,filepos_t pos);
     // unsigned short int getshortBE(int fnum,filepos_t pos);
     void  readdata(int fnum,filepos_t pos,filepos_t len,void* buf);
     void writedata(int fnum,filepos_t pos,filepos_t len,void* buf);
     void closeAllFiles();
     void setDefaults();
     void selectfile(int fnum,filepos_t pos);
 private: // variables
     unsigned int sectorsPerCluster;        // from device: 32    (0x0020)
     unsigned int bytesPerCluster;          // from device: 16384 (0x4000)
     unsigned int clustersPerFile;          // derived
     unsigned int totalClusters;            // derived
     unsigned int usedClusters;             // derived
     unsigned int rootDirCluster;           // usually 1
     unsigned int partitionID;              // from device
     unsigned int countFiles;               // derived
     unsigned int countDirs;                // derived
     unsigned int lastfile;                 // derived from device
     int lastfnum;                          // internal use
     FILE* fp;                              // internal use
     unsigned long int bytesPerDevice;      // huge! (256M/512M...8G/16G)
     std::string deviceName;                // from device
     bool deviceNameSet;                    // internal use
     std::string dirpath;                   // supplied by user
     std::vector<unsigned int>clustermap;   // from device
     std::vector<struct direntries>dirtree; // from device
};
