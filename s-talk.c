#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <poll.h>


//CMPT 300 Assignment 2 Written by Dakota Crozier 301370239



//struct to pass arguments to thread sending stuff over socket
struct arg_struct {//only used once so wasted memory is negligible
    char port[7];//max port number 65535 so 7 chars to be safe
    char remoteMachine[255];//max hostname length is 253 so 255 chars to be safe
};
//shared data
bool MUTUAL_ASSURED_DESTRUCTION = false;
const int MAX_BUFFER = 65000;// max udp packet size 
pthread_mutex_t bombMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t listMutexIN = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t listMutexOUT = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutexIN = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutexOUT = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_condIN  = PTHREAD_COND_INITIALIZER;
pthread_cond_t  condition_condOUT  = PTHREAD_COND_INITIALIZER;
List *ListIN = NULL;
List *ListOUT = NULL;


void server(char localPortStr[]);
void keyboard();
//void print();
void client(char remoteMachine[],char remotePort[]);
//dummy function to pass arguments to thread
void sendFunc(void* arguments){
    struct arg_struct *args = arguments;
    client(args->remoteMachine,args->port);
}

void freeStr(void* ptr){
    free((char*)ptr);
}


int main(int argc, char *argv[]) {
    if(argc!=4){
    printf("Invalid arguments\n");
    printf("Expected: s-talk [my port number] [remote machine name] [remote port number]");
    exit(1);
    }
    
    struct arg_struct args;
    strcpy(args.port,argv[3]);
    strcpy(args.remoteMachine, argv[2]);

    ListIN = List_create();
    ListOUT = List_create();

    int rc1,rc2,rc4;
    pthread_t thread1,thread2,thread4;
    if((rc1=pthread_create(&thread1,NULL,(void*)&server,(void*)argv[1]))){
        printf("Thread creation failed: %d\n",rc1);
    }
    if((rc2=pthread_create(&thread2,NULL,(void*)&keyboard,NULL))){
        printf("Thread creation failed: %d\n",rc2);
    }
     
    if((rc4=pthread_create(&thread4,NULL,(void*)&sendFunc,(void*)&args))){
        printf("Thread creation failed: %d\n",rc4);
    }
    //this was originally thread 3 but I forgot I could just use main
    //PRINTING THREAD 
     while(1){       
        pthread_mutex_lock(&bombMutex);//check if session ended
        if(MUTUAL_ASSURED_DESTRUCTION){
            break;
        }
        pthread_mutex_unlock(&bombMutex); 

        pthread_mutex_lock(&condition_mutexIN);//wait for something to be put on list
        while(List_count(ListIN) <1){
            pthread_cond_wait(&condition_condIN,&condition_mutexIN);
        }
        pthread_mutex_unlock(&condition_mutexIN);

        pthread_mutex_lock(&listMutexIN);//take off list CRITICAL SECTION
        char * msg = List_trim(ListIN);
        pthread_mutex_unlock(&listMutexIN);
        if(msg == NULL){//thread was woken up with NULL message to exit
            pthread_mutex_lock(&bombMutex);
            break;
        }
        printf("%s",msg);
        //check if leave message was received
        if(msg[0] == '!' && msg[1] == '\n'){//!\n\0
            pthread_mutex_lock(&bombMutex);
           MUTUAL_ASSURED_DESTRUCTION = true;
           pthread_mutex_unlock(&bombMutex);
        }
        free(msg);
  }
  //thread will always leave loop with bombMutex locked to avoid undefined behaviour
  pthread_mutex_unlock(&bombMutex);
  
    //incase client thread is blocked waiting for list_size add NULL msg element
    pthread_mutex_lock(&listMutexOUT);
    List_insert(ListOUT,NULL);
    pthread_mutex_unlock(&listMutexOUT);
    //wake up client thread
    pthread_mutex_lock(&condition_mutexOUT);
    pthread_cond_signal(&condition_condOUT);
    pthread_mutex_unlock(&condition_mutexOUT);
 
    pthread_join( thread1, NULL);
    pthread_join( thread2, NULL); 
    pthread_join( thread4, NULL); 

    // SHARED DATA CLEANUP
    List_free(ListIN,&freeStr);
    List_free(ListOUT,&freeStr);

    //documentation says not needed to destroy mutex as it only unintializes.
    return 0;
}

