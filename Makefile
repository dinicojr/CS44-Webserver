# Clarkson University
# CS 444/544: Operating Systems, Spring 2022
# Project: Prototyping a Web Server/Browser
# Created by Daqing Hou, dhou@clarkson.edu
#            Xinchao Song, xisong@clarkson.edu
# March 30, 2022
# Copyright © 2022 CS 444/544 Instructor Team. All rights reserved.
# Unauthorized use is strictly prohibited.

all: server browser

server: server.c net_util.h net_util.c
	gcc -std=c11 server.c net_util.c -o server -pthread

browser: browser.c net_util.h net_util.c
	gcc -std=c11 browser.c net_util.c -o browser -pthread
clean:
	rm -f *.o server browser
