/*
 * FreeRTOS V202104.00
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
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/**
 * @file tls_freertos.c
 * @brief TLS transport interface implementations. This implementation uses
 * mbedTLS.
 */

/* Standard includes. */
#include <assert.h>
#include <string.h>

/* FreeRTOS+TCP includes. */
//#include "FreeRTOS_IP.h"
#include <net/socket.h>
#include <random/rand32.h>

/* TLS transport header. */
#include "mbedtls_zephyr.h"

/* mbedTLS util includes. */
#include "mbedtls_error.h"
#include <mbedtls/debug.h>

/*-----------------------------------------------------------*/

/** 
 * @brief Each compilation unit that consumes the NetworkContext must define it. 
 * It should contain a single pointer as seen below whenever the header file
 * of this transport implementation is included to your project.
 *
 * @note When using multiple transports in the same compilation unit,
 *       define this pointer as void *.
 */
struct NetworkContext
{
    TlsTransportParams_t * pParams;
};

/*-----------------------------------------------------------*/

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a high-level code.
 */
static const char * pNoHighLevelMbedTlsCodeStr = "<No-High-Level-Code>";

/**
 * @brief Represents string to be logged when mbedTLS returned error
 * does not contain a low-level code.
 */
static const char * pNoLowLevelMbedTlsCodeStr = "<No-Low-Level-Code>";

/**
 * @brief Utility for converting the high-level code in an mbedTLS error to string,
 * if the code-contains a high-level code; otherwise, using a default string.
 */
#define mbedtlsHighLevelCodeOrDefault( mbedTlsCode )        \
    ( mbedtls_strerror_highlevel( mbedTlsCode ) != NULL ) ? \
    mbedtls_strerror_highlevel( mbedTlsCode ) : pNoHighLevelMbedTlsCodeStr

/**
 * @brief Utility for converting the level-level code in an mbedTLS error to string,
 * if the code-contains a level-level code; otherwise, using a default string.
 */
#define mbedtlsLowLevelCodeOrDefault( mbedTlsCode )        \
    ( mbedtls_strerror_lowlevel( mbedTlsCode ) != NULL ) ? \
    mbedtls_strerror_lowlevel( mbedTlsCode ) : pNoLowLevelMbedTlsCodeStr

/*-----------------------------------------------------------*/

/**
 * @brief Initialize the mbed TLS structures in a network connection.
 *
 * @param[in] pSslContext The SSL context to initialize.
 */
static void sslContextInit( SSLContext_t * pSslContext );

/**
 * @brief Free the mbed TLS structures in a network connection.
 *
 * @param[in] pSslContext The SSL context to free.
 */
static void sslContextFree( SSLContext_t * pSslContext );

/**
 * @brief Add X509 certificate to the trusted list of root certificates.
 *
 * OpenSSL does not provide a single function for reading and loading certificates
 * from files into stores, so the file API must be called. Start with the
 * root certificate.
 *
 * @param[out] pSslContext SSL context to which the trusted server root CA is to be added.
 * @param[in] pRootCa PEM-encoded string of the trusted server root CA.
 * @param[in] rootCaSize Size of the trusted server root CA.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setRootCa( SSLContext_t * pSslContext,
                          const uint8_t * pRootCa,
                          size_t rootCaSize );

/**
 * @brief Set X509 certificate as client certificate for the server to authenticate.
 *
 * @param[out] pSslContext SSL context to which the client certificate is to be set.
 * @param[in] pClientCert PEM-encoded string of the client certificate.
 * @param[in] clientCertSize Size of the client certificate.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setClientCertificate( SSLContext_t * pSslContext,
                                     const uint8_t * pClientCert,
                                     size_t clientCertSize );

/**
 * @brief Set private key for the client's certificate.
 *
 * @param[out] pSslContext SSL context to which the private key is to be set.
 * @param[in] pPrivateKey PEM-encoded string of the client private key.
 * @param[in] privateKeySize Size of the client private key.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setPrivateKey( SSLContext_t * pSslContext,
                              const uint8_t * pPrivateKey,
                              size_t privateKeySize );

/**
 * @brief Passes TLS credentials to the OpenSSL library.
 *
 * Provides the root CA certificate, client certificate, and private key to the
 * OpenSSL library. If the client certificate or private key is not NULL, mutual
 * authentication is used when performing the TLS handshake.
 *
 * @param[out] pSslContext SSL context to which the credentials are to be imported.
 * @param[in] pNetworkCredentials TLS credentials to be imported.
 *
 * @return 0 on success; otherwise, failure;
 */
