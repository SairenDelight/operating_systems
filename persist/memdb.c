//Vivian Hoang
//First fit with a lot of anger, it add to the next free segment of the heap and extends...no deletion was implemented.
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

off_t spaceLeft = 0;
off_t endOfFile = 0;
int update_heap(struct fhdr_s *, moffset_t, int);
int checkEntries(struct fhdr_s *, char[]);

int main(int argc, char * argv[]){
    //============= PRIVATE PROTOTYPES =================
    int init(struct fhdr_s *, int);
    int init_new_file(struct fhdr_s *);
    int set_end_of_file(char[]);
    int add_newEntry(moffset_t, struct fhdr_s *, char[]);
    void print_List(struct fhdr_s *);
    int insert(struct fhdr_s *);
    moffset_t get_last(moffset_t const,struct fhdr_s *);
    //============== END OF PROTOTYPES =================
    if(argc < 2){
        printf("No file was selected...\n");
        return -1;
    } else if ( argc > 3) {
        printf("Too many files selected. Enter only one file. \n");
        return -1;
    } else { 
            int map_flag = 0;
            int file = 0;
            if(strncmp(argv[1],"-t",2) == 0){ //http://www.cplusplus.com/reference/cstring/strcpy/
                map_flag = MAP_PRIVATE;
                file = 2;
            } else {
                map_flag = MAP_SHARED;
                file = 1;
            }
                struct stat st;
                int dat_file_exists = stat(argv[file],&st); 
                int fd = open(argv[file],  O_RDWR | O_CREAT,  S_IRWXU | S_IRWXO | S_IRWXG); //https://linux.die.net/man/2/stat
                if(fd == -1){
                    perror("Open");
                    close(fd);
                    exit(EXIT_FAILURE);
                }
            
                if(dat_file_exists == -1){
                    ftruncate(fd,INIT_SIZE); //Canvas homework mentions...
                    // printf("File passed is %s\n",argv[file]);
                }
                set_end_of_file(argv[file]);

            struct fhdr_s *fhdr = mmap(NULL, MAX_SIZE, PROT_EXEC| PROT_READ | PROT_WRITE, map_flag,fd, 0); //Man pages...
            moffset_t currentOffset = fhdr->free_start;
            spaceLeft  = (int)(endOfFile - (currentOffset + sizeof(struct entry_s)));
            // printf("Space left is %ld\n",spaceLeft);

            //Error return if mapping failed
            if(fhdr == MAP_FAILED){
                perror("Error mapping failed");
                close(fd);
                exit(EXIT_FAILURE);
            }

            init(fhdr,dat_file_exists); //this will initialize values if file doesn't exist already
            
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
                        if(isalpha(input[2]) || isdigit(input[2])){
                            currentOffset = fhdr->free_start;
                            off_t isOver = sizeof(struct entry_s) + MAXCHAR;
                            if(isOver > spaceLeft && endOfFile < MAX_SIZE){
                                int extend = endOfFile+1024;
                                if(extend > MAX_SIZE){
                                    printf("MAX SIZE reached! Cannot expand file!!");
                                } else {
                                    ftruncate(fd,endOfFile+1024);
                                    set_end_of_file(argv[file]);
                                }
                            } else {
                                //check if word exists in entries
                                int success = add_newEntry(currentOffset, fhdr, input); //this adds to heap
                                if(success != 0){
                                    puts("░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ ERROR MESSAGE ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░");
                                    printf("\nDear user,\n I know it's your favorite word...but it's already in the entry so please stop...I'm not adding it!\nFrom,\nVivian\n\n");
                                    puts("░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ END OF ERROR MESSAGE ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░");
                                }
                            }
                        } else {
                            printf("Command must be one of:\n l\n a string_to_add\n d string_to_del\n");
                        }
                        break;

                    
                    case 'd':
                        printf("This should delete the word from the file\n");
                        break;
                    default:
                        printf("Command must be one of:\n l\n a string_to_add\n d string_to_del\n");
                        break;
                }
            } while (1); //Press Ctrl+C to quit
            munmap(fhdr,MAX_SIZE);
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
    // printf("Prev offset at the beginning %lld\n",prevOffset);
    struct entry_s * current_entry = (struct entry_s *)((char *) fhdr + prevOffset);
    while(current_entry->next != 0){
        prevOffset = current_entry->next;
        // printf("Prev offset in loop %lld\n",prevOffset);
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
    /*This will copy the string into a proper null terminated character form*/
    int get_Length_Of_String(char[]);
    int inputLength = get_Length_Of_String(input); //the actual length with the null terminated character
    int excludeNullCharacterLength = inputLength - 1;
    char cp[inputLength];
    for(int i = 2, c = 0; c < excludeNullCharacterLength;i++,c++){
        cp[c] = input[i]; 
    }
    char temp[inputLength];
    strncpy(temp,cp,inputLength); //http://man7.org/linux/man-pages/man3/strcpy.3.html
    temp[excludeNullCharacterLength] = '\0';


    int exists = checkEntries(fhdr, temp); //this check if it exists...it'll return the previous offset before the add location
    // printf("The previous offset is %d\n",exists);
    if(exists >= 0){
        moffset_t tempOffset = 0;
        if(exists == 0){
            tempOffset = fhdr->data_start;
            fhdr->data_start = currentOffset;
        } else {
            struct entry_s *old_entry = (struct entry_s *)((char*)fhdr + exists);
            tempOffset = old_entry->next;
            old_entry->next = currentOffset;
        }
        struct entry_s *new_entry = (struct entry_s *)((char*)fhdr + currentOffset);
        new_entry->magic = ENTRY_MAGIC_DATA;
        new_entry->next = tempOffset;
        new_entry->len = inputLength;
        strncpy(new_entry->str,cp,inputLength);
        new_entry->str[excludeNullCharacterLength] = '\0';
        // printf("Magic: %d\n",new_entry->magic);
        // printf("Next: %lld\n",new_entry->next);
        // printf("Len is: %d\n",new_entry->len);
        // printf("The string in the new entry is %s\n",new_entry->str);
        update_heap(fhdr, currentOffset, new_entry->len);
        return 0;
    } else {
        // printf("The entries are equal so they will not be added...\n");
        return -1;
    }
}