void keyboard(){
    int bytesRead =0;
    struct pollfd fd;
    int ret = 0;
    fd.fd = STDIN_FILENO; 
    fd.events = POLLIN;
    char buffer[MAX_BUFFER];
    while(1){
        pthread_mutex_lock(&bombMutex);
            if(MUTUAL_ASSURED_DESTRUCTION){
                break;
            }
            pthread_mutex_unlock(&bombMutex); 
        ret = poll(&fd, 1, 500); // 0.5 second for timeout (could do lower but real waste of cpu)
        if(ret ==-1){
            //error handle
            perror("keyboard poll:");
            exit(-1);
        }if(ret == 0){//timeout
            //do nothing
            //could increase timeout length after conesective timeouts but it will take longer to close thread.
        }else{ 
            memset(buffer,'\0',MAX_BUFFER);//fill to be safe (this was not a fun bug)
            bytesRead = read(STDIN_FILENO, buffer, MAX_BUFFER-1);
            //check exit message
            if(buffer[0] == '!'&& buffer[1] == '\n'){
                pthread_mutex_lock(&bombMutex);
                MUTUAL_ASSURED_DESTRUCTION = true;
                pthread_mutex_unlock(&bombMutex);
            }
            char *buf = malloc( (sizeof(char))* ( bytesRead+1));//is +1 needed?
            strcpy(buf,buffer);
            //add to output list
            pthread_mutex_lock(&listMutexOUT);
            List_prepend(ListOUT,buf);
            pthread_mutex_unlock(&listMutexOUT);
            //wakeup client/send thread
            pthread_mutex_lock(&condition_mutexOUT);
            pthread_cond_signal(&condition_condOUT);
            pthread_mutex_unlock(&condition_mutexOUT);
        }
    }
    //THREAD EXITING
    pthread_mutex_unlock(&bombMutex);
   //wakeup printing thread incase its waiting for messages
    pthread_mutex_lock(&listMutexIN);
    List_insert(ListIN,NULL);
    pthread_mutex_unlock(&listMutexIN);

    pthread_mutex_lock(&condition_mutexIN);
    pthread_cond_signal(&condition_condIN);
    pthread_mutex_unlock(&condition_mutexIN);
} 







//thread to send data over udp
void client(char remoteMachine[],char remotePort[]) {
    int sockfd = -1;
    struct addrinfo hints, *servinfo, *p;
    int status =-1;
    //char* hello = "HELLO WORLD PLEASE";
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    status = getaddrinfo(remoteMachine,remotePort, &hints, &servinfo);
    if (status <0) {
        perror("client getaddrinfo:");
        exit(1);
    }
    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client socket:");
            continue;
        }

        break;
    }
    if (p == NULL) {
        perror("Unable to create socket");
        exit(1);
    }

    while(1){
        //wait for something in the output list
        pthread_mutex_lock(&condition_mutexOUT);
        while(List_count(ListOUT) <1){
            pthread_cond_wait(&condition_condOUT,&condition_mutexOUT);
        }
        pthread_mutex_unlock(&condition_mutexOUT);

        pthread_mutex_lock(&listMutexOUT);
        char * msg = List_trim(ListOUT);
        pthread_mutex_unlock(&listMutexOUT);

        if(msg == NULL){//check if wakeup NULL element
            pthread_mutex_lock(&bombMutex);
            break;
        }
        status = sendto(sockfd,msg, strlen(msg)+1,MSG_DONTWAIT,p->ai_addr,p->ai_addrlen);// +1 because strlen doesnt count nullchar..
        if(status <0){
            perror("sendto:");
            exit(1);
        }
        free(msg);
        pthread_mutex_lock(&bombMutex);
        if(MUTUAL_ASSURED_DESTRUCTION){
            break;
        }
        pthread_mutex_unlock(&bombMutex);
    }
    //Exiting thread cleanup
    pthread_mutex_unlock(&bombMutex);
    freeaddrinfo(servinfo);
    
    close(sockfd);
}













//thread to receive data on udp socket
void server(char localPortStr[]) {
    int status = -1;
    int sockfd = -1;
    socklen_t addr_len;
    struct addrinfo hints,*servinfo,*p;
     int numbytes = 0;
     struct sockaddr_storage localaddr;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET?
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags  = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    status = getaddrinfo(NULL, localPortStr, &hints, &servinfo);
    if(status <0){
    perror("server getaddrinfo:");
        exit(1);
   }

    //int yes = 1;
   // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        perror("Unable to create socket");
        exit(2);
    }
    addr_len =  sizeof localaddr;

    struct pollfd fd;
    int ret = 0;

    fd.fd = sockfd; 
    fd.events = POLLIN;
    char buf[MAX_BUFFER];
    while(1){ 
        //check if thread should exit
        pthread_mutex_lock(&bombMutex);
        if(MUTUAL_ASSURED_DESTRUCTION){
            break;
        }
        pthread_mutex_unlock(&bombMutex);
        
        memset(buf,'\0',MAX_BUFFER);// :(
        ret = poll(&fd, 1, 500); // half second for timeout 
        
        if(ret ==-1){
            //error handle
            perror("receiving poll:");
            exit(-1);
        }if(ret == 0){//timeout
            //do nothing
            //could increase timeout length after conesective timeouts but thread takes longer to close
        }else{       
            if ((numbytes = recvfrom(sockfd, buf, MAX_BUFFER, 0, (struct sockaddr *)&localaddr, &addr_len)) == -1) {
                    perror("recvfrom");
                    exit(1);
                }
                char * msg = malloc( sizeof(char) * ( numbytes));//save memory by only allocating just enough for message
                strcpy(msg,buf);
                if(msg[0] == '!' && msg[1] == '\n'){//!\n\0
                    pthread_mutex_lock(&bombMutex);
                    MUTUAL_ASSURED_DESTRUCTION = true;
                    pthread_mutex_unlock(&bombMutex);
                }
                //add message to list
                pthread_mutex_lock(&listMutexIN);
                List_prepend(ListIN,msg);
                pthread_mutex_unlock(&listMutexIN);
                //wakeup print
                pthread_mutex_lock(&condition_mutexIN);
                pthread_cond_signal(&condition_condIN);
                pthread_mutex_unlock(&condition_mutexIN);     
        }
    }

    //thread exiting
    pthread_mutex_unlock(&bombMutex);
    freeaddrinfo(servinfo);
    close(sockfd);
    
}