static int32_t setCredentials( SSLContext_t * pSslContext,
                               const NetworkCredentials_t * pNetworkCredentials );

/**
 * @brief Set optional configurations for the TLS connection.
 *
 * This function is used to set SNI and ALPN protocols.
 *
 * @param[in] pSslContext SSL context to which the optional configurations are to be set.
 * @param[in] pHostName Remote host name, used for server name indication.
 * @param[in] pNetworkCredentials TLS setup parameters.
 */
static void setOptionalConfigurations( SSLContext_t * pSslContext,
                                       const char * pHostName,
                                       const NetworkCredentials_t * pNetworkCredentials );

/**
 * @brief Setup TLS by initializing contexts and setting configurations.
 *
 * @param[in] pNetworkContext Network context.
 * @param[in] pHostName Remote host name, used for server name indication.
 * @param[in] pNetworkCredentials TLS setup parameters.
 *
 * @return #TLS_TRANSPORT_SUCCESS, #TLS_TRANSPORT_INSUFFICIENT_MEMORY, #TLS_TRANSPORT_INVALID_CREDENTIALS,
 * or #TLS_TRANSPORT_INTERNAL_ERROR.
 */
static TlsTransportStatus_t tlsSetup( NetworkContext_t * pNetworkContext,
                                      const char * pHostName,
                                      const NetworkCredentials_t * pNetworkCredentials );

/**
 * @brief Perform the TLS handshake on a TCP connection.
 *
 * @param[in] pNetworkContext Network context.
 * @param[in] pNetworkCredentials TLS setup parameters.
 *
 * @return #TLS_TRANSPORT_SUCCESS, #TLS_TRANSPORT_HANDSHAKE_FAILED, or #TLS_TRANSPORT_INTERNAL_ERROR.
 */
static TlsTransportStatus_t tlsHandshake( NetworkContext_t * pNetworkContext,
                                          const NetworkCredentials_t * pNetworkCredentials );

/**
 * @brief Initialize mbedTLS.
 *
 * @param[out] entropyContext mbed TLS entropy context for generation of random numbers.
 * @param[out] ctrDrgbContext mbed TLS CTR DRBG context for generation of random numbers.
 *
 * @return #TLS_TRANSPORT_SUCCESS, or #TLS_TRANSPORT_INTERNAL_ERROR.
 */
static TlsTransportStatus_t initMbedtls( mbedtls_entropy_context * pEntropyContext,
                                         mbedtls_ctr_drbg_context * pCtrDrgbContext );

/*-----------------------------------------------------------*/

#ifdef MBEDTLS_DEBUG_C
    static void prvTlsDebugPrint( void * ctx,
                                  int lLevel,
                                  const char * pcFile,
                                  int lLine,
                                  const char * pcStr )
    {
        /* Unused parameters. */
        ( void ) ctx;
        ( void ) pcFile;
        ( void ) lLine;

        /* Send the debug string to the portable logger. */
        printk( "mbedTLS: |%d| %s", lLevel, pcStr );
    }
#endif /* ifdef MBEDTLS_DEBUG_C */
/*-----------------------------------------------------------*/

