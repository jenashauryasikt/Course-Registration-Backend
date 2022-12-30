/*
 * Credential Server: Verify user identity
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
#define serverM_UDP_Port 24099 // serverM UDP static port number = 24000 + 099 = 24099
#define serverC_UDP_Port 21099 // serverC UDP static port number = 21000 + 099 = 21099


// some important variable initializations
int cred_UDP_sockfd; 								// socket descriptors
struct sockaddr_in serverM_UDP_addr, serverC_addr;  // server addresses
char op_msg_user[128], op_msg_pwd[128]; 			// user credentials


// obtain database of authentication credentials from cred.txt
void creds_obtain(vector<pair<string,string>> *table){ // store username and password in vector of pair
	
	string line;
	string file = "cred.txt";
	ifstream readfile(file);
	if (!readfile.is_open()){cout<<"Not Working"<<endl;}
	while (getline(readfile, line)){
		// split the line at the first "," 
		string username = "";
		string password = "";
		int count = 0;
		for (int i=0; i<strlen(line.c_str())-1; i++){
			if (line[i] != ',' && count == 0){ 		// store username
				username = username + line[i];
			}
			else if (line[i] == ','){ 				// split at ','
				count++;
			}
			else if (line[i] != ',' && count == 1){ // store password
				password = password + line[i];
			}
		}
		(*table).push_back(make_pair(username,password));
	}
	readfile.close();
}


// serverC socket creation (UDP)
void make_SocketUDPserverC(){

	cred_UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // IPv4 UDP socket
	if (cred_UDP_sockfd == -1){
		perror("--CUSTOM ERROR-- --Credential Server-- CANNOT OPEN UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}

	// Initialize UDP connection to main server
	memset(&serverC_addr, 0, sizeof(serverC_addr)); // empty struct
	
	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverC_addr.sin_addr.s_addr)); //IPv4 host address
	// serverC_addr.sin_port = htons(serverC_UDP_Port); // credential server UDP port

	// for Ubuntu -- Need to check version compatibility in future
	serverC_addr.sin_family = AF_INET; // IPv4 
	serverC_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverC_addr.sin_port = htons(serverC_UDP_Port); // credential server UDP port
	
	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/

	// bind socket
	if (::bind(cred_UDP_sockfd, (struct sockaddr *) &serverC_addr, sizeof(serverC_addr)) == -1){  //CHECK
		perror("--CUSTOM ERROR-- --Credential Server-- CANNOT BIND UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}
}

/* -------------------------------------------------------------------------------------------------------------
 * ------------------------------------------MAIN FUNCTION------------------------------------------------------
 * -------------------------------------------------------------------------------------------------------------
**/

int main(){

	// Prepare login credetials vector of pairs
	vector<pair<string,string>> cred_database;
	creds_obtain(&cred_database);

	// Booting up
	make_SocketUDPserverC();
	cout<<"The ServerC is up and running using UDP on port "<<serverC_UDP_Port<<"."<<endl; // successful message for booting up

	while (true){

		 /* Beej's guide: 5.8 - sendto() and recvfrom()
		  * for init connection and binding
		**/
		// receive username from main server
		socklen_t serverM_UDP_addr_size = sizeof(serverM_UDP_addr); 		// to prevent Ipv4/IPv6 complications
		if (recvfrom(cred_UDP_sockfd, op_msg_user, sizeof(op_msg_user), 0, (struct sockaddr *) &serverM_UDP_addr, &serverM_UDP_addr_size) == -1){ //CHECK
			perror("--CUSTOM ERROR-- --Credential Server-- CANNOT RECEIVE ENCRYPTED USERNAME\n");
			exit(EXIT_FAILURE);
		}
		// receive password from main server
		if (recvfrom(cred_UDP_sockfd, op_msg_pwd, sizeof(op_msg_pwd), 0, (struct sockaddr *) &serverM_UDP_addr, &serverM_UDP_addr_size) == -1){ //CHECK
			perror("--CUSTOM ERROR-- --Credential Server-- CANNOT RECEIVE ENCRYPTED PASSWORD\n");
			exit(EXIT_FAILURE);
		}
		printf("The ServerC received an authentication request from the Main Server.\n");
		
		// verify username
		bool user_flag = false;
		int user_no = 0;
		for (; user_no<cred_database.size(); user_no++){
			if (strcmp(cred_database[user_no].first.c_str(),op_msg_user) == 0){
				user_flag = true;		// correct username
				break;
			}
		}

		// verify password for same index
		bool pwd_flag = false;
		if (user_flag == true){
			if (strcmp(cred_database[user_no].second.c_str(),op_msg_pwd) == 0){
				pwd_flag = true;		// correct password
			}
		}

		// authentication code
		int notif = user_flag + pwd_flag; //  0->username wrong, 1->password wrong, 2->success
		stringstream ss;
		ss << notif;
		string n_str = ss.str();
		char const* notif_str = n_str.c_str(); // char array of the int notif

		// send authentication response to main server
		if (sendto(cred_UDP_sockfd, notif_str, sizeof(notif_str), 0, (struct sockaddr *) &serverM_UDP_addr, serverM_UDP_addr_size) == -1){ //CHECK
			perror("--CUSTOM ERROR-- --Credential Server-- CANNOT SEND AUTHENTICATION RESULTS\n");
			exit(EXIT_FAILURE);
		}

		 /* Beej's guide: 5.8 - sendto() and recvfrom()
		  * for init connection and binding
		**/
		
		printf("The ServerC finished sending the response to the Main Server.\n");
	}
	

	/*-------------------------------------------------------END----------------------------------------------------*/

	close(cred_UDP_sockfd);
	return 0;
} 

