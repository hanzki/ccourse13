/* 
 * File:   consoleui.h
 * Author: Hanzki
 *
 * Created on 5. huhtikuuta 2013, 17:06
 */

#ifndef CONSOLEUI_H
#define	CONSOLEUI_H


#define USER_EXIT       0
#define USER_CONNECT    1
#define USER_PRINT      2
#define USER_UNKNW_CMD   99

#define ERR_READ_ERROR -1
#define ERR_BUF_FULL   -2

/*
 * Recive command from given source
 * int src - file descriptor for source, for excample std in or socket
 * int* argc - number of arguments is saved to this address
 * char*** argv - array of a string arguments is saved here
 * Returns integer defined above as value for command (0-99) or error (<0)
 */
int getcommand(int src, int* argc, char*** argv);

/*
 * Free arquments malloced at get command
 * int argc - number of arguments
 * char** argv - array of argument strings
 */
void freeargs(int argc, char** argv);


#endif	/* CONSOLEUI_H */

