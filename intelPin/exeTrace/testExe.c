#include <signal.h>
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void funcA(const char* funcName){
    printf("A\n");
}

void funcB(){
    int *obj;
    obj = malloc(sizeof(int));
    obj[0] = 2;

    printf("%d\n", obj[0]);
    funcA(__func__);
    printf("B\n");
}

void funcC(){
    printf("C\n");
    printf("C\n");
}

void funcD(int a){
        
    if(a == 0){
        funcB();
        funcC();
        printf("D\n");
    }
    else{
        funcC();
        printf("D2\n");
    }
}



int main(){
     funcA(__func__);
     funcB();
     funcC();
     funcD(0);   
     funcD(1);

     return 0;
}
