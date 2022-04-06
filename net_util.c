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

#include "net_util.h"

#include <string.h>
#include <sys/socket.h>

/**
 * Sends the message through socket.
 *
 * @param socket_fd the socket id used to send the message
 * @param message the message to send
 * @return the number of characters sent
 */
ssize_t send_message(int socket_fd, const char message[]) {
    return send(socket_fd, message, BUFFER_LEN, 0);
}

/**
 * Receives the message through socket.
 *
 * @param socket_fd the socket id used to receive the message
 * @param message an array to store the received message;
 *                any data already in the array will be erased
 * @return the number of characters received
 */
ssize_t receive_message(int socket_fd, char message[]) {
    memset(message, 0, BUFFER_LEN);
    return recv(socket_fd, message, BUFFER_LEN, 0);
}
