/*
 * AWS IoT Device SDK for Embedded C 202103.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* Standard includes. */
#include <assert.h>
#include <string.h>

/* POSIX sockets includes. */
#include <errno.h>
//#include <netdb.h>
//#include <time.h>
//#include <unistd.h>
//#include <sys/time.h>
//#include <arpa/inet.h>

#include <net/socket.h>
#include <net/dns_resolve.h> //for testing if new dns function works

#include "sockets_zephyr.h"

/*-----------------------------------------------------------*/

/**
 * @brief Number of milliseconds in one second.
 */
#define ONE_SEC_TO_MS    ( 1000 )

/**
 * @brief Number of microseconds in one millisecond.
 */
#define ONE_MS_TO_US     ( 1000 )

/*-----------------------------------------------------------*/

/**
 * @brief Resolve a host name.
 *
 * @param[in] pHostName Server host name.
 * @param[in] hostNameLength Length associated with host name.
 * @param[out] pListHead The output parameter to return the list containing
 * resolved DNS records.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_DNS_FAILURE, #SOCKETS_CONNECT_FAILURE on error.
 */
static SocketStatus_t resolveHostName( const char * pHostName,
                                       size_t hostNameLength,
                                       struct zsock_addrinfo ** pListHead );

/**
 * @brief Traverse list of DNS records until a connection is established.
 *
 * @param[in] pListHead List containing resolved DNS records.
 * @param[in] pHostName Server host name.
 * @param[in] hostNameLength Length associated with host name.
 * @param[in] port Server port in host-order.
 * @param[out] pTcpSocket The output parameter to return the created socket.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_CONNECT_FAILURE on error.
 */
static SocketStatus_t attemptConnection( struct zsock_addrinfo * pListHead,
                                         const char * pHostName,
                                         size_t hostNameLength,
                                         uint16_t port,
                                         int32_t * pTcpSocket );

/**
 * @brief Connect to server using the provided address record.
 *
 * @param[in, out] pAddrInfo Address record of the server.
 * @param[in] port Server port in host-order.
 * @param[in] pTcpSocket Socket handle.
 *
 * @return #SOCKETS_SUCCESS if successful; #SOCKETS_CONNECT_FAILURE on error.
 */
static SocketStatus_t connectToAddress( struct sockaddr * pAddrInfo,
                                        uint16_t port,
                                        int32_t tcpSocket );

/**
 * @brief Log possible error using errno and return appropriate status.
 *
 * @param[in] errorNumber Error number.
 *
 * @return #SOCKETS_API_ERROR, #SOCKETS_INSUFFICIENT_MEMORY, #SOCKETS_INVALID_PARAMETER on error.
 */
static SocketStatus_t retrieveError( int32_t errorNumber );

/*-----------------------------------------------------------*/

void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	char *hr_family;
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		printf("DNS query was canceled");
		return;
	case DNS_EAI_FAIL:
		printf("DNS resolve failed");
		return;
	case DNS_EAI_NODATA:
		printf("Cannot resolve address");
		return;
	case DNS_EAI_ALLDONE:
		printf("DNS resolving finished");
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		printf("DNS resolving error (%d)", status);
		return;
	}

	if (!info) {
		return;
	}

	if (info->ai_family == AF_INET) {
		hr_family = "IPv4";
		addr = &net_sin(&info->ai_addr)->sin_addr;
	} else if (info->ai_family == AF_INET6) {
		hr_family = "IPv6";
		addr = &net_sin6(&info->ai_addr)->sin6_addr;
	} else {
		printf("Invalid IP address family %d", info->ai_family);
		return;
	}

	printf("%s %s address: %s 0x%04x", 
		(user_data ? (char *)user_data : "<null>"),
		hr_family,
		net_addr_ntop(info->ai_family, addr,
					 hr_addr, sizeof(hr_addr)),
		(( struct in_addr *)addr)->s_addr);
}

