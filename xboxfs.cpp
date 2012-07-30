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
            // remove count for the '0'th entry and root dir
            usedClusters-=2;
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
        // increment to check next file
        fnum++;
    }
}
void XBoxFATX::readDirectoryTree(unsigned int startCluster)
{
    // this'll be recursive, reading each cluster, scanning for entries
    // and calling itself when it finds a directory entry
    static int dirlevel=0;
    unsigned int currentCluster=startCluster;
    unsigned int buflen=0;
    int numclusters=0;
    unsigned char* dirbuf=NULL;
    //
    while (1) {
        unsigned int clusterPos=(currentCluster-1)*bytesPerCluster;
        numclusters++;
        // make buffer the right size
        buflen=(bytesPerCluster*numclusters);
        dirbuf=(unsigned char*)realloc(dirbuf,buflen);
        if (!dirbuf) {
            perror("ReadDirectoryTree - Malloc");
            exit(1);
        }
        // read new cluster
        long int offset=(long int)dirbuf+((numclusters-1)*bytesPerCluster);
        readdata(2,clusterPos,bytesPerCluster,(void*)offset);
        if (clustermap[currentCluster]==CLUSTEREND) {
            break;
        }
        // move to next cluster
        currentCluster=clustermap[currentCluster];
    }
    unsigned char* ptr=dirbuf;
    while (ptr<(dirbuf+buflen)) {
        switch (*ptr) {
            case 0xE5: // deleted entry
                break;
            case 0xFF: // unused entry
                break;
            default: // active entry
                struct direntries entry;
                entry.namelen=*ptr;
                entry.attributes=*(ptr+1);
                memcpy(entry.name,ptr+0x02,entry.namelen);
                entry.name[entry.namelen]=0;
                entry.startCluster=be32toh(*((unsigned int*)(ptr+0x2c)));
                entry.filesize=be32toh(*((unsigned int*)(ptr+0x30)));
                entry.createDate=be16toh(*((unsigned short int*)(ptr+0x34)));
                entry.createTime=be16toh(*((unsigned short int*)(ptr+0x36)));
                entry.lwriteDate=be16toh(*((unsigned short int*)(ptr+0x38)));
                entry.lwriteTime=be16toh(*((unsigned short int*)(ptr+0x3a)));
                entry.accessDate=be16toh(*((unsigned short int*)(ptr+0x3c)));
                entry.accessTime=be16toh(*((unsigned short int*)(ptr+0x3e)));
                dirtree.push_back(entry);
                if (entry.attributes&0x10) {
                    // subdirectory?
                    fprintf(stderr,"%*s%s/\n",(dirlevel*4),"",entry.name);
                }
                else {
                    fprintf(stderr,"%*s%s\n",(dirlevel*4),"",entry.name);
                }
                if (entry.attributes&0x10) {
                    // the recursive part!  call ourself when we find a subdir
                    dirlevel++;
                    readDirectoryTree(entry.startCluster);
                    dirlevel--;
                }
                break;
        }
        ptr+=64;
    }
    free(dirbuf);
}
char* XBoxFATX::readfile(std::string filename __attribute__ ((unused)) )
{
    // FIXME: dummy value
    return (char*)malloc(1);
}
void XBoxFATX::showinfo()
{
    fprintf(stderr,"\n");
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
