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
     char* readfile(std::string filename);
     void usage();
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
