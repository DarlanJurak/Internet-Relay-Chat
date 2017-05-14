/*
 * TCP server
 * O cliente envia uma mensagem e o servidor imprime a mensagem.
 * Basead no codigo de:
 * http://cs.ecs.baylor.edu/~donahoo/practical/CSockets/
 */
 /*
 *Implementar:
 1 - MSG
 *
 *
 *
 */

#include <stdio.h>      /* para printf() e fprintf() */
#include <sys/socket.h> /* para socket(), bind(), e connect() */
#include <arpa/inet.h>  /* para sockaddr_in e inet_ntoa() */
#include <stdlib.h>     /* para atoi() e exit() */
#include <string.h>     /* para memset() */
#include <unistd.h>     /* para close() */
#include <pthread.h>

#define MAXPENDING 5    /* Numero maximo de conexoes que podem ficar esperando serem aceitas */
#define BUFSIZE 32   /* Tamanho do buffer de recebimento */
#define NUMTHREADS 5    
#define NUMUSERS 5
#define NUMCHANNELS 5

#define CHANNEL_NAME_SIZE 32
#define CHANNEL_ADM_SIZE 32
#define NICK_SIZE 32
#define COMMAND_SIZE 32
#define LOGIN_SIZE 32
#define MSG_SIZE 32
//================================================ TYPES ================================================
struct user{

    char login[LOGIN_SIZE];
    char nick[NICK_SIZE];
    int sockID;
    char connectedChannel[CHANNEL_NAME_SIZE];

};

struct channel{

