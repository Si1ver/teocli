/*
 * File:   main.cpp
 * Author: Kirill Scherba <kirill@scherba.ru>
 *
 * Created on January 7, 2017, 2:18 PM
 */

#if defined(_WIN32)
#if !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "libteol0/teocli.hpp"

int main(int argc, char** argv) {
    // Welcome message
    std::cout << "Teonet L0 client example ver " TL0CN_VERSION <<
            " (C++ native client)" << std::endl << std::endl;

    // Check application parameters
    if (argc < 5) {
        std::cout << "Usage: " <<
               argv[0] << " <client_name> <server_address> <server_port> "
               "<peer_name> " << "[message]" << std::endl;

        exit(EXIT_SUCCESS);
    }

    // Teonet L0 server parameters
    const char *host_name = argv[1];
    const char *TCP_SERVER = argv[2];
    const int TCP_PORT = atoi(argv[3]);
    const char *peer_name = argv[4];
    const char *msg;
    if (argc > 5) {
        msg = argv[5];
    } else {
        msg = "Hello";
    }

    // Define receive packet size
    ssize_t rc;

    // Initialize and connect to teonet L0 server
    teo::Teocli *teo = new teo::Teocli(TCP_SERVER, TCP_PORT, TCP);
    if (teo->connected() > 0) {
        // Send (1) Initialization packet to L0 server
        ssize_t snd = teo->loginHost(host_name);
        if (snd == -1) {
            perror(strerror(errno));
        }

        std::cout << "\nSend " << snd << " bytes packet to L0 server, "
            "Initialization packet" << std::endl;

        // Get L0 Server answer (1.2)
        if ((rc = teo->recvTimeout(1000000)) == -1) {
            printf("Can't get answer from L0 server during timeout\n");
        } else {
            // Show L0 answer and continue send and receive data
            // Process received data
            if (rc > 0) {
                std::cout << "Receive " << rc << " bytes: " <<
                    teo->packet()->data_length << " bytes data from L0 server, " <<
                    "from peer " << teo->packet()->peer_name << ", cmd = " <<
                    (int)teo->packet()->cmd << ", data:" <<
                    (char*)teo->packetData() << std::endl;
            }

            // Send (2) peer list request to peer, command CMD_L_PEERS
            snd = teo->send(CMD_L_PEERS, peer_name, NULL, 0);
            std::cout << "Send " << snd << " bytes packet to L0 server to peer " <<
                    peer_name << ", " <<
                    "cmd = " << CMD_L_PEERS << " (CMD_L_PEERS)" << std::endl;

            // Send (2.5) clients list request to peer, command CMD_L_L0_CLIENTS
            snd = teo->send(CMD_L_L0_CLIENTS, peer_name, NULL, 0);
            std::cout << "Send " << snd << " bytes packet to L0 server to peer " <<
                    peer_name << ", " <<
                    "cmd = " << CMD_L_L0_CLIENTS << " (CMD_L_L0_CLIENTS)" << std::endl;

            std::cout << std::endl;

            // Send (3) echo request to peer, command CMD_L_ECHO
            snd = teo->sendEcho(peer_name, msg);
            if (snd == -1) {
                perror(strerror(errno));
            }
            printf("Send %d bytes packet to L0 server to peer %s, "
                   "cmd = %d (CMD_L_ECHO), "
                   "data: %s\n",
                   (int)snd, peer_name, CMD_L_ECHO, msg);

            std::cout << std::endl;

            // Receive (1) answer from server, CMD_L_PEERS_ANSWER -----------------
            while ((rc = teo->recv()) == -1) {
                teo->sleep(50);
            }

            // Process received data
            if (rc > 0) {
                std::cout << "Receive " << rc << " bytes: " <<
                        teo->packet()->data_length << " bytes data from L0 server, " <<
                        "from peer " << teo->packet()->peer_name << ", cmd = " <<
                        (int)teo->packet()->cmd << std::endl;

                // Process CMD_L_PEERS_ANSWER
                if (teo->packet()->cmd == CMD_L_PEERS_ANSWER && teo->packet()->data_length > 1) {
                    // Show peer list
                    const char *ln = "--------------------------------------------"
                                     "---------\n";
                    std::cout << ln << "Peers (" << teo->packetArpData()->length << "): \n" << ln;
                    for (int i = 0; i < (int)teo->packetArpData()->length; i++) {
                        printf("%-12s(%2d)   %-15s   %d %8.3f ms\n",
                                teo->packetArpData()->arp_data[i].name,
                                (int)teo->packetArpData()->arp_data[i].data.mode,
                                teo->packetArpData()->arp_data[i].data.addr,
                                teo->packetArpData()->arp_data[i].data.port,
                                teo->packetArpData()->arp_data[i].data.last_triptime);
                    }
                    std::cout << ln;
                }
            }

            std::cout << std::endl;

            // Receive (1.5) answer from server, CMD_L_L0_CLIENTS_ANSWER ------
            while ((rc = teo->recv()) == -1) {
                teo->sleep(50);
            }

            // Process received data
            if (rc > 0) {
                std::cout << "Receive " << rc << " bytes: " <<
                        teo->packet()->data_length <<
                        " bytes data from L0 server, " <<
                        "from peer " << teo->packet()->peer_name << ", cmd = " <<
                        (int)teo->packet()->cmd << std::endl;

                // Process CMD_L_L0_CLIENTS_ANSWER
                if (teo->packet()->cmd == CMD_L_L0_CLIENTS_ANSWER && teo->packet()->data_length > 1) {
                    // Show clients list
                    const char *ln = "--------------------------------------------"
                                     "---------\n";
                    std::cout << ln << "Clients (" << teo->packetClientData()->length << "): \n" << ln;
                    for (int i = 0; i < (int)teo->packetClientData()->length; i++) {
                        printf("%-12s\n", teo->packetClientData()->client_data[i].name);
                    }
                    std::cout << ln;
                }
            }

            std::cout << std::endl;

            // Receive (2) answer from server ---------------------------------
            while ((rc = teo->recv()) == -1) {
                teo->sleep(50);
            }

            // Process received data
            if (rc > 0) {
                printf("Receive %d bytes: %d bytes data from L0 server, "
                        "from peer %s, cmd = %d, data: %s\n",
                        (int)rc, (int)teo->packet()->data_length,
                        teo->packet()->peer_name, (int)teo->packet()->cmd,
                        (char*)teo->packetData());

                // Process CMD_L_ECHO_ANSWER
                if (teo->packet()->cmd == CMD_L_ECHO_ANSWER) {
                    // Show trip time
                    printf("Trip time: %d ms\n", teo->packetEchoAnswerTripTime());
                }
            }

            std::cout << std::endl;

            // Check received ECHO
            std::cout << "Test result: " <<
                    ((teo->packetData() != NULL &&
                      !strcmp(msg, (char*)teo->packetData())) ? "OK" : "ERROR") <<
                    std::endl;
        }
    }

    // Cleanup L0 Client library
    delete teo;

    return 0;
}
