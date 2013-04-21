/* 
 * File:   dhtnode.h
 * Author: User
 *
 * Created on 4. huhtikuuta 2013, 21:43
 */

#ifndef DHTNODE_H
#define	DHTNODE_H

#include <stdlib.h>

/**
 * Kill the node after printing error msg
 * @param reason - error message
 */
void die(const char *reason);

/**
 * Turn url and port number to TCP-format
 * Handles error for invalid return value from accept() with die and error msg
 * @param url - URL addres to use
 * @param port - Port number to use
 * @param data - Address to store result
 */
void format_url(const char* url, const char* port, unsigned char* data);

/**
 * Create hash for TCP address with SHA1
 * @param url - URL address
 * @param port - port number
 * @param hash - store hash here
 */
void hash_url(const char* url, const char* port, sha1_t hash);

/**
 * Create new connection to civen address
 * @param url - URL to connect to
 * @param port - Port to use for connection
 * @return - file descriptor for connection
 */
int create_connection(const char* url, const char* port);

/**
 * Creates listening socket in order to listen incoming connection reqests from 
 * server of from other nodes on specific port
 * @param port
 * @return Returns file descriptor listening socket 
 */
int create_listen_socket();


/**
 * Accepts connection from given socket, sends server shake and returns new socket
 * for this connection.
 * @param listensocket - integer file descriptor for listensocket 
 * @return integer for file descriptor of accepted socket
 */
int accept_connection(int listensocket);


#endif	/* DHTNODE_H */
