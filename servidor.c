/*
 * Servidor HTTP Simples em C
 * 
 * Copyright (C) 2025  Giovanna Paz
 *
 * Este programa é software livre: você pode redistribuí-lo e/ou modificá-lo
 * sob os termos da Licença Pública Geral GNU, conforme publicada pela
 * Free Software Foundation, na versão 3 da Licença, ou (a seu critério)
 * qualquer versão posterior.
 *
 * Este programa é distribuído na esperança de que seja útil,
 * mas SEM NENHUMA GARANTIA; sem mesmo a garantia implícita de
 * COMERCIALIZAÇÃO ou de ADEQUAÇÃO A UM DETERMINADO PROPÓSITO.
 * Consulte a Licença Pública Geral GNU para mais detalhes.
 *
 * Você deve ter recebido uma cópia da Licença Pública Geral GNU
 * junto com este programa. Caso contrário, veja <https://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <dirent.h>

#define PORTA 7070
#define BACKLOG 5           
#define DIRETORIO "./paginasServidor"   


void detectaTipoConteudo(const char *caminho, const char **tipo){
    if(strstr(caminho, ".html")) *tipo = "text/html";
    else if(strstr(caminho, ".png")) *tipo = "image/png";
    else if(strstr(caminho, ".jpg") || strstr(caminho, ".jpeg")) *tipo = "image/jpeg";
    else if(strstr(caminho, ".gif")) *tipo = "image/gif";
    else if(strstr(caminho, ".txt")) *tipo = "text/plain";
    else if(strstr(caminho, ".pdf")) *tipo = "application/pdf";
    else if(strstr(caminho, ".zip")) *tipo = "application/zip";
    else *tipo = "application/octet-stream";  //tipo padrão binário
}

int main(){

    char diretorio[] = DIRETORIO;
    printf("Servidor iniciando...\n");

    int sock;
    struct sockaddr_in end_servidor;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Erro ao criar o socket.\n");
        return EXIT_FAILURE;
    }

    //configurando endereço do servidor
    end_servidor.sin_family = AF_INET;
    end_servidor.sin_port = htons(PORTA);
    end_servidor.sin_addr.s_addr = INADDR_ANY;      //conexões de qualquer IP

    //bind - liga o socket ao endereço e porta
    if(bind(sock, (struct sockaddr*)&end_servidor, sizeof(end_servidor)) <0){
        printf("Erro no bind.\n");
        close(sock);
        return EXIT_FAILURE;
    }

    //colocando o servidor para ouvir
    if(listen(sock, BACKLOG) < 0){
        printf("Erro no listen.\n");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Servidor iniciado na porta %d. Aguardando conexões...\n", PORTA);

    //criando socket do clietne
    struct sockaddr_in cliente;
    socklen_t tam_cliente = sizeof(cliente);

    while(1){
        printf("\nAguardando nova conexão...\n");

        //retorna um novo socket ao aceitar a conexão
        int sock_cliente = accept(sock, (struct sockaddr*)&cliente, &tam_cliente);
        if(sock_cliente < 0){
            printf("Conexão não aceita.\n");
            continue;
        }

        //ip e porta do cliente
        printf("Conexão aceita de %s:%d\n", inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port));


        //lendo a requisição
        char buffer[4096];
        int bytes_recebidos = recv(sock_cliente, buffer, sizeof(buffer)-1, 0);
        
        if(bytes_recebidos < 0){
            printf("Erro ao ler a requisicao.\n");
            close(sock_cliente);
            continue; //return quando for para a função
        }

        buffer[bytes_recebidos] = '\0'; //'\0' para finalizar a string

        printf("Requisição recebida:\n%s\n", buffer);

        //processando a requisição
        char metodo[8], caminho[256], protocolo[16];

        if(sscanf(buffer, "%s %s %s", metodo, caminho, protocolo) != 3){
            printf("Requisição inválida.\n");
            close(sock_cliente);
            continue; 
        }

        printf("Método: %s\nCaminho: %s\nProtocolo: %s\n", metodo, caminho, protocolo);

        //arquivo a ser servido
        char arquivo[512];

        if(strcmp(caminho, "/") == 0){ //se for raiz
            snprintf(arquivo, sizeof(arquivo), "%s/index.html", diretorio);

            if (access(arquivo, F_OK) != 0) {
                //não tem index.html, gera listagem
                DIR *dir = opendir(diretorio);
                if (!dir) {
                    perror("Erro ao abrir diretório");
                    close(sock_cliente);
                    continue;
                }

                struct dirent *ent;
                char html[8192];
                snprintf(html, sizeof(html),
                    "<html><body><h2>Listagem de arquivos</h2><ul>\n");

                while ((ent = readdir(dir)) != NULL) {
                    //ignora "." e ".."
                    if (strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
                        char linha[512];
                        snprintf(linha, sizeof(linha),
                            "<li><a href=\"/%s\">%s</a></li>\n", ent->d_name, ent->d_name);
                        strncat(html, linha, sizeof(html) - strlen(html) - 1);
                    }
                }

                closedir(dir);
                strncat(html, "</ul></body></html>", sizeof(html) - strlen(html) - 1);

                char cabecalho[256];
                snprintf(cabecalho, sizeof(cabecalho),
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: %zu\r\n"
                    "Connection: close\r\n\r\n",
                    strlen(html));

                send(sock_cliente, cabecalho, strlen(cabecalho), 0);
                send(sock_cliente, html, strlen(html), 0);

                close(sock_cliente);
                continue; //encerra a conexão e aguarda nova
            }
        }
        else{
            snprintf(arquivo, sizeof(arquivo), "%s%s", diretorio, caminho);
        }

        //montando e enviando resposta HTTP
        if(access(arquivo, F_OK) == 0){  //o arquivo existe
            FILE *arq = fopen(arquivo, "rb");

            fseek(arq, 0, SEEK_END);        //vai para o final do arquivo
            long tamArq = ftell(arq);       //retorna a posição atual de onde esta o ponteiro p/ o arquivo
            fseek(arq, 0, SEEK_SET);

            char cabecalho[512];

            const char *tipo;
            detectaTipoConteudo(arquivo, &tipo);
            printf("Buscando arquivo: %s com tipo %s\n", arquivo, tipo);
            int tamCab = snprintf(cabecalho, sizeof(cabecalho),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %ld\r\n"
                "Connection: close\r\n"
                "\r\n",
                tipo, tamArq
            );

            send(sock_cliente, cabecalho, tamCab, 0);

            char bufferArq[4096];
            size_t bytesLidos;

            //fread -> le de um arquivo para um buffer
            while((bytesLidos = fread(bufferArq, 1, sizeof(bufferArq), arq)) > 0){
                send(sock_cliente, bufferArq, bytesLidos, 0);
            }

            fclose(arq);
        }
        else{
            char corpo[] = "<html><body><h1>404 Not Found</h1></body></html>";
            char resposta[256];
            sprintf(resposta,
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n"
                "Content-Type: text/html\r\n"
                "\r\n",
                strlen(corpo));

            send(sock_cliente, resposta, strlen(resposta), 0);
            send(sock_cliente, corpo, strlen(corpo), 0);
        }

        close(sock_cliente);
    }

    close(sock);
    return EXIT_SUCCESS;
}