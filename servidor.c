#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    char name[50];
    int limit;
    int count;
    int participants[MAX_CLIENTS];
} ChatRoom;

void createRoom(ChatRoom *rooms, int *roomCount) {
    if (*roomCount >= MAX_CLIENTS) {
        printf("Limite máximo de salas atingido.\n");
        return;
    }
    
    ChatRoom newRoom;
    printf("Digite o nome da sala: ");
    fgets(newRoom.name, sizeof(newRoom.name), stdin);
    newRoom.name[strcspn(newRoom.name, "\n")] = '\0';
    
    printf("Digite o limite de participantes: ");
    scanf("%d", &newRoom.limit);
    
    newRoom.count = 0;
    
    rooms[*roomCount] = newRoom;
    (*roomCount)++;
    
    printf("Sala criada com sucesso.\n");
}

void listParticipants(ChatRoom *rooms, int roomCount, int roomId) {
    if (roomId < 0 || roomId >= roomCount) {
        printf("Sala inválida.\n");
        return;
    }
    
    ChatRoom room = rooms[roomId];
    
    printf("Participantes da sala %s:\n", room.name);
    for (int i = 0; i < room.count; i++) {
        printf("Participante %d: %d\n", i + 1, room.participants[i]);
    }
}

void joinRoom(ChatRoom *rooms, int roomCount, int roomId, int clientId) {
    if (roomId < 0 || roomId >= roomCount) {
        printf("Sala inválida.\n");
        return;
    }
    
    ChatRoom *room = &rooms[roomId];
    
    if (room->count >= room->limit) {
        printf("Limite de participantes na sala atingido.\n");
        return;
    }
    
    room->participants[room->count] = clientId;
    room->count++;
    
    printf("Cliente %d ingressou na sala %s.\n", clientId, room->name);
}

void leaveRoom(ChatRoom *rooms, int roomCount, int roomId, int clientId) {
    if (roomId < 0 || roomId >= roomCount) {
        printf("Sala inválida.\n");
        return;
    }
    
    ChatRoom *room = &rooms[roomId];
    
    int participantIndex = -1;
    for (int i = 0; i < room->count; i++) {
        if (room->participants[i] == clientId) {
            participantIndex = i;
            break;
        }
    }
    
    if (participantIndex == -1) {
        printf("Cliente %d não está na sala %s.\n", clientId, room->name);
        return;
    }
    
    for (int i = participantIndex; i < room->count - 1; i++) {
        room->participants[i] = room->participants[i + 1];
    }
    
    room->count--;
    
    printf("Cliente %d saiu da sala %s.\n", clientId, room->name);
}

void handleClientMessage(int clientSocket, char *buffer, int bufferLength, ChatRoom *rooms, int roomCount) {
    printf("Mensagem recebida do cliente %d: %s", clientSocket, buffer);
    
    // Encontra a sala em que o cliente está
    int clientRoomIndex = -1;
    for (int i = 0; i < roomCount; i++) {
        ChatRoom *room = &rooms[i];
        for (int j = 0; j < room->count; j++) {
            if (room->participants[j] == clientSocket) {
                clientRoomIndex = i;
                break;
            }
        }
        if (clientRoomIndex != -1) {
            break;
        }
    }
    
    if (clientRoomIndex == -1) {
        printf("Cliente %d não está em nenhuma sala.\n", clientSocket);
        return;
    }
    
    // Encaminha a mensagem para os outros clientes na mesma sala
    ChatRoom *room = &rooms[clientRoomIndex];
    for (int i = 0; i < room->count; i++) {
        int participantSocket = room->participants[i];
        if (participantSocket != clientSocket) {
            if (send(participantSocket, buffer, bufferLength, 0) < 0) {
                perror("Erro ao enviar mensagem para outro cliente");
                exit(EXIT_FAILURE);
            }
        }
    }
}

int main() {
    int serverSocket;
    struct sockaddr_in serverAddress;
    int clientSockets[MAX_CLIENTS];
    fd_set readfds;
    int maxSocket;
    int activity;
    char buffer[BUFFER_SIZE];
    
    ChatRoom rooms[MAX_CLIENTS];
    int roomCount = 0;
    
    // Criação do socket do servidor
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Erro ao criar socket do servidor");
        exit(EXIT_FAILURE);
    }
    
    // Configuração do endereço do servidor
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(8888);
    
    // Vinculação do socket do servidor ao endereço
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Erro ao vincular o socket do servidor");
        exit(EXIT_FAILURE);
    }
    
    // Aguarda conexões de clientes
    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        perror("Erro ao aguardar conexões de clientes");
        exit(EXIT_FAILURE);
    }
    
    // Inicializa os sockets dos clientes como 0
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clientSockets[i] = 0;
    }
    
    // Loop principal
    while (1) {
        // Limpa o conjunto de descritores
        FD_ZERO(&readfds);
        
        // Adiciona o socket do servidor ao conjunto
        FD_SET(serverSocket, &readfds);
        maxSocket = serverSocket;
        
        // Adiciona os sockets dos clientes ao conjunto
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int clientSocket = clientSockets[i];
            
            if (clientSocket > 0) {
                FD_SET(clientSocket, &readfds);
            }
            
            if (clientSocket > maxSocket) {
                maxSocket = clientSocket;
            }
        }
        
        // Aguarda atividade em algum socket
        activity = select(maxSocket + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("Erro ao aguardar atividade nos sockets");
            exit(EXIT_FAILURE);
        }
        
        // Verifica se há atividade no socket do servidor
        if (FD_ISSET(serverSocket, &readfds)) {
            int newSocket;
            
            // Aceita uma nova conexão
            if ((newSocket = accept(serverSocket, NULL, NULL)) < 0) {
                perror("Erro ao aceitar conexão de cliente");
                exit(EXIT_FAILURE);
            }
            
            // Adiciona o novo socket ao array de sockets dos clientes
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = newSocket;
                    break;
                }
            }
            
            printf("Novo cliente conectado: %d\n", newSocket);
        }
        
        //Verifica se há alguma atividade no socket dos clientes
        for (int i = 0; i < MAX_CLIENTS; i++) {
        int clientSocket = clientSockets[i];
        
        if (FD_ISSET(clientSocket, &readfds)) {
            // Verifica se o cliente se desconectou
            if (read(clientSocket, buffer, sizeof(buffer)) == 0) {
                // Cliente desconectado
                close(clientSocket);
                clientSockets[i] = 0;
                printf("Cliente desconectado: %d\n", clientSocket);
            } else {
                // Manipula a mensagem recebida do cliente
                handleClientMessage(clientSocket, buffer, strlen(buffer), rooms, roomCount);
            }
        }
    }
    }
    
    return 0;
}