int mbedtls_platform_send( void * ctx,
                  const unsigned char * buf,
                  size_t len)
{
    int socket = (int) ctx;
    //printk("mbedtls_send called.\n Socket is: %d\n Buffer Length: %d\n", socket, len);
    ssize_t sendStatus = zsock_send(socket, buf, len, 0);

    return sendStatus;
}
/*-----------------------------------------------------------*/
int mbedtls_platform_recv( void * ctx,
                  unsigned char * buf,
                  size_t len )
{
    int socket = (int) ctx;

    int32_t selectStatus = -1;
    struct timeval recvTimeout;
    recvTimeout.tv_sec = ( ( ( int64_t ) 500 ) / 1000 );
    recvTimeout.tv_usec = ( 1000 * ( ( ( int64_t ) 500 ) % 1000 ) );
    fd_set readfds;
    ZSOCK_FD_ZERO( &readfds );
    ZSOCK_FD_SET( socket, &readfds );

    selectStatus = zsock_select( socket + 1,
                           &readfds,
                           NULL,
                           NULL,
                           &recvTimeout );

    ssize_t recvStatus = 0;
    if(selectStatus > 0) {
        //printk("mbedtls_recv called.\n Socket is: %d.\n Buffer Length: %d\n", socket, len);
        recvStatus = zsock_recv(socket, buf, len, 0);
        //printk("mbedtls_recv finished\n");
    }

    return recvStatus;
}
/*-----------------------------------------------------------*/

/**
 * @brief Function to generate a random number.
 *
 * @param[in] data Callback context.
 * @param[out] output The address of the buffer that receives the random number.
 * @param[in] len Maximum size of the random number to be generated.
 * @param[out] olen The size, in bytes, of the #output buffer.
 *
 * @return 0 if no critical failures occurred,
 * MBEDTLS_ERR_ENTROPY_SOURCE_FAILED otherwise.
 */
int mbedtls_platform_entropy_poll( void * data,
                                   unsigned char * output,
                                   size_t len,
                                   size_t * olen )
{
    int status = 0;
    int rngStatus = 0;

    assert( output != NULL );
    assert( olen != NULL );

    /* Context is not used by this function. */
    ( void ) data;

    /* TLS requires a secure random number generator; use the RNG provided
     * by Windows. This function MUST be re-implemented for other platforms. */
    rngStatus = sys_csrand_get(output, len);
        //BCryptGenRandom( NULL, output, len, BCRYPT_USE_SYSTEM_PREFERRED_RNG );

    if( rngStatus == 0 )
    {
        /* All random bytes generated. */
        *olen = len;
    }
    else
    {
        /* RNG failure. */
        *olen = 0;
        status = MBEDTLS_ERR_ENTROPY_SOURCE_FAILED;
    }

    return status;
}
/*-----------------------------------------------------------*/

static void sslContextInit( SSLContext_t * pSslContext )
{
    assert( pSslContext != NULL );

    mbedtls_ssl_config_init( &( pSslContext->config ) );
    mbedtls_x509_crt_init( &( pSslContext->rootCa ) );
    mbedtls_pk_init( &( pSslContext->privKey ) );
    mbedtls_x509_crt_init( &( pSslContext->clientCert ) );
    mbedtls_ssl_init( &( pSslContext->context ) );
}
/*-----------------------------------------------------------*/

static void sslContextFree( SSLContext_t * pSslContext )
{
    assert( pSslContext != NULL );

    mbedtls_ssl_free( &( pSslContext->context ) );
    mbedtls_x509_crt_free( &( pSslContext->rootCa ) );
    mbedtls_x509_crt_free( &( pSslContext->clientCert ) );
    mbedtls_pk_free( &( pSslContext->privKey ) );
    mbedtls_entropy_free( &( pSslContext->entropyContext ) );
    mbedtls_ctr_drbg_free( &( pSslContext->ctrDrgbContext ) );
    mbedtls_ssl_config_free( &( pSslContext->config ) );
}
/*-----------------------------------------------------------*/

