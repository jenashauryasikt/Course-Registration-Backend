/*
 * EE Server: Store the information of courses offered by EE department
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
#define serverEE_UDP_Port 23099 // serverEE UDP static port number = 23000 + 099 = 23099


// some important variable initializations
int EE_UDP_sockfd; 									// socket descriptors
struct sockaddr_in serverEE_addr, serverM_UDP_addr; // server addresses
char qcode[1024], qcat[1024]; 						// queries for course code and info category
char mult_res[1024], sing_res[1024]; 				// send results to main server


// A C++ class to hold the information abour various courses
class EE_Course{

	public:
		char CourseCode[128]; // Course Code
		char Credits[128];    // Course credits
		char Professor[128];  // Course professor
		char Days[128];	   	  // Days on which classes are held
		char CourseName[128]; // Course title

};

/* 
 * Obtain information about courses from the ee.txt file
**/

void CourseInfo(vector<EE_Course> *table){

	string line;
	string file = "ee.txt";
	ifstream readfile(file);
	if (!readfile.is_open()){cout<<"Not Working"<<endl;}
	//EE_Course pr;
	while (getline(readfile, line)){
		EE_Course pr;
		string ccode = "";
		string cred = "";
		string prof = "";
		string d = "";
		string cname = "";
		int count = 0;
		for (int i=0; i<strlen(line.c_str())-1; i++){
			if (line[i] == ','){
				count++;
			}
			else if (line[i] != ',' && count == 0){ // store CourseCode
				ccode = ccode + line[i];
			}
			else if (line[i] != ',' && count == 1){ // store Credits
				cred = cred + line[i];
			}
			else if (line[i] != ',' && count == 2){ // store Professor
				prof = prof + line[i];
			}
			else if (line[i] != ',' && count == 3){ // store Days
				d = d + line[i];
			}
			else if (line[i] != ',' && count == 4){ // store CourseName
				cname = cname + line[i];
			}
			strncpy(pr.CourseCode, ccode.c_str(), strlen(ccode.c_str())+1); // set CourseCode for object
			strncpy(pr.Credits, cred.c_str(), strlen(cred.c_str())+1);     // set Credits for object
			strncpy(pr.Professor, prof.c_str(), strlen(prof.c_str())+1);   // set Professor for object
			strncpy(pr.Days, d.c_str(), strlen(d.c_str())+1);           	// set Days for object
			strncpy(pr.CourseName, cname.c_str(), strlen(cname.c_str())+1); // set CourseName for object
		}
		(*table).push_back(pr); // add object to vector
	}
}


