#include "networker.h"
#include "dhtpacket.h"
#include "dhtnode.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>

void print_dht_packet(DHT_t* packet){
    printf("target:");
    for(int i = 0; i < 20; i++){
        printf(" %02x",packet->target[i]);
    }
    printf("\nsender:");
    for(int i = 0; i < 20; i++){
        printf(" %02x",packet->sender[i]);
    }
    printf("\ntype: %d\n",ntohs(packet->req_type));
    printf("length: %d\n",ntohs(packet->pl_length));
    printf("data:");
    for(int i = 0, len = ntohs(packet->pl_length);i<len;i++){
        printf(" %02x", packet->payload[i]); 
        //printf(" %c", packet->payload[i]); /*enable this if you want to see payload in chars*/
    }
    printf("\n");
}

int send_shake(int sock, int type){
    uint16_t shake;
    if (type == NTW_SERVER_SHAKE){
        shake = htons(DHT_SERVER_SHAKE); // Send server shaken
        printf("Sending server shake\n");
    }
    else if(type == NTW_CLIENT_SHAKE){
        shake = htons(DHT_CLIENT_SHAKE); // Send client shake
        printf("Sending client shake\n");
    }
    else die("Wrong type of shake for send shake"); // This was used incorrectly
    int len = sizeof shake;
    int ok = sendall(sock, (char*) &shake, &len);
    if(len != sizeof shake){ // Wrong length of shake send, thats odd?
        printf("Error while shaking: wrong length of shake send\n"); // Better inform user
    }
    if(ok == -1){ // Sending just failed 
        printf("Sending failed\n"); // Better inform user
    }
    return ok;
}

int recvdata(int connection, DHT_t* packet) {
    size_t status, rb = 0;

    struct timeval tv;
    fd_set readfds;

    int retval;
    if (packet == NULL) {
        unsigned char buf[2];
        while (rb < 2) {
            tv.tv_sec = 2;
            tv.tv_usec = 500000;
            FD_ZERO(&readfds);
            FD_SET(connection, &readfds);
            retval = select(connection + 1, &readfds, NULL, NULL, &tv);

            if(retval == -1){
                return NTW_ERR_SERIOUS;
            }
            if(retval == 0){
                return NTW_ERR_TIMEOUT;
            }
            if (FD_ISSET(connection, &readfds)) {
                status = recv(connection, buf + rb, 2 - rb, 0);
                
                if (status == -1) {
                    return NTW_ERR_SERIOUS;
                }

                if (status == 0) {
                    return NTW_DISCONNECT;
                }
                rb += status;
            } 
        }
        
        uint16_t* shake = (uint16_t*)buf;
        if (ntohs(*shake) == DHT_SERVER_SHAKE) {
            return NTW_SERVER_SHAKE;
        }
        if (ntohs(*shake) == DHT_CLIENT_SHAKE){
            return NTW_CLIENT_SHAKE;
        }
        return NTW_ERR_CORRUPT_PCK;
    }
    //package reading isn't tested properly so there may be bugs
    size_t package_len = DHT_HEADER_SIZE;
    unsigned char* buf = (unsigned char *) packet; 
    while(rb < package_len){
        tv.tv_sec = 2;
        tv.tv_usec = 500000;
        FD_ZERO(&readfds);
        FD_SET(connection, &readfds);
        retval = select(connection + 1, &readfds, NULL, NULL, &tv);
        
        if(retval == -1){
            return NTW_ERR_SERIOUS;
        }
        if(retval == 0){
            return NTW_ERR_TIMEOUT;
        }
        if (FD_ISSET(connection, &readfds)) {
            status = recv(connection, buf + rb, package_len - rb, 0);
            if (status == -1) {
                return NTW_ERR_SERIOUS;
            }

            if (status == 0) {
                return NTW_DISCONNECT;
            }
            rb += status;
            if(rb == DHT_HEADER_SIZE){
                package_len += ntohs(packet->pl_length);
            }
        }
    }
    return NTW_PACKET_OK;
}

int sendall(int s, char *buf, int *len) {
    int total = 0; // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while (total < *len) { //send remaining bytes until all were sent
        n = send(s, buf + total, bytesleft, 0); // or we get error
        if (n == -1) {
            break;
        }
        total += n;
        bytesleft -= n;
    }
    *len = total; // return number actually sent here
    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}

int send_dht_pckt(int connection, int type, sha1_t target, sha1_t sender, uint16_t pl_len, void* data){
    DHT_t pack;                         // prepare the pack
    int len = DHT_HEADER_SIZE;
    memcpy(pack.sender, sender, 20);
    memcpy(pack.target, target, 20);
    pack.req_type = htons(type);
    pack.pl_length = htons(pl_len);
    if(pl_len){                         // add payload if there is one
        memcpy(pack.payload, data, pl_len);
        len += pl_len;
    }
    
    //DEBUG PRINT
    printf("sending packet:\n");
    print_dht_packet(&pack);
    
    int ok = sendall(connection, (char*) &pack, &len); //send the packet
    if(ok == -1){                                       // check if ok
        printf("sending packet failed\n");
        return -1;
    }
    return 0;
}