static int32_t setRootCa( SSLContext_t * pSslContext,
                          const uint8_t * pRootCa,
                          size_t rootCaSize )
{
    int32_t mbedtlsError = -1;

    assert( pSslContext != NULL );
    assert( pRootCa != NULL );

    /* Parse the server root CA certificate into the SSL context. */
    mbedtlsError = mbedtls_x509_crt_parse( &( pSslContext->rootCa ),
                                           pRootCa,
                                           rootCaSize );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to parse server root CA certificate: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
    }
    else
    {
        mbedtls_ssl_conf_ca_chain( &( pSslContext->config ),
                                   &( pSslContext->rootCa ),
                                   NULL );
    }

    return mbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setClientCertificate( SSLContext_t * pSslContext,
                                     const uint8_t * pClientCert,
                                     size_t clientCertSize )
{
    int32_t mbedtlsError = -1;

    assert( pSslContext != NULL );
    assert( pClientCert != NULL );

    /* Setup the client certificate. */
    mbedtlsError = mbedtls_x509_crt_parse( &( pSslContext->clientCert ),
                                           pClientCert,
                                           clientCertSize );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to parse the client certificate: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
    }

    return mbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setPrivateKey( SSLContext_t * pSslContext,
                              const uint8_t * pPrivateKey,
                              size_t privateKeySize )
{
    int32_t mbedtlsError = -1;

    assert( pSslContext != NULL );
    assert( pPrivateKey != NULL );

    /* Setup the client private key. */
    mbedtlsError = mbedtls_pk_parse_key( &( pSslContext->privKey ),
                                         pPrivateKey,
                                         privateKeySize,
                                         NULL,
                                         0 );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to parse the client key: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
    }

    return mbedtlsError;
}
/*-----------------------------------------------------------*/

static int32_t setCredentials( SSLContext_t * pSslContext,
                               const NetworkCredentials_t * pNetworkCredentials )
{
    int32_t mbedtlsError = -1;

    assert( pSslContext != NULL );
    assert( pNetworkCredentials != NULL );

    /* Set up the certificate security profile, starting from the default value. */
    pSslContext->certProfile = mbedtls_x509_crt_profile_default;

    /* Set SSL authmode and the RNG context. */
    mbedtls_ssl_conf_authmode( &( pSslContext->config ),
                               MBEDTLS_SSL_VERIFY_REQUIRED );
    mbedtls_ssl_conf_rng( &( pSslContext->config ),
                          mbedtls_ctr_drbg_random,
                          &( pSslContext->ctrDrgbContext ) );
    mbedtls_ssl_conf_cert_profile( &( pSslContext->config ),
                                   &( pSslContext->certProfile ) );

    mbedtlsError = setRootCa( pSslContext,
                              pNetworkCredentials->pRootCa,
                              pNetworkCredentials->rootCaSize );

    if( ( pNetworkCredentials->pClientCert != NULL ) &&
        ( pNetworkCredentials->pPrivateKey != NULL ) )
    {
        if( mbedtlsError == 0 )
        {
            mbedtlsError = setClientCertificate( pSslContext,
                                                 pNetworkCredentials->pClientCert,
                                                 pNetworkCredentials->clientCertSize );
        }

        if( mbedtlsError == 0 )
        {
            mbedtlsError = setPrivateKey( pSslContext,
                                          pNetworkCredentials->pPrivateKey,
                                          pNetworkCredentials->privateKeySize );
        }

        if( mbedtlsError == 0 )
        {
            mbedtlsError = mbedtls_ssl_conf_own_cert( &( pSslContext->config ),
                                                      &( pSslContext->clientCert ),
                                                      &( pSslContext->privKey ) );
        }
    }

    return mbedtlsError;
}
/*-----------------------------------------------------------*/

