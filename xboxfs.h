#include <iostream>
#include <sstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <clocale>
#include <endian.h>
#include <string.h>

#define CLUSTEREND 0xFFFFFFFF
#define CLUSTERID  0xFFFFFFF8

struct direntries {
    int namelen;
    int attributes;
    char name[0x2a];
    unsigned int startCluster;
    unsigned int filesize;
    unsigned short int createDate;
    unsigned short int createTime;
    unsigned short int lwriteDate;
    unsigned short int lwriteTime;
    unsigned short int accessDate;
    unsigned short int accessTime;
};

class XBoxFATX {
 public: // methods
     std::string datafilename(int which);
     void showinfo();
     char* readfile(std::string filename);
     void usage();
     void readDirectoryTree(unsigned int startCluster);
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
     long int bytesPerDevice;   // huge! (256M/512M/768M...)
     std::string deviceName;    // from device
     std::string databasename;  // "Data"
     std::string dirpath;       // derived
     std::vector<unsigned int>clustermap;   // from device
     std::vector<struct direntries>dirtree; // from device
};
