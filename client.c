/*
 * TCP echo client
 * O cliente envia uma mensagem e o servidor responde com a mesma mensagem (eco).
 * Baseado no codigo de:
 * http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/
 */

#include <stdio.h>      /* para printf() e fprintf() */
#include <sys/socket.h> /* para socket(), connect(), send(), e recv() */
#include <arpa/inet.h>  /* para sockaddr_in and inet_addr() */
#include <stdlib.h>     /* para atoi() e exit() */
#include <string.h>     /* para memset() */
#include <unistd.h>     /* para close() */
#include <pthread.h>

#define BUFSIZE 32   /* Tamanho do buffer de recebimento */
#define NUMTHREADS 5    // Amount of threads

#define NICK_SIZE 32
#define CHANNEL_NAME_SIZE 32
#define COMMAND_SIZE 32
#define LOGIN_SIZE 32

void *ListenServer(void *clntSock_void);

pthread_t threads[NUMTHREADS];

int main(int argc, char *argv[])
{
    int             sock;                       /* Descritor de socket */
    struct          sockaddr_in echoServAddr;   /* Struct para especificar o endereco do servidor */
    unsigned short  echoServPort;               /* Numero da porta do servidor */
    char            *servIP;                    /* Endereco IP do servidor */
    //unsigned int    msgLen;                     /* Tamanho da string */
    char            *msg;
    int             notQuit = 1;    // Flag that keeps communication with user
    char            waitCommand[COMMAND_SIZE];
    char            login[LOGIN_SIZE];      // Username para realizar login
    char            command[COMMAND_SIZE];   // Commando enviado pelo usuario
    char            nick[NICK_SIZE];      // Novo nickname do usuario
    char            channelName[CHANNEL_NAME_SIZE];      // Novo nickname do usuario
    char            msgToUsers[BUFSIZE];
    char            subMSG[BUFSIZE];
    //int             wordCounter = 0;

    //Pthreads
    int pthreadsIndex = 0;
    int rc;

    //Strings
    /*login = malloc(sizeof(char));
    command = malloc(sizeof(char));
    nick = malloc(sizeof(char));
    channelName = malloc(sizeof(char));
    waitCommand = malloc(sizeof(char));
    msg = malloc(sizeof(char));*/
    strcpy(waitCommand, "");



    if (argc < 4)    /* Testa se o numero de argumentos esta correto */
    {
       fprintf(stderr, "Usage: %s <server IP> <port> <message>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* Primeiro argumetno: endereco IP do servidor */
    echoServPort = atoi(argv[2]); /* Segundo argumento: porta do servidor */
    msg = argv[3];         /* Terceiro argumetno: string com a mensagem a ser enviada ao servidor */
    //printf("ARG: %s\n", msg);

    /* Cria um socket TCP */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }

    /* Constroi struct de endereco do servidor */
    memset(&echoServAddr, 0, sizeof(echoServAddr));     /* Preenche struct com zeros */
    echoServAddr.sin_family      = AF_INET;             /* Família de enderecos de internet (IP) */
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);   /* Endereco IP do servidor */
    echoServAddr.sin_port        = htons(echoServPort); /* Porta do servidor */

    /* Estabelece uma conexao com o servidor */
    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        perror("connect");
        exit(1);
    }

    rc = pthread_create(&threads[pthreadsIndex], NULL, ListenServer, (void *)&sock);
    pthreadsIndex = pthreadsIndex +1;

    memset(command, '\0', BUFSIZE);
    memset(login, '\0', BUFSIZE);
    memset(nick, '\0', BUFSIZE);
    memset(channelName, '\0', BUFSIZE);
    memset(msgToUsers, '\0', BUFSIZE);
    memset(subMSG, '\0', BUFSIZE);

    /*for(i = 0; i < strlen(msg) -1; i++){

        if((msg[i] == ' ') && (msg[i+1] != '')){

            wordCounter++;

        }        
    }*/

    while( notQuit == 1 ){

        //printf("MSG recebido. Antes de extrair para 'command': %s\n", msg);
        strcpy(command, strtok( msg, " ,.-" ));            //Extrai o comando inserido pelo usuario
        //printf("Command identificado: %s\n", command);

        if (strcmp("/quit", command) == 0){

            notQuit = 0;
            sprintf (msg, "QUIT");

        }else if(strcmp("/login", command) == 0){

            strcpy(login, strtok( NULL, " ,.-" ));          //Extrai novo nick inserido pelo usuario */
            sprintf (msg, "LOGIN %s", login);         //Prepara comando para o servidor
            memset(login, '\0', BUFSIZE);

        }else if(strcmp("/nick", command) == 0){

            strcpy(nick, strtok( NULL, " ,.-" ));          //Extrai novo nick inserido pelo usuario */
            sprintf (msg, "NICK %s", nick);         //Prepara comando para o servidor
            memset(nick, '\0', BUFSIZE);


        }else if(strcmp("/create", command) == 0){

            strcpy(channelName, strtok( NULL, " ,.-" ));          //Extrai novo nome do canal inserido pelo usuario */
            sprintf (msg, "CREATE %s", channelName);         //Prepara comando para o servidor
            memset(channelName, '\0', BUFSIZE);

        }else if(strcmp("/remove", command) == 0){

            strcpy(channelName, strtok( NULL, " ,.-" ));      //Extrai nome do canal a ser deletado
            sprintf (msg, "REMOVE %s", channelName);         //Prepara comando para o servidor
            memset(channelName, '\0', BUFSIZE);

        }else if(strcmp("/list", command) == 0){

            sprintf (msg, "LIST");         //Prepara comando para o servidor

        }else if(strcmp("/join", command) == 0){

            strcpy(channelName, strtok( NULL, " ,.-" ));          //Extrai novo nome do canal inserido pelo usuario */
            sprintf (msg, "JOIN %s", channelName);         //Prepara comando para o servidor
            memset(channelName, '\0', BUFSIZE);

        }else if(strcmp("/part", command) == 0){

            //strcpy(channelName, strtok( NULL, " ,.-" ));          //Extrai nome do canal que o usuario quer sair */
            //sprintf (msg, "PART %s", channelName);         //Prepara comando para o servidor
            sprintf (msg, "PART");         //Prepara comando para o servidor
            //memset(channelName, '\0', BUFSIZE);

        }else if(strcmp("/names", command) == 0){

            //strcpy(channelName, strtok( NULL, " ,.-" ));          //Extrai nome do canal que o usuario quer ver os users */
            //sprintf (msg, "NAMES %s", channelName);         //Prepara comando para o servidor
            sprintf (msg, "NAMES");         //Prepara comando para o servidor
            //memset(channelName, '\0', BUFSIZE);

        }else if(strcmp("/kick", command) == 0){

            strcpy(channelName, strtok( NULL, " ,.-" ));          //Extrai nome do canal no qual o usuario quer kikar alguem
            strcpy(nick, strtok( NULL, " ,.-" ));          //Extrai nome do user a ser kikado do sinal
            sprintf (msg, "KICK %s %s", channelName, nick);         //Prepara comando para o servidor
            memset(channelName, '\0', BUFSIZE);
            memset(nick, '\0', BUFSIZE);

        }else if(strcmp("/msg", command) == 0){

            strcpy(nick, strtok( NULL, " ,.-" ));          //Extrai nick do user que vai receber a mensagem
            strcpy(msgToUsers, strtok( NULL, "&" ));          //Extrai msg a ser enviada
            sprintf(msg, "PRIVMSG %s %s %s", "channel", nick, msgToUsers);         //Prepara comando para o servidor
            memset(nick, '\0', BUFSIZE);
            memset(msgToUsers, '\0', BUFSIZE);

        }else{//MSG

            //printf("%s%s\n", "Identificou message for all, vai mandar: ", msg);
            strcpy(subMSG, msg);
            //printf("%s%s\n", "subMSG: ", subMSG);
            sprintf(msg, "MSG %s %s", "channel", subMSG);         //Prepara comando para o servidor
            //printf("%s%s\n", "MSG: ", msg);
            memset(subMSG, '\0', BUFSIZE);

        }

        memset(command, '\0', BUFSIZE);

        //msgLen = strlen(msg) + 1;   /* Determina o comprimento da string de entrada, incluindo o '\0' */

        /* Envia uma string para o servidor */
        send(sock, msg, strlen(msg) +1, 0);
        memset(msg, '\0', BUFSIZE);
        /*if (send(sock, msg, BUFSIZE, 0) != msgLen) {
            perror("send");
            exit(1);
        }*/

        //puts(waitCommand);
        gets(msg);

    }

    /*free(login);
    free(command);
    free(nick);
    free(waitCommand);
    free(msg);
    close(sock);
    exit(0);*/
}

void *ListenServer(void *serverSocket_void)
{
    char echoBuffer[BUFSIZE];        /* Buffer para a string de eco */
    //int recvMsgSize;                    /* Tamanho da mensagem recebida */

    int serverSocket = *((int *) serverSocket_void);

    int notQuit = 1;    // Flag that keeps communication with user

    while( notQuit == 1 ){

        /* Recebe uma mensagem de volta */
        recv(serverSocket, echoBuffer, BUFSIZE, 0);
        /*if ((recvMsgSize = recv(serverSocket, echoBuffer, BUFSIZE, 0)) < 0) {
            perror("recv");
            exit(1);
        }*/

        if( strcmp(echoBuffer, "EXIT") == 0){

            notQuit = 0;

        }else{

            printf("%s\n", echoBuffer);

        }
        
        //close(serverSocket);    /* Fecha o socket desse cliente */

    }

    pthread_exit(NULL);

}
