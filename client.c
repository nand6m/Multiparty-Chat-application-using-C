/* Importing required header files */
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <pthread.h>

#define BUF_SIZE 81 /* Buffer size is defined to 81 */

int sd;
/* If the string ends with '\n', replace it with '\t',
 * otherwise append a '\t' after the string.
 * '\t' indicates the end of the string. 
 */
void wrap(char str[]) {
	if ( str[strlen(str)-1] == '\n') {
		str[strlen(str)-1] = '\t';
	} else {
		strcat(str, "\t");
	}
}

/*  Replace '\t' in the end with null character. 
 */
void unwrap(char str[]) {
	str[strlen(str)-1] = '\0';
}

/* Keep reading until encounter '/t' which indicates the end of the message.
 */
void read_socket(int sd, char * targetStr) {
	char temp[20*BUF_SIZE] = "0";
	strcpy(targetStr, "");
	while (temp[strlen(temp) - 1] != '\t') {
		if (read(sd, temp, 20*BUF_SIZE) == -1) {
			perror("ERROR on read call.");
			exit(1);
		}
		strcat(targetStr, temp);
	}
	unwrap(targetStr);
}

/* Before write message, wrap it with '\t' in the ending.
 */
void write_socket(int sd, char * targetStr) {
	wrap(targetStr);
	if (write(sd, targetStr, strlen(targetStr)) == -1 ) {
		perror("Error on write call.");
		exit(1);
	}
	unwrap(targetStr);
}

/* When client use Control + c to terminate, this function is called and the process terminates.
 * This function will send a message with simple character that indicates the exit.
 */
void sigint_handler(int sig)
{
	char controlC[BUF_SIZE];
	controlC[0] = 27;
	write_socket(sd,controlC );
	exit(1);
}

/* If the user is unknown, server will return the value (userId + 100),
 * so client has to unwrap it to actual userId.
 */
void unwrap_UserId(int * userId) {
	if (*userId >= 100) {
		*userId -= 100;
	} 
}

/* The user list sent from server only contains essential data,
 * this function parse data into formated text to output.
 */
void parseUserList(char * userString) {
	char split[2];
	split[0] = '\f';
	char *token;
	token = strtok(userString, split);
	int i = 0;
	while( token != NULL ) {
		i++;
		printf("  %d. %s\n", i, token);
		token = strtok(NULL, split);
	}
}

int main(int argc, char *argv[]) {
	char hostname[100];
	int port;
	int count;
	struct sockaddr_in pin;
	struct hostent *hp;

	/* check for command line arguments */
	if (argc != 3)
	{
		printf("Usage: client host port\n");
		exit(1);
	}

	/* get hostname and port from argv */
	strncpy(hostname, argv[1], sizeof(hostname));
	hostname[99] = '\0';
	port = atoi(argv[2]);

	/* create an Internet domain stream socket */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Error creating socket");
		exit(1);
	}

	/* lookup host machine information */
	if ((hp = gethostbyname(hostname)) == 0) {
		perror("Error on gethostbyname call");
		exit(1);
	}

	/* fill in the socket address structure with host information */
	memset(&pin, 0, sizeof(pin));
	pin.sin_family = AF_INET;
	pin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	pin.sin_port = htons(port); /* convert to network byte order */

	printf("Connecting to %s:%d\n\n", hostname, port); 

	/* connect to port on host */
	if (connect(sd,(struct sockaddr *)  &pin, sizeof(pin)) == -1) {
		perror("Error on connect call");
		exit(1);
	}

	int userId;
	int i;
	char temp[BUF_SIZE];
	char name[BUF_SIZE];
	char selection[5];

	
	void sigint_handler(int sig); 
	char s[200];
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
    sa.sa_flags = 0; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1) {
    	perror("sigaction");
    	exit(1);
    }

	/* Ask user for name. */
    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);

	/* Send name to server. */ 
    write_socket(sd, name);

    /* Recieve userId. */
    read_socket(sd, temp);
    userId = atoi(temp);
    unwrap_UserId(&userId);

    /* If the user is already on line, the userId from server would be negative. */
    if (userId < 0) {
    	puts("\n********** LOGIN CONFLICTS **********");
    	return;
    }

    /* Keep handler command until get '5'. */
    while (selection[0] != '5') {
    	puts("\n==========  CHAT MENU  ==========");
    	printf("1.  Display names of all connected users\n");
    	printf("2.  Send text message to a specific user\n");
    	printf("3.  Send text message to all connected users\n");
    	printf("4.  View my messages\n");
    	printf("5.  Exit from Chat\n");
    	printf("=================================== \n");
	printf("Enter your choice : ");
		
    	/* Ask for menu selection. */
    	fgets(selection, BUF_SIZE, stdin);
    	puts("");
    	write_socket(sd, selection);
    	switch (selection[0]) {

    		case '1':
    		puts("Connected users:");
    		read_socket(sd, temp);	
    		parseUserList(temp);
    		break;

    		case '2':
    		/* Prompt for recipient and message content. */
    		printf("Enter recipient's name: ");
    		fgets(temp, BUF_SIZE, stdin);
    		printf("Enter a message: ");
    		fgets(selection, BUF_SIZE, stdin);
    		temp[strlen(temp) - 1] = 0;
    		printf("\nMessage posted to %s.\n", temp);
    		temp[strlen(temp)] = '\n';
    		strcat(temp, selection);
    		/* Send recipient and message content to server. */
    		write_socket(sd, temp);
    		break;

    		case '3':
    		printf("Enter a message: ");
    		fgets(temp, BUF_SIZE, stdin);
    		printf("\nMessage posted to all connected users.");
    		write_socket(sd, temp);
    		break;

    		case '4':
    		puts("Your messages:");
    		char messagesBuf[20*BUF_SIZE];
    		read_socket(sd, messagesBuf);
    		puts(messagesBuf);
    		break;

    		case '5':
    		puts("Ending the Chat, Thank you!");
    		break;

    		default:
    		/* Invalid command handler. */
    		puts("**********  INVALID COMMAND : PLEASE TYPE AGAIN  **********");
    		break;
    	}
    }
    close(sd);
}
