# A simple http server/client pair written in C for CS 3516

## Compilation:
  `make`

## Running the Client:
  `./http_client [-options] server_url port_number`

    [-options]
        -p: prints a table of RTT values
        
    server_url
        The url of the HTTP server you wish to contact
        
    port_number
        The port number (80 for default HTTP) of the server you wish to contact

## Running the Server:
  `./http_server port_number`

    port_number
        The port number you wish to run the server on 
