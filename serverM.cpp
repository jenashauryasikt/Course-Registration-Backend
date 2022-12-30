/*
 * Main Server: Coordinate with backend servers
**/

// mandatory header files
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>

// some helpful header files
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
// #include <filesystem>
#include <unistd.h>

using namespace std;


// allocate identities
#define localhost "127.0.0.1" //host address
#define serverM_TCP_Port 25099 // serverM TCP static port number = 25000 + 099 = 25099
#define serverM_UDP_Port 24099 // serverM UDP static port number = 24000 + 099 = 24099
#define serverEE_UDP_Port 23099 // serverEE UDP static port number = 23000 + 099 = 23099
#define serverCS_UDP_Port 22099 // serverCS UDP static port number = 22000 + 099 = 22099
#define serverC_UDP_Port 21099 // serverC UDP static port number = 21000 + 099 = 21099


// some important variable initializations
int main_TCP_sockfd, child_client_sockfd, main_UDP_sockfd; 													// socket descriptors
struct sockaddr_in serverM_addr, serverM_UDP_addr, client_addr, serverC_addr, serverEE_addr, serverCS_addr; // server addresses
char inp_msg_user[128], inp_msg_pwd[128]; 																    // comfortably allow for 5-50 char usernames and passwords
char op_msg_user[128], op_msg_pwd[128]; 																	// encrypt and send to credential server
int notif; 																									// Authentication encoding for login from serverC
char notif_str[32]; 																						// Character array for notif
char query_code[1024], query_cat[1024]; 																	// to allow sending big strings as messages
char res_str[1024]; 																						// to receive the query results of one course
char fin_res[8192]; 																						// to send the final result to the client

char lb[2] = "\n";      // line break character assignment


// Create TCP socket to connect with client
void make_MainSocketTCP(){

	main_TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 TCP socket
	if (main_TCP_sockfd == -1){							// CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT OPEN MAIN TCP SOCKET\n");
		exit(EXIT_FAILURE);
	}

	// Initialize TCP connection with client
	memset(&serverM_addr, 0, sizeof(serverM_addr)); // empty struct

	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverM_addr.sin_addr.s_addr)); // IPv4 host address
	// serverM_addr.sin_port = htons(serverM_TCP_Port); // main server TCP port for client connection/*

	// for Ubuntu -- Need to check version compatibility in future
	serverM_addr.sin_family = AF_INET; // IPv4 
	serverM_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverM_addr.sin_port = htons(serverM_TCP_Port); // main server TCP port for client connection
		
	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/

	// bind socket
	if (::bind(main_TCP_sockfd, (struct sockaddr *) &serverM_addr, sizeof(serverM_addr)) == -1){  //CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT BIND MAIN TCP SOCKET\n");
		exit(EXIT_FAILURE);
	}
}


// Listen for client TCP request
void client_listen(){

	if (listen(main_TCP_sockfd, 5) == -1){					// Choose to keep 5 clients queued at max
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT LISTEN FOR CLIENT TCP SOCKET\n");
		exit(EXIT_FAILURE);
	}
	/* Beej's guide: 5.5 - listen()
	 * for init connection and binding
	**/
}


// Create UDP socket for backend servers
void make_MainSocketUDP(){

	main_UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // IPv4 UDP socket 
	if (main_UDP_sockfd == -1){						// CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT OPEN MAIN UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}

	// Initialize
	memset(&serverM_UDP_addr, 0, sizeof(serverM_UDP_addr)); //empty struct
	
	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverM_UDP_addr.sin_addr.s_addr)); // IPv4 host address
	// serverM_UDP_addr.sin_port = htons(serverM_UDP_Port); // main server UDP port
	
	// for Ubuntu -- Need to check version compatibility in future
	serverM_UDP_addr.sin_family = AF_INET; // IPv4 
	serverM_UDP_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverM_UDP_addr.sin_port = htons(serverM_UDP_Port); // main server UDP port

	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/

	// bind socket
	if (::bind(main_UDP_sockfd, (struct sockaddr *) &serverM_UDP_addr, sizeof(serverM_UDP_addr)) == -1){ //CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT BIND MAIN UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Initialize UDP connection to backend servers 
**/

// Credential Server
void init_connect_serverC(){

	memset(&serverC_addr, 0, sizeof(serverC_addr)); //empty struct

	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverC_addr.sin_addr.s_addr)); // IPv4 host address
	// serverC_addr.sin_port = htons(serverC_UDP_Port); // Credential server UDP port
	
	// for Ubuntu -- Need to check version compatibility in future
	serverC_addr.sin_family = AF_INET; // IPv4 
	serverC_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverC_addr.sin_port = htons(serverC_UDP_Port); // Credential server UDP port

	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/
}


