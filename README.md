# Final
Final project for (os and exercises)

Compiling and execution

## For server 
gcc -o server server.c -lpthread

## For client
gcc -o client client.c


Client can be executed either without an argument, (will then await files) or with an argument (send files to other clients).
If the client wants to send files, it needs to be executed lastly, after the other clients are already connected.

## With argument
./client file.txt

## Without argument
./client 
