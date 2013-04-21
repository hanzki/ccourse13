/* 
 * File:   networker.h
 * Author: Hanzki
 *
 * Created on 6. huhtikuuta 2013, 18:16
 */

#ifndef NETWORKER_H
#define	NETWORKER_H

#include "dhtpacket.h"

#define NTW_DISCONNECT          0
#define NTW_PACKET_OK           1
#define NTW_SERVER_SHAKE        2
#define NTW_CLIENT_SHAKE        3

#define NTW_ERR_SERIOUS         -1
#define NTW_ERR_CORRUPT_PCK     -2
#define NTW_ERR_TIMEOUT         -3

/**
 * Debug function to print dht_packets
 * @param packet Packet that will be printed
 */
void print_dht_packet(DHT_t* packet);

/**
 * Function for sending server and client shakes
 * @param sock socket to which send shake
 * @param type NTW_SERVER_SHAKE or NTW_CLIENT_SHAKE
 * @return -1 on error, 0 on succes
 */
int send_shake(int sock, int type);

/**
 * Receive data from given connection and store it to given packet
 * @param connection 
 * @param packet Empty packet to which save received data
 * @return NTW_PACKET_OK on success, NTW_ERR_(TIMEOUT, DISCONNET, SERIOUS,
 *  CORRUPT_PCK)  on error and NTW_(SERVER, CLIENT)_SHAKE if received data was a
 * handshake
 */
int recvdata(int connection, DHT_t* packet);

/**
 * Send len number of bytes from char array buf trough socket s
 * @param s Socket to use
 * @param buf Poiter to data which to send
 * @param len How many bytes to send
 * @return -1 on failure, 0 on success
 */
int sendall(int s, char *buf, int *len);

/**
 * Send dht packet to connection with given infortmation
 * @param connection Socket to use
 * @param type See dhtpacket.h for types
 * @param target Target hash
 * @param sender Sender hash
 * @param pl_len Packet length
 * @param data Payload for the packet
 * @return Returns 0 on success, -1 on failure
 */
int send_dht_pckt(int connection, int type, sha1_t target, sha1_t sender, uint16_t pl_len, void* data);


#endif	/* NETWORKER_H */

