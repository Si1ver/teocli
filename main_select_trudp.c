/**
 * \file   main_select.c
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * \example main_select.c
 *
 * This is example of Teocli library which use select to check that data is
 * available ready to read. This application connect to network
 * L0 server, initialize (login) at the L0 server, and send and receive data to
 * from network peer.
 *
 * See server example parameters at:
 *   https://gitlab.ksproject.org/teonet/teocli/blob/master/README.md#basic-teocli-example
 *
 * ### This application parameters:
 *
 * **Usage:**   ./teocli_s <client_name> <server_address> <server_port> <peer_name> [message]
 *
 * **Example:** ./teocli_s C3 127.0.0.1 9000 teostream "Story about this world!"
 *
 * ### This application algorithm:
 *
 * *  Connect to L0 server with server_address and server_port parameters
 * *  Send ClientLogin request with client_name parameter
 * *  Send CMD_L_PEERS request to peer_name server
 * *  Receive peers data from peer_name server
 * *  Send CMD_L_ECHO request to peer_name server
 * *  Receive peers data from peer_name server
 * *  Close connection
 *
 * Created on October 19, 2015, 3:51 PM
 */

#if defined(_WIN32) || defined(_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !(defined(_WIN32) || defined(_WIN64))
#include <unistd.h>
#endif
#include <errno.h>

#include "libteol0/teonet_l0_client.h"
#include "libtrudp/src/trudp.h"
#include "libtrudp/src/utils.h"

#include <stdarg.h>

#define TL0CNS_VERSION "0.0.2"

// Application constants
#define SEND_MESSAGE_AFTER_MIN  15000 /* 16667 */ // uSec (mSec * 1000)
#define SEND_MESSAGE_AFTER  1000000
#define RECONNECT_AFTER 3000000 // uSec (mSec * 1000)
#define SHOW_STATISTIC_AFTER 500000 // uSec (mSec * 1000)

#define DEBUG 1
#define send_l0_event_udp(tcd, event, data, data_length, u_data) \
if (((teoLNullConnectData*)((trudpData *)((trudpChannelData *)tcd)->td)->user_data)->event_cb != NULL) { \
    ((teoLNullConnectData*)((trudpData *)((trudpChannelData *)tcd)->td)->user_data)->event_cb(tcd, event, data, data_length, ((teoLNullConnectData*)((trudpData *)((trudpChannelData *)tcd)->td)->user_data)->user_data); \
}

/**
 * Show error and exit
 *
 * @param fmt
 * @param ...
 */
static void die(char *fmt, ...)
{
	va_list ap;
	fflush(stdout);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(1);
}


static const int BUFFER_SIZE = 2048;

/**
 * Teonet L0 client event callback
 *
 * @param con
 * @param event
 * @param data
 * @param data_len
 * @param user_data
 */
