#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_CLIENTS 100
#define MAX_CHATROOMS 10
#define BUFFER_SIZE 2048
#define MAX_PARTICIPANTS 10


static _Atomic unsigned int cli_count = 0;
static int uid = 10;



/* Client struct */
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
} client;



/* Arrays for clients and chatrooms */
client *clients[MAX_CLIENTS];


pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;




// Definição da struct chatroom
typedef struct {
    char name[50];
    int num_participants;
    char participants[MAX_PARTICIPANTS][50];
} chatroom;


    chatroom* chatrooms[MAX_CHATROOMS] = { NULL };
    int num_chatrooms = 0;

// Função para criar uma nova chatroom
chatroom* create_chatroom(const char* name) {
    chatroom* new_chatroom = (chatroom*)malloc(sizeof(chatroom));
    if (new_chatroom != NULL) {
        strncpy(new_chatroom->name, name, sizeof(new_chatroom->name));
        new_chatroom->name[sizeof(new_chatroom->name) - 1] = '\0'; // Garantir terminação nula
        new_chatroom->num_participants = 0;
        printf("Chatroom '%s' created.\n", new_chatroom->name);
    } else {
        printf("Failed to create chatroom.\n");
    }
    return new_chatroom;
}

// Função para adicionar um participante à chatroom pelo nome
void add_participant(chatroom* room, const char* participant_name) {
    if (room->num_participants < MAX_PARTICIPANTS) {
        strncpy(room->participants[room->num_participants], participant_name, sizeof(room->participants[0]));
        room->participants[room->num_participants][sizeof(room->participants[0]) - 1] = '\0'; // Garantir terminação nula
        room->num_participants++;
        printf("Participant '%s' added to chatroom '%s'.\n", participant_name, room->name);
    } else {
        printf("Chatroom is already full.\n");
    }
}

// Função para listar as chatrooms existentes
char* list_chatrooms(chatroom* chatrooms[], int num_chatrooms) {
    char* result = (char*)malloc(1024 * sizeof(char)); // Aloca espaço para a string de resultado
    if (result == NULL) {
        perror("Error allocating memory");
        exit(EXIT_FAILURE);
    }

    if (num_chatrooms == 0) {
        sprintf(result, "No chatrooms found.\n");
    } else {
        sprintf(result, "List of chatrooms:\n");
        char temp[256];
        for (int i = 0; i < num_chatrooms; i++) {
            if (chatrooms[i] != NULL) {
                sprintf(temp, "%d. %s (%d participants)\n", i + 1, chatrooms[i]->name, chatrooms[i]->num_participants);
                strcat(result, temp);
            }
        }
    }

    return result;
}

// Função para apagar chatroom
void delete_chatroom(chatroom* room) {
    if (room != NULL) {
        free(room);
        printf("Chatroom has been deleted.\n");
    } else {
        printf("Chatroom is already empty.\n");
    }
}

// Função para tirar participante da sala
void remove_participant(chatroom* room, const char* participant_name) {
    for (int i = 0; i < room->num_participants; i++) {
        if (strcmp(room->participants[i], participant_name) == 0) {
            // Remover o participante da sala
            for (int j = i; j < room->num_participants - 1; j++) {
                strcpy(room->participants[j], room->participants[j + 1]);
            }
            room->num_participants--;
            printf("Participant '%s' removed from chatroom '%s'.\n", participant_name, room->name);
            return;
        }
    }
    printf("Participant '%s' not found in chatroom '%s'.\n", participant_name, room->name);
}



//------------------------------------------------------------------------------------------------



void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}


void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}


/* Print client adress */
void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}


/* Add clients to queue */
void queue_add(client *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}


/* Remove clients to queue */
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}


bool areNamesInSameChatroom(chatroom* chatrooms[], int num_chatrooms, const char name1[50], const char name2[50]) {
    for (int i = 0; i < num_chatrooms; i++) {
        if (chatrooms[i] != NULL) {
            for (int j = 0; j < chatrooms[i]->num_participants; j++) {
                if (strcmp(chatrooms[i]->participants[j], name1) == 0) {
                    for (int k = 0; k < chatrooms[i]->num_participants; k++) {
                        if (strcmp(chatrooms[i]->participants[k], name2) == 0) {
                            return true; // Ambos os nomes estão na mesma chatroom
                        }
                    }
                }
            }
        }
    }
    return false; // Os nomes não estão na mesma chatroom
}


