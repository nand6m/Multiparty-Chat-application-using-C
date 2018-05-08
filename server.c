
/* Include required libraries */
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <netdb.h>

#define BUF_SIZE 81 /* Define Buffer size */

void* handleClient(void *);

/* Declaring semaphores for mutual exclusion */
sem_t msg_mutex;
sem_t users_mutex;
sem_t online_mutex;


int main(int argc, char * argv[]) {

	int 		port;
	char 		host[100];
	int	       	sd, sd_current;
	int 		*sd_client;
	int 		addr_len;
	struct 		sockaddr_in sin;
	struct 		sockaddr_in pin; 
	pthread_t 	tid;
	pthread_attr_t attr;

	/* Init semaphores. */
	sem_init(&online_mutex, 0, 1);
	sem_init(&msg_mutex, 0, 1);
	sem_init(&users_mutex, 0, 1);
	

	if (argc != 2) {
		printf("Please enter server port\n");
		exit(1);
	}

	/* Get port from argv. */
	port = atoi(argv[1]);

	/* Create an internet domain stream socket. */
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("Failed to create socket");
		exit(1);
	}

	/* Complete the socket structure. */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;		/* Any address on this host. */
	sin.sin_port = htons(port);					/* Convert to network byte order. */

	/* Bind socket to the address and port number */
	if (bind(sd, (struct sockaddr *) &sin, sizeof(sin)) == -1) {
		perror("bind failed");
		exit(1);
	}

	
	if (listen(sd, 5) == -1) {
		perror("Failed on listen call");
		exit(1);
	}

	
	gethostname(host, 80); 
	printf("Server is running on %s:%d\n", host, port);

	/* Wait for a client to connect */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED); 
	addr_len = sizeof(pin);
	while (1)
	{
		if ((sd_current = accept(sd, (struct sockaddr *)  &pin, (socklen_t*)&addr_len)) == -1) 
		{
			perror("accept failed");
			exit(1);
		}

		sd_client = (int*)(malloc(sizeof(sd_current)));
		*sd_client = sd_current;

		pthread_create(&tid, &attr, handleClient, sd_client);
	}

	 /* close socket */
	close(sd);
}



int users_Pointer = 0;
int online[100] = {0};
int reciever[1000];

char users[100][BUF_SIZE];
char message[1000][BUF_SIZE];
char messages_Header[100][10][BUF_SIZE];
char messages[100][10][BUF_SIZE];
char tab[2] = "\t";

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

/* Replace '\t' in the end with null character. 
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
		if (read(sd, temp, BUF_SIZE) == -1) {
			perror("Failed on read call.");
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
	int size = -1;
	if ( (size = write(sd, targetStr, strlen(targetStr))) == -1 ) {
		perror("Failed on write call.");
		exit(1);
	}
	unwrap(targetStr);
}


/* Insert user into users array if they are unknown.
 */
int put_User(char * name) {
	sem_wait(&users_mutex);
	int i = 0;
	/* If the user is a known user, return the userId. */
	for (i=0; i<users_Pointer; i++){
		if (strcmp(name, users[i]) == 0) {
			sem_post(&users_mutex);
			return i;
		} 
	}
	/* If the user is an unknown user, return (100 + userId). */
	strcpy(users[users_Pointer], name);
	users_Pointer++;
	sem_post(&users_mutex);
	return (100 + users_Pointer - 1);
}

/* If the user is unknown, put_User functino will return the value (userId + 100),
 * so client has to unwrap it to actual userId.
 */
void unwrap_UserId(int * userId) {
	if (*userId >= 100) {
		*userId -= 100;
	} 
}

/*  Wrap the log description with time stamp and output. 
 */
void server_Log(char * logDes) {
	time_t now = time(0);
	char nowStr[30];
	strftime (nowStr, 30, "%m/%d/%y, %I:%M %p, ", localtime (&now));
	printf("%s%s\n", nowStr, logDes);
}


void save_Message(char * sender, int userId, char * message) {
	sem_wait(&msg_mutex);
	int i = 0;
	while (i < 10) {
		if (messages[userId][i][0] == 0) {
			strcpy(messages[userId][i], message);
			char header[BUF_SIZE];
			time_t now = time(0);
			char nowStr[30];
			strftime (nowStr, 30, "%m/%d/%y, %I:%M %p, ", localtime (&now));
			sprintf(header, "From %s, %s", sender, nowStr);
			strcpy(messages_Header[userId][i], header);
			sem_post(&msg_mutex);
			return;
		}
		i++;
	}
	sem_post(&msg_mutex);
}


