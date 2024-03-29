/**
 * \file   teonet_l0_client.h
 * \author Kirill Scherba <kirill@scherba.ru>
 *
 * Created on October 12, 2015, 12:32 PM
 */

/**
 * \page Teocli library Documentation
 *
 * * [Native library structures and functions](teonet__l0__client_8h.html)
 * * [C++ teocli class wrapper](classteocli.html)
 * * [Basic example](main_8c-example.html)
 */

#ifndef TEONET_L0_CLIENT_H
#define TEONET_L0_CLIENT_H

///! Teonet native client version (should change in linux/Makefile.am:7 too)
#define TL0CN_VERSION "0.0.17"

#include <stdint.h>
#include <stdbool.h>

#if !defined(HAVE_MINGW) && defined(_WIN32)
#else
#include <unistd.h>
#if !defined(usleep) && !defined(HAVE_MINGW) && defined(_WIN32)
extern int usleep(__useconds_t __useconds);
#endif
#endif

#include "teobase/socket.h"
#include "trudp.h"
#include "trudp_utils.h"

#include "teocli_api.h"

/**
 * L0 System commands
 */
enum CMD_L {
    CMD_L_INIT = 0,               ///< #0 Special command, key exchange or login
    CMD_L_ECHO = 65,              ///< #65 Echo command
    CMD_L_ECHO_ANSWER,            ///< #66 Answer to echo command
    CMD_L_PEERS = 72,             ///< #72 Get peers command
    CMD_L_PEERS_ANSWER,           ///< #73 Answer to get peers command
    CMD_L_AUTH = 77,              ///< #77 Auth command
    CMD_L_AUTH_ANSWER,            ///< #78 Auth answer command
    CMD_L_L0_CLIENTS,             ///< #79 Get clients list command
    CMD_L_L0_CLIENTS_ANSWER,      ///< #80 Clients list answer command
    CMD_L_SUBSCRIBE_ANSWER = 83,  ///< #83 Subscribe answer
    CMD_L_AUTH_LOGIN_ANSWER = 96, ///< #96 Auth server login answer
    CMD_L_L0_CRYPTO_KEY = 101,    ///< #101 L0 server crypto key exchange

    CMD_L_END = 127
};

#define L0_BUFFER_SIZE 4096
#define MAX_FD_NUMBER 65536

/**
 * L0 client events
 */
typedef enum teoLNullEvents {
    EV_L_CONNECTED,    ///< After connected to L0 server
    EV_L_DISCONNECTED, ///< After disconnected from L0 server
    EV_L_RECEIVED,     ///< Data received
    EV_L_TICK,         ///< Send after every teoLNullReadEventLoop calls
    EV_L_IDLE ///< Send after teoLNullReadEventLoop calls if data was not
              ///< received during timeout
} teoLNullEvents;

typedef void (*teoLNullEventsCb)(void *kc, teoLNullEvents event, void *data,
                                 size_t data_len, void *user_data);

/**
 * L0 client connection status
 */
typedef enum teoLNullConnectionStatus {
    CON_STATUS_CONNECTED = 1,
    CON_STATUS_NOT_CONNECTED = 0,
    CON_STATUS_SOCKET_ERROR = -1,
    CON_STATUS_HOST_ERROR = -2,
    CON_STATUS_CONNECTION_ERROR = -3,
    CON_STATUS_PIPE_ERROR = -4,
    CON_STATUS_ENCRYPTION_ERROR = -5,
} teoLNullConnectionStatus;

typedef enum PROTOCOL { TRUDP = 0, TCP = 1 } PROTOCOL;

// forward declaration, complete type in libteol0/teonet_l0_client_crypt.h
typedef struct teoLNullEncryptionContext teoLNullEncryptionContext;

/**
 * L0 client connect data
 */
typedef struct teoLNullConnectData {
    teonetSocket fd; ///< Socket descriptor

    teoLNullConnectionStatus status; ///< Connection status

    void *read_buffer;         ///< Pointer to saved buffer
    size_t read_buffer_offset; ///< Offset in read buffer
    size_t read_buffer_size;   ///< Read buffer size
    size_t last_packet_offset; ///< Last received packet offset (length)

    teoLNullEventsCb event_cb; ///< Event callback function
    void *user_data;           ///< User data

    int tcp_f; ///< TCP or UDP flag: TCP == 1
    int udp_reset_f;
    trudpData *td;         ///< TRUDP connection data
    trudpChannelData *tcd; ///< TRUDP channel data

    int pipefd[2]; ///< Pipe to use it in thread safe write function

    teoLNullEncryptionContext *client_crypt;

#if defined(_WIN32)
    HANDLE handles[2];
#endif
} teoLNullConnectData;

