/* 
 * File:   datastorage.h
 * Author: Hanzki
 *
 * Created on 17. huhtikuuta 2013, 22:10
 */

#ifndef DATASTORAGE_H
#define	DATASTORAGE_H

#include "dhtpacket.h"

struct item{
    sha1_t key;
    uint16_t size;
    unsigned char* data;
};

typedef struct item item_t;

/**
 * Initializes the internal data storage hash table
 * @return 0 on success and non-zero if error happened
 */
int initialize_storage();

/**
 * frees all the memory allocated to internal data storage and items stored into it
 */
void free_storage();

/**
 * hard copies data to new item struct and saves it into internal hash table.
 * If the load factor grows too high the hash table is extended.
 * @param key SHA1 key of data
 * @param size size of the data in bytes
 * @param data pointer
 * @return 0 on success and non-zero if error happened
 */
int store_data(sha1_t key, uint16_t size, void* data);

/**
 * deletes data item with given key from the internal hash table.
 * @param key SHA1 key of data
 * @return 0 on success and non-zero if error happened
 */
int delete_data(sha1_t key);

/**
 * Retrieves data from the internal hash table
 * @param key SHA1 key of data
 * @param container pointer to destination where the data is copied
 * @return number of bytes copied and negative number on errors
 */
int get_data(sha1_t key, void* container);

/**
 * Retrives a data item that belongs to otherkey from the internal hash table.
 * Can be called multiple times with return value of previous call as startindex
 * to retrieve all the data items belonging to otherkey
 * @param mykey my SHA1 key
 * @param otherkey their SHA1 key
 * @param container pointer to destination where found data is copied
 * @param startindex internal hash array index where searching is started
 * @return internal hash array index of the found data item + 1,
 * if the end of hash table is reached returns 0 and on error returns negative number
 */
int get_others_data(sha1_t mykey, sha1_t otherkey, void* container, int startindex);

/**
 * deletes all data items that belong to otherkey
 * @param mykey my SHA1 key
 * @param otherkey their SHA1 key
 * @return number of items deleted and negative on errors
 */
int delete_transferred(sha1_t mykey, sha1_t otherkey);
#endif	/* DATASTORAGE_H */

