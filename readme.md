# EE450 Socket Programming Project, Fall 2022

- **Name:** Shauryasikt Jena
- **USC ID:** 5057859099
- **Optional part for multiple course code queries included, for extra credit**

### Table of Contents

[toc]

### Overview

This report entails my work on creating a primitive model for a course information access website. The client is the interface via which the user logs in to access the course information database. There are three backend servers, one for authenticating the user's login credentials, and two for handling the departmental course queries for EE and CS. The client communicates with the backend servers via a main server, and the corresponding operations are shown on the appropriate screens of the executable files of these 5 communication end-points, with only the client handling user inputs.

### 1. Process Flow

This section summarizes the process of user authentication and query resolution across the 5 communication end-points.

The shorthands for the end-points are as follows:
- **Client** - Client
- **ServerM** - Main Server
- **ServerC** - Credential Server
- **ServerEE** - EE Department Server
- **ServerCS** - CS Department Server

> All of these are on the local host at **'127.0.0.1'**.

The subsections show the project's process flow in sequential order.

#### 1.0 Phase 0 - *Booting Up*

1. ServerM boots up with TCP on port 25099 and UDP on port 24099 
2. ServerC stores encrypted login credentials and boots up with UDP on port 21099
3. ServerEE stores EE department's courses info and boots up with UDP on port 23099
4. ServerCS stores CS department's courses info and boots up with UDP on port 22099
5. Client boots up with dynamically assigned TCP port

#### 1.1 Phase 1 - *Logging In*

1. Client prompts user to enter username
2. Client prompts user to enter password
3. Client sends unecnrypted login credentials to ServerM over TCP
4. ServerM encrypts login credentials
5. ServerM sends encrypted login credentials to ServerC over UDP

#### 1.2 Phase 2 - *Authenticating User*

1. ServerC verifies username and password in its stored user database
2. ServerC sends the success case or the appropriate fail case to ServerM over UDP
3. ServerM sends the success case or the appropriate fail case to Client over TCP
4. Client can go ahead with successful authentication, or restart Phase 1, or shut down after 3 failed attempts

#### 1.3 Phase 3 - *Sending Course Query*

If Phase 2 ends in success,

1. Client prompts user to enter course code (can be multiple courses)
2. Client prompts user to enter what information is required of the course out of Credit/Professor/Days/CourseName (skip if multiple courses)
3. Client sends query information to ServerM over TCP
4. ServerM discerns whether query is for single course or multiple courses
5. ServerM sends each course query to corresponding department server, ServerEE or ServerCS over UDP

#### 1.4 Phase 4 - *Receiving Query Results*

1. ServerEE/CS discerns whether the received course code is for categorical query or a part of multiple courses
2. ServerEE/CS prepares the answer to each received query
3. ServerEE/CS sends each course code query's resolution to ServerM over UDP
4. ServerM prepares the overall query resolution
5. ServerM sends the query's resolution to Client over TCP
6. Client prompts user for a new query, restarting Phase 3

### 2. Code Files

Brief description of the code files for the project, as they are called in sequential order, and the functions included in them.

#### 2.1 Makefile

- ***all:*** Compile all the C++ code files in order serverM.cpp, serverC.cpp, serverEE.cpp, serverCS.cpp, client.cpp
- ***g++ -std=c++11:*** Run the C++ code files in above order and prepare executable files of the same names as per C++11
- ***PHONY:*** Name the executable files without the './'

#### 2.2 serverM.cpp

Functions:
- ***make_MainSocketTCP:*** create TCP socket for client with port 25099
- ***client_listen:*** anticipate incoming TCP connection from client
- ***make_MainSocketUDP:*** create UDP socket of serverM with port 24099
- ***init_connect_serverC/serverEE/serverCS:*** initialize UDP connection with ServerC/ServerEE/ServerCS
- ***encrypt_msg:*** encrypt incoming username or password from client
- ***separate_queries:*** break multiple course query to separate queries
- ***sendEEcat/sendCScat:*** send course query with categorical query to ServerEE/ServerCS
- ***sendEEf/sendCSf:*** send course query, if part of multiple course query, to ServerEE/ServerCS

#### 2.3 serverC.cpp

Functions:
- ***creds_obtain:*** create vector of pairs to store all users' encrypted login credentials
- ***make_SocketUDPserverC:*** create UDP socket of serverC with port 21099

#### 2.4 serverEE.cpp/serverCS.cpp

Functions:
- ***CourseInfo:*** create vector of strings to store information of all EE/CS courses
- ***make_SocketUDPserverEE/serverCS:*** create UDP socket of serverEE/serverCS with port 23099/22099

#### 2.5 client.cpp

Functions:
- ***make_ClientSocketTCP:*** create TCP socket for serverM with dynamically assigned port
- ***getPort_TCP:*** obtain the dynamically assigned TCP port
- ***req_username:*** prompt user to enter username
- ***req_password:*** prompt user to enter password
- ***send_auth:*** send login credentials to ServerM
- ***auth_results:*** receive authentication results from ServerM
- ***auth_user:*** loop authentication until success or shut down by failure
- ***req_course:*** prompt user to enter course code
- ***req_info:*** prompt user to enter course's categorical query
- ***send_queryc:*** send course query to ServerM
- ***query_results:*** receive course query resolution from ServerM

### 3. Messages sent across client and servers

All messages are sent as character arrays, due to convenience of maintaining pointers and computing sizes of messages. 

#### 3.1 client.cpp

- ***<u>client --> ServerM</u> auth_msg_user:*** (char array 128) user username
- ***<u>client --> ServerM</u> auth_msg_pwd:*** (char array 128) user password
- ***<u>ServerM --> client</u> notif_str:*** (char array 32) (0,1,2) encoding for authentication, See Appendix
- ***<u>client --> ServerM</u> query_code:*** (char array 1024) course code for query
- ***<u>client --> ServerM</u> query_cat:*** (char array 1024) category query for user
- ***<u>ServerM --> client</u> q_results:*** (char array 8192) result of query