#define ARP_TABLE_IP_SIZE 48 // INET6_ADDRSTRLEN = 46

/**
 * KSNet ARP table data structure
 */
typedef struct ksnet_arp_data {
    int16_t mode; ///< Peers mode: -1 - This host, -2 undefined host, 0 - peer ,
                  ///< 1 - r-host, 2 - TCP Proxy peer
    char addr[ARP_TABLE_IP_SIZE]; ///< Peer IP address
    // \todo test is it possible to change this structure for running peers
    int16_t port; ///< Peer port

    double last_activity;      ///< Last time received data from peer
    double last_triptime_send; ///< Last time when triptime request send
    double last_triptime_got;  ///< Last time when triptime received

    double last_triptime; ///< Last triptime
    double triptime;      ///< Middle triptime

    double monitor_time; ///< Monitor ping time

    double connected_time; ///< Time when peer was connected to this peer
} ksnet_arp_data;

typedef struct ksnet_arp_data_ext {
    ksnet_arp_data data;
    char *type;
    uint32_t cque_id_peer_type;
} ksnet_arp_data_ext;

#define DIG_IN_TEO_VER 3

// Suppress MSVC compiler warning 'zero-sized array in struct'.
#if defined(TEONET_COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4200)
#endif

/**
 * Host info data structure
 */
typedef struct host_info_data {
    uint8_t ver[DIG_IN_TEO_VER]; ///< Version
    uint8_t string_ar_num;       ///< Number of elements in array length
    char string_ar[];            ///< String array structure: { name, type }
} host_info_data;

#pragma pack(push)
#pragma pack(1)

/**
 * KSNet ARP table whole data array
 */
typedef struct ksnet_arp_data_ar {
    uint32_t length;
    struct _arp_data {
        char name[ARP_TABLE_IP_SIZE];
        ksnet_arp_data data;
    } arp_data[];
} ksnet_arp_data_ar;

/**
 * L0 client packet data structure
 *
 */
typedef struct teoLNullCPacket {
    uint8_t cmd;              ///< Command
    uint8_t peer_name_length; ///< To peer name length (include leading zero)
    uint16_t data_length;     ///< Packet data length
    uint8_t reserved_1;       ///< Reserved 1
    uint8_t reserved_2;       ///< Reserved 2
    uint8_t checksum;         ///< Whole checksum
    uint8_t header_checksum;  ///< Header checksum
    char
        peer_name[]; ///< To/From peer name (include leading zero) + packet data
} teoLNullCPacket;

/**
 * L0 Server statistic data structure
 *
 */
typedef struct ksnLNullSStat {
    uint16_t visits;
} ksnLNullSStat;

/**
 * L0 Server visits subscribe data structure
 *
 */
typedef struct ksnLNullSVisitsData {
    uint16_t visits;
    char client[];
} ksnLNullSVisitsData;

/**
 * Clients list data structure
 */
typedef struct teonet_client_data_ar {
    uint32_t length;
    struct _client_data {
        char name[128];
        // ksnLNullData data;
    } client_data[];
} teonet_client_data_ar;

/**
 * teoSScr class list or CMD_SUBSCRIBE_ANSWER data
 */
typedef struct teoSScrData {
    uint16_t ev; ///< Event (used when send data to subscriber)
    uint8_t cmd; ///< Command ID (used when send data to subscriber)
    char data[]; ///< Remote peer name in list or data in CMD_SUBSCRIBE_ANSWER
} teoSScrData;

/**
 * Data for CMD_L0_INFO_ANSWER command
 */
typedef struct l0_info_data {
    uint32_t l0_tcp_port;
    char l0_tcp_ip_remote[];
} l0_info_data;

// Reset compiler warnings to previous state.
#if defined(TEONET_COMPILER_MSVC)
#pragma warning(pop)
#endif

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get output buffer size
 * @param peer_length
 * @param data_length
 */
