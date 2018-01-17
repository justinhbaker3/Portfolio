Target platform: Ubuntu 16.04 (64 bit)
Language: GLIBC 2.23 compiled under gcc 5.4.0

File list:
server.h - header file for server
server.c - implementation for server
compress.c - compression function
build.sh - bash script to build server execuatable
run.sh - bash script to run server once built


Overview:

This server provides a compression service that replaces consecutive letters with a
prefix indicating the number of times the letter is repeated followed by the letter
(E.g. aaabbbaafggg becomes 3a3baaf3g).

Ping, Get Stats, Reset Stats, and Compress requests are supported. 

^C to exit


Implementation Details:

One TCP connection is accepted at a time on port 4000 (port can be changed by 
modifying run.sh). The socket, bind, listen, and 
accept functions are used to establish the coonnection with the client. Once the 
connection is made, the header is read from the client and parsed to validate that 
the magic number is correct and to determine the request type and payload size. The 
header must be converted from network byte order to host order first. Then
the request is fulfilled and sent back to the client. The number of bytes sent and
recieved is tracked, as well as the compression ratio (lengths of compressed string
/ total lengths of uncompressed strings). 


Custom Status Codes:
33 - nonzero payload length specified in header for a ping, get stats, or
reset stats request.
34 - magic number in header incorrect
35 - invalid string to compress (string must only contain lowercase letters)
36 - payload length of 0 specified in header for a compress request


Assumptions:

1. Host uses little-endian byte ordering

2. A complete header is always recieved from the client.

3. The payload length specified in the header is always accurate. If the actual
length is too short, the server will attempt to read the remaining bytes forever.
If the actual length is too long, the server will only read the number of btes
specified in the header. 
However, as long as the actual length matches the specified length, the server
is guaranteed to read all the bytes despite any network delsy.

4. The client doesn't close the connection before the response is returned.


Potential Improvements:

Use multiple threads to handle multiple clients simultaneously. This can be
implemented by setting a fixed number of threads to service clients
and adding connections to an epoll system as they arrive. Then as data arrives,
threads are assigned to clients.

Adding detection for incomplete headers and payload sizes that don't match the size 
specified in the header.

Adding a way to exit without a SIGINT or similar


	

