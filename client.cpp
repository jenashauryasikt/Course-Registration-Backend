/*
 * Client: Used by a student to access the registration system
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
#define localhost "127.0.0.1" // host address
#define serverM_TCP_Port 25099 // serverM TCP static port number = 25000 + 099 = 25099


// some important variable initializations
int client_TCP_sockfd; 								// client's TCP socket descriptor
struct sockaddr_in serverM_addr, my_addr; 			// main server address
string username, password; 							// authentication credentials
char auth_msg_user[128], auth_msg_pwd[128]; 		// comfortably allow for 5-50 char usernames and passwords
int notif; 											// Login authentication encoded results
char notif_str[32]; 								// Character array for notif
string courseCode, courseCat; 						// course info query inputs
char query_code[1024], query_cat[1024]; 			// to allow sending big strings as messages
char q_results[8192]; 								// to receive query results
char myIP[16]; 										// for client IP address
unsigned int myPort; 								// to retrieve dynamically assigned TCP port number of client


// Client socket creation (TCP)
void make_ClientSocketTCP(){

	client_TCP_sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPv4 TCP socket 
	if (client_TCP_sockfd == -1){						// CHECK
		perror("--CUSTOM ERROR-- --Client-- CANNOT OPEN CLIENT TCP SOCKET\n");
		exit(EXIT_FAILURE);
	}

	// Initialize TCP connection to main server
	memset(&serverM_addr, 0, sizeof(serverM_addr)); // empty struct

	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverM_addr.sin_addr.s_addr)); // IPv4, host address
	// serverM_addr.sin_port = htons(serverM_TCP_Port); // main server TCP port for client connection

	// // for Ubuntu -- Need to check version compatibility in future
	serverM_addr.sin_family = AF_INET; // IPv4 
	serverM_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverM_addr.sin_port = htons(serverM_TCP_Port); // main server TCP port for client connection
	
	/* Beej's guide: 5.3 - bind()
	 * for init connection and binding
	**/

	// Request TCP connection to main server
	connect(client_TCP_sockfd, (struct sockaddr *) &serverM_addr, sizeof(serverM_addr));

	/* Beej's guide: 5.4 - connect()
	 * for init connection and binding
	**/

}


/*--------------Code sampled from listnukira's (GitHub) get_ip_v1.c file--------------*/
// Get client TCP port
void getPort_TCP(){

	memset(&my_addr, 0, sizeof(my_addr)); 	// empty struct
	socklen_t len = sizeof(my_addr);
	int aux_port = getsockname(client_TCP_sockfd, (struct sockaddr *) &my_addr, &len);
	if(aux_port==-1) {						// CHECK
		perror("--CUSTOM ERROR-- --Client-- CANNOT READ TCP PORT NUMBER\n");
		exit(EXIT_FAILURE);
	}
	inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
	myPort = ntohs(my_addr.sin_port); // TCP port obtained
}


// Unencrypted username input
void req_username(){

	cout<<"Please enter the username: ";
	// cin.ignore(); 		// to ignore \n or \r if there is a cin>> before this getline
	getline(cin, username); 
}


// Unencrypted password input
void req_password(){

	cout<<"Please enter the password: ";
	// cin.ignore(); 		// to ignore \n or \r if there is a cin>> before this getline
	getline(cin, password); 
	usleep(500000); 		// to buffer the time for the sockets to be in place
}


// send auth request to main server
void send_auth(string usn, string pwd){

	// send username
	strncpy(auth_msg_user, usn.c_str(), 128);
	if (send(client_TCP_sockfd, auth_msg_user, sizeof(auth_msg_user), 0) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Client-- CANNOT SEND USERNAME\n");
		exit(EXIT_FAILURE);
	}
	//send password
	strncpy(auth_msg_pwd, pwd.c_str(), 128);
	if (send(client_TCP_sockfd, auth_msg_pwd, sizeof(auth_msg_pwd), 0) == -1){ // CHECK
		perror("-- CUSTOM ERROR-- --Client-- CANNOT SEND PASSWORD\n");
		exit(EXIT_FAILURE);
	}

	/* Beej's guide: 5.7 - send() and recv()
	 * for init connection and binding
	**/

	cout<<usn<<" sent an authentication request to the main server."<<endl;    // <Username> sent an authentication request to the main server.
}


// Receive auth results
int auth_results(){

	if(recv(client_TCP_sockfd, notif_str, sizeof(notif_str), 0) == -1){		 // CHECK
		perror("--CUSTOM ERROR-- --Client-- CANNOT RECEIVE AUTHENTICATION RESULTS.\n");
		exit(EXIT_FAILURE);
	}

	/* Beej's guide: 5.7 - send() and recv()
	 * for init connection and binding
	**/
	
	int res = atoi(notif_str); // convert to int to be read as token
	return res;                // 0-> username wrong, 1-> password wrong, 2-> successful
}


