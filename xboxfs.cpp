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
    lastfile=FIRSTDATAFILE;
    unsigned int fnum=0;
    while (fnum<=lastfile) {
        // dummy read to pull file (if it exists!) into memory
        // exits with error if doesn't exist
        getintBE(fnum,0lu);
        // grab info while files are open
        if (fnum==0) {
            // Data0000
            // how big is this device?
            bytesPerDevice=getlongBE(fnum,0x240);
            // determine how many Data???? files total (Data0002->DataXXXX)
            lastfile=(bytesPerDevice/DEFAULTFILESIZE)+FIRSTDATAFILE;
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
            bytesPerCluster=(sectorsPerCluster*BYTESPERSECTOR);
            totalClusters=(bytesPerDevice/bytesPerCluster);
            clustersPerFile=(DEFAULTFILESIZE/bytesPerCluster);
            // read in the cluster map
            usedClusters=0;
            for (unsigned int num=0; num<(totalClusters+1); num++) {
                filepos_t offset=(num*sizeof(unsigned int))+0x1000;
                int clusterUse=getintBE(fnum,offset);
                clustermap.push_back(clusterUse);
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
    // lookup VOLNAMEFILE to find volume name
    wchar_t* volname=(wchar_t*)readfilecontents(VOLNAMEFILE);
    // if errors occur, just ignore them, use default deviceName
    if (volname) {
        filepos_t filesize=getfilesize(VOLNAMEFILE);
        // if not zero filesize? otherwise don't bother loading it
        if (filesize) {
            char* cbuf=(char*)malloc((filesize+1)*sizeof(char));
            if (cbuf) {
                convertUTF16(cbuf,volname,filesize);
                deviceName=std::string(cbuf);
                deviceNameSet=true;
                free(cbuf);
            }
        }
        // return buffer to heap
        free(volname);
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
        filepos_t clusterPos=((filepos_t)(currentCluster-1)*(filepos_t)bytesPerCluster);
        numclusters++;
        // extend buffer to make it right size
        buflen=((filepos_t)bytesPerCluster*(filepos_t)numclusters);
        clusterbuf=(unsigned char*)realloc(clusterbuf,buflen);
        if (!clusterbuf) {
            perror("readClusters - malloc");
            exit(1);
        }
        // read cluster directly into buffer
        unsigned char* offset=clusterbuf+((filepos_t)(numclusters-1)*(filepos_t)bytesPerCluster);
        readdata(FIRSTDATAFILE,clusterPos,bytesPerCluster,offset);
        // move to next cluster in chain
        currentCluster=clustermap[currentCluster];
    }
    // pass information back to caller
    *buffer=clusterbuf;
    *bufferlen=buflen;
}
void XBoxFATX::convertUTF16(char* outstr,wchar_t* instr,unsigned int length)
{
    char* ptr=outstr;
    unsigned int widestartchar=0;
    // is there a BOM?
    if (((*(unsigned short int*)instr)==0xFFFE)||((*(unsigned short int*)instr)==0xFEFF)) {
        // skip the BOM
        widestartchar=1;
    }
    // determine BE or LE from first two bytes (BOM)
    bool BE1orLE0=((*(unsigned short int*)instr)==0xFFFE);
    //
    for (unsigned int i=widestartchar; i<(length>>1); i++) {
        unsigned short int* wcptr=(unsigned short int*)instr+i;
        // get wide char, flip if needed
        wchar_t wc=(BE1orLE0)?be16toh(*wcptr):le16toh(*wcptr);
        // convert wide char (UTF-16) to multi-byte (UTF-8)
        int numchr=wctomb(ptr,wc);
        // if valid chars saved, bump pointer
        if (numchr>0) {
            ptr+=numchr;
        }
    }
    // zero terminate the multi-byte string
    *ptr=0;
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
                char namebuf[0x2a+1];
                direntry_t entry;
                entry.attributes=*(ptr+1);
                strncpy(namebuf,(char*)(ptr+0x02),0x2a);
                namebuf[*ptr]=0;
                entry.name=std::string(namebuf);
                entry.nestlevel    =nestlevel;
                entry.parentCluster=startCluster;
                entry.startCluster =be32toh(*((unsigned int*)(ptr+0x2c)));
                entry.filesize     =be32toh(*((unsigned int*)(ptr+0x30)));
                entry.createDate   =be16toh(*((unsigned short int*)(ptr+0x34)));
                entry.createTime   =be16toh(*((unsigned short int*)(ptr+0x36)));
                entry.lwriteDate   =be16toh(*((unsigned short int*)(ptr+0x38)));
                entry.lwriteTime   =be16toh(*((unsigned short int*)(ptr+0x3a)));
                entry.accessDate   =be16toh(*((unsigned short int*)(ptr+0x3c)));
                entry.accessTime   =be16toh(*((unsigned short int*)(ptr+0x3e)));
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
direntry_t* XBoxFATX::findFileEntry(std::string filename)
{
    std::vector<direntry_t>::iterator ptr=dirtree.begin();
    while ((ptr!=dirtree.end())&&(ptr->name!=filename)) {
        ptr++;
    }
    if (ptr==dirtree.end()) {
        // all entries checked, not found
        return NULL;
    }
    return &(*ptr);
}
filepos_t XBoxFATX::getfilesize(std::string filename)
{
    direntry_t* entry=findFileEntry(filename);
    if (!entry) {
        return 0;
    }
    return entry->filesize;
}
unsigned char* XBoxFATX::readfilecontents(std::string filename)
{
    direntry* entry=findFileEntry(filename);
    if (!entry) {
        return NULL;
    }
    unsigned char* buffer=NULL;
    filepos_t buflen=0;
    // read file data, returning malloc'd ram pointer
    // could be problematic if we're transferring a multi-gig file
    readClusters(entry->startCluster,&buffer,&buflen);
    // buflen will be multiple of cluster size, so fix it
    buflen=entry->filesize;
    // shorten buffer length to reflect file length
    buffer=(unsigned char*)realloc(buffer,buflen);
    // user must remember to free the ram
    return buffer;
}
void XBoxFATX::showinfo()
{
    printf("\n");
    // adjust width based on widest number to be displayed
    int width=6+(totalClusters>99999)+(totalClusters>999999)*2;
    printf("  Partition ID: %08X\n",partitionID);
    printf("   Device Name: \"%s\"%s\n",deviceName.c_str(),deviceNameSet?"":" (unnamed unit)");
    printf("    Total Size: %'*lu MiB (%'.2f GiB)\n",width,bytesPerDevice/ONEMEG,((double)bytesPerDevice/ONEGIG));
    printf("  Cluster Size: %'*lu KiB\n",width,(bytesPerCluster/ONEKAY));
    printf("Num Data files: %*d => 2+%d => (0000...%04d)\n",width,(lastfile+1),(lastfile-1),lastfile);
    printf(" Clusters/File: %'*u\n",width,clustersPerFile);
    printf("Total Clusters: %'*u\n",width,totalClusters);
    printf(" Used Clusters: %'*u (%.1f%% used)\n",width,usedClusters,((double)usedClusters*100.0/totalClusters));
    printf("     Num Files: %'*d\n",width,countFiles);
    printf("      Num Dirs: %'*d\n",width,countDirs);
    // printf("    Root Dir @: %'*u\n",width,rootDirCluster);
}
void XBoxFATX::zeroClusters()
{
    // create a buffer
    char* zerobuf=(char*)malloc(bytesPerCluster);
    if (!zerobuf) {
        perror("zeroClusters");
        exit(1);
    }
    // fill with zeros
    memset(zerobuf,0,bytesPerCluster);
    //
    double prevpercent=0.0;
    // find all unallocated clusters, write zeros to each cluster
    // performed in REVERSE order so most likely used files end up in the cache
    for (unsigned int i=clustermap.size()-1; i>0; i--) {
        if (clustermap[i]==0) {
            if (verbose) {
                double percent=100.0-(floor((double)i*1000.0/clustermap.size())/10.0);
                if (percent>prevpercent) {
                    printf("\rClearing Free Space: (%u) %4.1f%% ",(FIRSTDATAFILE+(i/clustersPerFile)),percent);
                    fflush(stdout);
                    prevpercent=percent;
                }
            }
            filepos_t clusterPos=((filepos_t)(i-1)*(filepos_t)bytesPerCluster);
            writedata(FIRSTDATAFILE,clusterPos,bytesPerCluster,zerobuf);
        }
    }
    free(zerobuf);
    closeAllFiles();
    if (verbose) {
        printf("\r%-30s\n","All Free Space Cleared");
    }
}
int main(int argc __attribute__ ((unused)),char* argv[])
{
    // perform setup and initial discovery
    XBoxFATX xbox=XBoxFATX(argv[1]);
    //
    // determine options used (if any)
    if ((argc>2)&&(strncmp(argv[2],"--zero",7)==0)) {
        xbox.verbose=true;
        xbox.zeroClusters();
    }
    //
    xbox.showinfo();

    return 0;
}