    char name[CHANNEL_NAME_SIZE];
    char admNick[CHANNEL_ADM_SIZE];
    char usersNicks[NUMUSERS][NICK_SIZE];
    int usersCounter;

};
//================================================ Commum function ================================================
void *handleTCPClient(void *clntSocket_void);
void echoClient(int clntSocket, char sendMessage[]);
void newUserWithLoginAndSockID(char login[], int sockID);
void newUserWithSockID(int sockID);
int searchUserByLogin(char login[]);// se não existe user com login retorna -1 else retorna userIndex
int searchUserByNick(char nick[]);// se não existe user com nick retorna -1 else retorna userIndex
int searchUserBySockID(int sockID);// se não existe user com sockID retorna -1 else retorna userIndex
int searchChannelByName(char channelName[]);// se não existe channel com esse nome retorna -1 else retorna channelIndex
int searchUserInChannelByNick(char nick[], char channel[]);// se não existe user com nick retorna -1 else retorna userIndex
//================================================ Server's services ================================================
void LOGIN(char login[], int clntSock);
void NICK(char nick[], int clntSock);
void CREATE(char channelName[], int clntSock);
void REMOVE(char channelName[], int clntSock);
void LIST(int clntSock);
void JOIN(char channelName[], int clntSock);
//void PART(char channelName[], int clntSock);
void PART(int clntSock);
//void NAMES(char channelName[], int clntSock);
void NAMES(int clntSock);
void KICK(char channelName[], char nick[], int clntSock);
void PRIVMSG(char nick[], char msg[], int clntSock);
void MSG(char msg_in[], int clntSock);
//================================================ Structures ================================================
pthread_t threads[NUMTHREADS];
struct user users[NUMUSERS]; // Ponteiro usado como array de users
struct channel channels[NUMCHANNELS]; // Ponteiro usado como array de users
int usersCounter = 0;
int channelsCounter = 0;
/*
    struct data *list[100]; //Declare a array of pointer to structures  
    //allocate memory for each element in the array
    list[count] = (struct data*) malloc( sizeof(struct data) ); 
*/
//================================================ MAIN ================================================
int main(int argc, char *argv[])
{
    int servSock;                    /* Descritor de socket para o servidor */
    int clntSock;                    /* Descritor de socket para o cliente */
    struct sockaddr_in echoServAddr; /* Struct para o endereco local */
    struct sockaddr_in echoClntAddr; /* Struct para o endereco do cliente */
    unsigned short echoServPort;     /* Numero da porta do servidor */
    unsigned int clntLen;            /* Tamanho da struct de endereco do cliente */

    //Pthreads
    int pthreadsIndex = 0;
    int rc;

    if (argc != 2)     /* Testa o numero correto de argumentos */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    echoServPort = atoi(argv[1]);  /* Primeiro argumento:  numero da porta local */

    /* Cria um socket TCP para receber conexoes de clientes */
    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        perror("socket");
        exit(1);
    }
      
    /* Constroi uma estrutura para o endereco local */
    memset(&echoServAddr, 0, sizeof(echoServAddr));   /* Preenche a struct com zeros */
    echoServAddr.sin_family = AF_INET;                /* Faminha de enderecos Internet (IP) */
    echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Permite receber de qualquer endereco/interface */
    echoServAddr.sin_port = htons(echoServPort);      /* Porta local */

    /* "Amarra" o endereco local (o qual inclui o numero da porta) para esse servidor */
    if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
        perror("bind");
        exit(1);
    }

    /* Faz com que o socket possa "escutar" a rede e receber conexoes de clientes */
    if (listen(servSock, MAXPENDING) < 0) {
        perror("listen");
        exit(1);
    }

    /* Configura o tamanho da struct de endereco do cliente */
    clntLen = sizeof(echoClntAddr);

    while(1){

        /* Espera por conexoes de clientes */
        if ((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, 
                               &clntLen)) < 0) {
            perror("accept");
            exit(1);
        }

        rc = pthread_create(&threads[pthreadsIndex], NULL, handleTCPClient, (void *)&clntSock);
        pthreadsIndex = pthreadsIndex +1;

        /* Nesse ponto, um cliente ja conectou!
         * O socket clntSock contem a conexao com esse cliente! */
        printf("Aceitei conexao do cliente %s\n", inet_ntoa(echoClntAddr.sin_addr));

    }

    //do /* Executa para sempre */
    //{
        
        /* Essa funcao e' chamada para tratar a conexao com o cliente, a qual e' definida pelo socket clntSock */
        //endCommunication = handleTCPClient(clntSock);

    //}while( endCommunication != 0 );//message != "exit");

    /* Fecha o socket do servidor, mas nesse exemplo a execucao numa chegara ateh aqui. */
    close(servSock);
}
//================================================ Function`s definitions ================================================
void *handleTCPClient(void *clntSocket_void)
{
    char echoBuffer[BUFSIZE];        /* Buffer para a string de eco */
    int recvMsgSize;                    /* Tamanho da mensagem recebida */
    int clntSock = *((int *) clntSocket_void);
    char login[LOGIN_SIZE];      // Username para realizar login
    char command[COMMAND_SIZE];   // Commando enviado pelo usuario
    char nick[NICK_SIZE];      // Novo nickname do usuario
    char channelName[CHANNEL_NAME_SIZE];      // Nome de canal
    char msg[MSG_SIZE];
    char msgToUsers[MSG_SIZE];

    int endCommunication = 0; //
    int userIndex;

    memset(echoBuffer, '\0', BUFSIZE);
    recv(clntSock, echoBuffer, BUFSIZE, 0);
    /*// Recebe uma mensagem do cliente
    if ((recvMsgSize = recv(clntSock, echoBuffer, BUFSIZE, 0)) < 0) {
        perror("recv");
        exit(1);
    }*/

    /* RECV TEST
    printf("Mensagem: %s\n", echoBuffer);

    // Sends back the message received
    echoClient(clntSock, echoBuffer);
    */

    memset(command, '\0', BUFSIZE);
    memset(login, '\0', BUFSIZE);
    memset(nick, '\0', BUFSIZE);
    memset(channelName, '\0', BUFSIZE);
    memset(msg, '\0', BUFSIZE);
    memset(msgToUsers, '\0', BUFSIZE);

    do{        
        //Pega primeira palavra (comando)
        strcpy(command, strtok (echoBuffer," ,.-"));

        if(strcmp(command, "QUIT") == 0){

            endCommunication = 1;
            PART(clntSock);
            sprintf(command, "EXIT"); //Sinaliza encerramento dacomunicacao para client
            echoClient(clntSock, command);


        }else if(strcmp(command, "LOGIN") == 0){
            printf("%s\n", "Reconheceu commando LOGIN");
            strcpy(login, strtok (NULL," ,.-")); // Extrai username para login
            printf("%s\n", "Fez strcpy()");
            userIndex = searchUserByLogin(login);
            printf("%s\n", "Fez searchUserByLogin()");
            if (userIndex == -1) newUserWithLoginAndSockID(login, clntSock);
            printf("%s\n", "Fez newUserWithLoginAndSockID()");
            LOGIN(login, clntSock);
            memset(login, '\0', BUFSIZE);
            

        }else if(strcmp(command, "NICK") == 0){

            userIndex = searchUserBySockID(clntSock); 
            if(userIndex == -1) newUserWithSockID(clntSock);
            strcpy(nick, strtok (NULL," ,.-")); // Extrai nick
            userIndex = searchUserByNick(nick);
            if(userIndex == -1){//Nick único

                NICK(nick, clntSock);   // Realiza alteracao de nick

            }
            memset(nick, '\0', BUFSIZE);

        }else if(strcmp(command, "CREATE") == 0){

            userIndex = searchUserBySockID(clntSock); 
            if(userIndex == -1) newUserWithSockID(clntSock);
            strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome do canal
            CREATE(channelName, clntSock);  // Cria novo canal
            memset(channelName, '\0', BUFSIZE);
            printf("CHANNEL: %s\n", channels[channelsCounter -1].name);
            printf("OWNER:: %s\n", channels[channelsCounter -1].admNick);

        }else if(strcmp(command, "REMOVE") == 0){

            strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome do canal
            REMOVE(channelName, clntSock);  // Cria novo canal
            memset(channelName, '\0', BUFSIZE);

        }else if(strcmp(command, "LIST") == 0){

            LIST(clntSock);

        }else if(strcmp(command, "JOIN") == 0){

            strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome do canal
            JOIN(channelName, clntSock);  // Cadastra usuario em um canal
            memset(channelName, '\0', BUFSIZE);

        }else if(strcmp(command, "PART") == 0){

            //strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome do canal
            //PART(channelName, clntSock);  // Descadastra usuario de um canal
            PART(clntSock);  // Descadastra usuario de um canal
            //memset(channelName, '\0', BUFSIZE);

        }else if(strcmp(command, "NAMES") == 0){

            //strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome do canal
            //printf("Vou chamar NAMES\n");
            //NAMES(channelName, clntSock);  // Mostra o nick dos usuarios de um canal
            NAMES(clntSock);  // Mostra o nick dos usuarios de um canal
            //memset(channelName, '\0', BUFSIZE);

        }else if(strcmp(command, "KICK") == 0){

            strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome do canal
            strcpy(nick, strtok (NULL," ,.-")); // Extrai nome do usuario a ser kikado
            KICK(channelName, nick, clntSock);  // Kika um user de um canal
            memset(channelName, '\0', BUFSIZE);
            memset(nick, '\0', BUFSIZE);

        }else if(strcmp(command, "PRIVMSG") == 0){

            strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome aleatório de canal
            strcpy(nick, strtok (NULL," ,.-")); // Extrai nome do usuario a ser kikado
            strcpy(msg, strtok (NULL,"&")); // Extrai mensagem a ser enviada
            PRIVMSG(nick, msg, clntSock);  // Envia uma mmensagem para um user específico
            memset(channelName, '\0', BUFSIZE);
            memset(nick, '\0', BUFSIZE);
            memset(msg, '\0', BUFSIZE);

        }else if(strcmp(command, "MSG") == 0){

            printf("%s\n", "Entrou no MSG");
            strcpy(channelName, strtok (NULL," ,.-")); // Extrai nome aleatório de canal
            memcpy(msgToUsers, &echoBuffer[12], 20);
            printf("%s%s\n", "msgToUsers: ", msgToUsers);
            MSG(msgToUsers, clntSock);
            memset(channelName, '\0', BUFSIZE);
            memset(msgToUsers, '\0', BUFSIZE);

        }else{

            printf("Comando Inválido\n");

        }
        memset(command, '\0', BUFSIZE);

        //Mantem comunicação
        if( endCommunication == 0){

            memset(echoBuffer, '\0', BUFSIZE);  
            recvMsgSize = recv(clntSock, echoBuffer, BUFSIZE, 0);
            /*if ((recvMsgSize = recv(clntSock, echoBuffer, BUFSIZE, 0)) < 0) {
                perror("recv");
                exit(1);
            }*/

        }


    }while( endCommunication == 0);

    close(clntSock); // Fecha o socket desse cliente

    /*if ( strcmp(echoBuffer, "exit") ) return 1;
    else return 0;*/

    pthread_exit(NULL);
}


