#include <stdlib.h>
#include<stdio.h>

int main(){
    int * what = malloc((sizeof(int)) *4);
    for(int i = 0; i < 4;i++){
        what[i] = i;
        printf("what[%d] = %d\n",i ,what[i]);
    }
    what[2] = 3;
    printf("what[%d] = %d\n",2 ,what[2]);
    free(what);
    return 0;
}