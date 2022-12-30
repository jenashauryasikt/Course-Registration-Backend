/*
 * CS Server: Store the information of courses offered by CS department
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
#define serverCS_UDP_Port 22099 // serverCS UDP static port number = 22000 + 099 = 22099


// some important variable initializations
int CS_UDP_sockfd; 									// socket descriptors
struct sockaddr_in serverCS_addr, serverM_UDP_addr; // server addresses
char qcode[1024], qcat[1024]; 						// queries for course code and info category
char mult_res[1024], sing_res[1024]; 				// send results to main server


// A C++ class to hold the information abour various courses
class CS_Course{

	public:
		char CourseCode[128]; // Course Code
		char Credits[128];    // Course credits
		char Professor[128];  // Course professor
		char Days[128];	      // Days on which classes are held
		char CourseName[128]; // Course title

};

/* 
 * Obtain information about courses from the cs.txt file
**/

void CourseInfo(vector<CS_Course> *table){

	string line;
	string file = "cs.txt";
	ifstream readfile(file);
	if (!readfile.is_open()){cout<<"Not Working"<<endl;}
	//CS_Course pr;
	while (getline(readfile, line)){
		CS_Course pr;
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


// serverCS socket creation (UDP)
void make_SocketUDPserverCS(){

	CS_UDP_sockfd = socket(AF_INET, SOCK_DGRAM, 0); // IPv4 UDP socket 
	if (CS_UDP_sockfd == -1){						// CHECK
		perror("--CUSTOM ERROR-- --CS Server-- CANNOT OPEN UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}

	// Initialize UDP connection to main server
	memset(&serverCS_addr, 0, sizeof(serverCS_addr)); // empty struct

	// // for MacOS (like author) -- Need to check version compatibility in future
	// inet_pton(AF_INET, localhost, &(serverCS_addr.sin_addr.s_addr)); // IPv4 host address
	// serverCS_addr.sin_port = htons(serverCS_UDP_Port); // credential server UDP port

	// for Ubuntu -- Need to check version compatibility in future
	serverCS_addr.sin_family = AF_INET; // IPv4 
	serverCS_addr.sin_addr.s_addr = inet_addr(localhost); // host address
	serverCS_addr.sin_port = htons(serverCS_UDP_Port); // credential server UDP port

	 /* Beej's guide: 5.3 - bind()
	  * for init connection and binding
	**/

	// bind socket
	if (::bind(CS_UDP_sockfd, (struct sockaddr *) &serverCS_addr, sizeof(serverCS_addr)) == -1){  //CHECK
		perror("--CUSTOM ERROR-- --CS Server-- CANNOT BIND UDP SOCKET\n");
		exit(EXIT_FAILURE);
	}
}

/* -------------------------------------------------------------------------------------------------------------
 * ------------------------------------------MAIN FUNCTION------------------------------------------------------
 * -------------------------------------------------------------------------------------------------------------
**/

int main(){

	// make database for CS courses
	vector<CS_Course> CS_table;
	CourseInfo(&CS_table);

	// Booting up
	make_SocketUDPserverCS();
	cout<<"The ServerCS is up and running using UDP on port "<<serverCS_UDP_Port<<"."<<endl; // successful message for booting up

	while (true){
		/*
		 * Beej's guide: 5.8 - sendto() and recvfrom()
		**/
		// receive course code from main server
		socklen_t serverM_UDP_addr_size = sizeof(serverM_UDP_addr); 	// to prevent Ipv4/IPv6 complications
		if (recvfrom(CS_UDP_sockfd, qcode, sizeof(qcode), 0, (struct sockaddr *) &serverM_UDP_addr, &serverM_UDP_addr_size) == -1){ // CHECK
			perror("--CUSTOM ERROR-- --CS Server-- CANNOT RECEIVE COURSE CODE\n");
			exit(EXIT_FAILURE);
		}

		// check marker for full course info
		if (qcode[0] == 'f'){  // proceed with full course info
			string tempqc = qcode;
			tempqc.erase(tempqc.begin()); //remove f marker
			char query_code[1024];
			strncpy(query_code, tempqc.c_str(), 1024);
			cout<<"The ServerCS received a request from the Main Server about "<<query_code<<"."<<endl;

			// verify course full info
			bool codeflag = false;
			int c_no = 0;
			for (; c_no<CS_table.size(); c_no++){
				if (strcmp(CS_table[c_no].CourseCode, query_code) == 0){
					codeflag = true; // course found
					break;
				}
			}

			string check; // result message
			string temp = query_code;

			if (codeflag == false){ 	// did not find the course
				check = "Didn't find the course: " + temp + ".\n";
				cout<<check;
			}
			else{ // found the course
				cout<<"The course information has been found for "<<query_code<<"."<<endl;
				string checkCode = CS_table[c_no].CourseCode;
				string checkCred = CS_table[c_no].Credits;
				string checkProf = CS_table[c_no].Professor;
				string checkDays = CS_table[c_no].Days;
				string checkName = CS_table[c_no].CourseName;
				check = checkCode + ": " + checkCred + ", " + checkProf + ", " + checkDays + ", " + checkName + "\n"; // full course info
			}

			// send result char array to the Main Server
			mult_res[0] = '\0';
			strncpy(mult_res, check.c_str(), strlen(check.c_str())+1);
			if (sendto(CS_UDP_sockfd, mult_res, sizeof(mult_res), 0, (struct sockaddr *) &serverM_UDP_addr, serverM_UDP_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --CS Server-- CANNOT SEND FULL COURSE INFO\n");
				exit(EXIT_FAILURE);
			}

			/*
			 * Beej's guide: 5.8 - sendto() and recvfrom()
			**/

			cout<<"The ServerCS finished sending the response to the main server."<<endl;
		}

		// single course
		else{ // receive category query
			if (recvfrom(CS_UDP_sockfd, qcat, sizeof(qcat), 0, (struct sockaddr *) &serverM_UDP_addr, &serverM_UDP_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --CS Server-- CANNOT RECEIVE CATEGORY QUERY\n");
				exit(EXIT_FAILURE);
			}

			/*
			 * Beej's guide: 5.8 - sendto() and recvfrom()
			**/

			cout<<"The ServerCS received a request from the Main Server about the "<<qcat<<" of "<<qcode<<"."<<endl;

			// verify course and categorical info
			bool codeflag = false;
			int c_no = 0;
			for (; c_no<CS_table.size(); c_no++){
				if (strcmp(CS_table[c_no].CourseCode, qcode) == 0){
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
					check2 = CS_table[c_no].Credits;
				}
				else if (temp2 == "Professor"){
					check2 = CS_table[c_no].Professor;
				}
				else if (temp2 == "Days"){
					check2 = CS_table[c_no].Days;
				}
				else if (temp2 == "CourseName"){
					check2 = CS_table[c_no].CourseName;
				}
				cout<<"The course information has been found: The "<<qcat<<" of "<<qcode<<" is "<<check2<<"."<<endl;
				check = "The " + temp2 + " of " + temp + " is " + check2 + ".\n"; // answer to query
			}

			// send result char array to the Main Server
			sing_res[0] = '\0';
			strncpy(sing_res, check.c_str(), strlen(check.c_str())+1);
			if (sendto(CS_UDP_sockfd, sing_res, sizeof(sing_res), 0, (struct sockaddr *) &serverM_UDP_addr, serverM_UDP_addr_size) == -1){ // CHECK
				perror("--CUSTOM ERROR-- --CS Server-- CANNOT SEND COURSE CATEGORICAL INFO\n");
				exit(EXIT_FAILURE);
			}

			/*
			 * Beej's guide: 5.8 - sendto() and recvfrom()
			**/
			
			cout<<"The ServerCS finished sending the response to the main server."<<endl;
		}

	}

	/*-------------------------------------------------------END----------------------------------------------------*/

	close(CS_UDP_sockfd);
	return 0;
}