static void setOptionalConfigurations( SSLContext_t * pSslContext,
                                       const char * pHostName,
                                       const NetworkCredentials_t * pNetworkCredentials )
{
    int32_t mbedtlsError = -1;

    assert( pSslContext != NULL );
    assert( pHostName != NULL );
    assert( pNetworkCredentials != NULL );

    if( pNetworkCredentials->pAlpnProtos != NULL )
    {
        /* Include an application protocol list in the TLS ClientHello
         * message. */
        mbedtlsError = mbedtls_ssl_conf_alpn_protocols( &( pSslContext->config ),
                                                        pNetworkCredentials->pAlpnProtos );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to configure ALPN protocol in mbed TLS: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
        }
    }

    /* Enable SNI if requested. */
    if( pNetworkCredentials->disableSni == 0 )
    {
        mbedtlsError = mbedtls_ssl_set_hostname( &( pSslContext->context ),
                                                 pHostName );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to set server name: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
        }
    }

    /* Set Maximum Fragment Length if enabled. */
    #ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

        /* Enable the max fragment extension. 4096 bytes is currently the largest fragment size permitted.
         * See RFC 8449 https://tools.ietf.org/html/rfc8449 for more information.
         *
         * Smaller values can be found in "mbedtls/include/ssl.h".
         */
        mbedtlsError = mbedtls_ssl_conf_max_frag_len( &( pSslContext->config ), MBEDTLS_SSL_MAX_FRAG_LEN_4096 );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to maximum fragment length extension: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
        }
    #endif /* ifdef MBEDTLS_SSL_MAX_FRAGMENT_LENGTH */
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsSetup( NetworkContext_t * pNetworkContext,
                                      const char * pHostName,
                                      const NetworkCredentials_t * pNetworkCredentials )
{
    TlsTransportParams_t * pTlsTransportParams = NULL;
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;
    int32_t mbedtlsError = 0;

    assert( pNetworkContext != NULL );
    assert( pNetworkContext->pParams != NULL );
    assert( pHostName != NULL );
    assert( pNetworkCredentials != NULL );
    assert( pNetworkCredentials->pRootCa != NULL );

    pTlsTransportParams = pNetworkContext->pParams;
    /* Initialize the mbed TLS context structures. */
    sslContextInit( pTlsTransportParams->sslContext );

    mbedtlsError = mbedtls_ssl_config_defaults( &( pTlsTransportParams->sslContext->config ),
                                                MBEDTLS_SSL_IS_CLIENT,
                                                MBEDTLS_SSL_TRANSPORT_STREAM,
                                                MBEDTLS_SSL_PRESET_DEFAULT );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to set default SSL configuration: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

        /* Per mbed TLS docs, mbedtls_ssl_config_defaults only fails on memory allocation. */
        returnStatus = TLS_TRANSPORT_INSUFFICIENT_MEMORY;
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        mbedtlsError = setCredentials( pTlsTransportParams->sslContext,
                                       pNetworkCredentials );

        if( mbedtlsError != 0 )
        {
            returnStatus = TLS_TRANSPORT_INVALID_CREDENTIALS;
        }
        else
        {
            // Optionally set SNI and ALPN protocols.
            setOptionalConfigurations( pTlsTransportParams->sslContext,
                                       pHostName,
                                       pNetworkCredentials );
        }
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t tlsHandshake( NetworkContext_t * pNetworkContext,
                                          const NetworkCredentials_t * pNetworkCredentials )
{
    TlsTransportParams_t * pTlsTransportParams = NULL;
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;
    int32_t mbedtlsError = 0;

    assert( pNetworkContext != NULL );
    assert( pNetworkContext->pParams != NULL );
    assert( pNetworkCredentials != NULL );

    pTlsTransportParams = pNetworkContext->pParams;
    /* Initialize the mbed TLS secured connection context. */
    mbedtlsError = mbedtls_ssl_setup( &( pTlsTransportParams->sslContext->context ),
                                      &( pTlsTransportParams->sslContext->config ) );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to set up mbed TLS SSL context: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

        returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
    }
    else
    {
        /* Set the underlying IO for the TLS connection. */

        /* MISRA Rule 11.2 flags the following line for casting the second
         * parameter to void *. This rule is suppressed because
         * #mbedtls_ssl_set_bio requires the second parameter as void *.
         */
        /* coverity[misra_c_2012_rule_11_2_violation] */
        mbedtls_ssl_set_bio( &( pTlsTransportParams->sslContext->context ),
                             ( void * ) pTlsTransportParams->tcpSocket,
                             mbedtls_platform_send,
                             mbedtls_platform_recv,
                             NULL );
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        // Perform the TLS handshake.
        do
        {
            printk("before handshake\n");
            //mbedtls_ssl_conf_dbg( &(pTlsTransportParams->sslContext->config), prvTlsDebugPrint, NULL );
            //mbedtls_debug_set_threshold( 4 );
            //printk("pTlsTransportParam sslContext struct/pointer is: %p", pTlsTransportParams->sslContext);
            mbedtlsError = mbedtls_ssl_handshake( &( pTlsTransportParams->sslContext->context ) );
        } while( ( mbedtlsError == MBEDTLS_ERR_SSL_WANT_READ ) ||
                 ( mbedtlsError == MBEDTLS_ERR_SSL_WANT_WRITE ) );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to perform TLS handshake: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );

            returnStatus = TLS_TRANSPORT_HANDSHAKE_FAILED;
        }
        else
        {
            LogInfo( ( "(Network connection %p) TLS handshake successful.",
                       pNetworkContext ) );
        }
    }

    printk("RETURNING FROM HANDSHAKE!");
    return returnStatus;
}
/*-----------------------------------------------------------*/