void event_cb(void *tcd, teoLNullEvents event, void *data,
            size_t data_len, void *user_data) {

    const struct app_parameters *param = user_data;

    switch(event) {

        case EV_L_CONNECTED:
        {
            int *fd = data;
            if(*fd > 0) {

                printf("Successfully connect to server\n");

//                // Send (1) Initialization packet to L0 server
//                ssize_t snd = teoLNullLogin(con, param->host_name);
//                if(snd == -1) perror(strerror(errno));
//                printf("\nSend %d bytes packet to L0 server, "
//                       "Initialization packet\n",
//                       (int)snd);

                // Send (2) peer list request to peer, command CMD_L_PEERS
                //snd = teoLNullSend(con, CMD_L_PEERS, param->peer_name, NULL, 0);

                //ssize_t snd = 0;
                char buf[BUFFER_SIZE];
                
                // Send peers request
                size_t pkg_length = teoLNullPacketCreate(buf, BUFFER_SIZE, /*79*/  CMD_L_PEERS, param->peer_name, NULL, 0);
                ssize_t snd = trudpChannelSendData(tcd, buf, pkg_length);
                //
                printf("Send %d bytes packet to L0 server to peer %s, "
                       "cmd = %d (CMD_L_PEERS)\n",
                       (int)snd, param->peer_name, CMD_L_PEERS);

                // Send lients request
                pkg_length = teoLNullPacketCreate(buf, BUFFER_SIZE, CMD_L_L0_CLIENTS, param->peer_name, NULL, 0);
                snd = trudpChannelSendData(tcd, buf, pkg_length);
                //
                printf("Send %d bytes packet to L0 server to peer %s, "
                       "cmd = %d (CMD_L_L0_CLIENTS)\n",
                       (int)snd, param->peer_name, CMD_L_L0_CLIENTS);

//                // Send (3) echo request to peer, command CMD_L_ECHO
//                //
//                // Add current time to the end of message (it should be return
//                // back by server)
//                snd = teoLNullSendEcho(con, param->peer_name, param->msg);
//                if(snd == -1) perror(strerror(errno));
//                printf("Send %d bytes packet to L0 server to peer %s, "
//                       "cmd = %d (CMD_L_ECHO), "
//                       "data: %s\n",
//                       (int)snd, param->peer_name, CMD_L_ECHO, param->msg);

                // Show empty line
                printf("\n");

            }
            else {

                printf("Can't connect to server\n");
            }

        } break;

        case EV_L_DISCONNECTED:
            printf("Disconnected ...\n");
            break;

        case EV_L_RECEIVED:
        {
            // Receive answer from server
            const size_t rc = data_len;
            teoLNullCPacket *cp = (teoLNullCPacket*) data;

            printf("Receive %d bytes: %hu bytes data from L0 server, "
                    "from peer %s, cmd = %hhu\n",
                    (int)rc, cp->data_length, cp->peer_name, cp->cmd);

            // Process commands
            switch(cp->cmd) {

                case CMD_L_PEERS_ANSWER:
                {
                    // Show peer list
                    if(cp->data_length > 1) {

                        ksnet_arp_data_ar *arp_data_ar = (ksnet_arp_data_ar *)
                                (cp->peer_name + cp->peer_name_length);
                        const char *ln =
                                "--------------------------------------------"
                                "---------\n";
                        printf("%sPeers (%u): \n%s", ln, arp_data_ar->length, ln);
                        int i;
                        for(i = 0; i < (int)arp_data_ar->length; i++) {

                            printf("%-12s(%2d)   %-15s   %d %8.3f ms\n",
                                arp_data_ar->arp_data[i].name,
                                arp_data_ar->arp_data[i].data.mode,
                                arp_data_ar->arp_data[i].data.addr,
                                arp_data_ar->arp_data[i].data.port,
                                arp_data_ar->arp_data[i].data.last_triptime);

                        }
                        printf("%s", ln);
                    }
                } break;
                
                case CMD_L_L0_CLIENTS_ANSWER: {

                    // Show peer list
                    teonet_client_data_ar *client_data_ar = (teonet_client_data_ar *)
                            (cp->peer_name + cp->peer_name_length);
                    const char *ln = "--------------------------------------------"
                                     "---------\n";
                    printf("%sClients (%u): \n%s", ln, client_data_ar->length, ln);
                    int i;
                    for(i = 0; i < (int)client_data_ar->length; i++) {

                        printf("%-12s\n", client_data_ar->client_data[i].name);
                    }
                    printf("%s", ln);

                } break;

                case CMD_L_ECHO_ANSWER:
                {
                    printf("Got echo answer command\n");
                    data = cp->peer_name + cp->peer_name_length;
                    int trip_time = teoLNullProccessEchoAnswer(data);

                    // Show data
                    printf("Data: %s\n", (char*)data);

                    // Show trip time
                    printf("Trip time: %d ms\n\n", trip_time);

                } break;

                case CMD_L_ECHO:
                {
                    printf("Got echo command\n");

                } break;

                case CMD_L_AUTH_LOGIN_ANSWER: {

                    printf("Got answer from authentication server\n");

                    const char *auth_data = (const char *)
                            (cp->peer_name + cp->peer_name_length);

                    // Show data
                    printf("Data: %s\n\n", auth_data);

                    connected_flag = 1;
                    send_l0_event_udp(tcd, EV_L_CONNECTED, &((trudpChannelData *)tcd)->fd, sizeof(((trudpChannelData *)tcd)->fd), NULL);
                }
                break;


                default:
                    printf("Got unknown command\n");
                    break;
            }

        } break;

        default:
            break;
    }
}






