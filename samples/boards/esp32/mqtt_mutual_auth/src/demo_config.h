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

#ifndef DEMO_CONFIG_H_
#define DEMO_CONFIG_H_

/**************************************************/
/******* DO NOT CHANGE the following order ********/
/**************************************************/

/* Logging related header files are required to be included in the following order:
 * 1. Include the header file "logging_levels.h".
 * 2. Define LIBRARY_LOG_NAME and  LIBRARY_LOG_LEVEL.
 * 3. Include the header file "logging_stack.h".
 */

/* Include header that defines log levels. */
#include "logging_levels.h"

/* Logging configuration for the Demo. */
#ifndef LIBRARY_LOG_NAME
    #define LIBRARY_LOG_NAME     "DEMO"
#endif
#ifndef LIBRARY_LOG_LEVEL
    #define LIBRARY_LOG_LEVEL    LOG_DEBUG
#endif

#include "logging_stack.h"

/************ End of logging configuration ****************/


/**
 * @brief Details of the MQTT broker to connect to.
 *
 * @note Your AWS IoT Core endpoint can be found in the AWS IoT console under
 * Settings/Custom Endpoint, or using the describe-endpoint API.
 *
 * #define AWS_IOT_ENDPOINT               "...insert here..."
 */
#define AWS_IOT_ENDPOINT       "a3rpgiyb2vwk8c-ats.iot.us-west-2.amazonaws.com"

/**
 * @brief AWS IoT MQTT broker port number.
 *
 * In general, port 8883 is for secured MQTT connections.
 *
 * @note Port 443 requires use of the ALPN TLS extension with the ALPN protocol
 * name. When using port 8883, ALPN is not required.
 */
#ifndef AWS_MQTT_PORT
    #define AWS_MQTT_PORT    ( 8883 )
#endif

/**
 * @brief Path of the file containing the server's root CA certificate.
 *
 * This certificate is used to identify the AWS IoT server and is publicly
 * available. Refer to the AWS documentation available in the link below
 * https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html#server-authentication-certs
 *
 * Amazon's root CA certificate is automatically downloaded to the certificates
 * directory from @ref https://www.amazontrust.com/repository/AmazonRootCA1.pem
 * using the CMake build system.
 *
 * @note This certificate should be PEM-encoded.
 * @note This path is relative from the demo binary created. Update
 * ROOT_CA_CERT_PATH to the absolute path if this demo is executed from elsewhere.
 */
#ifndef ROOT_CA_CERT_PEM
    #define ROOT_CA_CERT_PEM    "-----BEGIN CERTIFICATE-----\n"\
                                "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"\
                                "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"\
                                "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"\
                                "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"\
                                "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"\
                                "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"\
                                "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"\
                                "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"\
                                "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"\
                                "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"\
                                "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"\
                                "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"\
                                "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"\
                                "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"\
                                "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"\
                                "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"\
                                "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"\
                                "rqXRfboQnoZsG4q5WTP468SQvvG5\n"\
                                "-----END CERTIFICATE-----"
#endif

/**
 * @brief Path of the file containing the client certificate.
 *
 * Refer to the AWS documentation below for details regarding client
 * authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This certificate should be PEM-encoded.
 *
 * #define CLIENT_CERT_PATH    "...insert here..."
 */
