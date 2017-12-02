//***************************************
//Mark Maupin
//CS4310
//Project 2
//Implements join, leave, log, and list components of a C2C server
//****************************************

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <ctime>
#include <list>
#include <string>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <cstdint>
using namespace std;

/*
Member: tuple that holds the IP address and login time for active members
*/
struct Member{
	char agent_IP[20];
	timeval tv;
};

/*
ismember: returns true if the member is question is in the list data structure
false otherwise
*/
bool ismember(list<Member> members, struct sockaddr_in cl){
	for(list<Member>::iterator it = members.begin(); it != members.end(); ++it){
		if(strcmp(it->agent_IP, inet_ntoa(cl.sin_addr))==0)
			return true;
	}
	return false;
}

/*
receivedLog: Utility function for sending received messages to the log
*/
void receivedLog(struct timeval t, char* request, struct sockaddr_in cl){
	
	char tbuffer[80];
	struct tm * timeinfo;
	timeinfo = localtime(&t.tv_sec);
	strftime(tbuffer, 80, "%D %T:", timeinfo);

	ofstream log;
	log.open("log.txt", ios::app);
	log << "\n" << tbuffer << t.tv_usec/1000 << ": Received a \"" << request << "\" action from agent \"" << inet_ntoa(cl.sin_addr) << "\"\n";
	log.close();
}

/*
respondedLog: for sending responses to the log given they fit the following pattern
time|"responded to agent"|address|"with"|response
*/
void respondedLog(struct timeval t, string response, struct sockaddr_in cl){
	
	char tbuffer[80];
	struct tm * timeinfo;
	timeinfo = localtime(&t.tv_sec);
	strftime(tbuffer, 80, "%D %T:", timeinfo);

	ofstream log;
	log.open("log.txt", ios::app);
	log << "\n" << tbuffer << t.tv_usec/1000 << ": Responded to agent \"" << inet_ntoa(cl.sin_addr) << "\" with \"" << response << "\"\n";
	log.close();
}

int main(int argc, char *argv[]) {
	
	if(argc < 2){
		printf("Provide port in command line\n");
		exit(1);
	}
	
	int sockfd, newsockfd, portno;
	struct sockaddr_in server, client;
	socklen_t addrlen;
	char buffer[256];
	ofstream log;

	list<Member> members;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(sockfd < 0){
		fprintf(stderr, "Socket couldn't be opened\n");
		exit(1);
	}
	
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[1]));
	server.sin_addr.s_addr = INADDR_ANY;

	addrlen = sizeof(server);
	if(bind(sockfd, (struct sockaddr *) &server, addrlen) < 0){
		fprintf(stderr, "Error, failed to bind\n");
		exit(1);
	}
	
	listen(sockfd, 5);
	
	while(1) {

		addrlen = sizeof(client);
		newsockfd = accept(sockfd, (struct sockaddr *) &client, &addrlen);
		struct timeval now;
		gettimeofday(&now, NULL);
		bzero(buffer, 256);
		read(newsockfd, buffer, 256);

		receivedLog(now, buffer, client);

		if(strcmp(buffer, "#JOIN") == 0){
			
			if(ismember(members, client)){
				gettimeofday(&now, NULL);
				write(newsockfd, "$ALREADY MEMBER", 15);
				respondedLog(now, "$ALREADY MEMBER", client);				
			}else{
				Member temp;
				temp.tv = now;
				strcpy(temp.agent_IP, inet_ntoa(client.sin_addr));
				members.push_back(temp);
				gettimeofday(&now, NULL);
				write(newsockfd, "$OK", 3);
				respondedLog(now, "$OK", client);
			}

		}else if(strcmp(buffer, "#LEAVE") == 0){
			
			if(ismember(members, client)){
				
				for(list<Member>::iterator it = members.begin(); it != members.end();){
					if(strcmp(it->agent_IP, inet_ntoa(client.sin_addr)) == 0)
						it = members.erase(it);
					else
						++it;
				}
				
				gettimeofday(&now, NULL);
				write(newsockfd, "$OK", 3);
				respondedLog(now, "$OK", client);
			
			}else{
				
				gettimeofday(&now, NULL);
				write(newsockfd, "$NOT MEMBER", 11);
				respondedLog(now, "$NOT MEMBER", client);
			}

		}else if(strcmp(buffer, "#LIST") == 0){
			gettimeofday(&now, NULL);

			if(ismember(members, client)){
				for(list<Member>::iterator it = members.begin(); it != members.end(); ++it){
					bzero(buffer, 256);
					strcpy(buffer, "<");
					strcat(buffer, it->agent_IP);
					
					int dif = now.tv_sec - it->tv.tv_sec;
					
					strcat(buffer, ", ");
					strcat(buffer, to_string(dif).c_str());
					strcat(buffer, ">");
					write(newsockfd, buffer, strlen(buffer));
				}
				char tbuffer[80];
				struct tm * timeinfo;
				timeinfo = localtime(&now.tv_sec);
				strftime(tbuffer, 80, "%D %T:", timeinfo);

				ofstream log;
				log.open("log.txt", ios::app);
				log << "\n" << tbuffer << now.tv_usec/1000 << ": Responded to agent \"" << inet_ntoa(client.sin_addr) << "\" with the list\n";
				log.close();
			}else{
				char tbuffer[80];
				struct tm * timeinfo;
				timeinfo = localtime(&now.tv_sec);
				strftime(tbuffer, 80, "%D %T:", timeinfo);

				log.open("log.txt", ios::app);
				log << "\n" << tbuffer << now.tv_usec/1000 << ": No response is supplied to agent \"" << inet_ntoa(client.sin_addr) << "\" (agent not active)\n";
				log.close();
			}

		}else if(strcmp(buffer, "#LOG") ==0){
			gettimeofday(&now, NULL);

			if(ismember(members, client)){
				FILE *inFile;
				inFile = fopen("log.txt", "r");
				bzero(buffer, 256);
				while (fgets(buffer, 256, (FILE*)inFile)){
					write(newsockfd, buffer, strlen(buffer));
				}
				
				fclose(inFile);

				char tbuffer[80];
				struct tm * timeinfo;
				timeinfo = localtime(&now.tv_sec);
				strftime(tbuffer, 80, "%D %T:", timeinfo);

				log.open("log.txt", ios::app);
				log << "\n" << tbuffer << now.tv_usec/1000 << ": Responded to agent \"" << inet_ntoa(client.sin_addr) << "\" with the log\n";
				log.close();
			}else{
				char tbuffer[80];
				struct tm * timeinfo;
				timeinfo = localtime(&now.tv_sec);
				strftime(tbuffer, 80, "%D %T:", timeinfo);

				log.open("log.txt", ios::app);
				log << "\n" << tbuffer << now.tv_usec/1000 << ": No response is supplied to agent \"" << inet_ntoa(client.sin_addr) << "\" (agent not active)\n";
				log.close();
			}
		}
		close(newsockfd);
	}
}