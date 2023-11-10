# A simple http server/client pair written in C for CS 3516

## Compilation:
    
  Enter the following command in the root directory:
  
  `make`

## Running the Client:
  Enter the following command in the root directory:
  
  `./http_client [-options] server_url port_number`

    [-options]
        -p: prints a table of RTT values
        
    server_url
        The url of the HTTP server you wish to contact
        
    port_number
        The port number (80 for default HTTP) of the server you wish to contact

## Running the Server:
  Enter the following command in the root directory:
  
  `./http_server port_number`

    port_number
        The port number you wish to run the server on 
