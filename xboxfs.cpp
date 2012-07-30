#include "xboxfs.h"

XBoxFATX::XBoxFATX(char* path)
{
    if (!path) {
        usage();
    }
    // useful defaults
    bytesPerSector=512;
    databasename="Data";
    //
    // set locale so we get thousands grouping
    setlocale(LC_ALL,"");
    //
    // make sure dir path ends with '/'
    dirpath=std::string(path);
    if (dirpath.at(dirpath.length()-1)!='/') {
        dirpath+="/";
    }
    //
    // check for existence and readability of Data0/1/2...
    lastfile=2;
    int fnum=0;
    while (fnum<=lastfile) {
        std::string fname=datafilename(fnum);
        fp=fopen(fname.c_str(),"rb");
        // if not found, fail
        if (fp==NULL) {
            perror(fname.c_str());
            exit(1);
        }
        // grab info while files are open
        if (fnum==0) {
            bytesPerDevice=getlongBE(fnum,0x240);
            // determine how many files total (Data0002->DataXXXX)
            lastfile=(bytesPerDevice/(1024*1024*1024))+2;
        }
        else if (fnum==1) {
            // make sure it has the magic header (XTAF=FATX)
            if (getintBE(fnum,0x0)!=0x58544146) {
                fprintf(stderr,"Magic 'XTAF' header not found\n");
            }
            partitionID=getintBE(fnum,0x04);
            sectorsPerCluster=getintBE(fnum,0x08);
            rootDirCluster=getintBE(fnum,0x0c);
            bytesPerCluster=sectorsPerCluster*bytesPerSector;
            totalClusters=(bytesPerDevice/bytesPerCluster);
            // read in the cluster map
            usedClusters=0;
            for (int num=0; num<totalClusters; num++) {
                clustermap.push_back(getintBE(fnum,num*(sizeof(unsigned int))+0x1000));
                if (clustermap[num]!=0) {
                    usedClusters++;
                }
            }
            // remove count for the '0'th entry, not a real cluster
            usedClusters--;
        }
        else if (fnum==2) {
            // Read the entire directory contents into memory
            readDirectoryTree(rootDirCluster);
            // lookup 'name.txt' to find volume name
            // char* nametxt=readfile("name.txt");
            // if (!nametxt) {
            //     fprintf(stderr,"Unable to find volume name (name.txt)\n");
            //     exit(1);
            // }
        }
        // we're not concerned with other files at the moment,
        // just that they exist
        fclose(fp);
        fp=NULL;
        // increment to check next file
        fnum++;
    }
}
void XBoxFATX::readDirectoryTree(int startCluster)
{
    // this'll be recursive, reading each cluster, scanning for entries
    // and calling itself when it finds a directory entry
    char* dirbuf=(char*)malloc(bytesPerCluster);
    if (!dirbuf) {
        perror("ReadDirectoryTree - Malloc");
        exit(1);
    }
    unsigned int clusterPos=(startCluster-1)*bytesPerCluster;
    readdata(2,clusterPos,bytesPerCluster,dirbuf);

}
char* XBoxFATX::readfile(std::string filename __attribute__ ((unused)) )
{
    // FIXME: dummy value
    return (char*)malloc(1);
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
int main(int argc __attribute__ ((unused)),char* argv[])
{
    // perform setup and initial testing
    XBoxFATX xbox=XBoxFATX(argv[1]);
    xbox.showinfo();

    return 0;
}