void echoClient(int clntSocket, char sendMessage[]){

    /* Echo */
    send(clntSocket, sendMessage, strlen(sendMessage) +1, 0);
    memset(sendMessage, '\0', BUFSIZE);
    /*if (send(clntSocket, sendMessage, BUFSIZE, 0) != (strlen(sendMessage) + 1)) {
        perror("send");
        exit(1);
    }*/

}

void newUserWithLoginAndSockID(char login[], int sockID){

    struct user newUser;

    strcpy(newUser.login, login);
    strcpy(newUser.connectedChannel, "NULL");
    newUser.sockID = sockID;

    users[usersCounter] = newUser;
    usersCounter = usersCounter +1;

}

void newUserWithSockID(int sockID){

    struct user newUser;

    newUser.sockID = sockID;

    users[usersCounter] = newUser;
    usersCounter = usersCounter +1;

}

// se não existe user com login retorna -1 else retorna userIndex
int searchUserByLogin(char login[]){

    int i = 0;

    for(i = 0; i < usersCounter; i++){

        if ( strcmp(users[i].login, login) == 0){

            return i;

        }

    }

    return -1;

}

// se não existe user com nick retorna -1 else retorna userIndex
int searchUserByNick(char nick[]){

    int i = 0;

    for(i = 0; i < usersCounter; i++){

        if( strcmp(users[i].nick, nick) == 0){

            return i;

        }

    }
    return -1;

}

