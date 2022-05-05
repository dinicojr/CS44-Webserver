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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdint.h>

#define NUM_VARIABLES 26
#define NUM_SESSIONS 128
#define NUM_BROWSER 128
#define DATA_DIR "./sessions"
#define SESSION_PATH_LEN 128
#define HASH_SIZE 128

typedef struct browser_struct {
    bool in_use;
    int socket_fd;
    int session_id;
} browser_t;

typedef struct session_struct {
    bool in_use;
    bool variables[NUM_VARIABLES];
    double values[NUM_VARIABLES];
} session_t;

static browser_t browser_list[NUM_BROWSER];                             // Stores the information of all browsers.
// TODO: For Part 3.2, convert the session_list to a simple hashmap/dictionary.
typedef struct entry {

	session_t* session;
	int key;
	struct entry* next;
} entry_t;

typedef struct hashmap {
	entry** map;
	unsigned int capacity;
	unsigned int length;
} hashmap_t;

static session_t session_list[NUM_SESSIONS];                            // Stores the information of all sessions.

//static hashmap_t* m = (hashmap_t*) malloc(sizeof(hashmap_t));
static pthread_mutex_t browser_list_mutex = PTHREAD_MUTEX_INITIALIZER;  // A mutex lock for the browser list.
static pthread_mutex_t session_list_mutex = PTHREAD_MUTEX_INITIALIZER;  // A mutex lock for the session list.


hashmap_t* new_hashmap();

unsigned hash(hashmap_t* map, int key);

session_t* get(hashmap_t* map, int key);

int set(hashmap_t* map, session_t* session);

bool destroy(hashmap_t* map, int key);


// Returns the string format of the given session.
// There will be always 9 digits in the output string.
void session_to_str(int session_id, char result[]);

// Determines if the given string represents a number.
bool is_str_numeric(const char str[]);

// Process the given message and update the given session if it is valid.
bool process_message(int session_id, const char message[]);

// Broadcasts the given message to all browsers with the same session ID.
void broadcast(int session_id, const char message[]);

// Gets the path for the given session.
void get_session_file_path(int session_id, char path[]);

// Loads every session from the disk one by one if it exists.
void load_all_sessions();

// Saves the given sessions to the disk.
void save_session(int session_id);

// Assigns a browser ID to the new browser.
// Determines the correct session ID for the new browser
// through the interaction with it.
int register_browser(int browser_socket_fd);

// Handles the given browser by listening to it,
// processing the message received,
// broadcasting the update to all browsers with the same session ID,
// and backing up the session on the disk.
void browser_handler(void* browser_socket_fd);

// Starts the server.
// Sets up the connection,
// keeps accepting new browsers,
// and creates handlers for them.
void start_server(int port);

/**
 * Returns the string format of the given session.
 * There will be always 9 digits in the output string.
 *
 * @param session_id the session ID
 * @param result an array to store the string format of the given session;
 *               any data already in the array will be erased
 */
void session_to_str(int session_id, char result[]) {
    memset(result, 0, BUFFER_LEN);
	pthread_mutex_lock(&session_list_mutex);
    session_t session = session_list[session_id];
	pthread_mutex_unlock(&session_list_mutex);
    for (int i = 0; i < NUM_VARIABLES; ++i) {
        if (session.variables[i]) {
            char line[32];

            if (session.values[i] < 1000) {
                sprintf(line, "%c = %.6f\n", 'a' + i, session.values[i]);
            } else {
                sprintf(line, "%c = %.8e\n", 'a' + i, session.values[i]);
            }

            strcat(result, line);
        }
    }
}

/**
 * Determines if the given string represents a number.
 *
 * @param str the string to determine if it represents a number
 * @return a boolean that determines if the given string represents a number
 */
bool is_str_numeric(const char str[]) {
    if (str == NULL) {
        return false;
    }

    if (!(isdigit(str[0]) || (str[0] == '-') || (str[0] == '.'))) {
        return false;
    }

    int i = 1;
    while (str[i] != '\0') {
        if (!(isdigit(str[i]) || str[i] == '.')) {
            return false;
        }
        i++;
    }

    return true;
}

