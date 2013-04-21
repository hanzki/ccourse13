/* Reading the documentation for select and for TCP/IP is strongly advised;
   see e.g. man pages:
   select(2)
   select_tut(2)
   tcp(7)
   ip(7) */
//Pinkie Pie is the best pony

#include "dhtpacket.h"
#include "dhtnode.h"
#include "consoleui.h"
#include "networker.h"
#include "datastorage.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>


#define EXAMPLE_TCP_PORT        "9876"
#define MAX_CONNECTIONS         5

//macros for connection struct
#define ST_EMPTY                0 //socket is disconnected
#define ST_WF_SER_SHAKE         1 //waiting for server shake from server/node
#define ST_WF_CLI_SHAKE         2 //waiting for client shake from node
#define ST_ONLINE               3 //connected to server waiting for possible fake_ack
#define ST_IDLING               4 //full member of network waiting for possible register_begin
#define ST_RCV_DATA_REG         5 //receiving data from node which is followed by register ack
#define ST_RCV_DATA_DEREG       6 //receiving data from node which is followed by deregister begin
#define ST_WF_REG_DONE          7 //waiting for register done from server when neighbour is connecting
#define ST_WF_DEREG_ACK         8 //waiting for dereg ack from server when starting deregister sequence
#define ST_WF_DEREG_DONE        9 //waiting for deregister done from server so we can leave
#define ST_REG_DATA_RCVD        10 //node is disconnected but this is used to mark that data is received
#define ST_DEREG_DATA_SENT      11//all data sent and waiting for receipt through server

struct connection{
    int         socket;
    sha1_t      hash;
    int         state;
}; //Struct for holding information about connections

typedef struct connection con_t;


void die(const char *reason) { 
    fprintf(stdout, "Fatal error: %s\n", reason); 
    exit(1);
}

void format_url(const char* url, const char* port, unsigned char* data) {
    uint16_t portnum = atoi(port);   
    portnum = htons(portnum);        
    memcpy(data, &portnum, 2);      
    strcpy((char*) data + 2, url);
}

void hash_url(const char* url, const char* port, sha1_t hash) {
    size_t len = strlen(url) + 3;
    unsigned char data[len];
    format_url(url,port,data);
    SHA1(data, len, hash);
}

int create_connection(const char* url, const char* port) {
    int status, sock;           // Creates new socket and connects it to given
    struct addrinfo hints;      // url address and port.
    struct addrinfo *res;       // If something fails, report error and close.

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(url, port, &hints, &res)) != 0) {
        die(gai_strerror(status));
    }
    if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
        die(strerror(errno));
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) == -1) {
        die(strerror(errno));
    }
    return sock;
}