#### 3.2 serverM.cpp

- ***<u>client --> ServerM</u> inp_msg_user:*** (char array 128) unencrypted user username
- ***<u>client --> ServerM</u> inp_msg_pwd:*** (char array 128) unencrypted user password
- ***<u>ServerM --> ServerC</u> op_msg_user:*** (char array 128) encrypted user username
- ***<u>ServerM --> ServerC</u> op_msg_pwd:*** (char array 128) encrypted user password
- ***<u>ServerC/ServerM --> ServerM/client</u> notif_str:*** (char array 32) (0,1,2) encoding for authentication
- ***<u>client/ServerM --> ServerM/ServerEE/CS</u> query_code:*** (char array 1024) course code for query
- ***<u>client/ServerM --> ServerM/ServerEE/CS</u> query_cat:*** (char array 1024) info category for query
- ***<u>ServerEE/CS --> ServerM</u> res_str:*** (char array 1024) query resolution received for one course
- ***<u>ServerM --> client</u> fin_res:*** (char array 8192) final concatenated result for initial query

#### 3.3 serverC.cpp

- ***<u>ServerM --> ServerC</u> op_msg_user:*** (char array 128) encrypted user username
- ***<u>ServerM --> ServerC</u> op_msg_pwd:*** (char array 128) encrypted user password
- ***<u>ServerC --> ServerM</u> notif_str:*** (char array 32) (0,1,2) encoding for authentication

#### 3.4 serverEE.cpp/serverCS.cpp

- ***<u>ServerM --> ServerEE/CS</u> qcode:*** (char array 1024) course code
- ***<u>ServerM --> ServerEE/CS</u> qcode:*** (char array 1024) info category of course
- ***<u>ServerEE/CS --> ServerM</u> single_res:*** (char array 1024) query resolution for single course
- ***<u>ServerEE/CS --> ServerM</u> multi_res:*** (char array 1024) query resolution for multiple course

### 4. Idiosyncracies

- As mentioned in the manual, `make all` does not seem to work with the Makefile in the terminal of the VM. It works with a slight alteration as `make -f Makefile`, before opening the executables in five different terminals. This will show two warnings that may be ignored.

- ServerM closes if client is closed by Ctrl + C, or if the client closes after 3 failed login attempts. However, if they are both started with ./serverM and ./client respectively immediately after their closing, everything runs smoothly.

- This project does not deal with incorrect inputs in the cases where it is not expected, as mentioned in EE450 Piazza question@199, namely
	- Each category of each course has a valid attribute (not empty)
	- User credential file and course list files are in the same format as provided, i.e., same titles: "cred.txt", "cs.txt", "ee.txt" and same linebreak formats.

- The project is set for IPv4 only, NOT IPv6.


> Apart from this, the project works as directed in the Course Project manual.

### 5. Reused Code

1. **[Beej's Guide to Network Programming: Using Internet Sockets](https://beej.us/guide/bgnet/pdf/bgnet_a4_c_1.pdf)**
- *Section 5.3 - bind():*
	- client.cpp: 
		- Line 52: make_ClientSocketTCP() for init
	- serverM.cpp: 
		- Line 56: make_MainSocketTCP() for init and bind
		- Line 102: make_MainSocketUDP() for init and bind
		- Line 139: init_connect_serverC()
		- Line 159: init_connect_serverEE()
		- Line 211: init_connect_serverCS()
	- serverC.cpp:
		- Line 74: make_SocketUDPserverC() for init and bind
	- serverEE.cpp:
		- Line 107: make_SocketUDPserverEE() for init and bind
	- serverCS.cpp:
		- Line 107: make_SocketUDPserverCS() for init and bind

- *Section 5.4 - connect():*
	- client.cpp:
		- Line 77: connect

- *Section 5.5 - listen():*
	- serverM.cpp:
		- Line 89: client_listen()

- *Section 5.6 - accept():*
	- serverM.cpp:
		- Lines 320-325: Accept()

- *Section 5.7 - send() and recv():*
	- client.cpp():
		- Line 122: send_auth()
		- Line 146: auth_results()
		- Line 219: send_queryc()
		- Line 244: query_results()
	- serverM.cpp():
		- Lines 334, 339, 408, 418: recv
		- Lines 382, 530: send

- *Section 5.8 - sendto() and recvfrom():*
	- serverM.cpp:
		- Line 178: sendEEcat()
		- Line 196: sendEEf()
		- Line 229: sendCScat()
		- Line 247: sendCSf()
		- Lines 353, 357: sendto
		- Lines 370, 475, 484, 507, 514: recvfrom
	- serverC.cpp:
		- Lines 127, 132: recvfrom
		- Line 164: sendto
	- serverEE.cpp:
		- Lines 202, 266: sendto
		- Lines 159, 216: recvfrom
	- serverCS.cpp:
		- Lines 202, 266: sendto
		- Lines 159, 216: recvfrom

2. **[GitHub listnukira/get_ip_v1.c](https://gist.github.com/listnukira/4045436)** 
	- client.cpp():
		- Line 88: getPort_TCP()

### Appendix

- Authentication encoding 
	- 0: username wrong
	- 1: password wrong
	- 2: authentication successful

- serverM.cpp
	- Line 284: separate_queries() - add marker 'f' in front of course codes if they are a part of a multiple course code query

- serverC.cpp
	- store credentials in `vector<pair<username, password>`

- serverEE.cpp/serverCS.cpp
	- store course information in `vector<object>`, where each object is a course code with other categories as attributes.