/**
 * Process the given message and update the given session if it is valid.
 *
 * @param session_id the session ID
 * @param message the message to be processed
 * @return a boolean that determines if the given message is valid
 */
bool process_message_old(int session_id, const char message[]) {
    char *token;
    int result_idx;
    double first_value;
    char symbol;
    double second_value;

    // TODO: For Part 3.1, write code to determine if the input is invalid and return false if it is.
    // Hint: Also need to check if the given variable does exist (i.e., it has been assigned with some value)
    // for the first variable and the second variable, respectively.

    // Makes a copy of the string since strtok() will modify the string that it is processing.
    char data[BUFFER_LEN];
    strcpy(data, message);
    
	// Processes the result variable.
    token = strtok(data, " ");
    result_idx = token[0] - 'a';

    // Processes "=".
    token = strtok(NULL, " ");

    // Processes the first variable/value.
    token = strtok(NULL, " ");
    if (is_str_numeric(token)) {
        first_value = strtod(token, NULL);
    } else {
        int first_idx = token[0] - 'a';
        first_value = session_list[session_id].values[first_idx];
        
		if (!(first_value >= 'a' && first_value <= 'z') || (first_value >= 'A' && first_value <= 'Z')){
            return false;
        }else if (first_value >= 'A' && first_value <= 'Z'){
            first_value = islower(first_value);
        }
    }

    // Processes the operation symbol.
    token = strtok(NULL, " ");
    if (token == NULL) {
        session_list[session_id].variables[result_idx] = true;
        session_list[session_id].values[result_idx] = first_value;
        return true;
    }
    symbol = token[0];
    if (symbol != '+' || symbol != '-' || symbol != '*' || symbol != '/'){
		
        return false;
    }

    // Processes the second variable/value.
    token = strtok(NULL, " ");
    if (is_str_numeric(token)) {
        first_value = strtod(token, NULL);
    } else {
        int first_idx = token[0] - 'a';
        first_value = session_list[session_id].values[first_idx];
        if(!(first_value >= 'a' && first_value <= 'z') || (first_value >= 'A' && first_value <= 'Z')){
            return false;
        }else if (first_value >= 'A' && first_value <= 'Z'){
            first_value = islower(first_value);
        }
    }

    // No data should be left over thereafter.
    token = strtok(NULL, " ");

    session_list[session_id].variables[result_idx] = true;

    if (symbol == '+') {
        session_list[session_id].values[result_idx] = first_value + second_value;
    } else if (symbol == '-') {
        session_list[session_id].values[result_idx] = first_value - second_value;
    } else if (symbol == '*') {
        session_list[session_id].values[result_idx] = first_value * second_value;
    } else if (symbol == '/') {
        session_list[session_id].values[result_idx] = first_value / second_value;
    }

    return true;
}