// se não existe user com sockID retorna -1 else retorna userIndex
int searchUserBySockID(int sockID){

    int i = 0;

    for(i = 0; i < usersCounter; i++){

        if( users[i].sockID == sockID ){
            printf("%s %d\n", "searchUserBySockID retornou index: \n", i);
            return i;

        }

    }
    return -1;

}

// se não existe channel com esse nome retorna -1 else retorna channelIndex
int searchChannelByName(char channelName[]){

    int i = 0;

    for(i = 0; i < channelsCounter; i++){

        if( strcmp(channels[i].name, channelName) == 0 ){

            return i;

        }

    }
    return -1;

}

// se não existe user com nick retorna -1 else retorna userIndex
int searchUserInChannelByNick(char nick[], char channel[]){

    int channelIndex;
    int i = 0;
    int numberOfValidUsers = 0;

    channelIndex = searchChannelByName(channel);
    numberOfValidUsers = channels[channelIndex].usersCounter;

    for(i = 0; i < numberOfValidUsers; i++ ){

        if( strcmp(channels[channelIndex].usersNicks[i], nick) == 0){

            return i;

        }else if( strcmp(channels[channelIndex].usersNicks[i], "NULL") == 0){

            numberOfValidUsers++;

        }

    }
    return -1;

}

void LOGIN(char login[], int clntSock){
    printf("%s\n", "Entrou na LOGIN");
    char pch[MSG_SIZE];

    memset(pch, '\0', BUFSIZE);
    sprintf(pch, "Bem vindo\n");
    echoClient(clntSock, pch);

}

