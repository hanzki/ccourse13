#ifndef DHTPACKETTYPES_H
#define DHTPACKETTYPES_H

#include <stdint.h>

#define DHT_REGISTER_BEGIN 1        // packet types for registeration
#define DHT_REGISTER_ACK 2
#define DHT_REGISTER_FAKE_ACK 3
#define DHT_REGISTER_DONE 4

#define DHT_DEREGISTER_BEGIN 11     // packet types for deregisteration
#define DHT_DEREGISTER_ACK 12
#define DHT_DEREGISTER_DONE 13
#define DHT_DEREGISTER_DENY 14	

#define DHT_GET_DATA 21             // other packet types
#define DHT_PUT_DATA 22
#define DHT_DUMP_DATA 23
#define DHT_PUT_DATA_ACK 24
#define DHT_DUMP_DATA_ACK 25
#define DHT_SEND_DATA 26
#define DHT_TRANSFER_DATA 27
#define DHT_NO_DATA 28
	
#define DHT_ACQUIRE_REQUEST 31      // requests and acks
#define DHT_ACQUIRE_ACK 32
#define DHT_RELEASE_REQUEST 33
#define DHT_RELEASE_ACK 34

#define DHT_SERVER_SHAKE        0x413f  // shake values
#define DHT_CLIENT_SHAKE        0x4121

#define MAX_PAYLOAD_SIZE        65535   //other named constants
#define DHT_HEADER_SIZE         44

typedef unsigned char sha1_t[20]; //used for storing 160bit SHA1 hashes
struct dhtpck {
    sha1_t target;      // hash of key, or same as below
    sha1_t sender;      // senders hash
    uint16_t req_type;  // type of packet, see above
    uint16_t pl_length; // length of payload
    unsigned char payload[MAX_PAYLOAD_SIZE]; // Other data being send
};

typedef struct dhtpck DHT_t; // shorter name for struct dhtpck 

#endif