bool process_message(int session_id, const char message[]) {

	char* token;
	char symbol;
	int result_idx;
	double first_value, second_value;

	char data[BUFFER_LEN];
	strcpy(data, message);
	puts("data copied\n");

	//Process result
	token = strtok(data, " ");
	puts("1\n");
	if (!isalpha(token[0])) {
		puts("ERROR: Result not a variable\n");
		return false;
	}

	puts("2\n");
	result_idx = tolower(token[0]) - 'a';
	puts("result processed\n");

	//Processes "="
	token = strtok(NULL, " ");
	if (*token != '=') {
		puts("ERROR: Equals sign not found\n");
		return false;
	}
	puts("equals processed\n");
	
	//Processing first value
	
	token = strtok(NULL, " ");
	if (is_str_numeric(token)) {
		first_value = strtod(token, NULL);
	} else if (isalpha(*token)) {
		int first_idx = tolower(token[0]) - 'a';
		if (!session_list[session_id].variables[result_idx]) {
			puts("ERROR: First variable does not exist\n");
			return false;
		}
		
		first_value = session_list[session_id].values[first_idx];
	} else {
		puts("ERROR: Syntax error in the first variable\n");
		return false;
	}
	puts("first value processed\n");

	// Processes the operator
	
	token = strtok(NULL, " ");
	if (token == NULL) {
		session_list[session_id].variables[result_idx] = true;
		session_list[session_id].values[result_idx] = first_value;
		return true;
	}
	symbol = token[0];
	if (symbol != '+' && symbol != '-' && symbol != '*' && symbol != '/') {
		printf("ERROR: Invalid operator: %i\n", symbol);
		return false;
	}
	puts("operator processed\n");

	//Process the second value
	token = strtok(NULL, " ");
	if (is_str_numeric(token)) {
		second_value = strtod(token, NULL);
	} else if (isalpha(*token)) {
		int second_idx = tolower(token[0]) - 'a';
		if (!session_list[session_id].variables[result_idx]) {
			puts("ERROR: Second variable does not exist\n");
			return false;
		}

		second_value = session_list[session_id].values[second_idx];
	} else {
		puts("ERROR: Syntax error in the second variable\n");
		return false;
	}
	puts("second value processed\n");

	//No data should remain
	token = strtok(NULL, " ");

	if (token) {
		puts("ERROR: Too much input\n");
		return false;
	}

	puts("finshed processing\n");

	session_list[session_id].variables[result_idx] = true;

	switch (symbol) {
		case '+':
			session_list[session_id].values[result_idx] = first_value + second_value;
			break;
		case '-':
			session_list[session_id].values[result_idx] = first_value - second_value;
			break;
		case '*':
			session_list[session_id].values[result_idx] = first_value * second_value;
			break;
		case '/':
			session_list[session_id].values[result_idx] = first_value / second_value;
	}

	puts("math complete\n");

	return true;
}

/**
 * Broadcasts the given message to all browsers with the same session ID.
 *
 * @param session_id the session ID
 * @param message the message to be broadcasted
 */
void broadcast(int session_id, const char message[]) {
    for (int i = 0; i < NUM_BROWSER; ++i) {
        if (browser_list[i].in_use && browser_list[i].session_id == session_id) {
			pthread_mutex_lock(&browser_list_mutex);
            send_message(browser_list[i].socket_fd, message);
			pthread_mutex_unlock(&browser_list_mutex);
        }
    }
}

/**
 * Gets the path for the given session.
 *
 * @param session_id the session ID
 * @param path the path to the session file associated with the given session ID
 */
void get_session_file_path(int session_id, char path[]) {
    sprintf(path, "%s/session%d.dat", DATA_DIR, session_id);
}

/**
 * Loads every session from the disk one by one if it exists.
 */
void load_all_sessions() {
    // TODO: For Part 1.1, write your file operation code here.
    // Hint: Use get_session_file_path() to get the file path for each session.
    //       Don't forget to load all of sessions on the disk.
	
	FILE *fp;
	char path[SESSION_PATH_LEN];

	for (int i = 0; i < NUM_SESSIONS; i++) {
		get_session_file_path(i, path);
		
		int s_id = -1;
		sscanf(path, "./sessions/session%d.dat", &s_id);

		if (fp = fopen(path, "r")) {
			pthread_mutex_lock(&session_list_mutex);
			fread(&session_list[i], sizeof(struct session_struct), 1, fp);
			pthread_mutex_unlock(&session_list_mutex);
			fclose(fp);
		}
	}
}

/**
 * Saves the given sessions to the disk.
 *
 * @param session_id the session ID
 */
void save_session(int session_id) {
    // TODO: For Part 1.1, write your file operation code here.
    // Hint: Use get_session_file_path() to get the file path for each session.
	
	FILE* fp;
	char path[SESSION_PATH_LEN];
	pthread_mutex_lock(&session_list_mutex);
	struct session_struct current = session_list[session_id];
	pthread_mutex_unlock(&session_list_mutex);
	get_session_file_path(session_id, path);
	fp = fopen(path, "w");
	fwrite(&current, sizeof(struct session_struct), 1, fp);
	fclose(fp);
}

