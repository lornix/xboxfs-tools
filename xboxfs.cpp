#include "xboxfs.h"

XBoxFATX::XBoxFATX(char* path)
{
    if (!path) {
        usage();
    }
    // set up initial/default values
    setDefaults();
    //
    // make sure dir path ends with '/'
    dirpath=std::string(path);
    if (dirpath.at(dirpath.length()-1)!='/') {
        dirpath+="/";
    }
    //
    // check for existence and readability of Data0/1/2...
    lastfile=2;
    unsigned int fnum=0;
    while (fnum<=lastfile) {
        // dummy read to pull file (if it exists!) into memory
        getintBE(fnum,0lu);
        // grab info while files are open
        if (fnum==0) {
            // Data0000
            // how big is this device?
            bytesPerDevice=getlongBE(fnum,OFFBYTESPERDEVICE);
            // determine how many Data???? files total (Data0002->DataXXXX)
            lastfile=(bytesPerDevice/ONEGIG)+2;
        }
        else if (fnum==1) {
            // Data0001
            // make sure it has the magic header (XTAF=FATX)
            // oddly, it's in LE order, while rest of file is BE
            if (getintBE(fnum,0x0)!=FATXMAGIC_BE) {
                fprintf(stderr,"XTAF Magic header not found\n");
            }
            // 8 hex digit partition ID
            partitionID=getintBE(fnum,0x04);
            // how big is each 'chunk'
            sectorsPerCluster=getintBE(fnum,0x08);
            // where does the root directory begin?
            rootDirCluster=getintBE(fnum,0x0c);
            // makes it easier to determine file offsets
            bytesPerCluster=sectorsPerCluster*BYTESPERSECTOR;
            totalClusters=(bytesPerDevice/bytesPerCluster);
            // read in the cluster map
            usedClusters=0;
            for (unsigned int num=0; num<totalClusters; num++) {
                filepos_t offset=(num*sizeof(unsigned int))+0x1000;
                int retval=getintBE(fnum,offset);
                clustermap.push_back(retval);
                if (clustermap[num]!=0) {
                    usedClusters++;
                }
            }
            // decrement for the '0'th entry and root dir's first (only?) cluster
            usedClusters-=2;
        }
        // increment to check next file
        fnum++;
    }
    // total file/dir counts
    countFiles=0;
    countDirs=0;
    // Read the entire directory contents into memory
    // this function is recursive.
    readDirectoryTree(rootDirCluster);
    // lookup 'name.txt' to find volume name
    wchar_t* nametxt=(wchar_t*)readfilecontents("name.txt");
    // if errors occur, just ignore them, use default deviceName
    if (nametxt) {
        filepos_t filesize=getfilesize("name.txt");
        // if not zero filesize? otherwise don't bother loading it
        if (filesize) {
            char* cbuf=(char*)malloc((filesize+1)*sizeof(char));
            if (cbuf) {
                convertUTF16(cbuf,nametxt,filesize);
                deviceName=std::string(cbuf);
                deviceNameSet=true;
                free(cbuf);
            }
        }
        // return buffer to heap
        free(nametxt);
    }
    // all done! close fp
    closeAllFiles();
}
void XBoxFATX::readClusters(unsigned int startCluster,unsigned char** buffer,filepos_t* bufferlen)
{
    unsigned int currentCluster=startCluster;
    unsigned int numclusters=0;
    filepos_t buflen=0;
    unsigned char* clusterbuf=NULL;
    //
    while (currentCluster!=CLUSTEREND) {
        filepos_t clusterPos=(currentCluster-1)*bytesPerCluster;
        numclusters++;
        // make buffer the right size
        buflen=(bytesPerCluster*numclusters);
        clusterbuf=(unsigned char*)realloc(clusterbuf,buflen);
        if (!clusterbuf) {
            perror("readClusters - Malloc");
            exit(1);
        }
        // read new cluster
        unsigned char* offset=clusterbuf+((numclusters-1)*bytesPerCluster);
        readdata(2,clusterPos,bytesPerCluster,offset);
        // move to next cluster
        currentCluster=clustermap[currentCluster];
    }
    *buffer=clusterbuf;
    *bufferlen=buflen;
}
void XBoxFATX::convertUTF16(char* outstr,wchar_t* instr,unsigned int length)
{
    char* ptr=outstr;
    unsigned int startchar=0;
    // is there a BOM?
    if (((*(unsigned short int*)instr)==0xFFFE)||((*(unsigned short int*)instr)==0xFEFF)) {
        // skip the BOM
        startchar=1;
    }
    // determine BE or LE from first two bytes (BOM)
    bool BE1orLE0=((*(unsigned short int*)instr)==0xFFFE);
    for (unsigned int i=startchar; i<(length>>1); i++) {
        unsigned short int* wcptr=(unsigned short int*)instr+i;
        wchar_t wc=(BE1orLE0)?be16toh(*wcptr):*wcptr;
        int numchr=wctomb(ptr,wc);
        if (numchr>0) {
            // if valid chars saved, bump pointer
            ptr+=numchr;
        }
        *ptr=0;
    }
}
void XBoxFATX::readDirectoryTree(unsigned int startCluster)
{
    // this'll be recursive, reading each cluster, scanning for entries
    // and calling itself when it finds a directory entry
    int nestlevel=0;
    filepos_t buflen=0;
    unsigned char* dirbuf=NULL;
    readClusters(startCluster,&dirbuf,&buflen);
    //
    unsigned char* ptr=dirbuf;
    while (ptr<(dirbuf+buflen)) {
        switch (*ptr) {
            case 0xE5: // deleted entry
                break;
            case 0xFF: // unused entry
                break;
            case 0x00: // shouldn't happen, discard if it does
                break;
            default: // active entry
                char namebuf[DIRNAMELEN+1];
                struct direntries entry;
                entry.attributes=*(ptr+1);
                strncpy(namebuf,(char*)(ptr+0x02),DIRNAMELEN);
                namebuf[*ptr]=0;
                entry.name=std::string(namebuf);
                entry.startCluster=be32toh(*((unsigned int*)(ptr+0x2c)));
                entry.filesize    =be32toh(*((unsigned int*)(ptr+0x30)));
                entry.createDate  =be16toh(*((unsigned short int*)(ptr+0x34)));
                entry.createTime  =be16toh(*((unsigned short int*)(ptr+0x36)));
                entry.lwriteDate  =be16toh(*((unsigned short int*)(ptr+0x38)));
                entry.lwriteTime  =be16toh(*((unsigned short int*)(ptr+0x3a)));
                entry.accessDate  =be16toh(*((unsigned short int*)(ptr+0x3c)));
                entry.accessTime  =be16toh(*((unsigned short int*)(ptr+0x3e)));
                entry.nestlevel   =nestlevel;
                //
                if (entry.attributes&0x10) {
                    // count number of directories found
                    countDirs++;
                }
                else {
                    // count number of files found
                    countFiles++;
                }
                // store it
                dirtree.push_back(entry);
                //
                if (entry.attributes&0x10) {
                    // the recursive part! call ourself when we find a subdir
                    nestlevel++;
                    readDirectoryTree(entry.startCluster);
                    nestlevel--;
                }
                break;
                // end of switch default block
        }
        // skip to next entry (64 bytes each)
        ptr+=64;
    }
    free(dirbuf);
}
filepos_t XBoxFATX::getfilesize(std::string filename)
{
    std::vector<struct direntries>::iterator ptr=dirtree.begin();
    while ((ptr!=dirtree.end())&&(ptr->name!=filename)) {
        ptr++;
    }
    if (ptr==dirtree.end()) {
        // all entries checked, not found
        return 0;
    }
    return ptr->filesize;
}
unsigned char* XBoxFATX::readfilecontents(std::string filename)
{
    std::vector<struct direntries>::iterator ptr=dirtree.begin();
    while ((ptr!=dirtree.end())&&(ptr->name!=filename)) {
        ptr++;
    }
    if (ptr==dirtree.end()) {
        return NULL;
    }
    unsigned char* buffer=NULL;
    filepos_t buflen=0;
    // read file data, returning malloc'd ram pointer
    readClusters(ptr->startCluster,&buffer,&buflen);
    // buflen will be multiple of cluster size, so fix it
    buflen=ptr->filesize;
    // shorten buffer length to reflect file length
    buffer=(unsigned char*)realloc(buffer,buflen);
    // user must remember to free the ram
    return buffer;
}
void XBoxFATX::showinfo()
{
    printf("\n");
    printf("  Partition ID: %08X\n",partitionID);
    printf("   Device Name: \"%s\"%s",deviceName.c_str(),deviceNameSet?"\n":" (unnamed unit)\n");
    printf("    Total Size: %'luMeg (%'.2fGig)\n",bytesPerDevice/ONEMEG,(double)bytesPerDevice/ONEGIG);
    printf("Num Data files: (0000 -> %04d) => %d\n",lastfile,lastfile+1);
    printf(" Used Clusters: %'8u (%.2f%% used)\n",usedClusters,((double)usedClusters*100.0/totalClusters));
    printf("Total Clusters: %'8u\n",totalClusters);
    printf("    File count: %'8d\n",countFiles);
    printf("     Dir count: %'8d\n",countDirs);
    // printf("Cluster Size: %'uK bytes\n",bytesPerCluster/ONEKAY);
    // printf("  Root Dir @: %'u\n",rootDirCluster);
}
void XBoxFATX::zeroClusters()
{
    char* zerobuf=(char*)malloc(bytesPerCluster);
    if (!zerobuf) {
        perror("zeroClusters");
        exit(1);
    }
    // fill with zeros
    memset(zerobuf,0,bytesPerCluster);
    if (verbose) {
        printf("Zeroing Clusters:\r");
    }
    float prevpercent=0.0;
    // find all unallocated clusters, write zeros to each cluster
    for (unsigned int i=0; i<clustermap.size(); i++) {
        if (clustermap[i]==0) {
            if (verbose) {
                float percent=floorf((float)i*1000.0/clustermap.size())/10.0;
                if (percent>prevpercent) {
                    printf("Zeroing Clusters: %4.1f%%\r",percent);
                    prevpercent=percent;
                }
            }
            filepos_t clusterPos=((filepos_t)(i-1)*(filepos_t)bytesPerCluster);
            writedata(2,clusterPos,bytesPerCluster,zerobuf);
        }
    }
    if (verbose) {
        printf("All unused clusters zeroed\n");
    }
}
int main(int argc __attribute__ ((unused)),char* argv[])
{
    // perform setup and initial testing
    XBoxFATX xbox=XBoxFATX(argv[1]);
    //
    if ((argc>2)&&(strncmp(argv[2],"--zero",7)==0)) {
        xbox.verbose=true;
        xbox.zeroClusters();
    }
    // determine options used (if any)
    xbox.showinfo();

    return 0;
}
