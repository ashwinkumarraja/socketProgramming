/* f20170599@hyderabad.bits-pilani.ac.in Ashwin Kumar Raja   */

/*
	The code includes the client side programme to get the HTML content of a URL through a Squid Proxy Server.
	URL and Proxy details and filenames to save content are taken as arguments.
	Code also manages redirects and saves output in HTML and for particular websites even an image logo.
	Clear and modular code is included and can be understood by function definitions.
*/
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 

#define N 4096
#define CHUNK_SIZE 1024

char *url,*host,*path,*proxy_IP,*proxy_port,*username,*password,*auth,*fileName,*logoName;
int pport;

struct hostent *server;
struct sockaddr_in serv_addr;

int sockfd, bytes, sent, received, total,status;

char message[1024],response[1024];

FILE *html_output,*img_output;


//Base 64 Encoding of username and password
void encode(char* username,char* password){
	//auth = (char*)malloc(sizeof(N));
	const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	char* comb = (char*)malloc(N);

	strcpy(comb,username);
	strcat(comb,":");
	strcat(comb,password);
	
	int len = strlen(comb);

	char *input = comb;
	char   *out;

	if (input == NULL || len == 0)
		return;
	size_t initialLength = len;
	size_t newLength = initialLength;
	if(initialLength % 3 == 1)
		newLength+=2;
	else if(initialLength %3 == 2)
		newLength++;

	newLength *= 4;
	newLength /= 3;
	

	out  = malloc(newLength+1);
	out[newLength] = '\0';


	size_t  i,j,v;
	for (i=0, j=0; i<len; i+=3, j+=4) {

		v = input[i];
		v = i+1 < len ? v << 8 | input[i+1] : v << 8;
		v = i+2 < len ? v << 8 | input[i+2] : v << 8;

		out[j]   = b64chars[(v >> 18) & 0x3F];
		out[j+1] = b64chars[(v >> 12) & 0x3F];

		if (i+1 < len)
			out[j+2] = b64chars[(v >> 6) & 0x3F];
		else 
			out[j+2] = '=';
		
		if (i+2 < len)
			out[j+3] = b64chars[v & 0x3F];
		
		else 
			out[j+3] = '=';
		
	}

	auth = out;
	// auth = "Y3NmMzAzOmNzZjMwMw==";
	printf("%s\n", auth);
}
//Obtain Host and Path from URL
void separateHostandPath(){
	
	path="";
	//removing http://
	char* check = strstr(url,"://");
	puts(url);
	if(check!=NULL){
		*check++;
		*check++;
		*check++;
		//puts(check);
		char tempHost[1024];
		strcpy(tempHost,check);
		for(int i = 0 ; i <sizeof(tempHost);++i)
			if(tempHost[i]=='/')
				tempHost[i]='\0';

		puts(tempHost);
		strcpy(host,tempHost);
	}
	else{
		char tempHost[1024];
		strcpy(tempHost,url);
		for(int i = 0 ; i < sizeof(tempHost);++i){
			if(tempHost[i] == '/')
				tempHost[i] = '\0';
		}
	puts("HERE");
		puts(tempHost);
		strcpy(host,tempHost);
	}


	check = strstr(url,".");
	check = strstr(check,"/");
	path = (char*)malloc(N);
	if(check == NULL){
		path="";
		return;
	}
	*check++;
	if(check == NULL){
		path="";
		return;
	}

	strcpy(path,check);

	return;

}
//Create Socket (Network Overlay - IPV4 , TCP Connection - SOCK_STREAM)
void createSocket(){

	struct timeval tv;
	tv.tv_sec = 10;//(in seconds)
	tv.tv_usec = 0;//(in microseconds)
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) perror("ERROR opening socket");
    printf("Socket created \n");


}
//GetHost details (IP) using hostname , function does DNS lookup and gives hotent object which contains IP also.
void getHost(){
	server = gethostbyname(host);
    //server = gethostbyname(proxy_port);
    if (server == NULL) perror("ERROR, no such host");
}
//Establish connection with Proxy server
void connectToServer(){
	

	memset(&serv_addr, '0', sizeof(serv_addr)); 
    serv_addr.sin_family = AF_INET;//IPv4
    serv_addr.sin_port = htons(pport);//port proxy server
 	puts(proxy_IP);			
    if(inet_pton(AF_INET, proxy_IP, &serv_addr.sin_addr)<=0)
    {
        puts("inet_pton error occured");
        exit(1);
    } 
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        perror("ERROR connecting");
}
//Send Request Message
void createMessage(){

	char *message_fmt = "GET http://%s/%s HTTP/1.1\r\nHost: %s\r\nAccept: text/html,image/webp,*/*\r\nProxy-Authorization: Basic %s\r\nProxy-Connection: Keep-Alive\r\n\r\n";
	//printf("%s\n",path);
	snprintf(message,sizeof(message),message_fmt,host,path,host,auth);
	
    printf("Request:\n%s\n",message);
}
//Send Requst through socket
void sendHTTP_request(){
	
	//printf("%s\n",message );
	total = strlen(message);
    sent = 0;

    if(send(sockfd,message,strlen(message),0)<0)
	{
		printf("Message failed");
	}
    printf("HTTP Request Sent\n");
}
//Get status message in response
int checkStatus(char* temp){
	if (strstr(temp, "HTTP/1.1 3"))
    	return 3;
    else if(strstr(temp, "HTTP/1.1 2"))
    	return 2;
    else if(strstr(temp, "HTTP/1.1 4"))
    	return 4;
    else 
    	return 5;
       
}
//Extract HTTP response especially headers
void recieve_HTTP_response(){

	recv(sockfd, response, 1, 0);

	puts("Extracting response status");

	char temp[500] = "\0";
    while (response[0] != '\r')
    {
        strcat(temp, response);
        recv(sockfd, response, 1, 0);
	}
	recv(sockfd, response, 1, 0);
	puts(temp);
	puts("\n");
	status = checkStatus(temp);

}

