//First fit kill me now
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <ctype.h>
#include "memdb.h"
#define MAXCHAR 50


struct LinkedList {
    moffset_t head;
    moffset_t tail;
};



int endOfFile = INIT_SIZE;
int update_heap(struct fhdr_s *, moffset_t, int);



int main(int argc, char * argv[]){
    //============= PRIVATE PROTOTYPES =================
    int init(struct fhdr_s *, int);
    int init_new_file(struct fhdr_s *);
    int set_end_of_file(char*);
    int add_newEntry(moffset_t, struct fhdr_s *, char[]);
    void print_List(struct fhdr_s *);

    moffset_t get_last(moffset_t const,struct fhdr_s *);
    //============== END OF PROTOTYPES =================

    if(argc < 2){
        printf("No file was selected...\n");
        return -1;
    } else if ( argc > 2) {
        printf("Too many files selected. Enter only one file. \n");
        return -1;
    } else { 
            struct stat st;
            int dat_file_exists = stat(argv[1],&st); 
            int fd = open(argv[1],  O_RDWR | O_CREAT,  S_IRWXU | S_IRWXO | S_IRWXG); //https://linux.die.net/man/2/stat
            if(fd == -1){
                perror("Open");
                close(fd);
                exit(EXIT_FAILURE);
            }
            ftruncate(fd,INIT_SIZE);

            set_end_of_file(argv[1]);

            struct fhdr_s *fhdr = mmap(NULL, INIT_SIZE, PROT_EXEC| PROT_READ | PROT_WRITE, MAP_SHARED,fd, 0); 
            //Error return if mapping failed
            if(fhdr == MAP_FAILED){
                perror("Error mapping failed");
                close(fd);
                exit(EXIT_FAILURE);
            }

            init(fhdr,dat_file_exists);
            moffset_t tail = get_last(fhdr->data_start,fhdr);
            struct LinkedList * list;
            list->head = (moffset_t)fhdr->data_start;
            list->tail = tail;
            printf("The tail offset is %lld\n",list->tail);
            // if(dat_file_exists == -1){ //if file does not exists initialize variables
                // init_new_file(fhdr);
            // } else { //if file does exists initialize new variables
                

            // }

            int i = 0;
            do {
                char input[MAXCHAR];
                fgets(input, MAXCHAR, stdin);
                switch(input[0]){
                    case 'l':{
                        print_List(fhdr);
                        break;
                    }

                    case 'a':
                        if(isalpha(input[2])){
                            moffset_t currentOffset = fhdr->free_start;
                            add_newEntry(currentOffset, fhdr, input); //this adds to heap
                            //add into linked list where it's less or between a word...O(n)..
                            // insert_into_list(head);
                        } else {
                            printf("Command must be one of:\n l\n a string_to_add\n d string_to_del\n");

                        }
                        // if(msync(fhdr,INIT_SIZE,MS_SYNC) == -1){
                        //     perror("Could not sync file to disk");
                        //     exit(EXIT_FAILURE);
                        // }
                        break;

                    
                    case 'd':
                        printf("This should delete the word from the file\n");
                        break;
                    default:
                        printf("Command must be one of:\n l\n a string_to_add\n d string_to_del\n");
                        break;
                }
            } while (1); //Press Ctrl+C to quit
            munmap(fhdr,INIT_SIZE);
            close(fd);
            
        }
    return 0;
}






//This will get the previous offset for the tail replacement...
moffset_t get_last(moffset_t const start,struct fhdr_s *fhdr){
    moffset_t prevOffset = start;
    if(prevOffset == 0){ //This means that this is a new file so there is no previous Offset...
        prevOffset = (sizeof(*fhdr));
        return prevOffset;
    }
    printf("Prev offset at the beginning %lld\n",prevOffset);
    struct entry_s * current_entry = (struct entry_s *)((char *) fhdr + prevOffset);
    while(current_entry->next != 0){
        prevOffset = current_entry->next;
        printf("Prev offset in loop %lld\n",prevOffset);
        current_entry = (struct entry_s *)((char *) fhdr + prevOffset);
    }
    return prevOffset;
}