// EE Server
void init_connect_serverEE(){

	memset(&serverEE_addr, 0, sizeof(serverEE_addr)); //empty struct
	
	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverEE_addr.sin_addr.s_addr)); // IPv4 host address
	// serverEE_addr.sin_port = htons(serverEE_UDP_Port); // EE server UDP port
	
	// for Ubuntu -- Need to check version compatibility in future
	serverEE_addr.sin_family = AF_INET; // IPv4 
	serverEE_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverEE_addr.sin_port = htons(serverEE_UDP_Port); // EE server UDP port
	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/
}


// Send query with category to EE Server (for single course)
void sendEEcat(){
	init_connect_serverEE();
	if (sendto(main_UDP_sockfd, query_code, sizeof(query_code), 0, (struct sockaddr *) &serverEE_addr, sizeof(serverEE_addr)) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE CODE TO EE SERVER\n");
		exit(EXIT_FAILURE);
	}
	if (sendto(main_UDP_sockfd, query_cat, sizeof(query_cat), 0, (struct sockaddr *) &serverEE_addr, sizeof(serverEE_addr)) == -1){ // CHECK
	perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE QUERY TO EE SERVER\n");
	exit(EXIT_FAILURE);
	}

	 /* Beej's guide: 5.8 - sendto() and recvfrom()
	  * for init connection and binding
	**/

}

// Send query for full course info to EE Server (for multiple courses)
void sendEEf(char testqc[1024]){
	init_connect_serverEE();
	if (sendto(main_UDP_sockfd, testqc, sizeof(testqc), 0, (struct sockaddr *) &serverEE_addr, sizeof(serverEE_addr)) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE CODE TO EE SERVER\n");
		exit(EXIT_FAILURE);
	}

	 /* Beej's guide: 5.8 - sendto() and recvfrom()
	  * for init connection and binding
	**/

}


// CS Server
void init_connect_serverCS(){
	memset(&serverCS_addr, 0, sizeof(serverCS_addr)); //empty struct

	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverCS_addr.sin_addr.s_addr)); // IPv4 host address
	// serverCS_addr.sin_port = htons(serverCS_UDP_Port); // CS server UDP port
	
	// for Ubuntu -- Need to check version compatibility in future
	serverCS_addr.sin_family = AF_INET; // IPv4 
	serverCS_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverCS_addr.sin_port = htons(serverCS_UDP_Port); // CS server UDP port
	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/
}


// Send query with category to CS Server (for single course)
void sendCScat(){
	init_connect_serverCS();
	if (sendto(main_UDP_sockfd, query_code, sizeof(query_code), 0, (struct sockaddr *) &serverCS_addr, sizeof(serverCS_addr)) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE CODE TO CS SERVER\n");
		exit(EXIT_FAILURE);
	}
	if (sendto(main_UDP_sockfd, query_cat, sizeof(query_cat), 0, (struct sockaddr *) &serverCS_addr, sizeof(serverCS_addr)) == -1){ // CHECK
	perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE QUERY TO CS SERVER\n");
	exit(EXIT_FAILURE);
	}

	 /* Beej's guide: 5.8 - sendto() and recvfrom()
	  * for init connection and binding
	**/

}