void NICK(char nick[], int clntSock){

    int userIndex = 0;
    struct user userTemp;
    char pch[MSG_SIZE];

    //memset(pch, '\0', BUFSIZE);
    userIndex = searchUserBySockID(clntSock);
    if( userIndex != -1){

        userTemp = users[userIndex];
        strcpy(userTemp.nick, nick);
        users[userIndex] = userTemp;

    }

    sprintf(pch, "Você alterou o nick para %s\n", nick );
    echoClient(clntSock, pch);

}

void CREATE(char channelName[], int clntSock){

    struct channel newChannel;
    int userIndex;

    userIndex = searchUserBySockID(clntSock);

    
    strcpy(newChannel.name, channelName);
    strcpy(newChannel.admNick, users[userIndex].nick);
    newChannel.usersCounter = 0;

    channels[channelsCounter] = newChannel;
    channelsCounter = channelsCounter +1;

}

void REMOVE(char channelName[], int clntSock){

    int channelIndex;
    int i=0;
    int numberOfValidUsers = 0;
    char pch[MSG_SIZE];
    int userIndex = 0;
    char admNick[NICK_SIZE];
    int admIndex = 0;

    channelIndex = searchChannelByName(channelName);
    if( channelIndex > -1 ){//Se existe o canal

        memset(admNick, '\0', BUFSIZE);
        strcpy(admNick, channels[channelIndex].admNick);

        admIndex = searchUserBySockID(clntSock);
        if( strcmp(admNick, users[admIndex].nick ) == 0 ){//Se quem chamou o REMOVE é adm do canal

            numberOfValidUsers = channels[channelIndex].usersCounter;
            for(i = 0; i < numberOfValidUsers; i++){

                if( strcmp(channels[channelIndex].usersNicks[i], "NULL" ) == 0 ){

                    numberOfValidUsers++;

                }else{//Kika e avisa q foi kikado

                    userIndex = searchUserByNick(channels[channelIndex].usersNicks[i]);
                    sprintf(pch, "Você foi kikado do canal %s\n", channels[channelIndex].name );
                    KICK(channelName, channels[channelIndex].usersNicks[i], clntSock);
                    echoClient(users[userIndex].sockID, pch);

                }

            }
            memset(channels[channelIndex].name, '\0', BUFSIZE);
            strcpy(channels[channelIndex].name, "NULL");
            channelsCounter--;
        }
    }
}

void LIST(int clntSock){

    int i = 0;
    char pch[MSG_SIZE];
    int validChannels = 0;

    memset(pch, '\0', BUFSIZE);
    sprintf(pch, "Lista de canais no servidor:\n");
    echoClient(clntSock, pch);

    validChannels = channelsCounter;
    for(i = 0; i < validChannels; i++){


        if( strcmp(channels[i].name, "NULL") == 0){

            validChannels++;

        }else{

            printf("%s %s\n", "Current Channel: ", channels[i].name);

            memset(pch, '\0', BUFSIZE);
            sprintf(pch, "#%s", channels[i].name);
            echoClient(clntSock, pch);
            memset(pch, '\0', BUFSIZE);

        }
    }

}

