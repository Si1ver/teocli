/*
 * Copyright (c) 1996-2018 Kirill Scherba <kirill@scherba.ru>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <string>

#include "teonet_l0_client.h"

namespace teo {

typedef teoLNullEvents Events; //! L0 client Events
typedef teoLNullCPacket Packet; //! L0 client Packet

/**
 * Teocli class.
 *
 * This is Teonet L0 client C++ wrapper
 */
class Teocli {

public:

    /**
     * Teocli event callback (teoLNullEventsCb)
     */
    typedef void (*EventsCb)(Teocli &cli, Events event, void *data,
                         size_t data_len, void *user_data);

private:

    void *userData = NULL;
    EventsCb eventCallBack = NULL;
    teoLNullConnectData *con = NULL;
    std::string clientName = "teocli++-default";

    /**
     * Initialize L0 client library.
     *
     * Calls once per application to initialize this client library.
     */
    void init() { teoLNullInit(); }

    /**
     * Cleanup L0 client library.
     *
     * Calls once per application to cleanup this client library.
     */
    void cleanup() { teoLNullCleanup(); }

    /**
     * Return pointer to user data
     *
     * @return Pointer to user data, or NULL if not set
     */
    void* getUserData() const {
        return userData;
    }

    /**
     * Return pointer to Event CallBack
     *
     * @return Pointer to Event CallBack, or NULL if not set
     */
    EventsCb getEventCallBack() const {
        return eventCallBack;
    }

    /**
     * Set con member
     *
     * @param con
     */
    void setCon(teoLNullConnectData* con) {
        this->con = con;
    }

public:

    /**
     * Teonet client simple constructor
     */
    Teocli(const char *client = "teocli++") : clientName(client) { init(); }
    Teocli(const char *server, int port, PROTOCOL proto) : Teocli() {
      connect(server, port, NULL, NULL, proto);
    }
    Teocli(const char *client, const char *server, int port, EventsCb event_cb,
      void *user_data, PROTOCOL proto) : Teocli(client) {
      connect(server, port, user_data, event_cb, proto);
    }

    /**
     * Teonet client simple destructor
     */
    virtual ~Teocli() {
        disconnect();
        cleanup();
    }

public:

    /**
     * Create TCP client and connect to server with event callback
     *
     * @param server Server IP or name
     * @param port Server port
     * @param user_data Pointer to user data which will be send to event
     *                  callback, may be NULL or Missed
     * @param event_cb Pointer to event callback function, may be NULL or Missed
     *
     * @return connected status
     * @retval  0 if disconnected
     * @retval  0 - Success connection
     * @retval -1 - Create socket error
     * @retval -2 - HOST NOT FOUND error
     * @retval -3 - Client-connect() error
     */
    int connect(const char *server, int port,
        void *user_data, EventsCb event_cb, PROTOCOL connection_flag) {

        eventCallBack = event_cb ? event_cb :
          [](teo::Teocli &cli, teo::Events event, void *data,
            size_t data_length, void *user_data) {
            if(event != EV_L_TICK && event != EV_L_IDLE)
            cli.eventCb(event, data, data_length, user_data);
        };
        userData = user_data;
        con = teoLNullConnectE(server, port,
                eventCallBack == NULL ? NULL : callbackBind, this, connection_flag);

        return connected();
    }

    /**
     * Disconnect from server and free teoLNullConnectData
     *
     */
    void disconnect() {
        teoLNullDisconnect(con);
        con = NULL;
    }

    /**
     * Shutdown connection (disconnect from server)
     */
    void shutdown() {
      teoLNullShutdown(con);
    }

    /**
     * Check if this class is connected to L0 server
     *
     * @return connected status
     * @retval 0 if disconnected
     * @retval >0 - Success connection
     * @retval -1 - Create socket error
     * @retval -2 - HOST NOT FOUND error
     * @retval -3 - Client-connect() error
     */
    int connected() const {
        return con == NULL ? 0 : con->status;
    }

    /**
     * Create initialize L0 client packet
     *
     * @param host_name Name of this L0 client
     *
     * @return Length of send data or -1 at error
     */
    ssize_t loginHost(const char* host_name) {
        return teoLNullLogin(con, host_name);
    }

    /**
     * Create initialize L0 client packet
     *
     * @return Length of send data or -1 at error
     */
    virtual ssize_t login() {
        return loginHost(clientName.c_str());
    }

    /**
     * Set client name
     *
     * @param client_name
     */
    void setClientName(const std::string &client_name) {
      clientName = client_name;
    }

