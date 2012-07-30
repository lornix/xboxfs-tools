#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <unistd.h>
#include <endian.h>
#include <string.h>

// clustermap code values
#define CLUSTEREND 0xFFFFFFFF
#define CLUSTERID  0xFFFFFFF8
// max length of directory name entry
#define DIRNAMELEN (0x2a)

struct direntries {
    int namelen;
    int attributes;
    std::string name; // max of 0x2a (42) characters
    unsigned int startCluster;
    unsigned int filesize;
    unsigned short int createDate;
    unsigned short int createTime;
    unsigned short int lwriteDate;
    unsigned short int lwriteTime;
    unsigned short int accessDate;
    unsigned short int accessTime;
    int nestlevel;
};

class XBoxFATX {
 public: // methods
     std::string datafilename(int which);
     void showinfo();
     unsigned char* readfilecontents(std::string filename);
     unsigned int getfilesize(std::string filename);
     void usage();
     void readDirectoryTree(unsigned int startCluster);
     void readClusters(unsigned int startCluster,unsigned char** dirbuf,unsigned int* buflen);
     void convertUTF16(char* outstr,wchar_t* instr,unsigned int len);
 public: // -structors
     XBoxFATX(char* path);
     ~XBoxFATX();
 private: // methods
     unsigned long  int getlongBE( int fnum,long int pos);
     unsigned       int getintBE(  int fnum,long int pos);
     unsigned short int getshortBE(int fnum,long int pos);
     void readdata(int fnum,long int pos,int len,void* buf);
 private: // variables
     FILE* fp;                  // internal use
     int bytesPerSector;        // usually 512
     int sectorsPerCluster;     // from device: 32    (0x0020)
     int bytesPerCluster;       // from device: 16384 (0x4000)
     int totalClusters;         // derived
     int usedClusters;          // derived
     int rootDirCluster;        // usually 1
     int partitionID;           // from device
     int lastfile;              // derived from device
     int countFiles;            // derived
     int countDirs;             // derived
     long int bytesPerDevice;   // huge! (256M/512M/768M...)
     std::string deviceName;    // from device
     std::string databasename;  // "Data"
     std::string dirpath;       // derived
     std::vector<unsigned int>clustermap;   // from device
     std::vector<struct direntries>dirtree; // from device
};
