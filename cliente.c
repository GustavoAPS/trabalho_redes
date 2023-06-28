#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int clientSocket;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE];
    
    // Criação do socket do cliente
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Erro ao criar socket do cliente");
        exit(EXIT_FAILURE);
    }
    
    // Configuração do endereço do servidor
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddress.sin_addr)) <= 0) {
        perror("Endereço de servidor inválido");
        exit(EXIT_FAILURE);
    }
    
    // Conexão com o servidor
    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Erro ao conectar-se ao servidor");
        exit(EXIT_FAILURE);
    }
    
    printf("Conectado ao servidor.\n");
    
    while (1) {
        // Leitura da mensagem do usuário
        printf("Digite a mensagem: ");
        fgets(buffer, sizeof(buffer), stdin);
        
        // Envio da mensagem para o servidor
        if (send(clientSocket, buffer, strlen(buffer), 0) < 0) {
            perror("Erro ao enviar mensagem para o servidor");
            exit(EXIT_FAILURE);
        }
        
        // Limpa o buffer
        memset(buffer, 0, sizeof(buffer));
        
        // Recebimento da resposta do servidor
        if (recv(clientSocket, buffer, sizeof(buffer), 0) < 0) {
            perror("Erro ao receber resposta do servidor");
            exit(EXIT_FAILURE);
        }
        
        printf("Resposta do servidor: %s\n", buffer);
        
        // Verifica se o cliente quer sair
        if (strncmp(buffer, "sair", 4) == 0) {
            break;
        }
    }
    
    // Fechamento do socket do cliente
    close(clientSocket);
    
    return 0;
}
