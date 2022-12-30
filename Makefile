all: serverM.cpp serverC.cpp serverEE.cpp serverCS.cpp client.cpp # compile all files in order 
	g++ -std=c++11 serverM.cpp -o serverM #output in executable file serverM
	g++ -std=c++11 serverC.cpp -o serverC #output in executable file serverC
	g++ -std=c++11 serverEE.cpp -o serverEE #output in executable file serverEE
	g++ -std=c++11 serverCS.cpp -o serverCS #output in executable file serverCS
	g++ -std=c++11 client.cpp -o client #output in executable file client

.PHONY: serverM # make executable file with these names
serverM: ./serverM

.PHONY: serverC
serverC: ./serverC

.PHONY: serverEE
serverEE: ./serverEE

.PHONY: serverCS
serverCS: ./serverCS

.PHONY: client
client: ./client