/**
 * Assigns a browser ID to the new browser.
 * Determines the correct session ID for the new browser through the interaction with it.
 *
 * @param browser_socket_fd the socket file descriptor of the browser connected
 * @return the ID for the browser
 */
int register_browser(int browser_socket_fd) {
    int browser_id;

    // TODO: For Part 2.2, identify the critical sections where different threads may read from/write to
    //  the same shared static array browser_list and session_list. Place the lock and unlock
    //  code around the critical sections identified.

    for (int i = 0; i < NUM_BROWSER; ++i) {
        	
		pthread_mutex_lock(&browser_list_mutex);
		if (!browser_list[i].in_use) {
            browser_id = i;
            browser_list[browser_id].in_use = true;
            browser_list[browser_id].socket_fd = browser_socket_fd;
			pthread_mutex_unlock(&browser_list_mutex);
            break;
        }
		pthread_mutex_unlock(&browser_list_mutex);
    }

    char message[BUFFER_LEN];
    receive_message(browser_socket_fd, message);

    int session_id = strtol(message, NULL, 10);
    if (session_id == -1) {
        for (int i = 0; i < NUM_SESSIONS; ++i) {
			pthread_mutex_lock(&session_list_mutex);
            if (!session_list[i].in_use) {
                session_id = i;
                session_list[session_id].in_use = true;
				pthread_mutex_unlock(&session_list_mutex);	
                break;
            }
			pthread_mutex_unlock(&session_list_mutex);	
        }
    }
	pthread_mutex_lock(&browser_list_mutex);
    browser_list[browser_id].session_id = session_id;
	pthread_mutex_unlock(&browser_list_mutex);

    sprintf(message, "%d", session_id);
    send_message(browser_socket_fd, message);

    return browser_id;
}

/**
 * Handles the given browser by listening to it, processing the message received,
 * broadcasting the update to all browsers with the same session ID, and backing up
 * the session on the disk.
 *
 * @param browser_socket_fd the socket file descriptor of the browser connected
 */
void browser_handler(void * browser_socket) {
	int browser_socket_fd = (int) (uintptr_t) browser_socket;
    int browser_id;

    browser_id = register_browser(browser_socket_fd);
	
    pthread_mutex_lock(&browser_list_mutex);
    int socket_fd = browser_list[browser_id].socket_fd;
	printf("sock brws %d %d\n", browser_socket_fd, socket_fd);
    int session_id = browser_list[browser_id].session_id;
    pthread_mutex_unlock(&browser_list_mutex);
    printf("Successfully accepted Browser #%d for Session #%d.\n", browser_id, session_id);

    while (true) {
        char message[BUFFER_LEN];
        char response[BUFFER_LEN];
	
        ssize_t output = receive_message(socket_fd, message);
        printf("Received message from Browser #%d for Session #%d: %s\n", browser_id, session_id, message);
		printf("status: %li\n", output);
		perror("WHY");
        if ((strcmp(message, "EXIT") == 0) || (strcmp(message, "exit") == 0)) {
            close(socket_fd);
            pthread_mutex_lock(&browser_list_mutex);
            browser_list[browser_id].in_use = false;
            pthread_mutex_unlock(&browser_list_mutex);
            printf("Browser #%d exited.\n", browser_id);
            return;
        }

//        if (message[0] == '\0') {
//            continue;
//        }

        bool data_valid = process_message(session_id, message);
        if (!data_valid) {
            // TODO: For Part 3.1, add code here to send the error message to the browser.
			printf("not valid\n");
            sprintf(response, "user input invalid\n");
            continue;
        }

		//sprintf(response, "not yet implemented\n");
        session_to_str(session_id, response);
        broadcast(session_id, response);

        save_session(session_id);
    }
}

/**
 * Starts the server. Sets up the connection, keeps accepting new browsers,
 * and creates handlers for them.
 *
 * @param port the port that the server is running on
 */