/* When user login, set its online flag to 1.
 */
void set_Online(int userId, int isOnline) {
	sem_wait(&online_mutex);
	online[userId] = isOnline;
	sem_post(&online_mutex);
}

void* handleClient(void *arg) {

	char selection[5] = "0";
	char name[BUF_SIZE];
	int userId;
	char temp[BUF_SIZE];
	int sd = *((int*)arg);
	int i = 0;
	free(arg);
	read_socket(sd, name);

	if (name[0] == 27) {
		return;
	}

	userId = put_User(name);
	if (userId >= 100) {
		char logDes[BUF_SIZE];
		sprintf(logDes, "Connection by unknown user %s.", name);
		server_Log(logDes);
	} else {
		char logDes[BUF_SIZE];
		sprintf(logDes, "Connection by known user %s.", name);
		server_Log(logDes);
	}
	sprintf(temp, "%d", userId);
	unwrap_UserId(&userId);
	if (online[userId]) {
		strcpy(temp, "-1");
		write_socket(sd, temp);
		return;
	}

	/* Notify client the userId */
	write_socket(sd, temp);

	
	set_Online(userId, 1);

	/* Keep respond to client until get '5'. */
	while (selection[0] != '5') {
		read_socket(sd, selection);
		char logDes[BUF_SIZE];
		/* Control + c handler. */
		if (selection[0] == 27) {
			sprintf(logDes, "%s exits.", name);
			server_Log(logDes);
			set_Online(userId, 0);
			return;
		}
		switch (selection[0]) {

			case '1':
			sprintf(logDes, "%s displays all connected users.", name);
			server_Log(logDes);
			i = 0;
			strcpy(temp, "");
			/* Iterate the users array and check online flag,
			 * send the online user list. */
			while (i < users_Pointer) {
				if (online[i]) {
					char item[BUF_SIZE];
					sprintf(item, "%s\f", users[i]);
					strcat(temp, item);
				}
				i++;
			}
			write_socket(sd, temp);
			break;

			case '2':
			read_socket(sd, temp);
			/* Control + c handler. */
			if (temp[0] == 27) {
				sprintf(logDes, "%s exits.", name);
				server_Log(logDes);
				set_Online(userId, 0);
				return;
			}
			char split[2];
			split[0] = 10;
			char *token;
			token = strtok(temp, split);
			/* message[0] stores the recipient, 
			 * message[1] stores the message content. */
			char message[2][BUF_SIZE];
			i = 0; 
			
			while( i<=1 ) 
			{
				strcpy(message[i], token);
				i++;
				token = strtok(NULL, split);
			}
			
			int recipientId = put_User(message[0]);
			unwrap_UserId(&recipientId);
			/* Store message. */
			save_Message(name, recipientId, message[1]);
			sprintf(logDes, "%s posts a message for %s.", name, message[0]);
			server_Log(logDes);
			break;

			case '3':
			read_socket(sd, temp);
			if (temp[0] == 27) {
				sprintf(logDes, "%s exits.", name);
				server_Log(logDes);
				set_Online(userId, 0);
				return;
			}
			i = 0;
			/* Save message to every users */
			while (i < users_Pointer) {
				if (online[i]) {
					save_Message(name, i, temp);
				}
				i++;
			}
			sprintf(logDes, "%s posts a message for all currently connected users.", name);
			server_Log(logDes);
			break;


			case '4':
			sprintf(logDes, "%s gets messages.", name);
			server_Log(logDes);
			i = 0;
			char messagesBuf[20*BUF_SIZE];
			strcpy(messagesBuf, "");
			/* Send the user's all messages. */
			while (messages[userId][i][0] != 0 && i < 10) {
				char bullet[5];
				sprintf(bullet, "  %d. ", i+1);
				strcat(messagesBuf, bullet);
				strcat(messagesBuf, messages_Header[userId][i]);
				strcat(messagesBuf, messages[userId][i]);
				strcat(messagesBuf, "\n");
				i++;
			}
			/* Notify user there is no messages. */
			if (messagesBuf[0] == 0) {
				strcat(messagesBuf, "  No Messages for you.\n");
			}
			write_socket(sd, messagesBuf);
			break;

			case '5':
			sprintf(logDes, "%s exits.", name);
			server_Log(logDes);
			set_Online(userId, 0);
			break;

			/* Invalid command case. */
			default:
			break;
		}
	}
	close(sd);
}