void JOIN(char channelName[], int clntSock){

    int channelIndex = 0;
    int userIndex = 0;
    int nextUserNick = 0;
    char msg[MSG_SIZE];

    channelIndex = searchChannelByName(channelName);
    userIndex = searchUserBySockID(clntSock);

    if( channelIndex > -1){

        printf("Na JOIN searchChannelByName() encontrou: %s\n", channels[channelIndex].name);

        if( userIndex > -1){

            printf("Na JOIN searchUserBySockID() encontrou: %s\n", users[userIndex].nick);

            nextUserNick = channels[channelIndex].usersCounter;

            memset(channels[channelIndex].usersNicks[nextUserNick], '\0', BUFSIZE);
            strcpy(channels[channelIndex].usersNicks[nextUserNick], users[userIndex].nick);
            channels[channelIndex].usersCounter = channels[channelIndex].usersCounter +1;

            memset(users[userIndex].connectedChannel, '\0', BUFSIZE);
            strcpy(users[userIndex].connectedChannel, channels[channelIndex].name);

            memset(msg, '\0', BUFSIZE);
            sprintf(msg, "Você entrou no canal #%s\n", channels[channelIndex].name);
            echoClient(clntSock, msg);


        }

    }

}

void PART(int clntSock){

    int channelIndex = 0;
    int userIndex = 0;
    int userInChannelIndex = 0;
    char msg[MSG_SIZE];

    userIndex = searchUserBySockID(clntSock);

    if( userIndex > -1 ){

        channelIndex = searchChannelByName(users[userIndex].connectedChannel);

        if( channelIndex > -1 ){

            printf("Na PART searchUserBySockID() encontrou: %s\n", users[userIndex].nick);
            userInChannelIndex = searchUserInChannelByNick(users[userIndex].nick, users[userIndex].connectedChannel);

            if( userInChannelIndex > -1 ){

                printf("Na PART searchChannelByName() encontrou: %s\n", channels[channelIndex].name);

                memset(channels[channelIndex].usersNicks[userInChannelIndex], '\0', BUFSIZE);
                strcpy(channels[channelIndex].usersNicks[userInChannelIndex], "NULL");
                channels[channelIndex].usersCounter = channels[channelIndex].usersCounter -1;

                memset(users[userIndex].connectedChannel, '\0', BUFSIZE);
                strcpy(users[userIndex].connectedChannel, "NULL");

                memset(msg, '\0', BUFSIZE);
                sprintf(msg, "Você saiu do canal #%s\n", channels[channelIndex].name);
                echoClient(clntSock, msg);

            }

        }
    }

}

void NAMES(int clntSock){

    //printf("Entrei na NAMES\n");
    int channelIndex = 0;
    int i = 0;
    int numberOfValidUsers = 0;
    char nick[NICK_SIZE];
    char msg[MSG_SIZE];
    int j = 0;
    int userIndex = 0;


    userIndex = searchUserBySockID(clntSock);
    if( userIndex > -1 ){

        channelIndex = searchChannelByName(users[userIndex].connectedChannel);
        if( channelIndex > -1 ){

            numberOfValidUsers = channels[channelIndex].usersCounter;

            for(i = 0; i < numberOfValidUsers; i++){

                if( strcmp(channels[channelIndex].usersNicks[i], "NULL") == 0){

                    numberOfValidUsers++;

                }else{

                    memset(nick, '\0', BUFSIZE);
                    strcpy(nick, channels[channelIndex].usersNicks[i]);
                    memset(msg, '\0', BUFSIZE);
                    sprintf(msg, "User %i: %s", j, nick);
                    echoClient(clntSock, msg);
                    memset(msg, '\0', BUFSIZE);
                    printf("Names: %s\n", nick);
                    memset(nick, '\0', BUFSIZE);
                    j++;

                }

            }

        }

    }

}

