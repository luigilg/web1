#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define PROTOPORT 5193   /* número padrão da porta do protocolo */
#define QLEN      6      /* tamanho da fila de requisições */

void *tratar_conexao(void *sd2);
void enviar_arquivo(int sd, const char *caminho_arquivo, const char *tipo_conteudo);
char *obter_tipo_conteudo(const char *caminho_arquivo);

int visitas = 0;          /* conta conexões de clientes */

void *funcao_thread(void *lpParam) {
    tratar_conexao(lpParam);
    return NULL;
}

int main(int argc, char **argv) {
    struct protoent *ptrp;   /* ponteiro para uma entrada na tabela de protocolos */
    struct sockaddr_in sad;  /* estrutura para guardar o endereço do servidor */
    struct sockaddr_in cad;  /* estrutura para guardar o endereço do cliente */
    int sd, sd2;             /* descritores de socket */
    int porta;               /* número da porta do protocolo */
    int alen;                /* comprimento do endereço */
    pthread_t thread;

    memset((char *)&sad, 0, sizeof(sad)); /* limpa a estrutura sockaddr */
    sad.sin_family = AF_INET;             /* define a família como Internet */
    sad.sin_addr.s_addr = INADDR_ANY;     /* define o endereço IP local */

    /* Verifica o argumento da linha de comando para a porta do protocolo e extrai */
    /* o número da porta se um for especificado. Caso contrário, usa o valor padrão */
    /* da porta dado pela constante PROTOPORT */

    if (argc > 1) {                 /* se argumento especificado */
        porta = atoi(argv[1]);      /* converte argumento para binário */
    } else {
        porta = PROTOPORT;          /* usa o número padrão da porta */
    }
    if (porta > 0)                  /* testa valor ilegal */
        sad.sin_port = htons((u_short)porta);
    else {                          /* imprime mensagem de erro e sai */
        fprintf(stderr, "número da porta inválido %s\n", argv[1]);
        exit(1);
    }

    /* Mapeia o nome do protocolo de transporte TCP para o número do protocolo */
    ptrp = getprotobyname("tcp");
    if (ptrp == NULL) {
        fprintf(stderr, "não é possível mapear \"tcp\" para o número do protocolo");
        exit(1);
    }

    /* Cria um socket */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "falha na criação do socket\n");
        exit(1);
    }

    /* Associa um endereço local ao socket */
    if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        fprintf(stderr, "falha na associação (bind)\n");
        close(sd);
        exit(1);
    }

    /* Especifica o tamanho da fila de requisições */
    if (listen(sd, QLEN) < 0) {
        fprintf(stderr, "falha no escutar (listen)\n");
        close(sd);
        exit(1);
    }

    /* Loop principal do servidor - aceita e trata requisições */
    while (1) {
        alen = sizeof(cad);
        if ((sd2 = accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
            fprintf(stderr, "falha na aceitação (accept)\n");
            close(sd);
            exit(1);
        }
        printf("\nServidor atendendo conexão %d\n", visitas);
        fflush(stdout);
        pthread_create(&thread, NULL, funcao_thread, &sd2);
    }
}

void *tratar_conexao(void *sd2) {
    int sd = *(int *)sd2;
    char buffer[1024];
    int n;

    visitas++;

    memset(buffer, 0, sizeof(buffer));
    n = recv(sd, buffer, sizeof(buffer) - 1, 0);
    if (n < 0) {
        perror("ERRO ao ler do socket");
        close(sd);
        return NULL;
    }

    printf("Aqui está a mensagem:\n%s\n", buffer);

    if (strncmp(buffer, "GET ", 4) == 0) {
        char *caminho_arquivo = strtok(buffer + 4, " ");
        if (strcmp(caminho_arquivo, "/") == 0) {
            caminho_arquivo = "/index.html";
        }
        char *tipo_conteudo = obter_tipo_conteudo(caminho_arquivo);
        enviar_arquivo(sd, caminho_arquivo + 1, tipo_conteudo);
    }

    close(sd);
    return NULL;
}

char *obter_tipo_conteudo(const char *caminho_arquivo) {
    char *ext = strrchr(caminho_arquivo, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    // Adicione mais tipos conforme necessário
    return "application/octet-stream";
}

void enviar_arquivo(int sd, const char *caminho_arquivo, const char *tipo_conteudo) {
    FILE *arquivo;
    char buffer[1024];
    size_t bytes_lidos;

    arquivo = fopen(caminho_arquivo, "rb");
    if (!arquivo) {
        perror("ERRO ao abrir o arquivo");
        char *nao_encontrado = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nArquivo não encontrado.";
        send(sd, nao_encontrado, strlen(nao_encontrado), 0);
        return;
    }

    fseek(arquivo, 0, SEEK_END);
    long tamanho_arquivo = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", tipo_conteudo, tamanho_arquivo);
    send(sd, buffer, strlen(buffer), 0);

    while ((bytes_lidos = fread(buffer, 1, sizeof(buffer), arquivo)) > 0) {
        send(sd, buffer, bytes_lidos, 0);
    }

    fclose(arquivo);
}