//Get Location of redirection from Location Header
void getLocation(){
	while (1){
        recv(sockfd, response, 1, 0);
        if (response[0] == '\r')
        {
            recv(sockfd, response, 1, 0);
            break;
        }
        char newURL[300] = "\0";
        while (response[0] != '\r')
        {
            strcat(newURL, response);
            recv(sockfd, response, 1, 0);
        }
        if (strstr(newURL, "Location: "))
        {
            strcpy(url, newURL + 10);
            puts(url);
            puts("\n");
        }
        recv(sockfd, response, 1, 0);
    }
}
// Get Content Length
long long getContentLength(){
	response[0] = '\0';
	long long contentLength = 0;
    puts("Extracting Content Length");
    while (1)
    {
        char temp[300] = "\0 ";
        strcat(temp, response);
        while (response[0] != '\r')
        {
            recv(sockfd, response, 1, 0);
            strcat(temp, response);
        }
        
        if (strstr(temp, "Content-Length: "))
        {
        	// puts(temp);
            char *len = temp + 16;
            contentLength = atoll(len);
        }
        recv(sockfd, response, 1, 0);
        recv(sockfd, response, 1, 0);
        // puts(response);
        if (response[0] == '\r')
        {
            recv(sockfd, response, 1, 0);
            break;
        }
    }
    // puts(contentLength);
    // printf("%lld",contentLength);
    return contentLength;
}
//Parse HTML to output file
void getHTML(long long contentLength){
	puts("Parsing HTML\n");
	int bytes_received;

	html_output = fopen(fileName, "wb");
	// html_output = fopen("content.html", "wb");
	if (contentLength > 0){
        while (contentLength--){
            bytes_received = recv(sockfd, response, 1, 0);
            if (bytes_received == -1)
            {
                perror("Did not receive");
                //exit(3);
            }
            
            response[bytes_received] = '\0';
            // puts(response);
            fwrite(response, bytes_received, 1, html_output);
        }
    }
    else{
        while ((bytes_received = recv(sockfd, response, 1, 0)) > 0){
            response[bytes_received] = '\0';
            fwrite(response, bytes_received, 1, html_output);
        }
    }
    fclose(html_output);
    puts("HTML parsed \n");
}