//This will check all the entries and return the offset before the possible add location
int checkEntries(struct fhdr_s * fhdr,char temp[]){
     moffset_t currentOffset = fhdr->data_start; //This is where the supposed Linked List starts
     moffset_t prevOffset = 0;
    struct entry_s *entry = (struct entry_s *)(fhdr + 1); //This is place the address pass the file header
    while(currentOffset != 0){
            entry = (struct entry_s *) ((char *) fhdr + currentOffset);
            int result = strcmp(temp,entry->str);
             if(result == 0){
                 return -1;
             } else if(result <= 0){
                // printf("%s is less than or equal to %s\n",temp,entry->str);
                return prevOffset;
            } else {
                // printf("%s is more than %s\n",temp,entry->str);
                prevOffset = currentOffset;
                currentOffset = entry->next;
            }
    }
    return prevOffset;
}




//This will update the heap of what is free at the end...
int update_heap(struct fhdr_s * fhdr, moffset_t currentOffset, int len){
    
    if(fhdr->data_start != 0){
        currentOffset +=sizeof(struct entry_s) + len; //update currentOffset to be after new entry
        fhdr->free_start =  currentOffset;
        struct entry_s *free_entry = (struct entry_s *) ((char*) fhdr + currentOffset); //currentOffset is now AFTER new entry
        int lengthOfFile = (int)(endOfFile - (currentOffset + sizeof(*free_entry))); //get new length of free space
        int additional = currentOffset + sizeof(*free_entry);
        free_entry->magic = ENTRY_MAGIC_FREE;
        free_entry->next = 0;
        free_entry->len = lengthOfFile;
        spaceLeft = lengthOfFile;
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

//This will set the global variable endOfFile to the total amount of bytes of the file
int set_end_of_file(char filename[]){
    // printf("File name is %s\n",filename);
    struct stat s;
    stat(filename,&s);
    endOfFile = s.st_size;
    // printf("The end of file is...%ld\n",endOfFile);
}

//This will initialize a new file
int init_new_file(struct fhdr_s * fhdr){
    // puts("New file...");
    fhdr->magic = FILE_MAGIC;
    fhdr->free_start = (moffset_t)sizeof(*fhdr);
    fhdr->data_start = 0;
    int lengthOfFile = (int)(endOfFile - (sizeof(*fhdr) + sizeof(struct entry_s)));
    // printf("The address of fhdr is %p\n",fhdr);
    // printf("The value of fhdr->magic is %d\n",fhdr->magic);
    // printf("The address of the value is fhdr->magic is %p\n",&fhdr->magic);
    // printf("The address of the value is fhdr->free_start is %p\n",&fhdr->free_start);
    // printf("The value is fhdr->free_start is %lld\n",fhdr->free_start);
    // printf("The value is fhdr->data_start is %lld\n",fhdr->data_start);

    update_heap(fhdr,fhdr->free_start,lengthOfFile);
    return 0;
}

int init(struct fhdr_s *fhdr, int fileExists){
    if(fileExists == -1){
        init_new_file(fhdr);
    } //The data should already be saved in the entry.
    return 0;
}