#ifndef __CELLULAR_SIM70XX_H__
#define __CELLULAR_SIM70XX_H__

/* AT Command timeout for PDN activation */
#define PDN_ACTIVATION_PACKET_REQ_TIMEOUT_MS       ( 150000UL )

/* AT Command timeout for PDN deactivation. */
#define PDN_DEACTIVATION_PACKET_REQ_TIMEOUT_MS     ( 40000UL )

/* AT Command timeout for Socket connection */
#define SOCKET_CONNECT_PACKET_REQ_TIMEOUT_MS       ( 150000UL )

#define PACKET_REQ_TIMEOUT_MS                      ( 5000UL )

/* AT Command timeout for Socket disconnection */
#define SOCKET_DISCONNECT_PACKET_REQ_TIMEOUT_MS    ( 12000UL )

#define DATA_SEND_TIMEOUT_MS                       ( 50000UL )
#define DATA_READ_TIMEOUT_MS                       ( 50000UL )

/**
 * @brief DNS query result.
 */
typedef enum cellularDnsQueryResult
{
    CELLULAR_DNS_QUERY_SUCCESS,
    CELLULAR_DNS_QUERY_FAILED,
    CELLULAR_DNS_QUERY_MAX,
    CELLULAR_DNS_QUERY_UNKNOWN
} cellularDnsQueryResult_t;

typedef struct cellularModuleContext cellularModuleContext_t;

/**
 * @brief DNS query URC callback fucntion.
 */
typedef void ( * CellularDnsResultEventCallback_t )( cellularModuleContext_t * pModuleContext,
                                                     char * pDnsResult,
                                                     char * pDnsUsrData );

typedef enum cellularSockEventBit_t
{
    SOCK_EVENT_BIT_PDN = ( 1 << 0 ),
    SOCK_EVENT_BIT_CIP = ( 1 << 1 ),
    SOCK_EVENT_BIT_RECV = ( 1 << 2 ),
    SOCK_EVENT_BIT_SEND = ( 1 << 3 ),
} cellularSockEventBit_t;

typedef struct cellularModuleContext
{
    /* DNS related variables. */
    PlatformMutex_t dnsQueryMutex; /* DNS query mutex to protect the following data.   */
    QueueHandle_t pktDnsQueue;     /* DNS queue to receive the DNS query result.       */
    uint8_t dnsResultNumber;       /* DNS query result number.                 */
    uint8_t dnsIndex;              /* DNS query current index.                 */
    char * pDnsUsrData;            /* DNS user data to store the result.       */

    CellularDnsResultEventCallback_t dnsEventCallback;

    CellularPdnConfig_t pdnCfg;  /* save PDN/APN setting                     */
    EventGroupHandle_t pdnEvent; /* for AT+CNACT wait +APP PDP: response     */
    uint16_t pdnDataCnt;
} cellularModuleContext_t;

CellularPktStatus_t _Cellular_ParseSimstat( char * pInputStr,
                                            CellularSimCardStatus_t * pSimState );

BOOL IsValidCID( int cid );
BOOL IsValidPDP( int pdp );

#endif /* ifndef __CELLULAR_SIM70XX_H__ */