void getHTMLImg(long long contentLength){

	puts("Parsing HTML Imagee\n");
	int bytes_received;

	html_output = fopen(logoName, "wb");
	// html_output = fopen("content.gif", "wb");
	if (contentLength > 0){
        while (contentLength--){
            bytes_received = recv(sockfd, response, 1, 0);
            if (bytes_received == -1)
            {
                perror("Did not receive");
                //exit(3);
            }
            response[bytes_received] = '\0';
            fwrite(response, bytes_received, 1, html_output);
        }
    }
    else{
        while ((bytes_received = recv(sockfd, response, 1, 0)) > 0){
            response[bytes_received] = '\0';
            fwrite(response, bytes_received, 1, html_output);
        }
    }
    fclose(html_output);
    puts("HTML parsed \n");
}

void getImageLocation(){
	// html_output = fopen("content.html","rb");
	html_output = fopen(fileName,"rb");
	char buffer[1024];
	char lol[1024];
	char* found = (char*)malloc(N);
	// puts("HERE");
	while(fgets(buffer,N,html_output)!=NULL){
		found = strstr(buffer,"IMG SRC");
		if(found != NULL){
			puts("HERE");
			puts(found);
			found = strstr(found,"\"");

			*found++;
			// puts(found);
			char temmp[N];
			strcpy(temmp,found);
			// puts(temmp);
			for(int i = 0 ;i< sizeof(temmp);++i){
				// printf("%d\n", i);
				if(temmp[i] == '\"'){
					// printf("%c\n",temmp[i]);
					temmp[i] = '\0';
					break;
				}
			}
			puts(temmp);
			path = (char*)malloc(N);
			strcpy(path,temmp);
			// puts("HERE");
			// strcpy(found,temmp);
			break;
		}
	}
	// strcpy(path,found);
	fclose(html_output);
	puts("Image Location found \n");

}
void getImg(){
	puts("Parsing Image \n");
	close(sockfd);
	getImageLocation();
	
	createSocket();

	getHost();

	connectToServer();

	createMessage();

	sendHTTP_request();	

	recieve_HTTP_response();

	printf("%s" , response);

	if(status == 2){
			long long contentLength = getContentLength();
			getHTMLImg(contentLength);
			puts("DONE");
	}
	else{
			puts("Closing Connection \n");
	}

	return;

}

int main(int argc,char* argv[]){
	

	url = (char*)malloc(N);
	
	host = (char*)malloc(N);
	strcpy(host,argv[1]);
	strcpy(url,argv[1]);

	proxy_IP = (char*)malloc(N);
	strcpy(proxy_IP,argv[2]);
	

	proxy_port = (char*)malloc(N);
	strcpy(proxy_port,argv[3]);
	
	pport = atoi(proxy_port);
	
	username = (char*)malloc(N);
	
	password = (char*)malloc(N);
	strcpy(username,argv[4]);
	strcpy(password,argv[5]);

	encode(username,password);

	fileName = (char*)malloc(N);
	strcpy(fileName,argv[6]);
	logoName = (char*)malloc(N);
	strcpy(logoName,argv[7]);

	

	
	while(1){
		separateHostandPath();
		
		
		createSocket();

		getHost();

		connectToServer();

		createMessage();


		sendHTTP_request();

		recieve_HTTP_response();

		printf("%s" , response);

		if(status==2){
			long long contentLength = getContentLength();
			getHTML(contentLength);

			char* checkHost = strstr(url,"info.in2p3.fr");
			if(checkHost !=NULL){
				getImg();
				close(sockfd);
				break;
			}
			

		}
		else if(status==3){
			getLocation();
			close(sockfd);
			continue;
		}
		else if(status==4){
			puts("Error Response from server. Closing Connection\n");
		}
		else{
			puts("Closing Connection \n");
		}
		close(sockfd);
		break;
	
	}
	

	

	return 0;


}
