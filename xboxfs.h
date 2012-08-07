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
// clustermap code values
#define CLUSTEREND (0xFFFFFFFFu)        // value for 'end-of-chain' in clustermap
#define ROOTDIRID  (0xFFFFFFF8u)        // Not used, yet
// useful constants, easier to visualize what's going on
#define ONEKAY           (1024lu)       // ok, so how would YOU spell it?
#define ONEMEG      (1024*1024lu)
#define ONEGIG (1024*1024*1024lu)
//
#define VOLNAMEFILE       "name.txt"
#define FIRSTDATAFILE     (2)
#define DEFAULTBASENAME   "Data"
#define DEFAULTFILESIZE   (ONEGIG)
#define DEFAULTDEVICENAME "Memory Unit"
#define BYTESPERSECTOR    (512)         // it could change, right?
#define FATXMAGIC_LE      (0x46415458u) // "FATX"
#define FATXMAGIC_BE      (0x58544146u) // "XTAF"

// helpful defines to describe flags
#define WITHFILES    (true)
#define WITHOUTFILES (false)

// utility functions
void version();
void usage();

typedef unsigned long int filepos_t;

struct direntry {
    int nestlevel;
    int attributes;
    bool isdir;
    std::string name;
    unsigned int startCluster;
    unsigned int parentCluster;
    filepos_t filesize;
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
     direntry_t* findFileEntry(std::string filename);
     void readDirectoryTree(unsigned int startCluster);
     void readClusters(unsigned int startCluster,unsigned char** dirbuf,filepos_t* buflen);
     void convertUTF16(char* outstr,wchar_t* instr,unsigned int len);
     void zeroClusters();
     void showtree(std::string startpath,bool showfiles);
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
     int currentfnum;                       // which file currently open
     FILE* fp;                              // FILE* for current file
     filepos_t bytesPerDevice;              // huge! (256M/512M...8G/16G)
     std::string deviceName;                // read from name.txt (if found)
     bool deviceNameSet;                    // did we find name.txt to use?
     std::string dirpath;                   // where to find data files
     std::vector<unsigned int>clustermap;   // built from device
     std::vector<direntry_t>dirtree;        // built from device
 private: // methods
     unsigned long  int getlongBE( int fnum,filepos_t pos);
     unsigned       int getintBE(  int fnum,filepos_t pos);
     void  readdata(int fnum,filepos_t pos,filepos_t len,void* buf);
     void writedata(int fnum,filepos_t pos,filepos_t len,void* buf);
     void setDefaults();
     void closeAllFiles();
     void selectfile(int fnum,filepos_t pos);
     std::string datafilename(int which);
};