static TlsTransportStatus_t initMbedtls( mbedtls_entropy_context * pEntropyContext,
                                         mbedtls_ctr_drbg_context * pCtrDrgbContext )
{
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;
    int32_t mbedtlsError = 0;

    /* Set the mutex functions for mbed TLS thread safety. */
    /*mbedtls_threading_set_alt( mbedtls_platform_mutex_init,
                               mbedtls_platform_mutex_free,
                               mbedtls_platform_mutex_lock,
                               mbedtls_platform_mutex_unlock );*/

    /* Initialize contexts for random number generation. */
    mbedtls_entropy_init( pEntropyContext );
    mbedtls_ctr_drbg_init( pCtrDrgbContext );

    /* Add a strong entropy source. At least one is required. */
    mbedtlsError = mbedtls_entropy_add_source( pEntropyContext,
                                               mbedtls_platform_entropy_poll,
                                               NULL,
                                               32,
                                               MBEDTLS_ENTROPY_SOURCE_STRONG );

    if( mbedtlsError != 0 )
    {
        LogError( ( "Failed to add entropy source: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                    mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
        returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        /* Seed the random number generator. */
        mbedtlsError = mbedtls_ctr_drbg_seed( pCtrDrgbContext,
                                              mbedtls_entropy_func,
                                              pEntropyContext,
                                              NULL,
                                              0 );

        if( mbedtlsError != 0 )
        {
            LogError( ( "Failed to seed PRNG: mbedTLSError= %s : %s.",
                        mbedtlsHighLevelCodeOrDefault( mbedtlsError ),
                        mbedtlsLowLevelCodeOrDefault( mbedtlsError ) ) );
            returnStatus = TLS_TRANSPORT_INTERNAL_ERROR;
        }
    }

    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        LogDebug( ( "Successfully initialized mbedTLS." ) );
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

TlsTransportStatus_t MBedTLS_Connect( NetworkContext_t * pNetworkContext,
                                           const ServerInfo_t * pServerInfo,
                                           const NetworkCredentials_t * pNetworkCredentials,
                                           uint32_t receiveTimeoutMs,
                                           uint32_t sendTimeoutMs )
{
    TlsTransportParams_t * pTlsTransportParams = NULL;
    TlsTransportStatus_t returnStatus = TLS_TRANSPORT_SUCCESS;
    SocketStatus_t socketStatus = 0;

    const char * pHostName = pServerInfo->pHostName;
    //uint16_t port = pServerInfo->port;

    if( ( pNetworkContext == NULL ) ||
        ( pNetworkContext->pParams == NULL ) ||
        ( pHostName == NULL ) ||
        ( pNetworkCredentials == NULL ) )
    {
        LogError( ( "Invalid input parameter(s): Arguments cannot be NULL. pNetworkContext=%p, "
                    "pHostName=%p, pNetworkCredentials=%p.",
                    pNetworkContext,
                    pHostName,
                    pNetworkCredentials ) );
        returnStatus = TLS_TRANSPORT_INVALID_PARAMETER;
    }
    else if( ( pNetworkCredentials->pRootCa == NULL ) )
    {
        LogError( ( "pRootCa cannot be NULL." ) );
        returnStatus = TLS_TRANSPORT_INVALID_PARAMETER;
    }
    else
    {
        /* Empty else for MISRA 15.7 compliance. */
    }

    pTlsTransportParams = pNetworkContext->pParams;
    /* Establish a TCP connection with the server. */
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        pTlsTransportParams = pNetworkContext->pParams;
        printk("going into sockets_connect\n");
        socketStatus = Sockets_Connect( &(pTlsTransportParams->tcpSocket),
                                        pServerInfo,
                                        receiveTimeoutMs,
                                        sendTimeoutMs );
        printk("sockets_connect finished\n");
        
        if( socketStatus != 0 )
        {
            LogError( ( "Failed to connect to %s with error %d.",
                        pHostName,
                        socketStatus ) );
            returnStatus = TLS_TRANSPORT_CONNECT_FAILURE;
        }
    }

    /* Initialize mbedtls. */
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        returnStatus = initMbedtls( &( pTlsTransportParams->sslContext->entropyContext ),
                                    &( pTlsTransportParams->sslContext->ctrDrgbContext ) );
    }

    printk("before tlsSetup\n");
    // Initialize TLS contexts and set credentials.
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        returnStatus = tlsSetup( pNetworkContext, pHostName, pNetworkCredentials );
    }
    printk("tlsSetup Finished");

    // Perform TLS handshake.
    if( returnStatus == TLS_TRANSPORT_SUCCESS )
    {
        returnStatus = tlsHandshake( pNetworkContext, pNetworkCredentials );
    }
    printk("tls Handkshook");

    // Clean up on failure.
    if( returnStatus != TLS_TRANSPORT_SUCCESS )
    {
        if( ( pNetworkContext != NULL ) && ( pNetworkContext->pParams != NULL ) )
        {
            sslContextFree( pTlsTransportParams->sslContext );

            //if( pTlsTransportParams->tcpSocket != FREERTOS_INVALID_SOCKET )
            //{
            //    ( void ) zsock_close( pTlsTransportParams->tcpSocket );
            //}
        }
    }
    else
    {
        LogInfo( ( "(Network connection %p) Connection to %s established.",
                   pNetworkContext,
                   pHostName ) );
    }

    return returnStatus;
}
/*-----------------------------------------------------------*/

