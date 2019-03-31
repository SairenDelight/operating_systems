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
    struct Node * head;
};

struct Node {
    struct Node * next;
    char * data;
};

int endOfFile = INIT_SIZE;
int update_heap(struct fhdr_s *, moffset_t, int);
int init_new_file(struct fhdr_s *);





int main(int argc, char * argv[]){
    int add(struct Node*,struct LinkedList *);
    int add_newEntry(moffset_t, struct fhdr_s *, char[]);

    char * getWord(char[]);
    if(argc < 2){
        printf("No file was selected...\n");
        return -1;
    } else if ( argc > 2) {
        printf("Too many files selected. Enter only one file. \n");
        return -1;
    } else { 
            struct stat st;
            int dat_file_exists = stat(argv[1],&st); 
            int fd = open(argv[1],  O_RDWR | O_CREAT, S_IRUSR | S_IWUSR); //https://linux.die.net/man/2/stat
            ftruncate(fd,INIT_SIZE);

            int endFd = open(argv[1], O_RDONLY); //Just using this to get the bytes from the end of the file
            endOfFile = (int) lseek(endFd,0,SEEK_END);
            close(endFd);
            
            struct fhdr_s *fhdr = mmap(NULL, INIT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,fd, 0); 
            //Error return if mapping failed
            if(fhdr == MAP_FAILED){
                perror("Error mapping failed");
                close(fd);
                exit(EXIT_FAILURE);
            }

            //Check if dat file exists already to set offsets
            if(dat_file_exists == -1){
                init_new_file(fhdr);
                struct entry_s *entry = (struct entry_s *) ((char *) fhdr + sizeof(*fhdr));
            } 
                int i = 0;
                do {
                    char input[MAXCHAR];
                    fgets(input, MAXCHAR, stdin);
                    switch(input[0]){
                        case 'l':{
                            moffset_t currentOffset = fhdr->data_start; //This is where the supposed Linked List starts
                            struct entry_s *entry = (struct entry_s *)(fhdr + 1); //This is place the address pass the file header
                            while(currentOffset != 0){
                                    entry = (struct entry_s *) ((char *) fhdr + currentOffset);
                                    printf("%s\n",entry->str);
                                    currentOffset = entry->next;
                            }
                            break;

                        }


                        case 'a':
                            if(isalpha(input[2])){
                                moffset_t currentOffset = fhdr->free_start;
                                add_newEntry(currentOffset, fhdr, input);
                                //sort entries
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
    printf("The address of fhdr is %p\n",fhdr);
    fhdr->magic = FILE_MAGIC;
    fhdr->free_start = (moffset_t)sizeof(*fhdr);
    fhdr->data_start = 0;
    int lengthOfFile = (int)(endOfFile - (sizeof(*fhdr) + sizeof(struct entry_s)));
    printf("The length of File is: %d\n",lengthOfFile);
    update_heap(fhdr,fhdr->free_start,lengthOfFile);
    return 0;
}


int add(struct Node * current, struct LinkedList *list){
    current->next = list->head;
    list->head = current;
    return 0;
}
