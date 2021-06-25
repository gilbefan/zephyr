# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(mqtt_mutual_auth)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

# Include MQTT library's source and header path variables.
include( ${CMAKE_SOURCE_DIR}/coreMQTT/mqttFilePaths.cmake )

# Include backoffAlgorithm library file path configuration.
include( ${CMAKE_SOURCE_DIR}/backoffAlgorithm/backoffAlgorithmFilePaths.cmake )

# Include logging sources. Does this work???
include( ${CMAKE_SOURCE_DIR}/logging-stack/logging.cmake )

target_sources(app
    PRIVATE 
        ${SOCKETS_SOURCES}
        ${MQTT_SOURCES}
        ${MQTT_SERIALIZER_SOURCES}
        ${BACKOFF_ALGORITHM_SOURCES}
)

target_include_directories(app
    PUBLIC
    ${MQTT_INCLUDE_PUBLIC_DIRS}
        ${BACKOFF_ALGORITHM_INCLUDE_PUBLIC_DIRS}
        ${CMAKE_CURRENT_LIST_DIR}
        ${LOGGING_INCLUDE_DIRS}
)