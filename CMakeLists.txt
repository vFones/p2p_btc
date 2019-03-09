################ CMake #######################

cmake_minimum_required(VERSION 3.10.2[...3.13.4])
project(retiBTC)

#set gcc flags and requirements
if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
  set(CMAKE_C_COMPILER gcc-8)
endif()
message(STATUS "Using C compiler: ${CMAKE_C_COMPILER}")

set(CMAKE_C_FLAGS "-Wall -Wextra -pedantic -o3 -g -pthread")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#set project folder
set(RETIBTC ${CMAKE_SOURCE_DIR}/retibtc)

#set libraries folder
link_directories(${RETIBTC}/lib)
set(LIBRARY_OUTPUT_PATH ${RETIBTC}/lib/static)

#set use of system style include (e.g. #include <libs.h>)
include_directories(SYSTEM ${RETIBTC}/include)

#set output executable
set(CMAKE_BINARY_DIR ${RETIBTC}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

#search and set OpenSSL libs even if Darwin arch
find_package(OpenSSL REQUIRED)
if( OpenSSL_FOUND )
  set(OPENSSL_USE_STATIC_LIBS TRUE)
    if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
      set(OPENSSL_ROOT_DIR /usr/local/opt/openssl)
      set(OPENSSL_LIBRARIES /usr/local/opt/openssl/lib)
      set(OPENSSL_INCLUDE_DIR /usr/local/opt/openssl/include/)
    endif()
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
    message(STATUS "OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
endif()

#add all files in lib/ as libraries
AUX_SOURCE_DIRECTORY(${RETIBTC}/lib LIBS_FILES)
add_library(libraries ${LIBS_FILES})

#link libraries to executable
set(N_NODE_LIBS ${RETIBTC}/src/n_node/node_handler.c ${RETIBTC}/src/n_node/wallet_handler.c)
add_executable(n_node ${RETIBTC}/src/n_node/main.c ${N_NODE_LIBS})
target_link_libraries(n_node libraries OpenSSL::SSL)

#set(WALLET_LIBS src/wallet/wallet.c)
#add_executable(wallet src/wallet/main.c ${WALLET_LIBS})

#target_link_libraries(peer libraries OpenSSL::SSL)
#target_link_libraries(wallet libraries OpenSSL::SSL)
