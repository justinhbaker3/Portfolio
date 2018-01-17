#define PING 1
#define GETSTATS 2
#define RESETSTATS 3
#define COMPRESS 4

#define MAGICNUMBER 0x53545259

#define MAX_MESSAGE_SIZE 16*1024

typedef struct header_info{
	uint32_t magic_number;
	uint16_t payload_length;
	uint16_t req_code;
} header_info;


/* 
* Compression function. Takes a string and compresses it in place.
* Returns the compressed length or -1 if an error occurs.
*/ 
int compress(char * s);


/*
* Reads and parses header from client and returns in a header_info struct
*/
header_info parse_header(int client_fd);


/*
* Composes a response header given the payload length and server status
*/
void * compose_header(uint16_t payload_length, uint16_t status);


/*
* Sends an error header to the client given the type of error
*/
void send_error(int client_fd, int error);


/* Handles PING requests */
int handle_ping(int client_fd, header_info header);

/* Handles GET_STATS requests */
int handle_get_stats(int client_fd, header_info header);

/* Handles RESET_STATS requests */
int handle_reset_stats(int client_fd, header_info header);

/* Handles compress requests */
int handle_compress(int client_fd, header_info header);


/* 
* Processes a single client by reading header and calling the appropriate
* request function
*/
int process_client(int client_fd);


/* 
* reads all data from socket in case of interrupt or network delay
* used for potentially long reads in handle_compress()
*/
ssize_t read_all(int socket, char * buf, size_t count);


