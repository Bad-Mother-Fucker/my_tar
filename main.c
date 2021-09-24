#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

typedef struct names_s {
    char** array;
    int count;
}names;

typedef struct metadata_s {
    off_t  size;
    mode_t  mode;
    unsigned uid;
    unsigned gid;
    struct timespec modification_time;
    unsigned nameLen;
    char name[20];
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
void readArchiveMetadata(int ad);
metadata getArchivedFileMetadata(char* filename, int ad);
void archiveIfNewer(char* filename, int ad);
void extract(int, const char[]);
int lastIndexOf(char c, char* s);



int main(int argc, char* argv []) {

    analizeArg(argc, argv);
    return 0;
}


int analizeArg( int argc, char* argv []) {

    if (argc < 3) {
        argc = 4;
        argv[1] = "-x";
        argv[2] = "../Archive.tar";
        argv[3] = "..";

    };

    if (argContains("-c", argc, argv) != -1) {
        int index = argContains("-f", argc, argv);
        char* archiveName = index == -1 ? "../Archive.tar": argv[index+1];
        int archiveDesc = createArchive(archiveName);

        names filenames = getFilenames(argc,argv);
        archive(filenames, archiveDesc);
        closeArchive(archiveDesc);
    } else
        if (argContains("-r", argc, argv) != -1) {
        int index = argContains("-f", argc, argv);
        if(index == -1) {
            perror("-f option is required with an archive name");
            exit(1);
        }
        char* archiveName = argv[index+1];
        int ad = open(archiveName, O_WRONLY|O_APPEND);

        if (ad == -1) {
            perror("Error opening archive: ");
            exit(1);
        }

        names filenames = getFilenames(argc, argv);
        archive(filenames,ad);
        closeArchive(ad);
    } else
        if (argContains("-t",argc ,argv)!=-1) {

            char* archiveName = argv[2];
            int ad = open(archiveName, O_RDONLY);

            if (ad == -1) {
                perror("Error opening archive: ");
                exit(1);
            }
            readArchiveMetadata(ad);
        } else
        if (argContains("-u", argc,argv)!=-1){

            int index = argContains("-f", argc, argv);
            if(index == -1) {
                perror("-f option is required with an archive name");
                exit(1);
            }
            char* archiveName = argv[index+1];
            int ad = open(archiveName, O_WRONLY|O_APPEND);
            names filenames = getFilenames(argc,argv);
            for (int i = 0; i< filenames.count; i++){
                archiveIfNewer(filenames.array[i],ad);
            }

        }else
            if(argContains("-x", argc,argv) ){

                char* archiveName = argv[2];
                int ad = open(archiveName, O_RDONLY);

                if (ad == -1) {
                    perror("Error opening archive: ");
                    exit(1);
                }
                extract(ad, argv[3]);
            }


    return 0;
}

int closeArchive(int ad){
    close(ad);
    return 0;
}


void extract(int ad, const char *dirname) {

    metadata md;


    while (read(ad, &md, sizeof (md)) > 0){

        const char * filename = &md.name[lastIndexOf('/',md.name)+1];
        char* name = ("%s/%s", dirname, filename);

        int fd = open(name, O_RDONLY);
        if (fd != -1) {
            metadata stat = getInfo(md.name);
            if (stat.modification_time.tv_nsec < md.modification_time.tv_nsec){
                unlink(stat.name);
                fd = open(("%s/%s", dirname, filename), O_RDONLY|O_CREAT);
            }

        }else {
            fd = open(("%s/%s", dirname, filename), O_RDONLY|O_CREAT);
        }

        char buf[md.size];
        read(ad, buf, md.size);
        write(fd, buf, sizeof (buf));
        close(fd);
    }


}

metadata getArchivedFileMetadata(char* filename, int ad){
    metadata md;
    while (read(ad, &md, sizeof (md)) > 0){
        if (strcmp(filename, md.name) == 0)
            return md;
        lseek(ad, md.size, SEEK_CUR);
    }
    strcpy(md.name,"");
    return md;
}

void archiveIfNewer(char* filename, int ad) {
    metadata new_md = getInfo(filename);
    metadata a_md = getArchivedFileMetadata(filename, ad);
    if(strcmp(a_md.name, "")==0 || new_md.modification_time.tv_nsec > a_md.modification_time.tv_nsec )
        writeData(new_md, ad);

}

void readArchiveMetadata(int ad) {
    metadata md;
    while (read(ad, &md, sizeof (md)) > 0){
        printf("filename: %s \tuid: %u \tgid: %u \tmode: %u \tsize: %lld \n", md.name,md.uid,md.gid,md.mode, md.size);
        lseek(ad, md.size, SEEK_CUR);
    }
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

    write(ad,&m, sizeof (m));
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
    strcpy(md.name, filename);
    md.nameLen = strlen(filename);
    printf("mod time: %ld", md.modification_time.tv_nsec);
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

int lastIndexOf(char c, char* s) {
    int i = 0;
    int cont = 0;
    while(*s != '\0') {
        if (c == *s)
            i=cont;
        cont++;
        s++;
    }
    return i;
}