// Send query for full course info to CS Server (for multiple courses)
void sendCSf(char testqc[1024]){
	init_connect_serverCS();
	if (sendto(main_UDP_sockfd, testqc, sizeof(testqc), 0, (struct sockaddr *) &serverCS_addr, sizeof(serverCS_addr)) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE CODE TO CS SERVER\n");
		exit(EXIT_FAILURE);
	}

	 /* Beej's guide: 5.8 - sendto() and recvfrom()
	  * for init connection and binding
	**/

}


// Encryption Scheme offset each character/digit by 4, special characters unaffected
void encrypt_msg(char msg[128]){

	int size_str = strlen(msg);
	for (int i=0; i<size_str; i++){
		if (msg[i]>='a' && msg[i]<='z'){
			int temp1 = (msg[i]-'a'+4) % 26; // offset by 4
			msg[i] = temp1 + 'a';
		}
		else if (msg[i]>='A' && msg[i]<='Z'){
			int temp2 = (msg[i]-'A'+4) % 26; // offset by 4
			msg[i] = temp2 + 'A';
		}
		else if (msg[i]>='0' && msg[i]<='9'){
			int temp3 = (msg[i]-'0'+4) % 10; // offset by 4
			msg[i] = temp3 + '0';
		}
		else {continue;}
	}
}


// Break multiple course query to separate queries
void separate_queries(vector<string> *asks){

	string ask;
	string qc = query_code;
	for (int i=0; i<strlen(qc.c_str()); i++){
		ask = "f"; 							//set marker for full info on the course
		while(qc[i] != ' ' && i<strlen(qc.c_str())){
			ask = ask + qc[i];
			i++;
		}
		(*asks).push_back(ask);
	}
}

/* -------------------------------------------------------------------------------------------------------------
 * ------------------------------------------MAIN FUNCTION------------------------------------------------------
 * -------------------------------------------------------------------------------------------------------------
**/