#define CLIENT_CERT_PEM       "-----BEGIN CERTIFICATE-----\n"\
                               "MIIDWjCCAkKgAwIBAgIVAMXPfkNrFuiSyyrAGx6/Bgo3Dwu+MA0GCSqGSIb3DQEB\n"\
                               "CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"\
                               "IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMTA2MDQwMTE1\n"\
                               "MDBaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"\
                               "dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCZeIUzlKiSOSJGlrIH\n"\
                               "tdv/1JZD4wvCW82ZCYFAsIDkZ4JC3qVJeYD/IbhwFETuXXGt8CHGvl8BaEoNFNJr\n"\
                               "9qnub8qnC1fR13/FrLoi+q6IB7XqQSDKbc7ZVdnlcth/+SErNxublAly7AtbhCQ3\n"\
                               "sK0aFF1qactyIuGKJACAYuSwDs37p6xNCoLWN//jLFcXGcsjYsOxj1Q88NI8Kb6i\n"\
                               "0DW8fiBbLqTpJUcPugdIHV4dfmscqLuDHpJ22jPhL086ZBsaLr25JAbRQbQv/1cy\n"\
                               "PfImOXBSLt0wSmJQFAiL2xrlKtEapxpI92VsItx48R/oOZP0JywZuWMklBkYU29z\n"\
                               "SiuDAgMBAAGjYDBeMB8GA1UdIwQYMBaAFFNchlPQ4bkQZ/4mS4R9evXlJmw8MB0G\n"\
                               "A1UdDgQWBBTMm8A5ySSHcd6WrfSRlSHU63coGDAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"\
                               "DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEARq0n382wRrxpc2xW0Zn9Nv6a\n"\
                               "XWuFlmrAC507qIH3v2R6f1o+v4PbaTpqRsUj8n1qZmfuVi13Gp8cddsCJiKsVlqA\n"\
                               "3P05mzRe0nzbK28sRXKDK6jloiwCBCxac/EmjRVDfeIbMG1Ze/nhsyuoqEpmNpob\n"\
                               "+FHLkuz0D2vVfCRg2bdiqTxIyclHPyRnzArvts/UwRFtuW33Qwsf3K/lGPYLlQ86\n"\
                               "iKes0VmHuFPFGQ8KnJw2MehqnExCAhzrNJi63XkkUjzejvk/KVyzKeVG4jdP3P9L\n"\
                               "M8K9J3NXBZo8US2ZPEM/2Jj2Gv7CxO/cUKQ5F2F5W88q8z9swTloYBY45l/oQw==\n"\
                               "-----END CERTIFICATE-----"

/**
 * @brief Path of the file containing the client's private key.
 *
 * Refer to the AWS documentation below for details regarding client
 * authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This private key should be PEM-encoded.
 *
 * #define CLIENT_PRIVATE_KEY_PATH    "...insert here..."
 */