    /**
     * Get client name
     *
     * @return client_name Reference to client name string
     */
    const std::string& getClientName() const {
      return clientName;
    }

    /**
     * Send command to L0 server
     *
     * Create L0 clients packet and send it to L0 server
     *
     * @param cmd Command
     * @param peer_name Peer name to send to
     * @param data Pointer to data
     * @param data_length Length of data
     *
     * @return Length of send data or -1 at error
     */
    ssize_t send(int cmd, const char *peer_name, const void *data,
            size_t data_length) {
        return teoLNullSend(con, cmd, peer_name, data, data_length);
    }

    /**
     * Send **UNRELIABLE** command to L0 server
     *
     * Create L0 clients packet and send it to L0 server
     *
     * @param cmd Command
     * @param peer_name Peer name to send to
     * @param data Pointer to data
     * @param data_length Length of data
     *
     * @return Length of send data or -1 at error
     */
    ssize_t sendUnreliable(int cmd, const char *peer_name, const void *data,
            size_t data_length) {
        return teoLNullSendUnreliable(con, cmd, peer_name, data, data_length);
    }

    /**
     * Send Echo command
     *
     * @param peer_name
     * @param msg
     * @return
     */
    ssize_t sendEcho(const char *peer_name, const char *msg) {
        return teoLNullSendEcho(con, peer_name, msg);
    }

    /**
     * Process echo answer data
     * @return Trip time in ms
     */
    int packetEchoAnswerTripTime() {
        return teoLNullProccessEchoAnswer((char*) packetData());
    }

    /**
     * Receive packet from L0 server and split or combine it
     *
     * @return Size of packet or Packet state code
     * @retval >0 Packet received
     * @retval -1 Packet not receiving yet (got part of packet)
     * @retval -2 Wrong packet received (dropped)
     */
    ssize_t recv() {
        return teoLNullRecv(con);
    }

    /**
     * Receive data from L0 server during timeout
     *
     * @param con Pointer to teoLNullConnectData
     * @param timeout Timeout in uSeconds
     *
     * @return Number of received bytes or -1 at timeout
     */
    ssize_t recvTimeout(uint32_t timeout) {
        return teoLNullRecvTimeout(con, timeout);
    }

    /**
     * Wait socket data during timeout and call callback if data received
     *
     * @param con Pointer to teoLNullConnectData
     * @param timeout Timeout of wait socket read event in ms
     *
     * @return 0 - if disconnected or 1 other way
     */
    int eventLoop(int timeout = 0) {
        return teoLNullReadEventLoop(con, timeout);
    }
    static void eventLoopE(void *par) {
        Teocli *cli = (Teocli*) par;
        cli->eventLoop();
    }

    /**
     * Sleep
     *
     * @param ms Time to sleep in ms
     */
    void sleep(int ms) const {
        teoLNullSleep(ms);
    }

    /**
     * Return read buffer of last recv call
     * @return Pointer to teoLNullCPacket
     */
    teoLNullCPacket *packet() const {
        return (teoLNullCPacket*) con->read_buffer;
    }

    /**
     * Return packet arp data of last recv call
     * @return Pointer to ksnet_arp_data_ar
     */
    ksnet_arp_data_ar *packetArpData() const {
        return (ksnet_arp_data_ar *)
                    (packet()->peer_name + packet()->peer_name_length);
    }

    /**
     * Return packet clients array data
     * @return Pointer to teonet_client_data_ar
     */
    teonet_client_data_ar *packetClientData() const {
        return (teonet_client_data_ar *)
                    (packet()->peer_name + packet()->peer_name_length);
    }

    /**
     * Return packet data
     * @return Pointer to void data
     */
    void *packetData() const {
        return (void*) (packet()->peer_name + packet()->peer_name_length);
    }

private:

    /**
     * Teocli C++ callback bind function declaration
     *
     * @param con
     * @param event
     * @param data
     * @param data_len
     * @param user_data
     */
    static void callbackBind (void *con, Events event, void *data,
            size_t data_len, void *user_data) {

        Teocli &cli = *(Teocli *) user_data;

        if(event == EV_L_CONNECTED) {

            cli.setCon((teoLNullConnectData *)con);
            if(cli.connected() > 0) {

                // Send (1) Initialization packet to L0 server, *** REQUERED ***
                ssize_t snd = cli.login();
                if(snd == -1) std::cout << "Can't send login to server\n";
            }
        }

        cli.getEventCallBack()(cli, event, data, data_len, cli.getUserData());
    }

public:

    virtual void eventCb(teo::Events event, void *data, size_t data_len,
      void *user_data) {

    }
};

}