static SocketStatus_t resolveHostName( const char * pHostName,
                                       size_t hostNameLength,
                                       struct zsock_addrinfo ** pListHead )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    int32_t dnsStatus = -1;
    struct zsock_addrinfo hints;

    assert( pHostName != NULL );
    assert( hostNameLength > 0 );

    /* Unused parameter. These parameters are used only for logging. */
    ( void ) hostNameLength;

    /* Add hints to retrieve only TCP sockets in getaddrinfo. */
    ( void ) memset( &hints, 0, sizeof( hints ) );

    /* Address family of either IPv4 or IPv6. */
    hints.ai_family = AF_UNSPEC; //Changed to AF_INET from AF_UNSPEC
    /* TCP Socket. */
    hints.ai_socktype = ( int32_t ) SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    printk("host name is: %s\n", pHostName);
    printk("pListHead is: %p\n", pListHead);
    printk("pListHead second pointer is: %p\n", *pListHead);
    /* Perform a DNS lookup on the given host name. */
    dnsStatus = zsock_getaddrinfo( pHostName, NULL, &hints, pListHead );
    printk("getaddrinfo returns with status %d\n", dnsStatus);
    printk("pListHead is: %p\n", pListHead);
    printk("pListHead second pointer is: %p\n", *pListHead);

    if( dnsStatus != 0 )
    {
        LogError( ( "Failed to resolve DNS: Hostname=%.*s, ErrorCode=%d.\n",
                    ( int32_t ) hostNameLength,
                    pHostName,
                    errno ) ); //Changed: return errno instead of dnsStatus
        returnStatus = SOCKETS_DNS_FAILURE;
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

static SocketStatus_t connectToAddress( struct sockaddr * pAddrInfo,
                                        uint16_t port,
                                        int32_t tcpSocket )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    int32_t connectStatus = 0;
    char resolvedIpAddr[ INET6_ADDRSTRLEN ];
    socklen_t addrInfoLength;
    uint16_t netPort = 0;
    struct sockaddr_in * pIpv4Address;
    struct sockaddr_in6 * pIpv6Address;

    assert( pAddrInfo != NULL );
    assert( pAddrInfo->sa_family == AF_INET || pAddrInfo->sa_family == AF_INET6 );
    assert( tcpSocket >= 0 );

    /* Convert port from host byte order to network byte order. */
    netPort = htons( port );

    if( pAddrInfo->sa_family == ( sa_family_t ) AF_INET )
    {
        /* MISRA Rule 11.3 flags the following line for casting a pointer of
         * a object type to a pointer of a different object type. This rule
         * is suppressed because casting from a struct sockaddr pointer to
         * a struct sockaddr_in pointer is supported in POSIX and is used
         * to obtain the IP address from the address record. */
        /* coverity[misra_c_2012_rule_11_3_violation] */
        pIpv4Address = ( struct sockaddr_in * ) pAddrInfo;
        /* Store IPv4 in string to log. */
        pIpv4Address->sin_port = netPort;
        addrInfoLength = ( socklen_t ) sizeof( struct sockaddr_in );
        ( void ) zsock_inet_ntop( ( int32_t ) pAddrInfo->sa_family,
                            &pIpv4Address->sin_addr,
                            resolvedIpAddr,
                            ( socklen_t ) sizeof( resolvedIpAddr ) );
    }
    else
    {
        /* MISRA Rule 11.3 flags the following line for casting a pointer of
         * a object type to a pointer of a different object type. This rule
         * is suppressed because casting from a struct sockaddr pointer to
         * a struct sockaddr_in6 pointer is supported in POSIX and is used
         * to obtain the IPv6 address from the address record. */
        /* coverity[misra_c_2012_rule_11_3_violation] */
        pIpv6Address = ( struct sockaddr_in6 * ) pAddrInfo;
        /* Store IPv6 in string to log. */
        pIpv6Address->sin6_port = netPort;
        addrInfoLength = ( socklen_t ) sizeof( struct sockaddr_in6 );
        ( void ) zsock_inet_ntop( ( int32_t ) pAddrInfo->sa_family,
                            &pIpv6Address->sin6_addr,
                            resolvedIpAddr,
                            ( socklen_t ) sizeof( resolvedIpAddr ) );
    }

    LogDebug( ( "Attempting to connect to server using the resolved IP address:"
                " IP address=%s.",
                resolvedIpAddr ) );

    /* Attempt to connect. */
    connectStatus = zsock_connect( tcpSocket, pAddrInfo, addrInfoLength );

    if( connectStatus == -1 )
    {
        LogWarn( ( "Failed to connect to server using the resolved IP address: IP address=%s.",
                   resolvedIpAddr ) );
        ( void ) zsock_close( tcpSocket );
        returnStatus = SOCKETS_CONNECT_FAILURE;
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

static SocketStatus_t attemptConnection( struct zsock_addrinfo * pListHead,
                                         const char * pHostName,
                                         size_t hostNameLength,
                                         uint16_t port,
                                         int32_t * pTcpSocket )
{
    SocketStatus_t returnStatus = SOCKETS_CONNECT_FAILURE;
    const struct zsock_addrinfo * pIndex = NULL;

    assert( pListHead != NULL );
    assert( pHostName != NULL );
    assert( hostNameLength > 0 );
    assert( pTcpSocket != NULL );

    /* Unused parameters when logging is disabled. */
    ( void ) pHostName;
    ( void ) hostNameLength;

    printk( "Attempting to connect to: Host=%.*s.",
                ( int32_t ) hostNameLength,
                pHostName );

    /* Attempt to connect to one of the retrieved DNS records. */
    for( pIndex = pListHead; pIndex != NULL; pIndex = pIndex->ai_next )
    {
        printk("for loop once\n");
        *pTcpSocket = zsock_socket( pIndex->ai_family,
                              pIndex->ai_socktype,
                              pIndex->ai_protocol );

        if( *pTcpSocket == -1 )
        {
            continue;
        }

        /* Attempt to connect to a resolved DNS address of the host. */
        returnStatus = connectToAddress( pIndex->ai_addr, port, *pTcpSocket );

        /* If connected to an IP address successfully, exit from the loop. */
        if( returnStatus == SOCKETS_SUCCESS )
        {
            break;
        }
    }
    printk("exited for loop\n");

    if( returnStatus == SOCKETS_SUCCESS )
    {
        LogDebug( ( "Established TCP connection: Server=%.*s.\n",
                    ( int32_t ) hostNameLength,
                    pHostName ) );
    }
    else
    {
        LogError( ( "Could not connect to any resolved IP address from %.*s.",
                    ( int32_t ) hostNameLength,
                    pHostName ) );
    }

    zsock_freeaddrinfo( pListHead );

    return returnStatus;
}
/*-----------------------------------------------------------*/

static SocketStatus_t retrieveError( int32_t errorNumber )
{
    SocketStatus_t returnStatus = SOCKETS_API_ERROR;

    LogError( ( "A transport error occured: %d.", errorNumber ) ); //removed strerror(errorNumber) and made it just decimal

    if( ( errorNumber == ENOMEM ) || ( errorNumber == ENOBUFS ) )
    {
        returnStatus = SOCKETS_INSUFFICIENT_MEMORY;
    }
    else if( ( errorNumber == ENOTSOCK ) || ( errorNumber == EDOM ) || ( errorNumber == EBADF ) )
    {
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else
    {
        /* Empty else. */
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

SocketStatus_t Sockets_Connect( int32_t * pTcpSocket,
                                const ServerInfo_t * pServerInfo,
                                uint32_t sendTimeoutMs,
                                uint32_t recvTimeoutMs )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;
    struct zsock_addrinfo * pListHead = NULL;
    struct zsock_timeval transportTimeout;
    int32_t setTimeoutStatus = -1;

    if( pServerInfo == NULL )
    {
        LogError( ( "Parameter check failed: pServerInfo is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else if( pServerInfo->pHostName == NULL )
    {
        LogError( ( "Parameter check failed: pServerInfo->pHostName is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else if( pTcpSocket == NULL )
    {
        LogError( ( "Parameter check failed: pTcpSocket is NULL." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else if( pServerInfo->hostNameLength == 0UL )
    {
        LogError( ( "Parameter check failed: hostNameLength must be greater than 0." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }
    else
    {
        /* Empty else. */
    }

    if( returnStatus == SOCKETS_SUCCESS )
    {
        returnStatus = resolveHostName( pServerInfo->pHostName,
                                        pServerInfo->hostNameLength,
                                        &pListHead );
    }
    printk("resolveHostName finished with returnStatus: %d\n", returnStatus);

    if( returnStatus == SOCKETS_SUCCESS )
    {
        returnStatus = attemptConnection( pListHead,
                                          pServerInfo->pHostName,
                                          pServerInfo->hostNameLength,
                                          pServerInfo->port,
                                          pTcpSocket );
    }
    printk("attemptConnection finished with returnStatus: %d\n", returnStatus);

    /* Set the send timeout. */
    /*if( returnStatus == SOCKETS_SUCCESS )
    {
        transportTimeout.tv_sec = ( ( ( int64_t ) 500 ) / ONE_SEC_TO_MS );//( ( ( int64_t ) sendTimeoutMs ) / ONE_SEC_TO_MS );
        transportTimeout.tv_usec = ( ONE_MS_TO_US * ( ( ( int64_t ) 500 ) % ONE_SEC_TO_MS ) );//( ONE_MS_TO_US * ( ( ( int64_t ) sendTimeoutMs ) % ONE_SEC_TO_MS ) );

        setTimeoutStatus = zsock_setsockopt( *pTcpSocket,
                                       SOL_SOCKET,
                                       SO_SNDTIMEO,
                                       &transportTimeout,
                                       ( socklen_t ) sizeof( transportTimeout ) );

        if( setTimeoutStatus < 0 )
        {
            LogError( ( "Setting socket send timeout failed." ) );
            returnStatus = retrieveError( errno );
        }
        //debugging code:
        /*printk("transportTimeout.tv_sec is %ld\n transportTimeout.tv_usec is %ld", transportTimeout.tv_sec, transportTimeout.tv_usec);

        struct zsock_timeval sendTimeout;
        socklen_t sendTimeoutLen;
        int testret = zsock_getsockopt( *pTcpSocket,
                          SOL_SOCKET,
                          SO_SNDTIMEO,
                          &sendTimeout,
                          &sendTimeoutLen );

        printk("IN SOCKETS: getsockopt return val is %d\n", testret);
        printk("IN SOCKETS: sendTimeout.tv_sec is %ld\n sendTimeout.tv_usec is %ld", sendTimeout.tv_sec, sendTimeout.tv_usec);*/
    //}

    /* Set the receive timeout. */
    /*if( returnStatus == SOCKETS_SUCCESS )
    {
        transportTimeout.tv_sec = ( ( ( int64_t ) 500 ) / ONE_SEC_TO_MS );//( ( ( int64_t ) recvTimeoutMs ) / ONE_SEC_TO_MS );
        transportTimeout.tv_usec = ( ONE_MS_TO_US * ( ( ( int64_t ) 500 ) % ONE_SEC_TO_MS ) );//( ONE_MS_TO_US * ( ( ( int64_t ) recvTimeoutMs ) % ONE_SEC_TO_MS ) );

        setTimeoutStatus = zsock_setsockopt( *pTcpSocket,
                                       SOL_SOCKET,
                                       SO_RCVTIMEO,
                                       &transportTimeout,
                                       ( socklen_t ) sizeof( transportTimeout ) );

        if( setTimeoutStatus < 0 )
        {
            LogError( ( "Setting socket receive timeout failed." ) );
            returnStatus = retrieveError( errno );
        }
    }*/

    return returnStatus;
}
/*-----------------------------------------------------------*/

SocketStatus_t Sockets_Disconnect( int32_t tcpSocket )
{
    SocketStatus_t returnStatus = SOCKETS_SUCCESS;

    if( tcpSocket > 0 )
    {
        ( void ) zsock_shutdown( tcpSocket, ZSOCK_SHUT_RDWR );
        ( void ) zsock_close( tcpSocket );
    }
    else
    {
        LogError( ( "Parameter check failed: tcpSocket was negative." ) );
        returnStatus = SOCKETS_INVALID_PARAMETER;
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/
