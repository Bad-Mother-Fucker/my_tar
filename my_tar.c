#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

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
    time_t modification_time;
    unsigned nameLen;
    char name[20];
}metadata;

int analizeArg(int, char* []);
int createArchive(char*);
int flagContains(char, char*[]);
names getFilenames(int argc, char* argv[] );
int archive(names,int);
int closeArchive(int);
metadata getInfo(char*);
int writeData(metadata, int);
void copyFileContent(char* , int );
void readArchiveMetadata(int ad);
metadata getArchivedFileMetadata(char* filename, int ad);
void archiveIfNewer(char* filename, int ad);
void extract(int);
int lastIndexOf(char c, char* s);
int my_strlen(const char* s);
int my_strcmp(const char * s1, const char* s2);
char* my_strcpy(char* param_1, const char* param_2);



int main(int argc, char* argv []) {

    analizeArg(argc, argv);
    return 0;
}

char* my_strcpy(char* param_1, const char* param_2) {
    int i = 0;
    for(i = 0 ; i< my_strlen(param_2); i++) {
        param_1[i] = param_2[i];
    }
    param_1[i] = '\0';

    return param_1;
}

int my_strcmp(const char * s1, const char* s2) {
    int flag = 0;
    while(*s1!= '\0'|| *s2!= '\0'){
        if(*s1==*s2){
            s1++;
            s2++;
        }else if ((*s1== '\0' && *s2!= '\0') ||(*s1!= '\0' && *s2== '\0') || *s1 != *s2) {
            flag = 1;
            break;
        }
        else flag = 0;
    }
    return flag;

}

int my_strlen(const char* s) {
    if(s==NULL) return -1;
    int i = 0;
    while(s[i]!= '\0')
        i++;
    return i;
}





int analizeArg( int argc, char* argv []) {

    if (argc < 3) {
        perror("Too few arguments");
        exit(1);
    };

    if (flagContains('c', argv) ) {
        int f = flagContains('f', argv);
        char* archiveName = f == 0 ? "../Archive.tar": argv[2];
        int archiveDesc = createArchive(archiveName);

        names filenames = getFilenames(argc,argv);
        archive(filenames, archiveDesc);
        closeArchive(archiveDesc);
    } else
        if (flagContains('r', argv)) {
        int f = flagContains('f', argv);
        if(f == 0) {
            perror("-f option is required with an archive name");
            exit(1);
        }
        char* archiveName = argv[2];
        int ad = open(archiveName, O_WRONLY|O_APPEND);

        if (ad == -1) {
            perror("Error opening archive: ");
            exit(1);
        }

        names filenames = getFilenames(argc, argv);
        archive(filenames,ad);
        closeArchive(ad);
    } else
        if (flagContains('t' ,argv)) {

            char* archiveName = argv[2];
            int ad = open(archiveName, O_RDONLY);

            if (ad == -1) {
                perror("Error opening archive: ");
                exit(1);
            }
            readArchiveMetadata(ad);
        } else
        if (flagContains('u', argv)){

            int f = flagContains('f', argv);
            if(f == 0) {
                perror("-f option is required with an archive name");
                exit(1);
            }
            char* archiveName = argv[2];
            int ad = open(archiveName, O_RDWR|O_APPEND);
            names filenames = getFilenames(argc,argv);
            for (int i = 0; i< filenames.count; i++){
                archiveIfNewer(filenames.array[i],ad);
            }

        }else
            if(flagContains('x', argv) ){

                char* archiveName = argv[2];
                int ad = open(archiveName, O_RDONLY);

                if (ad == -1) {
                    perror("Error opening archive: ");
                    exit(1);
                }
                extract(ad);
            }


    return 0;
}

int closeArchive(int ad){
    close(ad);
    return 0;
}


void extract(int ad) {

    metadata md;


    while (read(ad, &md, sizeof (md)) > 0){

        const char * filename = &md.name[lastIndexOf('/',md.name)+1];

        int fd = open(filename, O_RDWR);
        if (fd != -1) {
            metadata stat = getInfo(md.name);
            if (stat.modification_time < md.modification_time){
                unlink(stat.name);
                fd = open(filename, O_RDWR|O_CREAT);
            }

        }else {
            fd = open(filename, O_RDWR|O_CREAT);
        }

        char buf[md.size];
        int rd = read(ad, buf, md.size);
        if(rd ==-1)
            perror("Error reading file: ");
        int wr = write(fd, buf, sizeof (buf));

        if(wr ==-1)
            perror("Error writing file: ");

        close(fd);
    }


}



void archiveIfNewer(char* filename, int ad) {
    metadata new_md = getInfo(filename);
    off_t tot_off;
    metadata md;
    time_t lastEdit = 0;

    while (read(ad, &md,  sizeof (metadata)) > 0){

        if (my_strcmp(new_md.name, md.name) == 0){
          if (md.modification_time > lastEdit) {
              lastEdit = md.modification_time;
          }

        }

        lseek(ad, md.size, SEEK_CUR);

    }
    if (lastEdit < new_md.modification_time ) {

        writeData(new_md, ad);
    }else {
        printf("Newer file version exists in the archive\n");
        lseek(ad, 0, SEEK_END);
        return;
    }



}

void readArchiveMetadata(int ad) {
    metadata md;
    while (read(ad, &md, sizeof (md)) > 0){
        printf("filename: %s \tuid: %u \tgid: %u \tmode: %u \tsize: %ld \tlast edit timestamp: %ld\n", md.name,md.uid,md.gid,md.mode, md.size, md.modification_time);
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
    if(stat(filename, &stat_s)!=-1) {
        metadata md;
        md.gid = stat_s.st_gid;
        md.mode = stat_s.st_mode;
        md.modification_time = stat_s.st_mtime;
        md.uid = stat_s.st_uid;
        md.size = stat_s.st_size;
        my_strcpy(md.name, filename);
        md.nameLen = my_strlen(filename);
        printf("mod time: %ld", md.modification_time);
        return md;
    }else {
        perror(filename);
        exit(1);
    }



}

int flagContains(char  arg, char*argv[]) {
    char* flags = argv[1];
    for (int i = 0; i< my_strlen(flags);i++)
        if (flags[i] == arg)
            return 1;
    return 0;
}



int isAFilename(char* str) {
    if(str[0] != '-'){
        unsigned long len = my_strlen(str);
        const char* ext = &str[len -3];
        if(my_strcmp(ext, "tar") == 0) return 0;
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
            namesArray.array[namesArray.count] = (char *) malloc(sizeof(char) * (my_strlen(argv[i]) + 1));
            my_strcpy(namesArray.array[namesArray.count], argv[i]);
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
