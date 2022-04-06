/*
 ***************************************************************************
 * Clarkson University                                                     *
 * CS 444/544: Operating Systems, Spring 2022                              *
 * Project: Prototyping a Web Server/Browser                               *
 * Created by Daqing Hou, dhou@clarkson.edu                                *
 *            Xinchao Song, xisong@clarkson.edu                            *
 * March 30, 2022                                                          *
 * Copyright © 2022 CS 444/544 Instructor Team. All rights reserved.       *
 * Unauthorized use is strictly prohibited.                                *
 ***************************************************************************
 */

#ifndef PROJECT_NETWORK_H
#define PROJECT_NETWORK_H

#include <sys/socket.h>

#define DEFAULT_HOST_IP "127.0.0.1"
#define DEFAULT_PORT 7000
#define BUFFER_LEN 1024

// Sends the message through socket.
ssize_t send_message(int socket_fd, const char message[]);

// Receives the message through socket.
ssize_t receive_message(int socket_fd, char message[]);

#endif //PROJECT_NETWORK_H