void KICK(char channelName[], char nick[], int clntSock){

    int channelIndex = 0;
    char admNick[NICK_SIZE];
    int admIndex = 0;
    int userKikadoIndex = 0;
    char msg_out[MSG_SIZE];
    int userIndex = 0;

    channelIndex = searchChannelByName(channelName);
    if( channelIndex > -1 ){

        memset(admNick, '\0', BUFSIZE);
        strcpy(admNick, channels[channelIndex].admNick);

        admIndex = searchUserBySockID(clntSock);
        if( strcmp(admNick, users[admIndex].nick ) == 0 ){
            //Kikar
            userKikadoIndex = searchUserInChannelByNick(nick, channelName);
            if( userKikadoIndex > -1 ){
                printf("%s%s \n", "User a ser kikado: ", channels[channelIndex].usersNicks[userKikadoIndex]);

                userIndex = searchUserByNick(channels[channelIndex].usersNicks[userKikadoIndex]);
                if(userIndex > -1){

                    memset(msg_out, '\0', BUFSIZE);
                    sprintf(msg_out, "Voce foi kikado do canal %s", channels[channelIndex].name);
                    echoClient(users[userIndex].sockID, msg_out);

                }

                memset(channels[channelIndex].usersNicks[userKikadoIndex], '\0', BUFSIZE);
                strcpy(channels[channelIndex].usersNicks[userKikadoIndex], "NULL");
                channels[channelIndex].usersCounter = channels[channelIndex].usersCounter -1;




            }

        }

    }
}

void PRIVMSG(char nick[], char msg_in[], int clntSock){

    int userIndex;
    char msg_out[MSG_SIZE];
    int senderIndex = 0;

    userIndex = searchUserByNick(nick);
    senderIndex = searchUserBySockID(clntSock);
    if( userIndex > -1 ){

        memset(msg_out, '\0', BUFSIZE);
        sprintf(msg_out, "<%s> %s\n", users[senderIndex].nick, msg_in);
        echoClient(users[userIndex].sockID, msg_out);
        memset(msg_out, '\0', BUFSIZE);

    }

}

void MSG(char msg_in[], int clntSock){

    int userIndex = 0;
    int channelIndex = 0;
    int numberOfValidUsers = 0;
    int i = 0;
    int senderIndex = 0;

    char msg[BUFSIZE];
    char msg_out[BUFSIZE];

    memset(msg, '\0', BUFSIZE);
    strcpy(msg, msg_in);
    userIndex = searchUserBySockID(clntSock);//Pega indice do usuario que vai enviar msg
    printf("%s%d\n", "searchUserBySockID no MSG retornou: ", userIndex);
    if( userIndex > -1 ){

        //if( strcmp(users[userIndex].connectedChannel, "NULL") != 0 ){

        channelIndex = searchChannelByName(users[userIndex].connectedChannel);//Pega canal do que o user esta
        if( channelIndex > -1 ){

            printf("%s%s\n", "Usuario esta no canal: ", users[userIndex].connectedChannel);
            numberOfValidUsers = channels[channelIndex].usersCounter;
            for(i = 0; i < numberOfValidUsers; i++){

                if( strcmp(channels[channelIndex].usersNicks[i], "NULL" ) == 0 ){

                    numberOfValidUsers++;

                }else{

                    userIndex = searchUserByNick(channels[channelIndex].usersNicks[i]);
                    if( userIndex > -1 ){
                        printf("%s%s%s%s\n", "Vai enviar ", msg_in,  "para o user: ", channels[channelIndex].usersNicks[i]);
                        printf("%s%i\n", "SockID: ", users[userIndex].sockID);
                        printf("%s%i\n", "user index: ", userIndex);
                        if(users[userIndex].sockID != clntSock){

                            senderIndex = searchUserBySockID(clntSock);
                            memset(msg_out, '\0', BUFSIZE);
                            sprintf(msg_out, "<%s> %s\n", users[senderIndex].nick, msg);
                            strcpy(msg_in, msg_out);
                            echoClient(users[userIndex].sockID, msg_in);
                            memset(msg_out, '\0', BUFSIZE);

                        }

                    }
                }

            }
            memset(msg, '\0', BUFSIZE);

        }

        //}
    }

}