// Authnentication process
int auth_user(){

	int count = 0; 								// count of login attempts
	while (true){
		if(count == 3){
			break;								// shut down after 3 failed login attempts
		}
		req_username();
		req_password();
		
		send_auth(username, password);			// send auth request to main server
		
		int auth_ans = auth_results();			// receive auth results
		if (auth_ans == 2){ 					// successful login
			cout<<username<<" received the result of authentication using TCP over port "<<myPort<<". Authentication is successful."<<endl;
			break;
		}
		else if (auth_ans == 1){				// wrong password
			cout<<username<<" received the result of authentication using TCP over port "<<myPort<<". Authentication failed: Password does not match"<<endl;
			count++;
			cout<<"Attempts remaining: "<<3-count<<endl;
		}
		else if (auth_ans == 0){				// wrong username
			cout<<username<<" received the result of authentication using TCP over port "<<myPort<<". Authentication failed: Username Does not exist"<<endl;
			count++;
			cout<<"Attempts remaining: "<<3-count<<endl;
		}
	}
	return count;								// number of failed login attempts registered
}


// Course code query input
void req_course(){

	cout<<"Please enter the course code to query: ";
	// cin.ignore(); 		// to ignore \n or \r if there is a cin>> before this getline
	getline(cin, courseCode); // can handle multiple course queries with gap
}


// Course info query input
void req_info(){

	cout<<"Please enter the category (Credit / Professor / Days / CourseName): ";
	// cin.ignore(); 		// to ignore \n or \r if there is a cin>> before this getline
	getline(cin, courseCat);
	if (!(courseCat=="Credit"||courseCat=="Professor"||courseCat=="Days"||courseCat=="CourseName")){
		printf("Invalid Category, enter again.\n");
		req_info();
	}
}


// send course query to main server
void send_queryc(string cCode, string cCat){

	// send course code
	strncpy(query_code, cCode.c_str(), 1024);
	if (send(client_TCP_sockfd, query_code, sizeof(query_code), 0) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Client-- CANNOT SEND COURSE CODE\n");
		exit(EXIT_FAILURE);
	}
	// send course category
	if (cCat == ""){													// implying multiple courses
		int check1 = 1; 												// if category empty
		cout<<username<<" sent a request with multiple CourseCode to the main server."<<endl;
	}
	else{																// single course
		strncpy(query_cat, cCat.c_str(), 1024);							// send category as well
		if (send(client_TCP_sockfd, query_cat, sizeof(query_cat), 0) == -1){ // CHECK
			perror("--CUSTOM ERROR-- --Client-- CANNOT SEND INFO CATEGORY\n");
			exit(EXIT_FAILURE);
		}
		cout<<username<<" sent a request to the main server."<<endl;
	}

	/* Beej's guide: 5.7 - send() and recv()
	 * for init connection and binding
	**/

}


// Receive course query results
void query_results(){

	if(recv(client_TCP_sockfd, q_results, sizeof(q_results), 0) == -1){ // CHECK
		perror("--CUSTOM ERROR-- --Client-- CANNOT RECEIVE COURSE QUERY RESULTS.\n");
		exit(EXIT_FAILURE);
	}

	/* Beej's guide: 5.7 - send() and recv()
	 * for init connection and binding
	**/

	cout<<"The client received the response from the Main server using TCP over port "<<myPort<<"."<<endl;
}



/* -------------------------------------------------------------------------------------------------------------
 * ------------------------------------------MAIN FUNCTION------------------------------------------------------
 * -------------------------------------------------------------------------------------------------------------
**/

int main(){

	// establish connection with the main server
	make_ClientSocketTCP();
	getPort_TCP();
	printf("The client is up and running.\n"); 		// success message for the client booting up

	int attempts = auth_user();
	if (attempts == 3){								// 3 registered attempts that failed
		printf("Authentication Failed for 3 attempts. Client will shut down.\n"); // close client
		close(client_TCP_sockfd);
		return 0;
	}

	// Enter the course query phase
	while (true){

		courseCode = ""; // reset value for further requests
		courseCat = "";  // reset value for further requests
		
		// Course Code
		req_course();
		if (strlen(courseCode.c_str())<9){ // implying one course
			// Category of information
			req_info();
		}

		// send to main server
		send_queryc(courseCode, courseCat);

		// receive query results
		query_results();

		if (strlen(courseCode.c_str())>9){ // implying multiple courses query
			cout<<"CourseCode: Credits, Professor, Days, Course Name"<<endl;
		}
		cout<<q_results<<endl;
		printf("-----Start a new request-----\n"); 	// re-run the loop for next query
	}

	/*-------------------------------------------------------END----------------------------------------------------*/

	close(client_TCP_sockfd);
	return 0;
}