void start_server(int port) {
    // Loads every session if there exists one on the disk.
    load_all_sessions();

    // Creates the socket.
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Binds the socket.
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(port);
	setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, (void *) 1, sizeof(int));
    if (bind(server_socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("Socket bind failed");
        exit(EXIT_FAILURE);
    }

    // Listens to the socket.
    if (listen(server_socket_fd, SOMAXCONN) < 0) {
        perror("Socket listen failed");
        exit(EXIT_FAILURE);
    }
    printf("The server is now listening on port %d.\n", port);

    // Main loop to accept new browsers and creates handlers for them.
	
	pthread_t thread_ids[NUM_BROWSER];
	int index = 0;
    while (true) {
        struct sockaddr_in browser_address;
        socklen_t browser_address_len = sizeof(browser_address);
		int browser_socket_fd = accept(server_socket_fd, (struct sockaddr *) &browser_address, &browser_address_len);
        if ((browser_socket_fd) < 0) {
            perror("Socket accept failed");
            continue;
        }

        // Starts the handler thread for the new browser.
        // TODO: For Part 2.1, create a thread to run browser_handler() here.
		pthread_create(&thread_ids[index], NULL, (void *) browser_handler, (void *) (intptr_t) browser_socket_fd);
	//	pthread_join(thread_id, NULL);
		index++;
	//	browser_handler(browser_socket_fd);
    }

	for (int i = 0; i < index; i++) {
		pthread_join(thread_ids[i], NULL);
	}

    // Closes the socket.
    close(server_socket_fd);
}


hashmap_t* new_hashmap() {

	hashmap_t* map = malloc(sizeof(hashmap_t));
	map->capacity = HASH_SIZE;
	map->length = 0;
	map->map = calloc(map->capacity, sizeof(entry_t*));
	return map;
}

unsigned hash(hashmap_t* map, int key) {

	return key * 37 % HASH_SIZE;
}

session_t* get(hashmap_t* map, int key) {

	entry_t* current;
	
	for (current = map->map[hash(map, key)]; current; current = current->next) {
		if (!strcmp(current->key, key)) {
			return current->session;
		}
	}
}


int set(hashmap_t* map, int key, session_t* session) {

	unsigned index = hash(map, key);
	entry_t* current;

	for (current = map->map[index]; current; current = current->next) {
		if (!strcmp(current->key, key)) {
			current->session = session;
			return current->key;
		}
	}

	entry_t* e = malloc(sizeof(*e));
	e->key = key;
	e->session = session;
	p->next = map->map[index];
	map->map[index] = p;
	this->length++;
	return index;
}

bool destroy(hashmap_t* map, int key) {
	int index = hash(map, key);
	entry_t* entry = map->map[index];

	if (!entry->next) {
		free(entry->session);
		free(entry);
		return true;
	}

	entry_t* previous = NULL;

	while (entry) {
		if (entry-> key == key) {
			entry_t* next = entry->next;
			if (next) {
				if (!previous) {
					entry_t* new_entry;
					new_entry->session = entry->session;
					new_entry->next = next->next;
					new_entry->key = key;
					map->map[index] = new_entry;
				} else {
					previous->next = next;
				}
			}
			free(entry->next);
			free(entry->session);
			free(entry);
			entry = NULL;

			return true;
		}
		previous = entry;
		entry = entry->next;
	}

	return false;
}

/**
 * The main function for the server.
 *
 * @param argc the number of command-line arguments passed by the user
 * @param argv the array that contains all the arguments
 * @return exit code
 */
int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;

    if (argc == 1) {
    } else if ((argc == 3)
               && ((strcmp(argv[1], "--port") == 0) || (strcmp(argv[1], "-p") == 0))) {
        port = strtol(argv[2], NULL, 10);

    } else {
        puts("Invalid arguments.\n");
        exit(EXIT_FAILURE);
    }

    if (port < 1024) {
        puts("Invalid port.\n");
        exit(EXIT_FAILURE);
    }

    start_server(port);

    exit(EXIT_SUCCESS);
}