//This will print the entire list
void print_List(struct fhdr_s *fhdr){
    moffset_t currentOffset = fhdr->data_start; //This is where the supposed Linked List starts
    struct entry_s *entry = (struct entry_s *)(fhdr + 1); //This is place the address pass the file header
    while(currentOffset != 0){
            entry = (struct entry_s *) ((char *) fhdr + currentOffset);
            printf("%s\n",entry->str);
            currentOffset = entry->next;
    }
}


//This will add the new entry to the heap
int add_newEntry(moffset_t currentOffset, struct fhdr_s *fhdr, char input[]){
    //add return -1 if entry exceeds size of file
    int get_Length_Of_String(char[]);
    struct entry_s *new_entry = (struct entry_s *)((char*)fhdr + currentOffset);
    new_entry->magic = ENTRY_MAGIC_DATA;

    int inputLength = get_Length_Of_String(input); //the actual length with the null terminated character
    int excludeNullCharacterLength = inputLength - 1;
    for(int i = 2, c = 0; c < excludeNullCharacterLength;i++,c++){
        new_entry->str[c] = input[i]; //so apparently getting rid of the null terminated character makes it format prettier in dbdump which makes no sense but okay...
    }
    new_entry->len = inputLength;
    if(fhdr->data_start == 0){
        fhdr->data_start = currentOffset;
    }
    update_heap(fhdr, currentOffset, new_entry->len);
    return 0;
}





//This will update the heap of what is free at the end...
int update_heap(struct fhdr_s * fhdr, moffset_t currentOffset, int len){
    
    if(fhdr->data_start != 0){
        currentOffset +=sizeof(struct entry_s) + len; //update currentOffset to be after new entry
        fhdr->free_start =  currentOffset;
        struct entry_s *free_entry = (struct entry_s *) ((char*) fhdr + currentOffset); //currentOffset is now AFTER new entry
        int lengthOfFile = (int)(endOfFile - (currentOffset + sizeof(*free_entry))); //get new length of free space
        // printf("The length of file is now: %d\n",lengthOfFile);
        free_entry->magic = ENTRY_MAGIC_FREE;
        free_entry->next = 0;
        free_entry->len = lengthOfFile;
    } else {
        struct entry_s *free_entry = (struct entry_s *) ((char*) fhdr + currentOffset); //this is the address
        free_entry->magic = ENTRY_MAGIC_FREE;
        free_entry->len = len;
        free_entry->next = 0;
    }
    return 0;
}

//This will return the count of the string
int get_Length_Of_String(char input[]){
    int counter = 0;
    int i = 2;
    while(1){
        counter++;
        i++;
        if(input[i] == '\0' )
            break;
    }
    return counter;
}

//This will initialize a new file
int init_new_file(struct fhdr_s * fhdr){
    puts("New file...");
    fhdr->magic = FILE_MAGIC;
    fhdr->free_start = (moffset_t)sizeof(*fhdr);
    fhdr->data_start = 0;
    int lengthOfFile = (int)(endOfFile - (sizeof(*fhdr) + sizeof(struct entry_s)));
    update_heap(fhdr,fhdr->free_start,lengthOfFile);
    return 0;
}

int init(struct fhdr_s *fhdr, int fileExists){
    printf("The address is %p\n",fhdr);
    if(fileExists != -1){
        puts("File exists...");
        printf("The address of fhdr is %p\n",fhdr);
        printf("The value of fhdr->magic is %d\n",fhdr->magic);
        printf("The address of the value is fhdr->magic is %p\n",&fhdr->magic);
        printf("The address of the value is fhdr->free_start is %p\n",&fhdr->free_start);
        printf("The value is fhdr->free_start is %lld\n",fhdr->free_start);
        printf("The value is fhdr->data_start is %lld\n",fhdr->data_start);
        fhdr->magic= FILE_MAGIC;
        fhdr->free_start = *(moffset_t*)(sizeof(short));
        printf("Print the free_start which is...%lld\n",fhdr->free_start);
        fhdr->data_start = *(moffset_t*)(sizeof(short) + sizeof(moffset_t));
        printf("Print the data_start which is...%lld\n",fhdr->data_start);
    } else {
        init_new_file(fhdr);
    }
    return 0;
}

//This will set the global variable endOfFile to the total amount of bytes of the file
int set_end_of_file(char* filename){
    int endFd = open(filename, O_RDONLY); //Just using this to get the bytes from the end of the file
    endOfFile = (int) lseek(endFd,0,SEEK_END);
    close(endFd);
}