#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CHATROOMS 10
#define MAX_PARTICIPANTS 10

// Definição da struct chatroom
typedef struct {
    char name[50];
    int num_participants;
    char participants[MAX_PARTICIPANTS][50];
} chatroom;

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
void list_chatrooms(chatroom* chatrooms[], int num_chatrooms) {
    if (num_chatrooms == 0) {
        printf("No chatrooms found.\n");
    } else {
        printf("List of chatrooms:\n");
        for (int i = 0; i < num_chatrooms; i++) {
            if (chatrooms[i] != NULL) {
                printf("%d. %s (%d participants)\n", i + 1, chatrooms[i]->name, chatrooms[i]->num_participants);
            }
        }
    }
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

int main() {
    chatroom* chatrooms[MAX_CHATROOMS] = { NULL };
    int num_chatrooms = 0;

    char command[50];
    while (1) {
        printf("\nEnter command (create/add/delete/list/exit): ");
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0'; // Remover o caractere de nova linha

        if (strcmp(command, "exit") == 0) 
        {
            break;
        } else if (strcmp(command, "create") == 0) 
        {
            if (num_chatrooms < MAX_CHATROOMS) {
                char room_name[50];
                printf("Enter chatroom name: ");
                fgets(room_name, sizeof(room_name), stdin);
                room_name[strcspn(room_name, "\n")] = '\0'; // Remover o caractere de nova linha

                // Verificar se a chatroom com o mesmo nome já existe
                int found = 0;
                for (int i = 0; i < num_chatrooms; i++) {
                    if (chatrooms[i] != NULL && strcmp(chatrooms[i]->name, room_name) == 0) {
                        found = 1;
                        printf("Chatroom '%s' already exists.\n", room_name);
                        break;
                    }
                }

                if (!found) {
                    chatroom* new_room = create_chatroom(room_name);
                    if (new_room != NULL) {
                        chatrooms[num_chatrooms] = new_room;
                        num_chatrooms++;
                    }
                }
            } else {
                printf("Maximum number of chatrooms reached.\n");
            }
        } else if (strcmp(command, "add") == 0) {
            if (num_chatrooms == 0) {
                printf("No chatrooms found.\n");
            } else {
                char room_name[50];
                printf("Enter chatroom name: ");
                fgets(room_name, sizeof(room_name), stdin);
                room_name[strcspn(room_name, "\n")] = '\0'; // Remover o caractere de nova linha

                // Procurar a chatroom com o nome fornecido
                int found = 0;
                for (int i = 0; i < num_chatrooms; i++) {
                    if (chatrooms[i] != NULL && strcmp(chatrooms[i]->name, room_name) == 0) {
                        found = 1;
                        char participant_name[50];
                        printf("Enter participant name: ");
                        fgets(participant_name, sizeof(participant_name), stdin);
                        participant_name[strcspn(participant_name, "\n")] = '\0'; // Remover o caractere de nova linha

                        add_participant(chatrooms[i], participant_name);
                        break;
                    }
                }

                if (!found) {
                    printf("Chatroom '%s' not found.\n", room_name);
                }
            }
        } else if (strcmp(command, "delete") == 0) {
            if (num_chatrooms == 0) {
                printf("No chatrooms found.\n");
            } else {
                char room_name[50];
                printf("Enter chatroom name: ");
                fgets(room_name, sizeof(room_name), stdin);
                room_name[strcspn(room_name, "\n")] = '\0'; // Remover o caractere de nova linha

                // Procurar a chatroom com o nome fornecido
                int found = 0;
                for (int i = 0; i < num_chatrooms; i++) {
                    if (chatrooms[i] != NULL && strcmp(chatrooms[i]->name, room_name) == 0) {
                        found = 1;
                        delete_chatroom(chatrooms[i]);
                        chatrooms[i] = NULL;
                        break;
                    }
                }

                if (!found) {
                    printf("Chatroom '%s' not found.\n", room_name);
                }
            }
        } else if (strcmp(command, "list") == 0) 
        {
            list_chatrooms(chatrooms, num_chatrooms);
        } else {
            printf("Invalid command.\n");
        }
    }

    // Liberar a memória das chatrooms criadas
    for (int i = 0; i < num_chatrooms; i++) {
        if (chatrooms[i] != NULL) {
            delete_chatroom(chatrooms[i]);
        }
    }

    return 0;
}
