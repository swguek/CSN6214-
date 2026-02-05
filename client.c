#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9000

int main(){
    int sock =0;
    struct sockaddr_in server_addr;
    char buffer[1024] = {0};

    sock=socket(AF_INET, SOCK_STREAM,0);

    server_addr.sin_family =AF_INET;
    server_addr.sin_port =htons(PORT);
    inet_pton(AF_INET,"127.0.0.1",&server_addr.sin_addr);

    connect(sock,(struct sockaddr *)&server_addr, sizeof(server_addr));
    printf("Connected to the Snake and Ladder Server!\n");

    while (1){
        memset (buffer, 0,1024);
        int n= recv(sock,buffer,sizeof(buffer)-1,0);
        if(n<=0){
            printf("Connection closed by server.\n");
            break;
        }

        printf("%s",buffer);

        if(strstr(buffer,"Waiting for your move") !=NULL || strstr(buffer,"Your turn")!=NULL){
            printf("\n--- THIS IS YOUR TURN! ---\n");
            printf("Press [Enter] to roll your dice:");
            getchar();
            send(sock,"roll",4,0);
        }
    }
    close(sock);
    return 0;
}