#define CLIENT_PRIVATE_KEY_PEM       "-----BEGIN RSA PRIVATE KEY-----\n"\
                                     "MIIEogIBAAKCAQEAmXiFM5SokjkiRpayB7Xb/9SWQ+MLwlvNmQmBQLCA5GeCQt6l\n"\
                                     "SXmA/yG4cBRE7l1xrfAhxr5fAWhKDRTSa/ap7m/KpwtX0dd/xay6IvquiAe16kEg\n"\
                                     "ym3O2VXZ5XLYf/khKzcbm5QJcuwLW4QkN7CtGhRdamnLciLhiiQAgGLksA7N+6es\n"\
                                     "TQqC1jf/4yxXFxnLI2LDsY9UPPDSPCm+otA1vH4gWy6k6SVHD7oHSB1eHX5rHKi7\n"\
                                     "gx6Sdtoz4S9POmQbGi69uSQG0UG0L/9XMj3yJjlwUi7dMEpiUBQIi9sa5SrRGqca\n"\
                                     "SPdlbCLcePEf6DmT9CcsGbljJJQZGFNvc0orgwIDAQABAoIBAAY/bczHxIUPZmHR\n"\
                                     "SLYfpTEpCmUBHT1wV1JfnK9xgA3MWPU38yqmk6XPTZ7idFvv2ezm1bktjtiT0k/u\n"\
                                     "BRL4G7rlOi7Ku2yDhWQawAkkg25uQe2jcR5Mkhsu74vzvPSYw8zqlWkkAa54BY5Y\n"\
                                     "NtE9lA53vcXTQMEIwn3JMWwImp7AK2Xk4EIUDE30jM4bnmDAdDr9jo9Zp4cmHJEw\n"\
                                     "bfGhNNK9JIEDRpma/pftviiKj96FXNKN3tBXMEPrW7hWyAZnm6HDYbl9gKdMm257\n"\
                                     "0QqEX6BebdnFR1T+i4vp2ZO121RBBPORxylwgSQUFDvdvKgLl/2oIg096LaRbE70\n"\
                                     "hlJJoCECgYEAyvW7OpURgoO9EASocTLalS/zWvoJO5+6iDalwnnRRmChG1We1yo8\n"\
                                     "Fd4l9GLgpkM67W8HcDO0Xh19PnUSsOr2598H4s0z/lxHLl9jK+RVxFKeQ0mN39o0\n"\
                                     "+g1TXjSXcxWLzp+KxlMbtoJEwApYCwEq7e+5cqZtw+xV7v/2shEABIUCgYEAwZPo\n"\
                                     "BAzDzp5kaUgFlBrcGBEzYt9rVlSD2Hs/wBank0AKYbJL6CTZ0hzSDOtPGgDo+af/\n"\
                                     "lGfJVKyvgxOYjbFVNWcAI8LxvxKaTwR/C5uvK3PXOIdefCidOTNEKyaTMinIWGW9\n"\
                                     "Q+pgO7cWgDo2Ak8Lkxc5K7F54AmY2dfNdPVLEmcCgYBCIKhdIP6wgI3UL0BWiZQw\n"\
                                     "cYwfw0HI3HUhNnTxlVE7VfaH+nCjeE5Vbaj+JE0/vr3vaHKK+z20QHVzKR/q5tOF\n"\
                                     "nuRp4CQlpxlGIawlO+M58RVBpTCFlAoYwsi78/S4eXew3ciQb7mi0HToATbIdxvu\n"\
                                     "pGHO2iz/Xfxtv5Y4VFOmJQKBgA5sScY7eT9k19BsWFYPpkR4y55bTTESfPxdOoPC\n"\
                                     "bFvxUpSy5yr1y/D2ysPBqJQkazdmyAkzrnGLqEwY4q3z/4hKE12WwdJdvXeTuy+P\n"\
                                     "Ov7+05NXqrePgx9m7XZVmnQLXu0PL4nm5aKtYpTNQYbsgs1D1rOltpH1xRi+FoXX\n"\
                                     "XO+TAoGAatEDD7v/2Beb7mcMOBsU4VInDo3G1kc1lBW5QWA+EAG8hdROekQB8NG0\n"\
                                     "7a38CxujoYlD1b5nb2882pR0cIo+PfWtK+1s2h12cZ6aqXNRSJE279pUKzXK27Vt\n"\
                                     "buw6jdxr+d7U4uYBq949VhXB/MhfTFW7H+A3l3UuUNEq7bIDcF0=\n"\
                                     "-----END RSA PRIVATE KEY-----"

/**
 * @brief The username value for authenticating client to MQTT broker when
 * username/password based client authentication is used.
 *
 * Refer to the AWS IoT documentation below for details regarding client
 * authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * As mentioned in the link above, an authorizer setup needs to be done to use
 * username/password based client authentication.
 *
 * @note AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client. If this config is defined,
 * the username and password will be used instead of the client certificate and
 * private key for client authentication.
 *
 * #define CLIENT_USERNAME    "...insert here..."
 */

/**
 * @brief The password value for authenticating client to MQTT broker when
 * username/password based client authentication is used.
 *
 * Refer to the AWS IoT documentation below for details regarding client
 * authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * As mentioned in the link above, an authorizer setup needs to be done to use
 * username/password based client authentication.
 *
 * @note AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client.
 *
 * #define CLIENT_PASSWORD    "...insert here..."
 */

/**
 * @brief MQTT client identifier.
 *
 * No two clients may use the same client identifier simultaneously.
 */
#ifndef CLIENT_IDENTIFIER
    #define CLIENT_IDENTIFIER    "testclient"
#endif

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define NETWORK_BUFFER_SIZE       ( 1024U )

/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define OS_NAME                   "Ubuntu"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define OS_VERSION                "18.04 LTS"

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define HARDWARE_PLATFORM_NAME    "PC"

/**
 * @brief The name of the MQTT library used and its version, following an "@"
 * symbol.
 */
#include "core_mqtt.h"
#define MQTT_LIB    "core-mqtt@" MQTT_LIBRARY_VERSION

#endif /* ifndef DEMO_CONFIG_H_ */
