#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h> 

void *handle_client(void *client_ptr) {
    int client_fd = *((int *)client_ptr);
    free(client_ptr);

    char buffer[1024];
    while (1) {
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) break;

        buffer[bytes_received] = '\0'; // Null-terminate for string searching

        // Check if the command contains "ECHO" (case-insensitive search is safer)
        char *echo_pos = strcasestr(buffer, "ECHO");
        
        if (echo_pos) {
            // Find the start of the argument after "ECHO\r\n"
            // In RESP, the argument starts after the '$' and length line
            char *next_line = strstr(echo_pos, "\r\n");
            if (next_line) {
                char *bulk_string_start = strstr(next_line + 2, "\r\n");
                if (bulk_string_start) {
                    char *data_start = bulk_string_start + 2;
                    char *data_end = strstr(data_start, "\r\n");
                    
                    if (data_end) {
                        int data_len = data_end - data_start;
                        char response[1024];
                        // Format the response as a Bulk String: $<length>\r\n<data>\r\n
                        int resp_len = sprintf(response, "$%d\r\n%.*s\r\n", data_len, data_len, data_start);
                        send(client_fd, response, resp_len, 0);
                    }
                }
            }
        } else {
            // Default to PONG for anything else (like PING)
            send(client_fd, "+PONG\r\n", 7, 0);
        }
    }

    printf("Client disconnected\n");
    close(client_fd);
    return NULL;
}


int main() {
	// Disable output buffering
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);
	
	// You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	// Uncomment the code below to pass the first stage
	
	 int server_fd, client_addr_len;
	 struct sockaddr_in client_addr;
	
	 server_fd = socket(AF_INET, SOCK_STREAM, 0);
	 if (server_fd == -1) {
	 	printf("Socket creation failed: %s...\n", strerror(errno));
	 	return 1;
	 }
	//
	// // Since the tester restarts your program quite often, setting SO_REUSEADDR
	// // ensures that we don't run into 'Address already in use' errors
	 int reuse = 1;
	 if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
	 	printf("SO_REUSEADDR failed: %s \n", strerror(errno));
	 	return 1;
	 }
	//
	 struct sockaddr_in serv_addr = { .sin_family = AF_INET ,
	 								 .sin_port = htons(6379),
	 								 .sin_addr = { htonl(INADDR_ANY) },
	 								};
	
	 if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
	 	printf("Bind failed: %s \n", strerror(errno));
	 	return 1;
	 }
	//
	 int connection_backlog = 5;
	 if (listen(server_fd, connection_backlog) != 0) {
	 	printf("Listen failed: %s \n", strerror(errno));
	 	return 1;
	 }
	//
	printf("Waiting for a client to connect...\n");
	
	//2. Creating a buffer to hold the incoming data
	
	//3. Reading the data from the client
	
	while(1){

	
	client_addr_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
    
    if (client_fd == -1) {
        printf("Accept failed: %s\n", strerror(errno));
        continue; // Keep trying for the next client
    }
    printf("New client connected!\n");

    // Allocate memory for the file descriptor to pass to the thread safely
    int *new_sock = malloc(sizeof(int));
    *new_sock = client_fd;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, (void *)new_sock) != 0) {
        printf("Could not create thread\n");
        free(new_sock);
    }
    
    // Detach the thread so its resources are freed automatically when it's done
    pthread_detach(thread_id);
}
	//4. Cleaning Up
	
	close(server_fd);

	return 0;
}