int create_listen_socket(char* port) {
    int status, sock;           
    struct addrinfo hints;      
    struct addrinfo *servinfo;  

    memset(&hints, 0, sizeof hints);    // make sure the struct is empty
    hints.ai_family = AF_INET;          // use IPv4
    hints.ai_socktype = SOCK_STREAM;    // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;        // fill in my IP for me
    //filling addrinfo
    if ((status = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        die(gai_strerror(status));
    }
    //creating socket
    if ((sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        die(strerror(errno));
    }
    //bind socket to a port
    if (bind(sock, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        die(strerror(errno));
    }
    //start listening the socket
    if (listen(sock, MAX_CONNECTIONS) == -1) {
        die(strerror(errno));
    }
    return sock;
}

int accept_connection(int listensocket){
    int a = accept(listensocket, NULL, 0); //accept new connection from listensocket
    if(a > 0){  // if connection is filedescriptor pointer
        send_shake(a, NTW_SERVER_SHAKE); // Send server shake
    }
    else{ // Failed to recive valid file descriptor from accept
        printf("Error with accept: %s\n", strerror(errno));
        die("Accepting connection failed\n");
    }
    return a;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { // Checking for correct arquments
        printf("Useage: dhtnode own_hostname own_port\n");
        exit(0);
    }
    char* my_url = argv[1];
    char* my_port = argv[2];
    
    sha1_t my_hash; // Create hash for own address
    hash_url(my_url, my_port, my_hash);
    fd_set master;
    fd_set rfds;
    int retval;
    int running = 1;
    int listensock = create_listen_socket(my_port);
    con_t server;
    memset(&server,0,sizeof server);
    con_t left;
    memset(&left,0,sizeof left);
    con_t right;
    memset(&right,0,sizeof right);
    FD_SET(STDIN_FILENO, &master);
    FD_SET(listensock, &master);
    
    int fdmax = listensock;
    struct timeval looptime;

    while (running) {
        looptime.tv_sec = 1;
        looptime.tv_usec = 0;
        rfds = master;
        retval = select(fdmax + 1, &rfds, NULL, NULL, &looptime);

        if (retval == -1)
            die("select failed");
        else if (retval) {
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                int argcount;
                char** arguments = NULL;
                int cmd = getcommand(STDIN_FILENO, &argcount, &arguments);
                printf("got command\n");
                if (cmd < 0) {
                    //TODO handle different errors differently
                    printf("error: %d\n", cmd);
                    exit(-1);
                }
                switch (cmd) {
                    case USER_EXIT:
                        if(server.state != ST_EMPTY && (argcount == 0 || strcmp(arguments[0],"force"))){//remember strcmp returns 0 on match
                            printf("trying to exit\n");
                            send_dht_pckt(server.socket,DHT_DEREGISTER_BEGIN,my_hash,my_hash,0,NULL);
                        }
                        else{
                            printf("exiting\n");
                            running = 0;
                        }
                        freeargs(argcount, arguments);
                        break;
                    case USER_CONNECT:
                        if(argcount != 2){
                            printf("usage: connect url port\n");
                            freeargs(argcount,arguments);
                            break;
                        }
                        if(server.state != ST_EMPTY){
                            printf("Already connected to the server. Use exit to disconnect.\n");
                            freeargs(argcount,arguments);
                            break;
                        }
                        printf("connecting to %s:%s\n", arguments[0], arguments[1]);
                        int sock = create_connection(arguments[0], arguments[1]);
                        printf("connection made\n");
                        FD_SET(sock, &master);
                        if(sock > fdmax) fdmax = sock;
                        server.socket = sock;
                        server.state = ST_WF_SER_SHAKE;
                        
                        freeargs(argcount, arguments);
                        break;
                    case USER_PRINT:
                        printf("printing...\n");
                        for(int i = 0; i < argcount; i++){
                            printf("%s:\n",arguments[i]);
                            if(!strcmp(arguments[i],"sockets")){
                               printf("server, %d %s\n",server.socket,(server.socket != 0 && FD_ISSET(server.socket,&master))? "is set":"not set"); 
                               printf("right, %d %s\n",right.socket,(right.socket != 0 && FD_ISSET(right.socket,&master))? "is set":"not set"); 
                               printf("left, %d %s\n",left.socket,(left.socket != 0 && FD_ISSET(left.socket,&master))? "is set":"not set"); 
                            }
                            else if(!strcmp(arguments[i],"states")){
                                printf("server, %d\n",server.state);
                                printf("right, %d\n",right.state);
                                printf("left, %d\n",left.state);
                            }
                        }
                        freeargs(argcount,arguments);
                        break;
                    case USER_UNKNW_CMD:
                        printf("unknown command, line was:");
                        for (int i = 0; i < argcount; i++) {
                            printf(" %s", arguments[i]);
                        }
                        printf("\n");
                        freeargs(argcount, arguments);
                        break;
                }
            }
            //check for new connections
            if (FD_ISSET(listensock, &rfds)) {
                printf("someone is connecting\n");
                if(right.state == ST_EMPTY){
                    right.socket = accept_connection(listensock);
                    if(right.socket < 0){
                        die(strerror(errno));
                    }
                    FD_SET(right.socket, &master);
                    if(right.socket > fdmax) fdmax = right.socket;
                    right.state = ST_WF_CLI_SHAKE;
                } else if (left.state == ST_EMPTY) {
                    left.socket = accept_connection(listensock);
                    if(left.socket < 0){
                        die(strerror(errno));
                    }
                    FD_SET(left.socket, &master);
                    if(left.socket > fdmax) fdmax = left.socket;
                    left.state = ST_WF_CLI_SHAKE;
                }
            }
            //check for msg from server
            if(server.state != ST_EMPTY && FD_ISSET(server.socket, &rfds)){
                int status;
                if(server.state == ST_WF_SER_SHAKE){
                    printf("getting shake\n");
                    status = recvdata(server.socket, NULL);
                    if(status == NTW_SERVER_SHAKE){
                        printf("shaking back\n");
                        send_shake(server.socket, NTW_CLIENT_SHAKE);
                        unsigned char formated_url[strlen(my_url)+3];
                        format_url(my_url, my_port, formated_url);
                        printf("sending DHT_REGISTER_BEGIN\n");
                        send_dht_pckt(server.socket,DHT_REGISTER_BEGIN,my_hash,my_hash,strlen(my_url)+3,formated_url);
                        server.state = ST_ONLINE;
                    }
                    //TODO error handling
                    /*On correct shake shake back and send DHT_REGISTER_BEGIN
                     with our sha1 as sender and target and
                     our port+url as payload
                     switch state to ST_ONLINE*/
                } else{
                    DHT_t packet;
                    memset(&packet, 0, sizeof packet);
                    status = recvdata(server.socket, &packet);
                    if(status != NTW_PACKET_OK){
                        switch(status){
                            case NTW_ERR_SERIOUS:
                                die(strerror(errno));
                                break;
                            case NTW_DISCONNECT:
                                die("server disconnected");
                                break;
                            default:
                                printf("getting packet failed somehow\n");
                                memset(&packet, 0, sizeof packet);
                                break;
                        }
                    }
                    printf("received from server:\n");
                    print_dht_packet(&packet);
                    //TODO error handling
                    uint16_t reqt = ntohs(packet.req_type);
                    if(server.state == ST_ONLINE && reqt == DHT_REGISTER_FAKE_ACK){
                        printf("fake ack received\n");
                        /*send DHT_REGISTER_DONE to server
                         target and sender are both our hash
                         state to ST_IDLING*/
                        int a = send_dht_pckt(server.socket, DHT_REGISTER_DONE, my_hash, my_hash, 0, NULL);
                        if(a == 0) server.state = ST_IDLING;
                        else exit(1); // or error handling
                        
                    } else if(server.state == ST_IDLING && reqt == DHT_REGISTER_BEGIN){
                        printf("dht_register_begin received\n");
                        /*open connection to the new node
                         which port+url is in packet and
                         send data + DHT_REGISTER_ACK
                         our hash sender and target
                         state to ST_WF_REG_DONE*/
                        char his_url[packet.pl_length-2];
                        strcpy(his_url,(char*)packet.payload+2);
                        uint16_t* port = (uint16_t*)packet.payload;
                        *port = ntohs(*port);
                        char his_port[10];
                        sprintf(his_port,"%u",*port);
                        int sock = create_connection(his_url,his_port);
                        FD_SET(sock, &master);
                        if(sock > fdmax) fdmax = sock;
                        con_t* pal;
                        pal = (right.state == ST_EMPTY) ? &right : &left;
                        memcpy(pal->hash,packet.sender,20);
                        pal->socket = sock;
                        pal->state = ST_WF_SER_SHAKE;
                        //server.state = ST_WF_REG_DONE;
                        
                    } else if(server.state == ST_IDLING && reqt == DHT_REGISTER_DONE){
                        printf("received reg done\n");
                        //server.state = ST_IDLING;
                        /*dump old data and return to ST_IDLING*/
                    } else if(server.state == ST_IDLING && reqt == DHT_DEREGISTER_ACK){
                        printf("dereg ack received\n");
                        /*open connection to neighbours their urls are as
                         payload of the package and send data and
                         DHT_DEREGISTER_BEGIN (our hash as sender and target)
                         change to ST_WF_DEREG_DONE*/
                        uint16_t port_buf = ((uint16_t*)packet.payload)[0];
                        port_buf = ntohs(port_buf);
                        char port1[10];
                        sprintf(port1,"%u",port_buf);
                        char url1[30];
                        strcpy(url1,(char*)(packet.payload+2));
                        
                        port_buf = ((uint16_t*)(packet.payload+3+strlen(url1)))[0];
                        port_buf = ntohs(port_buf);
                        char port2[10];
                        sprintf(port2,"%u",port_buf);
                        char url2[30];
                        strcpy(url2,(char*)(packet.payload+5+strlen(url1)));
                        printf("parsing got:\nurl1 = %s:%s\nurl2 = %s:%s\n", url1, port1, url2, port2);
                        
                        //connect to right neighbour
                        right.socket = create_connection(url1,port1);
                        FD_SET(right.socket, &master);
                        if(right.socket > fdmax) fdmax = right.socket;
                        hash_url(url1,port1,right.hash);
                        right.state = ST_WF_SER_SHAKE;
                        //connect to left neighbour
                        left.socket = create_connection(url2,port2);
                        FD_SET(left.socket, &master);
                        if(left.socket > fdmax) fdmax = left.socket;
                        hash_url(url2,port2,left.hash);
                        left.state = ST_WF_SER_SHAKE;
                        
                        server.state = ST_WF_DEREG_DONE;
                    } else if(server.state == ST_IDLING && reqt == DHT_DEREGISTER_DENY){
                        printf("dereg deny received\n");
                        /*tell user that leaving is impossible
                         return to ST_IDLING*/
                        printf("Unable to exit. You are the last node. To over-ride this use exit force command.\n");
                        server.state = ST_IDLING;
                    } else if(server.state == ST_WF_DEREG_DONE && reqt == DHT_DEREGISTER_DONE){
                        if(right.state == ST_DEREG_DATA_SENT && !memcmp(right.hash, packet.sender, 20)){
                            FD_CLR(right.socket, &master);
                            close(right.socket);
                            memset(&right,0,sizeof right);
                        }
                        else if(left.state == ST_DEREG_DATA_SENT && !memcmp(left.hash, packet.sender, 20)){
                            FD_CLR(left.socket, &master);
                            close(left.socket);
                            memset(&left,0,sizeof left);
                        }
                        if(left.state == ST_EMPTY && right.state == ST_EMPTY){
                            printf("leaving the network\n");
                            close(server.socket);
                            FD_CLR(server.socket, &master);
                            memset(&server,0,sizeof server);
                            printf("left the network\n");
                        }
                        /*close the connection to server
                         state to ST_EMPTY*/
                        }
                    }
                }
            }
            for(int i = 0; i<2;i++){
                con_t* neighbour = (i == 0) ? &right : &left;
            //check for msg from right and left neighbour
            if(neighbour->state != ST_EMPTY && FD_ISSET(neighbour->socket, &rfds)){
                int status;
                if(neighbour->state == ST_WF_CLI_SHAKE){
                    /* prepare to receive data
                     switch state to ST_RCV_DATA_REG or
                     ST_RCV_DATA_DEREG depending on server state*/
                    status = recvdata(neighbour->socket, NULL);
                    if(status == NTW_CLIENT_SHAKE){
                        if(server.state == ST_ONLINE){
                            neighbour->state = ST_RCV_DATA_REG;
                        }
                        if(server.state == ST_IDLING){ 
                            neighbour->state = ST_RCV_DATA_DEREG;
                        }
                    } else {
                        switch(status){
                            case NTW_ERR_SERIOUS:
                                die(strerror(errno));
                                break;
                            case NTW_DISCONNECT:
                                printf("%s disconnected\n",(i)?"left":"right");
                                FD_CLR(neighbour->socket,&master);
                                memset(neighbour,0,sizeof *neighbour);
                                break;
                            default:
                                printf("getting shake failed somehow\n");
                                break;
                        }
                    }
                } else if (neighbour->state == ST_WF_SER_SHAKE){
                    /* answer with client shake and pump your data to the neighbour
                     after that send DHT_REGISTER_ACK or DHT_DEREG_BEGIN
                     either way after this the connection to neighbour can be disconnected*/
                    status = recvdata(neighbour->socket, NULL);
                    if(status == NTW_SERVER_SHAKE){
                        printf("shaking back\n");
                        send_shake(neighbour->socket, NTW_CLIENT_SHAKE);
                        if(server.state == ST_IDLING){ //we should end with DHT_REGISTER_ACK
                            send_dht_pckt(neighbour->socket,DHT_REGISTER_ACK,my_hash,my_hash,0,NULL);
                            FD_CLR(neighbour->socket, &master);
                            close(neighbour->socket);
                            memset(neighbour,0,sizeof *neighbour);
                        }
                        if(server.state == ST_WF_DEREG_DONE){ //we should end with DHT_DEREGISTER_BEGIN
                            send_dht_pckt(neighbour->socket,DHT_DEREGISTER_BEGIN,my_hash,my_hash,0,NULL);
                            FD_CLR(neighbour->socket, &master);
                            close(neighbour->socket);
                            neighbour->state = ST_DEREG_DATA_SENT;
                        }
                    } else {
                        switch(status){
                            case NTW_ERR_SERIOUS:
                                die(strerror(errno));
                                break;
                            case NTW_DISCONNECT:
                                die("neighbour disconnected");
                                break;
                            default:
                                printf("getting shake failed somehow\n");
                                break;
                        }
                    }
                    
                } else{
                    DHT_t packet;
                    memset(&packet, 0, sizeof packet);
                    status = recvdata(neighbour->socket, &packet);
                    printf("received package from %s:\n",(i)?"left":"right");
                    print_dht_packet(&packet);
                    if(status != NTW_PACKET_OK){
                        switch(status){
                            case NTW_ERR_SERIOUS:
                                die(strerror(errno));
                                break;
                            case NTW_DISCONNECT:
                                printf("%s neighbour disconnected.\n",(i)?"left":"right");
                                FD_CLR(neighbour->socket,&master);
                                close(neighbour->socket);
                                memset(neighbour,0,sizeof * neighbour);
                                break;
                            case NTW_ERR_TIMEOUT:
                                printf("package from %s timed out!\n",(i)?"left":"right");
                                memset(&packet,0,sizeof packet);
                                break;
                        }
                    }
                    uint16_t reqt = ntohs(packet.req_type);
                    if(neighbour->state == ST_RCV_DATA_REG && reqt == DHT_REGISTER_ACK){
                        /*all data is now received */
                        FD_CLR(neighbour->socket, &master);
                        close(neighbour->socket);
                        memset(neighbour,0,sizeof *neighbour);
                        neighbour->state = ST_REG_DATA_RCVD;
                        if(right.state == ST_REG_DATA_RCVD && left.state == ST_REG_DATA_RCVD){
                            send_dht_pckt(server.socket, DHT_REGISTER_DONE, my_hash, my_hash, 0, NULL);
                            right.state = ST_EMPTY;
                            left.state = ST_EMPTY;
                            server.state = ST_IDLING;
                        }
                    } else if(neighbour->state == ST_RCV_DATA_DEREG && reqt == DHT_DEREGISTER_BEGIN){
                        send_dht_pckt(server.socket, DHT_DEREGISTER_DONE, packet.sender, my_hash, 0, NULL);
                        FD_CLR(neighbour->socket, &master);
                        close(neighbour->socket);
                        memset(neighbour,0,sizeof *neighbour);
                    }
                }
            }
        }
    }

    close(listensock);

    return 0;
}