/* Send message to all clients except sender */
void send_message(char *s, int uid){

	
    char word1[50], word2[50], word3[50];

    int num_items = sscanf(s, "%[^:]: %s %s", word1, word2, word3);

    if (num_items == 2) {
        // Se apenas duas palavras foram lidas, a terceira palavra é opcional e podemos atribuir um valor padrão.
        strcpy(word3, "default_value");
    }

    //printf("\nPalavra 1: %s\n", word1);
    //printf("Palavra 2: %s\n", word2);
    //printf("Palavra 3: %s\n\n", word3);

	if (strstr(s, "/list") != NULL)
	{
		printf("*LIST CHATROOMS*\n");

		//list_chatrooms(chatrooms, num_chatrooms);
		char* list_output = list_chatrooms(chatrooms, num_chatrooms);
    	printf("%s", list_output);
    	// Lembre-se de liberar a memória alocada para a string de resultado após o uso.

		pthread_mutex_lock(&clients_mutex);

		for(int i=0; i<MAX_CLIENTS; ++i){
			if(clients[i]){
				if(clients[i]->uid == uid){
					if(write(clients[i]->sockfd, list_output, strlen(list_output)) < 0)
					{
						perror("ERROR: write to descriptor failed");
						break;
					}
				}
			}
		}
    	free(list_output);

		pthread_mutex_unlock(&clients_mutex);


	}

	//  Identifica o comando create
	if (strstr(s, "/create") != NULL)
	{
		if (num_chatrooms < MAX_CHATROOMS) 
		{
			word3[strcspn(word3, "\n")] = '\0'; // Remover o caractere de nova linha

			// Verificar se a chatroom com o mesmo nome já existe
			int found = 0;
			for (int i = 0; i < num_chatrooms; i++) 
			{
				if (chatrooms[i] != NULL && strcmp(chatrooms[i]->name, word3) == 0) 
				{
					found = 1;
					printf("Chatroom '%s' already exists.\n", word3);
					break;
				}
			}

			if (!found) 
			{
				chatroom* new_room = create_chatroom(word3);
				if (new_room != NULL) {
					chatrooms[num_chatrooms] = new_room;
					num_chatrooms++;
				}
			}
		} else 
		{
			printf("Maximum number of chatrooms reached.\n");
		}
	
	}

	if (strstr(s, "/delete") != NULL)
	{
		if (num_chatrooms == 0) {
			printf("No chatrooms found.\n");
		} else {
			
			word3[strcspn(word3, "\n")] = '\0'; // Remover o caractere de nova linha

			// Procurar a chatroom com o nome fornecido
			int found = 0;
			for (int i = 0; i < num_chatrooms; i++) {
				if (chatrooms[i] != NULL && strcmp(chatrooms[i]->name, word3) == 0) {
					found = 1;
					delete_chatroom(chatrooms[i]);
					chatrooms[i] = NULL;
					printf("Chatroom '%s' deleted.\n", word3);
					break;
				}
			}

			if (!found) {
				printf("Chatroom '%s' not found.\n", word3);
			}
		}
	}

	if (strstr(s, "/join") != NULL)
	{
		if (num_chatrooms == 0) {
                printf("No chatrooms found.\n");
            } else {
                word3[strcspn(word3, "\n")] = '\0';
                int found = 0;
                for (int i = 0; i < num_chatrooms; i++) {
                    if (chatrooms[i] != NULL && strcmp(chatrooms[i]->name, word3) == 0) {
                        found = 1;
                        add_participant(chatrooms[i], word1);
                        break;
                    }
                }

                if (!found) {
                    printf("Chatroom '%s' not found.\n", word3);
                }
            }
	}

	if (strstr(s, "/exit") != NULL)
	{
		// Remover o participante da sala
		for (int i = 0; i < num_chatrooms; i++) {
			if (chatrooms[i] != NULL) {
				remove_participant(chatrooms[i], word1);
			}
		}
		printf("*EXIT CHATROOM*\n");
	}                              

	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
	{
		if(clients[i])
		{
			bool result = areNamesInSameChatroom(chatrooms, num_chatrooms, clients[i]->name, word1);
			if(result)
			{
				if(write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);

}


/* Handle communication with the client */
void *handle_client(void *arg){
	char buff_out[BUFFER_SIZE];
	char name[32];
	int leave_flag = 0;

	cli_count++;
	client *cli = (client *)arg;

	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("Didn't enter the name.\n");
		leave_flag = 1;
	} else{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s has joined\n", cli->name);
		printf("%s", buff_out);
		send_message(buff_out, cli->uid);
	}

	bzero(buff_out, BUFFER_SIZE);

	while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SIZE, 0);
		
		if (receive > 0)
		{
			if(strlen(buff_out) > 0)
			{
				send_message(buff_out, cli->uid);
				str_trim_lf(buff_out, strlen(buff_out));
				printf("%s -> %s\n", buff_out, cli->name);
			}
		} else if (receive == 0 || strcmp(buff_out, "exit") == 0){
			sprintf(buff_out, "%s has left\n", cli->name);
			printf("%s", buff_out);
			send_message(buff_out, cli->uid);
			leave_flag = 1;
		} else {
			printf("ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SIZE);
	}

    /* Delete client from queue and yield thread */
	close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

	return NULL;
}


int main(int argc, char **argv){


	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;

    /* Socket settings */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    /* Ignore pipe signals */
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
	}

	/* Bind */
    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      perror("ERROR: Socket binding failed");
      return EXIT_FAILURE;
    }

    /* Listen */
    if (listen(listenfd, 10) < 0) {
      perror("ERROR: Socket listening failed");
      return EXIT_FAILURE;
	}

	printf("=== CHATROOM SERVER ONLINE===\n");
	printf("LOGS:\n");

	while(1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		/* Check if max clients is reached */
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		/* Client settings */
		client *cli = (client *)malloc(sizeof(client));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Add client to the queue and fork thread */
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		/* Reduce CPU usage */
		sleep(1);
	}

	return EXIT_SUCCESS;
}