static inline size_t teoLNullBufferSize(size_t peer_length, size_t data_length) {
  return sizeof(teoLNullCPacket) + peer_length + data_length;
}

#if defined(_WIN32) && !defined(HAVE_MINGW)
void TEOCLI_API WinSleep(uint32_t dwMilliseconds);
#define teoLNullSleep(ms) WinSleep(ms)
#else
#define teoLNullSleep(ms) usleep(ms * 1000)
#endif

// Hight level functions
TEOCLI_API void teoLNullInit();
TEOCLI_API void teoLNullCleanup();

TEOCLI_API teoLNullConnectData *
teoLNullConnect(const char *server, int16_t port, PROTOCOL connection_flag);
TEOCLI_API teoLNullConnectData *
teoLNullConnectE(const char *server, int16_t port, teoLNullEventsCb event_cb,
                 void *user_data, PROTOCOL connection_flag);
TEOCLI_API void teoLNullDisconnect(teoLNullConnectData *con);
TEOCLI_API void teoLNullShutdown(teoLNullConnectData *con);

TEOCLI_API ssize_t teoLNullLogin(teoLNullConnectData *con,
                                 const char *host_name);
TEOCLI_API ssize_t teoLNullSend(teoLNullConnectData *con, uint8_t cmd,
                                const char *peer_name, const void *data,
                                size_t data_length);
TEOCLI_API ssize_t teoLNullSendUnreliable(teoLNullConnectData *con, uint8_t cmd,
                                          const char *peer_name, const void *data,
                                          size_t data_length);
TEOCLI_API ssize_t teoLNullSendEcho(teoLNullConnectData *con,
                                    const char *peer_name, const char *msg);
TEOCLI_API int64_t teoLNullProccessEchoAnswer(const char *msg);
TEOCLI_API ssize_t teoLNullRecv(teoLNullConnectData *con);
TEOCLI_API ssize_t teoLNullRecvCheck(teoLNullConnectData *con, char *buf,
                                     ssize_t rc);
TEOCLI_API ssize_t teoLNullRecvTimeout(teoLNullConnectData *con,
                                       uint32_t timeout);
TEOCLI_API bool teoLNullReadEventLoop(teoLNullConnectData *con, int timeout);

// Low level functions
TEOCLI_API size_t teoLNullPacketCreateLogin(teoLNullEncryptionContext *ctx, void *buffer, size_t buffer_length,
                                            const char *host_name);
TEOCLI_API size_t teoLNullPacketCreateEcho(teoLNullEncryptionContext *ctx, void *msg_buf, size_t buf_len,
                                           const char *peer_name,
                                           const char *msg);
TEOCLI_API size_t teoLNullPacketCreate(teoLNullEncryptionContext *ctx, void *buffer, size_t buffer_length,
                                       uint8_t command, const char *peer,
                                       const uint8_t *data, size_t data_length);
TEOCLI_API ssize_t teoLNullPacketSend(teoLNullConnectData *con, const char *data,
                                      size_t data_length);

TEOCLI_API uint8_t *teoLNullPacketGetPayload(teoLNullCPacket *packet);
TEOCLI_API teoLNullCPacket *teoLNullPacketGetFromBuffer(uint8_t *data, size_t data_len);

// Teonet utils functions
TEOCLI_API uint8_t get_byte_checksum(const uint8_t* data, size_t data_length);

#if defined(TEONET_COMPILER_GCC)
#define DEPRECATED_FUNCTION __attribute__((deprecated))
#else
#define DEPRECATED_FUNCTION __declspec(deprecated)
#endif

DEPRECATED_FUNCTION static inline void set_nonblock(int sd);
// DEPRECATED
static inline void set_nonblock(int sd) {
    teosockSetBlockingMode(sd, TEOSOCK_NON_BLOCKING_MODE);
}

DEPRECATED_FUNCTION static inline int set_tcp_nodelay(int sd);
// DEPRECATED
static inline int set_tcp_nodelay(int sd) {
    return teosockSetTcpNodelay(sd);
}

TEOCLI_API const char *STRING_teoLNullConnectionStatus(teoLNullConnectionStatus v);
TEOCLI_API const char *STRING_teoLNullEvents(teoLNullEvents v);

#ifdef __cplusplus
}
#endif

#endif /* TEONET_L0_CLIENT_H */