void MBedTLS_Disconnect( NetworkContext_t * pNetworkContext )
{
    TlsTransportParams_t * pTlsTransportParams = NULL;
    SocketStatus_t tlsStatus = 0;

    if( ( pNetworkContext != NULL ) && ( pNetworkContext->pParams != NULL ) )
    {
        pTlsTransportParams = pNetworkContext->pParams;
        /* Attempting to terminate TLS connection. */
        tlsStatus = ( SocketStatus_t ) mbedtls_ssl_close_notify( &( pTlsTransportParams->sslContext->context ) );

        /* Ignore the WANT_READ and WANT_WRITE return values. */
        if( ( tlsStatus != ( SocketStatus_t ) MBEDTLS_ERR_SSL_WANT_READ ) &&
            ( tlsStatus != ( SocketStatus_t ) MBEDTLS_ERR_SSL_WANT_WRITE ) )
        {
            if( tlsStatus == 0 )
            {
                LogInfo( ( "(Network connection %p) TLS close-notify sent.",
                           pNetworkContext ) );
            }
            else
            {
                LogError( ( "(Network connection %p) Failed to send TLS close-notify: mbedTLSError= %s : %s.",
                            pNetworkContext,
                            mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                            mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );
            }
        }
        else
        {
            /* WANT_READ and WANT_WRITE can be ignored. Logging for debugging purposes. */
            LogInfo( ( "(Network connection %p) TLS close-notify sent; ",
                       "received %s as the TLS status can be ignored for close-notify."
                       ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ? "WANT_READ" : "WANT_WRITE",
                       pNetworkContext ) );
        }

        /* Call socket shutdown function to close connection. */
        Sockets_Disconnect( pTlsTransportParams->tcpSocket );

        /* Free mbed TLS contexts. */
        sslContextFree( pTlsTransportParams->sslContext );
    }

    /* Clear the mutex functions for mbed TLS thread safety. */
    //mbedtls_threading_free_alt();
}
/*-----------------------------------------------------------*/

int32_t MBedTLS_recv( NetworkContext_t * pNetworkContext,
                           void * pBuffer,
                           size_t bytesToRecv )
{
    TlsTransportParams_t * pTlsTransportParams = NULL;
    int32_t tlsStatus = 0;

    assert( ( pNetworkContext != NULL ) && ( pNetworkContext->pParams != NULL ) );

    pTlsTransportParams = pNetworkContext->pParams;
    tlsStatus = ( int32_t ) mbedtls_ssl_read( &( pTlsTransportParams->sslContext->context ),
                                              pBuffer,
                                              bytesToRecv );

    if( ( tlsStatus == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( tlsStatus == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        LogDebug( ( "Failed to read data. However, a read can be retried on this error. "
                    "mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                    mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );

        /* Mark these set of errors as a timeout. The libraries may retry read
         * on these errors. */
        tlsStatus = 0;
    }
    else if( tlsStatus < 0 )
    {
        LogError( ( "Failed to read data: mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                    mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );
    }
    else
    {
        /* Empty else marker. */
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/

int32_t MBedTLS_send( NetworkContext_t * pNetworkContext,
                           const void * pBuffer,
                           size_t bytesToSend )
{
    TlsTransportParams_t * pTlsTransportParams = NULL;
    int32_t tlsStatus = 0;

    assert( ( pNetworkContext != NULL ) && ( pNetworkContext->pParams != NULL ) );

    pTlsTransportParams = pNetworkContext->pParams;
    tlsStatus = ( int32_t ) mbedtls_ssl_write( &( pTlsTransportParams->sslContext->context ),
                                               pBuffer,
                                               bytesToSend );

    if( ( tlsStatus == MBEDTLS_ERR_SSL_TIMEOUT ) ||
        ( tlsStatus == MBEDTLS_ERR_SSL_WANT_READ ) ||
        ( tlsStatus == MBEDTLS_ERR_SSL_WANT_WRITE ) )
    {
        LogDebug( ( "Failed to send data. However, send can be retried on this error. "
                    "mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                    mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );

        /* Mark these set of errors as a timeout. The libraries may retry send
         * on these errors. */
        tlsStatus = 0;
    }
    else if( tlsStatus < 0 )
    {
        LogError( ( "Failed to send data:  mbedTLSError= %s : %s.",
                    mbedtlsHighLevelCodeOrDefault( tlsStatus ),
                    mbedtlsLowLevelCodeOrDefault( tlsStatus ) ) );
    }
    else
    {
        /* Empty else marker. */
    }

    return tlsStatus;
}
/*-----------------------------------------------------------*/