// serverEE socket creation (UDP)
void make_SocketUDPserverEE(){

	EE_UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // IPv4 UDP socket 
	if (EE_UDP_sockfd == -1){						// CHECK
		perror("--CUSTOM ERROR-- --EE Server-- CANNOT OPEN UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}

	// Initialize UDP connection to main server
	memset(&serverEE_addr, 0, sizeof(serverEE_addr)); // empty struct
	
	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverEE_addr.sin_addr.s_addr)); // IPv4 host address
	// serverEE_addr.sin_port = htons(serverEE_UDP_Port); // credential server UDP port

	// for Ubuntu -- Need to check version compatibility in future
	serverEE_addr.sin_family = AF_INET; // IPv4 
	serverEE_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverEE_addr.sin_port = htons(serverEE_UDP_Port); // credential server UDP port

	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/

	// bind socket
	if (::bind(EE_UDP_sockfd, (struct sockaddr *) &serverEE_addr, sizeof(serverEE_addr)) == -1){  //CHECK
		perror("--CUSTOM ERROR-- --EE Server-- CANNOT BIND UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}
}

/* -------------------------------------------------------------------------------------------------------------
 * ------------------------------------------MAIN FUNCTION------------------------------------------------------
 * -------------------------------------------------------------------------------------------------------------
**/

int main(){

	// make database for EE courses
	vector<EE_Course> EE_table;
	CourseInfo(&EE_table);

	// Booting up
	make_SocketUDPserverEE();
	cout<<"The ServerEE is up and running using UDP on port "<<serverEE_UDP_Port<<"."<<endl; // successful message for booting up

	while (true){
		/*
		 * Beej's guide: 5.8 - sendto() and recvfrom()
		**/
		// receive course code from main server
		socklen_t serverM_UDP_addr_size = sizeof(serverM_UDP_addr); // to prevent Ipv4/IPv6 complications
		if (recvfrom(EE_UDP_sockfd, qcode, sizeof(qcode), 0, (struct sockaddr *) &serverM_UDP_addr, &serverM_UDP_addr_size) == -1){ //CHECK
			perror("--CUSTOM ERROR-- --EE Server-- CANNOT RECEIVE COURSE CODE\n");
			exit(EXIT_FAILURE);
		}

		// check marker for full course info
		if (qcode[0] == 'f'){  			  // proceed with full course info
			string tempqc = qcode;
			tempqc.erase(tempqc.begin()); // remove f marker
			char query_code[1024];
			strncpy(query_code, tempqc.c_str(), strlen(tempqc.c_str())+1);
			cout<<"The ServerEE received a request from the Main Server about "<<query_code<<"."<<endl;

			// verify course full info
			bool codeflag = false;
			int c_no = 0;
			for (; c_no<EE_table.size(); c_no++){
				if (strcmp(EE_table[c_no].CourseCode, query_code) == 0){
					codeflag = true; 	// course found
					break;
				}
			}

			string check; // result message
			string temp = query_code;

			if (codeflag == false){ 		// did not find the course
				check = "Didn't find the course: " + temp + ".\n";
				cout<<check;
			}
			else{ // found the course
				cout<<"The course information has been found for "<<query_code<<"."<<endl;
				string checkCode = EE_table[c_no].CourseCode;
				string checkCred = EE_table[c_no].Credits;
				string checkProf = EE_table[c_no].Professor;
				string checkDays = EE_table[c_no].Days;
				string checkName = EE_table[c_no].CourseName;
				check = checkCode + ": " + checkCred + ", " + checkProf + ", " + checkDays + ", " + checkName + "\n"; // full course info
			}

			// send result char array to the Main Server
			mult_res[0] = '\0';
			strncpy(mult_res, check.c_str(), strlen(check.c_str())+1);
			if (sendto(EE_UDP_sockfd, mult_res, sizeof(mult_res), 0, (struct sockaddr *) &serverM_UDP_addr, serverM_UDP_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --EE Server-- CANNOT SEND FULL COURSE INFO\n");
				exit(EXIT_FAILURE);
			}

			 /* Beej's guide: 5.8 - sendto() and recvfrom()
			  * for init connection and binding
			**/

			cout<<"The ServerEE finished sending the response to the main server."<<endl;
		}

		// single course
		else{ // receive category query
			if (recvfrom(EE_UDP_sockfd, qcat, sizeof(qcat), 0, (struct sockaddr *) &serverM_UDP_addr, &serverM_UDP_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --EE Server-- CANNOT RECEIVE CATEGORY QUERY\n");
				exit(EXIT_FAILURE);
			}

			/*
			 * Beej's guide: 5.8 - sendto() and recvfrom()
			**/
			
			cout<<"The ServerEE received a request from the Main Server about the "<<qcat<<" of "<<qcode<<"."<<endl;

			// verify course and categorical info
			bool codeflag = false;
			int c_no = 0;
			for (; c_no<EE_table.size(); c_no++){
				if (strcmp(EE_table[c_no].CourseCode, qcode) == 0){
					codeflag = true; // course found
					break;
				}
			}

			string check; // result message
			string check2; // helper string
			string temp = qcode;

			if (codeflag == false){ // did not find the course
				check = "Didn't find the course: " + temp + ".\n";
				cout<<check;
			}
			else{ // found the course
				string temp2 = qcat;
				if (temp2 == "Credit"){
					check2 = EE_table[c_no].Credits;
				}
				else if (temp2 == "Professor"){
					check2 = EE_table[c_no].Professor;
				}
				else if (temp2 == "Days"){
					check2 = EE_table[c_no].Days;
				}
				else if (temp2 == "CourseName"){
					check2 = EE_table[c_no].CourseName;
				}
				cout<<"The course information has been found: The "<<qcat<<" of "<<qcode<<" is "<<check2<<"."<<endl;
				check = "The " + temp2 + " of " + temp + " is " + check2 + ".";  // answer to query
			}

			// send result char array to the Main Server
			sing_res[0] = '\0'; // reset
			strncpy(sing_res, check.c_str(), strlen(check.c_str())+1);
			if (sendto(EE_UDP_sockfd, sing_res, sizeof(sing_res), 0, (struct sockaddr *) &serverM_UDP_addr, serverM_UDP_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --EE Server-- CANNOT SEND COURSE CATEGORICAL INFO\n");
				exit(EXIT_FAILURE);
			}

			 /* Beej's guide: 5.8 - sendto() and recvfrom()
			  * for init connection and binding
			**/

			cout<<"The ServerEE finished sending the response to the main server."<<endl;
		}

	}

	/*-------------------------------------------------------END----------------------------------------------------*/

	close(EE_UDP_sockfd);
	return 0;
}

