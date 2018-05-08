# Multiparty-Chat-application-using-C
Chat application designed to handled 100 users (clients), it could be scaled further. 
Developed using concepts of Socket programming, Threading, Semaphores and Mutual exclusion.

Design considerations: 
Currently application can support up to 100 users to chat simultaneously, this could be scaled further if required. 
Size of each text message is limited to 80 characters, this could be upgraded as well. 
For each user, history of past 10 messages are stored for retrieval. 

Steps for execution of program:
Steps to compile:
(Server) : gcc server.c -lpthread -o server
(client) : gcc client.c -lpthread -o client
Steps to execute (Using port 2000 for example)
./server 2000
./client csgrads1 2000


Multiple users could connect using different session using Putty terminal. 
For example, I have used 4 clients to demonstrate Multiparty chat application. 

Server used: 
csgrads1.utdallas.edu

Clients used: 
net05.utdallas.edu (Client 1 – John)
net06.utdallas.edu (Client 2 – Peter)
net07.utdallas.edu (Client 3 – Robert)
net08.utdallas.edu (Client 4 – Leo) 
