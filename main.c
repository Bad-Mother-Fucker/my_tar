#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>

typedef struct names_s {
    char** array;
    int count;
}names;

typedef struct metadata_s {
    size_t  size;
    mode_t  mode;
    int owner;
    int group;
    time_t modification_time;
    unsigned nameLen;
    char* name;
}metadata;

int analizeArg(int, char* []);
int createArchive(char*);
int argContains(char*, int, char*[]);
names getFilenames(int argc, char* argv[] );
int archive(names,int);
int closeArchive(char*);
metadata getInfo(char*);
int writeData(metadata, char*);


int main(int argc, char* argv []) {

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
        writeData(met, filenames.array[i]);
    }
}

int argContains(char * arg,int argc, char*argv[]) {
    for (int i = 0; i< argc; i++)
        if(strcmp(arg, argv[i]) == 0)
            return i;
    return -1;
}

int analizeArg( int argc, char* argv []) {

   if (argc < 3) return -1;

   if (strcmp(argv[1],"-c") == 0) {
       int index = argContains("-f", argc, argv);
       char* archiveName = index == -1 ? "Archive.tar": argv[index+1];
       int archiveDesc = createArchive(archiveName);
       names filenames = getFilenames(argc,argv);
       archive(filenames, archiveDesc);
       closeArchive(archiveName);
   }



}

int isAFile(char* str) {
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
        if(isAFile(argv[i])){
            namesArray.array[i] = (char *) malloc(sizeof(char) * (strlen(argv[i]) + 1));
            strcpy(namesArray.array[i], argv[i]);
            namesArray.count ++;
        }
    }

    return namesArray;

}
