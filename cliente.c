/*
 * Cliente HTTP Simples em C
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
#include <sys/socket.h>     //criar sockets
#include <netinet/in.h>     //conectar ao host
#include <arpa/inet.h>     
#include <netdb.h>          //host para endereço IP
#include <unistd.h>
#include <sys/types.h>


typedef struct{
    char *protocolo;
    char *host;
    char *porta;
    char *caminho;
    char *extArquivo;
}URL;

URL extrairUrl(char *link){
    URL url;
    char *p;

    //extrai protocolo e host
    p = strstr(link, "://"); //encontra a substring "://" e retorna o ponteiro para o inicio dela
    if(p != NULL){
        *p = '\0';                  //substitui ':' por '\0' para separar o protocolo ('/0' indica o fim de uma string)
        url.protocolo = link;
        url.host = p + 3;
    }
    else{
        printf("Requisição inválida: protocolo não encontrado.\n");
        exit(EXIT_FAILURE);
    }

    //extrai caminho
    p = strchr(url.host, '/'); //encontra o caractere '/' na string e retorna o ponteiro para o inicio dela
    if(p != NULL){
        *p = '\0';
        url.caminho = p + 1;
    }
    else{
        url.caminho = "";      //caminho não especificado
    }

    //extrai porta
    url.porta = strchr(url.host, ':');
    if(url.porta != NULL){
        *url.porta = '\0';
        url.porta++;
    }
    else{
        url.porta = "80";
    }

    //extrai extensão do arquivo
    url.extArquivo = strrchr(url.caminho, '.'); //encontra a ultima ocorrencia
    if(url.extArquivo != NULL){
        url.extArquivo++;
    }

    return url;
}

void imprimirUrl(URL url){
    printf("Protocolo: %s\n", url.protocolo);
    printf("Host: %s\n", url.host);
    printf("Porta: %s\n", url.porta);
    printf("Caminho: %s\n", url.caminho);
    printf("Extensão do arquivo: %s\n", url.extArquivo);
}

int main(int argc, char *argv[]){

    if(argc != 2){
        fprintf(stderr, "\nUse: %s <solicitação>\n\n", argv[0]);
        return EXIT_FAILURE;
    }

    printf("\n\t\t========== Navegador Mandioca iniciado. ==========\n\n");
    
    printf("Solicitação: %s\n", argv[1]);

    printf("\n\t\t========== Extraindo URL da solicitação... ==========\n\n");
    URL url = extrairUrl(argv[1]);
    imprimirUrl(url);

    printf("\n\t\t========== Traduzindo para IP... ==========\n\n");
    struct hostent *he;
    struct in_addr **lista_enderecos;

    if((he = gethostbyname(url.host)) == NULL){
        printf("Erro ao traduzir o host para IP.\n");
        return EXIT_FAILURE;
    }

    lista_enderecos =  (struct in_addr **)he->h_addr_list;
    printf("Endereço IP do host: %s\n", inet_ntoa(*lista_enderecos[0]));

    printf("\n\t\t========== Criando socket... ==========\n\n");
    int sock;
    struct sockaddr_in end_servidor;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("Erro ao criar o socket.\n");
        return EXIT_FAILURE;
    }

    //configurando dados do servidor
    end_servidor.sin_family = AF_INET;
    end_servidor.sin_port = htons(atoi(url.porta));
    end_servidor.sin_addr = *lista_enderecos[0];

    //conectando
    if(connect(sock, (struct sockaddr*)&end_servidor, sizeof(end_servidor)) < 0){
        printf("Erro ao conectar ao servidor.\n");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Concectado a %s:%s\n", url.host, url.porta);

    printf("\n\t\t========== Enviando requisição HTTP... ==========\n\n");
    char requisicao[1024];

    //montando requisição HTTP
    snprintf(requisicao, 1024,
        "GET /%s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: MandiocaBrowser/1.0\r\n"
        "\r\n",
        url.caminho, url.host
    );

    send(sock, requisicao, strlen(requisicao), 0);
    printf("Requisição enviada:\n%s", requisicao);

    printf("\n\t\t========== Recebendo resposta... ==========\n\n");
    
    char saida[256];
    if(url.extArquivo == NULL || strlen(url.extArquivo) == 0){
        snprintf(saida, 256, "index.html");
        //url.extArquivo = "saida.html";
    }
    else{
        char *nome = strrchr(url.caminho, '/');
        if(nome != NULL && strlen(nome) > 1){
            snprintf(saida, 256, "s%s.%s", nome+1, url.extArquivo);
        }
        else{
            nome = strrchr(url.caminho, '.');
            if(nome != NULL){
                *nome = '\0';
            }
            snprintf(saida, 256, "%s.%s", url.caminho, url.extArquivo);
        }
        printf("Salvando arquivo com extensão: %s\n", url.extArquivo);
        //snprintf(saida, 256, "saida.%s", url.extArquivo);
        printf("nome do arquivo de saida: %s\n", saida);
    }
    FILE *arq = fopen(saida, "wb");
    if(!arq){
        perror("Erro ao criar arquivo de saída");
        close(sock);
        return EXIT_FAILURE;
    }

    char buffer[4096];
    int bytes_recebidos;
    int cabecalho = 0;

    while((bytes_recebidos = recv(sock, buffer, sizeof(buffer), 0)) > 0){
        if(!cabecalho){
            //Verifica erro 404
            if (strstr(buffer, "404 Not Found") != NULL) {
                printf("\nErro 404: Página não encontrada.\n");
                fclose(arq);
                close(sock);
                remove(saida); //apaga o arquivo vazio
                return EXIT_FAILURE;
            }
            char *p = strstr(buffer, "\r\n\r\n");
            if(p != NULL){
                cabecalho = 1; 
                p += 4;
                fwrite(p, 1, bytes_recebidos-(p-buffer), arq);
            }
        }
        else{
            fwrite(buffer, 1, bytes_recebidos, arq);
        }
    }

    fclose(arq);
    close(sock);
    printf("Resposta recebida e salva em '%s'.\n", saida);
    printf("\n\t\t========== Navegador Mandioca finalizado. ==========\n\n");

    return EXIT_SUCCESS;
}