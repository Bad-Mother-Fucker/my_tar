#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

typedef struct names_s {
    char** array;
    int count;
}names;

typedef struct metadata_s {
    size_t  size;
    mode_t  mode;
    unsigned uid;
    unsigned gid;
    struct timespec modification_time;
    unsigned nameLen;
    char* name;
}metadata;

int analizeArg(int, char* []);
int createArchive(char*);
int argContains(char*, int, char*[]);
names getFilenames(int argc, char* argv[] );
int archive(names,int);
int closeArchive(int);
metadata getInfo(char*);
int writeData(metadata, int);
void copyFileContent(char* , int );


int main(int argc, char* argv []) {

    analizeArg(argc, argv);
    return 0;
}


int analizeArg( int argc, char* argv []) {

    if (argc < 3) {
        argc = 4;
        argv[1] = "-c";
        argv[2] = "../hi.txt";
        argv[3] = "../chichi.txt";
    };

    if (argContains("-c", argc, argv) != -1) {
        int index = argContains("-f", argc, argv);
        char* archiveName = index == -1 ? "../Archive.tar": argv[index+1];
        int archiveDesc = createArchive(archiveName);


        names filenames = getFilenames(argc,argv);
        archive(filenames, archiveDesc);
        closeArchive(archiveDesc);
    }
    return 0;
}

int closeArchive(int ad){
    close(ad);
    return 0;
}

int createArchive(char* archiveName){
    int fd = open(archiveName, O_WRONLY | O_CREAT);
    if(fd == -1) {
        perror("Error creating archive:");
        exit(-1);
    }
    return fd;
}


int archive(names filenames, int ad) {
    for(int i = 0; i< filenames.count; i++) {
        metadata met = getInfo(filenames.array[i]);
        writeData(met, ad);
    }
    return 0;
}

int writeData(metadata m, int ad) {
    write(ad, "name ", sizeof(char)*5);
    write(ad, "mode ", sizeof(char)*5);
    write(ad, "mod time ", sizeof(char )*9);
    //write(ad, &m.name, sizeof(char)*strlen(m.name));
    copyFileContent(m.name, ad);
    return 0;
}



metadata getInfo(char * filename) {
    struct stat stat_s;
    stat(filename, &stat_s);
    metadata md;
    md.gid = stat_s.st_gid;
    md.mode = stat_s.st_mode;
    md.modification_time = stat_s.st_mtimespec;
    md.uid = stat_s.st_uid;
    md.size = stat_s.st_size;
    md.name = filename;
    md.nameLen = strlen(filename);

    return md;

}

int argContains(char * arg,int argc, char*argv[]) {
    for (int i = 0; i< argc; i++)
        if(strcmp(arg, argv[i]) == 0)
            return i;
    return -1;
}



int isAFilename(char* str) {
    if(strcmp(str, "-c")!=0 && strcmp(str, "-f")!=0 && strcmp(str, "-r")!=0 && strcmp(str, "-u")!=0){
        unsigned long len = strlen(str);
        const char* ext = &str[len -3];
        if(strcmp(ext, "tar") == 0) return 0;
        else return 1;
    }
    return 0;
}

 names getFilenames(int argc, char* argv[]) {

    names namesArray;
    namesArray.count = 0;
    namesArray.array = malloc(sizeof(char *) * argc-3);
    for( int i = 1; i< argc; i++) {
        if(isAFilename(argv[i])){
            namesArray.array[namesArray.count] = (char *) malloc(sizeof(char) * (strlen(argv[i]) + 1));
            strcpy(namesArray.array[namesArray.count], argv[i]);
            namesArray.count ++;
        }
    }

    return namesArray;

}


void copyFileContent(char* filename, int ad ) {

    ssize_t numRead;
    char buf[BUF_SIZE];

    /* transfer data until we encounter end of input or an error */
    int fd = open(filename, O_RDONLY);
    if (fd == -1) perror("Errore apertura");
    while ((numRead = read(fd, buf, BUF_SIZE)) > 0)
    {
        if (write(ad, buf, numRead) != numRead)
            perror("couldn't write whole buffer");
    }

    if (numRead == -1)
        perror("read");

    if (close(fd) == -1)
        perror("close input");

}