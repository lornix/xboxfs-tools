#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <cmath>
#include <unistd.h>
#include <endian.h>
#include <string.h>

// my info
#define AUTHORNAME  "L. R. Nix"         // it's me, Mario!
#define AUTHOREMAIL "lornix@lornix.com"
//
// clustermap code values
const unsigned int CLUSTEREND=(0xFFFFFFFFu); // value for 'end-of-chain' in clustermap
const unsigned int ROOTDIRID =(0xFFFFFFF8u); // Not used, yet
//
// useful constants, easier to visualize what's going on
const unsigned long int ONEKAY=(1024lu); // ok, so how would YOU spell it?
const unsigned long int ONEMEG=(1024*1024lu);
const unsigned long int ONEGIG=(1024*1024*1024lu);
//
const std::string  VOLNAMEFILE          = "name.txt";
const std::string  DEFAULTBASENAME      = "Data";
const std::string  DEFAULTDEVICENAME    = "Memory Unit";
const unsigned int FIRSTDATAFILE        = (2);
const unsigned long int DEFAULTFILESIZE = (ONEGIG);
const unsigned int BYTESPERSECTOR       = (512); // it could change, right?
//
// FATX magic constants
const unsigned int FATXMAGIC_LE=(0x46415458u); // "FATX"
const unsigned int FATXMAGIC_BE=(0x58544146u); // "XTAF"

// helpful defines to describe flags
const bool WITHFILES    = true;
const bool WITHOUTFILES = false;

// utility functions
void version();
void usage();

typedef unsigned long int filepos_t;

struct direntry {
    bool isdir;
    std::string name;
    filepos_t filesize;
    unsigned int nestlevel;
    unsigned int attributes;
    unsigned int startCluster;
    unsigned int parentCluster;
    unsigned short int createDate;
    unsigned short int createTime;
    unsigned short int lwriteDate;
    unsigned short int lwriteTime;
    unsigned short int accessDate;
    unsigned short int accessTime;
};

typedef struct direntry direntry_t;

class XBoxFATX {
 public: // variables
     bool verbose;              // verbose output?
 public: // methods
     void showinfo();
     const direntry_t* findFileEntry(std::string filename);
     void readDirectoryTree(unsigned int startCluster);
     void readClusters(unsigned int startCluster,unsigned char** dirbuf,filepos_t* buflen);
     void convertUTF16(char* outstr,wchar_t* instr,filepos_t len);
     void zeroClusters();
     int showtree(std::string startpath,bool showfiles);
     unsigned char* readfilecontents(std::string filename);
     filepos_t getfilesize(std::string filename);
 public: // -structors
     XBoxFATX(char* path);
     ~XBoxFATX();
 private: // variables
     unsigned int sectorsPerCluster;        // from device: 32    (0x0020)
     unsigned int bytesPerCluster;          // from device: 16384 (0x4000)
     unsigned int clustersPerFile;          // figured from device size
     unsigned int totalClusters;            // figured from device size
     unsigned int usedClusters;             // figured from cluster table
     unsigned int rootDirCluster;           // usually 1
     unsigned int partitionID;              // from device
     unsigned int countFiles;               // count of files on device
     unsigned int countDirs;                // count of dirs on device
     unsigned int lastfile;                 // number of last data file
     unsigned int currentfnum;              // which file currently open
     unsigned int nestlevel;                // depth of dir tree
     FILE* fp;                              // FILE* for current file
     filepos_t bytesPerDevice;              // huge! (256M/512M...8G/16G)
     std::string deviceName;                // read from name.txt (if found)
     bool deviceNameSet;                    // did we find name.txt to use?
     std::string dirpath;                   // where to find data files
     std::vector<unsigned int>clustermap;   // built from device
     std::vector<direntry_t>dirtree;        // built from device
 private: // methods
     unsigned long  int getlongBE( unsigned int fnum,filepos_t pos);
     unsigned       int getintBE(  unsigned int fnum,filepos_t pos);
     void  readdata(unsigned int fnum,filepos_t pos,filepos_t len,void* buf);
     void writedata(unsigned int fnum,filepos_t pos,filepos_t len,void* buf);
     void setDefaults();
     void closeAllFiles();
     void selectfile(unsigned int fnum,filepos_t pos);
     std::string datafilename(int which);
};
