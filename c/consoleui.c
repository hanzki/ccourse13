#include "consoleui.h"
#include <stdlib.h>
#include <sys/unistd.h>
#include <string.h>

#define COMMAND_CONNECT "connect"
#define COMMAND_EXIT    "exit"
#define COMMAND_PRINT   "print"

int getcommand(int src, int* argc, char*** argv){
    const size_t BUFSIZE = 300;
    int stat;
    char buffer[BUFSIZE];
    stat = read(src, buffer, BUFSIZE-1);
    if(stat == BUFSIZE){
        return ERR_BUF_FULL;
    }
    if(stat < 0){
        return ERR_READ_ERROR;
    }
    buffer[stat] = '\0';
    int arglen = 0;
    char** args = NULL;
    
    char* tok;
    tok = strtok(buffer," \n\t\v\f\r");
    
    int cmd;
    
    if(tok == NULL){
        *argc = arglen;
        *argv = args;
        return USER_UNKNW_CMD;
    }
    if(!strcmp(tok, COMMAND_CONNECT)){
        cmd = USER_CONNECT;
    }
    else if(!strcmp(tok, COMMAND_EXIT)){
        cmd = USER_EXIT;
    }
    else if(!strcmp(tok, COMMAND_PRINT)){
        cmd = USER_PRINT;
    } else {
        cmd = USER_UNKNW_CMD;
        args = realloc(args, (sizeof *args)*(arglen + 1));
        args[arglen] = malloc((sizeof **args)*(strlen(tok)+1));
        strcpy(args[arglen],tok);
        arglen++;
    }
    
    tok = strtok(NULL, " \n\t\v\f\r");
    while(tok != NULL){
        args = realloc(args, (sizeof *args)*(arglen + 1));
        args[arglen] = malloc((sizeof **args)*(strlen(tok)+1));
        strcpy(args[arglen],tok);
        arglen++;
        tok = strtok(NULL, " \n\t\v\f\r");
    }
    *argc = arglen;
    *argv = args;

    return cmd;
}

void freeargs(int argc, char** argv){
    for(int i = 0; i < argc; i++){
        free(argv[i]);
    }
    free(argv);
}