#include  <signal.h>
volatile sig_atomic_t quit_flag = 0;

void INThandler(int sig)
{
    printf("Catch CTRL-C SIGNAL !!!\n");
    quit_flag = 1;
}



/**
 * Main L0 Native client example function
 *
 * @param argc Number of arguments
 * @param argv Arguments array
 *
 * @return
 */
int main(int argc, char** argv) {
   signal(SIGINT, INThandler); 
    // Welcome message
    printf("Teonet L0 client with Select and Event Loop Callback example "
           "version " TL0CN_VERSION " (Native TCP/UDP Client)\n\n");

    // Check application parameters
    if(argc < 5) {
        printf("Usage: %s <client_name> <server_address> <server_port> <peer_name> "
               "[message]\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Teonet L0 server parameters
    struct app_parameters param;
    param.host_name = argv[1]; //"C3";
    param.tcp_server = argv[2]; //"127.0.0.1"; //"10.12.35.53"; //
    param.tcp_port = atoi(argv[3]); //9000;
    param.peer_name = argv[4]; //"teostream";
    if(argc > 5) param.msg = argv[5];
    else param.msg = "Hello"; // from TRUdp client :)";

    // Initialize L0 Client library
    teoLNullInit();
     
    teoLNullConnectData* con = l0_connect(event_cb, &param, TR_UDP);

    // Connect to L0 TR-UDP server
    // Bind UDP port and get FD (start listening at port)

    int port = 9090; //atoi(o_local_port); // Local port
    int fd = trudpUdpBindRaw(&port, 1);
    if(fd <= 0) die("Can't bind UDP port ...\n");
    else printf("Start listening at UDP port %d\n", port);

    if(fd > 0) {
        
        // Initialize TR-UDP
        remote_port_i = param.tcp_port;
        remote_address = (char*)param.tcp_server;
//        teoLNullConnectData* con = trudpLNullConnect(event_cb, &param);
        trudpData *td = trudpInit(fd, port, trudpEventCback, con);

        printf("TR-UDP port created, fd = %d\n", td->fd);

        // Create read buffer
        buffer = malloc(BUFFER_SIZE);

        uint32_t tt, tt_s = 0, tt_c = 0, tt_ss = 0;
        const int DELAY = 500000; // uSec
        unsigned long num = 0;


        trudpChannelData *tcd = NULL;

        // Event loop
        while(!quit_flag) {

            trudpNetworkSelectLoop(td, SEND_MESSAGE_AFTER < DELAY ?
                SEND_MESSAGE_AFTER : DELAY);

            // Current timestamp
            tt = trudpGetTimestamp();

            // Connect
            if(!connected_flag && (tt - tt_c) > RECONNECT_AFTER) {
                if (tcd) {
                    trudpChannelDestroy(tcd);
                    tcd = NULL;
                }
                tcd = trudpLNullLogin(td, param.host_name);
                tt_c = tt;
            }

            // When connected
            if(connected_flag) {
                // Send Echo command every 1 second
                if((tt - tt_s) > SEND_MESSAGE_AFTER * 1) {

                    //printf("tick...\n");

                    char buf[BUFFER_SIZE];
                    // Send ping
                    size_t pkg_length = teoLNullPacketCreateEcho(buf, BUFFER_SIZE, param.peer_name, param.msg);
                    trudpChannelSendData(tcd, buf, pkg_length);
                    
                    tt_s = tt;
                }
                else trudpProcessKeepConnection(td);
            }
        }

        // Destroy TR-UDP
        teoLNullDisconnect(con);
        trudpChannelDestroy(tcd);

        trudpDestroy(td);
        free(buffer);
    }

    return (EXIT_SUCCESS);
}