int main(){

	
	// Booting up
	make_MainSocketTCP();
	make_MainSocketUDP();
	printf("The main server is up and running.\n"); // success message for main server booting up

	// While loop for 2 processes of authentication and query resolution.
	while (true){


		// Accept client TCP connection with child for communication
		/*
		 * Beej's guide: 5.6 - accept()
		 * struct sockaddr_in and socklen_t used
		**/
		client_listen();

		socklen_t client_addr_size = sizeof(client_addr); 						// to prevent Ipv4/IPv6 complications
		child_client_sockfd = accept(main_TCP_sockfd, (struct sockaddr *) &client_addr, &client_addr_size); 
		if (child_client_sockfd == -1){											// CHECK
			perror("--CUSTOM ERROR-- --Main Server-- CANNOT ACCEPT CLIENT CONNECTION\n");
			exit(EXIT_FAILURE);
		}

		// operate with client and credential servers for login
		bool flag1 = true;
		while (flag1){
			/*
			 * Beej's guide: 5.7 - send() and receive()
			**/
			// receive username from client
			if (recv(child_client_sockfd, inp_msg_user, sizeof(inp_msg_user), 0) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE CLIENT USERNAME\n");
				exit(EXIT_FAILURE);
			}
			// receive password from client
			if (recv(child_client_sockfd, inp_msg_pwd, sizeof(inp_msg_pwd), 0) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE CLIENT PASSWORD\n");
				exit(EXIT_FAILURE);
			}
			cout<<"The main server received the authentication for "<<inp_msg_user<<" using TCP over port "<<serverM_TCP_Port<<"."<<endl;
			
			strncpy(op_msg_user, inp_msg_user, strlen(inp_msg_user)+1);
			strncpy(op_msg_pwd, inp_msg_pwd, strlen(inp_msg_pwd)+1);
			// encrypt auth messages
			encrypt_msg(op_msg_user);
			encrypt_msg(op_msg_pwd);
			
			// send to credential server
			init_connect_serverC();
			if (sendto(main_UDP_sockfd, op_msg_user, sizeof(op_msg_user), 0, (struct sockaddr *) &serverC_addr, sizeof(serverC_addr)) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND USERNAME TO CREDENTIAL SERVER\n");
				exit(EXIT_FAILURE);
			}
			if (sendto(main_UDP_sockfd, op_msg_pwd, sizeof(op_msg_pwd), 0, (struct sockaddr *) &serverC_addr, sizeof(serverC_addr)) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND PASSWORD TO CREDENTIAL SERVER\n");
				exit(EXIT_FAILURE);
			}

			 /* Beej's guide: 5.8 - sendto() and recvfrom()
			  * for init connection and binding
			**/

			printf("The main server sent an authentication request to serverC.\n");

			// Receive authentication results from the credential server
			socklen_t serverC_addr_size = sizeof(serverC_addr); // to prevent Ipv4/IPv6 complications
			if (recvfrom(main_UDP_sockfd, notif_str, sizeof(notif_str), 0, (struct sockaddr *) &serverC_addr, &serverC_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE AUTH RESULTS FROM CREDENTIAL SERVER\n");
				exit(EXIT_FAILURE);
			}

			 /* Beej's guide: 5.8 - sendto() and recvfrom()
			  * for init connection and binding
			**/

			cout<<"The main server received the result of the authentication request from ServerC using UDP over port "<<serverM_UDP_Port<<"."<<endl;

			// Send authentication results to the client
			if (send(child_client_sockfd, notif_str, sizeof(notif_str), 0) == -1){ 			// CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND AUTH RESULTS TO THE CLIENT\n");
				exit(EXIT_FAILURE);
			}

			/* Beej's guide: 5.7 - send() and recv()
			 * for init connection and binding
			**/

			printf("The main server sent the authentication result to the client.\n");

			int res = atoi(notif_str);
			if (res==2){						// implies successful login
				flag1 = false; 					// exit authentication loop for query loop now
			}
		}


		// operate with client and department backend servers for course queries
		bool flag2 = true;
		while (flag2){

			/*
			 * Beej's guide: 5.7 - send() and receive()
			**/
			// receive course code from client
			if (recv(child_client_sockfd, query_code, sizeof(query_code), 0) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE COURSE CODE INFO\n");
				exit(EXIT_FAILURE);
			}

			bool multiple_courses = true;  		// flag for multiple courses query
			
			if (strlen(query_code)<9){			// implies single course
				multiple_courses = false;
				// receive course info query from client
				if (recv(child_client_sockfd, query_cat, sizeof(query_cat), 0) == -1){ // CHECK
					perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE COURSE QUERY INFO\n");
					exit(EXIT_FAILURE);
				}
				cout<<"The main server received from "<<inp_msg_user<<" to query course "<<query_code<<
				" about "<<query_cat<<" using TCP over port "<<serverM_TCP_Port<<"."<<endl;
			}

			else{								// multiple courses
				cout<<"The main server received from "<<inp_msg_user<<" to query courses "<<query_code<<
				" using TCP over port "<<serverM_TCP_Port<<"."<<endl;
			}

			// send queries to appropriate backend server
			vector<string> queries; // in case multiple courses

			// single course
			if (multiple_courses == false){
				if (query_code[0]=='E'){ // EE Server
					sendEEcat();
					printf("The main server sent a request to serverEE.\n");
				}
				else { // CS Server, will also handle invalid course codes not starting with E
					sendCScat();
					printf("The main server sent a request to serverCS.\n");
				}
			}

			// multiple courses
			else { 
				separate_queries(&queries); 						// each course code treated as a separate query
				sort(queries.begin(), queries.end());
				queries.erase(unique(queries.begin(),queries.end()), queries.end()); // get rid of duplicate entries
				int n = 0;
				while(n < queries.size()){
					char qcode[1024];
					strncpy(qcode, queries[n].c_str(), 1024);
					if (queries[n][1] == 'E'){ 					// EE Server
						sendEEf(qcode); 						// no categorical information
						cout<<"The main server sent a request to serverEE."<<endl;
					}
					else{ 										// CS Server, also handles invalid course codes
						sendCSf(qcode); 						// no categorical information
						cout<<"The main server sent a request to serverCS."<<endl;
					}
					n++;
				}
			}

			// Receive results from backend department servers
			
			// single course
			if (multiple_courses == false){
				fin_res[0] ='\0';
				if (query_code[0]=='E'){ 								  // EE server
					socklen_t serverEE_addr_size = sizeof(serverEE_addr); // to prevent Ipv4/IPv6 complications
					res_str[0] = '\0';
					if (recvfrom(main_UDP_sockfd, res_str, sizeof(res_str), 0, (struct sockaddr *) &serverEE_addr, &serverEE_addr_size) == -1){ // CHECK
						perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE CATEGORICAL RESULTS FROM EE SERVER\n");
						exit(EXIT_FAILURE);
					}
					cout<<"The main server received the response from the serverEE using UDP over port "<<serverM_UDP_Port<<"."<<endl;
				}
				else{ 													  // CS server
					socklen_t serverCS_addr_size = sizeof(serverCS_addr); // to prevent Ipv4/IPv6 complications
					res_str[0] = '\0';
					if (recvfrom(main_UDP_sockfd, res_str, sizeof(res_str), 0, (struct sockaddr *) &serverCS_addr, &serverCS_addr_size) == -1){ // CHECK
						perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE CATEGORICAL RESULTS FROM CS SERVER\n");
						exit(EXIT_FAILURE);
					}
					cout<<"The main server received the response from the serverCS using UDP over port "<<serverM_UDP_Port<<"."<<endl;
				}

					 /* Beej's guide: 5.8 - sendto() and recvfrom()
					  * for init connection and binding
					**/

				strncat(fin_res, res_str, strlen(res_str)+1); // to be sent to the client
				strncat(fin_res, lb, 2); 					  // insert line break at end
			}

			// multiple courses
			else{
				fin_res[0] = '\0';
				int m = 0;
				while(m < queries.size()){
					res_str[0] = '\0'; 										// set empty before getting new results
					if (queries[m][1] == 'E'){ 								// EE server
						socklen_t serverEE_addr_size = sizeof(serverEE_addr); // to prevent Ipv4/IPv6 complications
						if (recvfrom(main_UDP_sockfd, res_str, sizeof(res_str), 0, (struct sockaddr *) &serverEE_addr, &serverEE_addr_size) == -1){ // CHECK
							perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE CATEGORICAL RESULTS FROM EE SERVER\n");
							exit(EXIT_FAILURE);
						}
					}
					else{ 													  // CS server
						socklen_t serverCS_addr_size = sizeof(serverCS_addr); // to prevent Ipv4/IPv6 complications
						if (recvfrom(main_UDP_sockfd, res_str, sizeof(res_str), 0, (struct sockaddr *) &serverCS_addr, &serverCS_addr_size) == -1){ // CHECK
							perror("--CUSTOM ERROR-- --Main Server-- CANNOT RECEIVE CATEGORICAL RESULTS FROM CS SERVER\n");
							exit(EXIT_FAILURE);
						}
					}

					 /* Beej's guide: 5.8 - sendto() and recvfrom()
					  * for init connection and binding
					**/
					
					strncat(fin_res, res_str, strlen(res_str)+1);
					m++;
				}
			}

			// Send the final result to the client
			if (send(child_client_sockfd, fin_res, sizeof(fin_res), 0) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --Main Server-- CANNOT SEND COURSE INFO TO THE CLIENT\n");
				exit(EXIT_FAILURE);
			}

			/* Beej's guide: 5.7 - send() and recv()
			 * for init connection and binding
			**/

			cout<<"The main server sent the query information to the client."<<endl;
		}
		close(child_client_sockfd);
	}
	
	/*-------------------------------------------------------END----------------------------------------------------*/

	close(main_UDP_sockfd);
	close(main_TCP_sockfd);
	return 0;
}

