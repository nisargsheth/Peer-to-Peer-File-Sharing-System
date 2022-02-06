// Client side C/C++ program to demonstrate Socket programming 
#include <bits/stdc++.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#define PORT 8080 
// #define CSPORT 6490
#define CHUNK_SIZE (512*1024)


using namespace std;

int CSPORT;
unordered_map<string, vector<int>> file_to_chunks; // file to chunks for upload.
unordered_map<int,bool> chunks_received;

map<int, vector<int>> peerport_to_chunks; // for download.
vector<int> current_file_chunks;
unordered_map<string,int> indices;

string requested_file;
string requested_file_size;
FILE* fp;
struct sock_serv {
    int sock;
	int serv_port;
};
struct serv_fun {
    int serv;
	int fun;
	// vector<int> chunks_requested;
};
void parse_cmd(string str, vector<string> &parsed, char del){
    parsed.clear();
    string tmp = "";
    for(auto it : str){
        if(it==del){
            parsed.push_back(tmp);
            tmp = "";
        }
        else{
            tmp.push_back(it);
        }
    }
    parsed.push_back(tmp);
}

int get_file_size(std::string filename)
{
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}

bool file_exists(const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")){
        fclose(file);
        return true;
    }
	else{
        return false;
    }   
}


pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
void* handler(void* arg){
	cout << "in client as server handler " << endl;
	char buffer[32*1024] = {0}; 
	int new_socket = *((int *) arg);
	recv(new_socket,buffer,32*1024,0);

	vector<string> response_parsed;
	parse_cmd(buffer, response_parsed, ' ');

	cout << "command requested: " << response_parsed[0] << endl;
	// clean this shit
	// if(response_parsed[0]=="chunk_info"){
	if(response_parsed[0].compare("chunk_info")==0){
		cout << "chunk_info requested for file " << response_parsed[1] << endl;
		
	
		string chunks_vector="";
		if(file_to_chunks.find(response_parsed[1])==file_to_chunks.end()){
			cout << "file not found" << endl;
		}
		else{
			cout << "file found, bit vector of chunks: " << endl;
			// for(auto it : file_to_chunks[response_parsed[1]]){
			// 	cout << it << " ";
			// 	chunks_vector += it + ",";
			// } 
			// cout << endl;

			for(int i=0; i<file_to_chunks[response_parsed[1]].size(); i++){
				//cout << file_to_chunks[response_parsed[1]][i] << " ";
				chunks_vector = chunks_vector + to_string(file_to_chunks[response_parsed[1]][i]);
				chunks_vector.push_back(',');
			}

		}
		// printf("%s\n",chunks_vector.c_str());
		send(new_socket , chunks_vector.c_str() , strlen(chunks_vector.c_str()) , 0 );
		// cout << "send return val" << send(new_socket , chunks_vector.c_str() , strlen(chunks_vector.c_str()) , 0 ) << endl; 
		// printf("YO?\n");

	}


	else if(response_parsed[0].compare("give_chunk")==0){
		// printf("chunk_request_init: %s\n",buffer);
		// cout << "stoi " << stoi(response_parsed[1]) << endl;
		int numb_of_chunks_req = stoi(response_parsed[1]);
		// memset(buffer, 0, sizeof(buffer));
		// recv(new_socket,buffer,1024,0);
		FILE* fp = fopen(response_parsed[2].c_str(),"r");
		

		string dummy = "dummy";
		send(new_socket , dummy.c_str() , strlen(dummy.c_str()) , 0 );


        



		// for(int i=1; i<=numb_of_chunks_req; i++){
		// 	// memset(buffer, 0, sizeof(buffer));
		// 	// recv(new_socket,buffer,1024,0);	
		// 	memset(buffer, 0, sizeof(buffer));
		// 	recv(new_socket,buffer,32*1024,0);	
		// 	printf("chunk_requested: %s\n", buffer);
		// 	// cout << "chunk requested: " << buffer << endl;
		// 	int chunk_no = stoi(buffer);
		// 	fseek(fp,CHUNK_SIZE*(chunk_no-1),SEEK_SET);
		// 	string s;
		// 	for(int i=1; i<=CHUNK_SIZE; i++){
		// 		s.push_back(fgetc(fp));
		// 	}
		// 	send(new_socket , s.c_str() , strlen(s.c_str()) , 0 );
		// 	s.clear();

		// }
		cout << "no of chunk req " << numb_of_chunks_req << endl;
        for(int i=1; i<=numb_of_chunks_req-1; i++){
			cout << i << " ";
			// memset(buffer, 0, sizeof(buffer));
			// recv(new_socket,buffer,1024,0);	
			memset(buffer, 0, sizeof(buffer));
			recv(new_socket,buffer,32*1024,0);	
			// printf("chunk_requested: %s\n", buffer);
			// cout << "chunk requested: " << buffer << endl;
			int chunk_no = stoi(buffer);
			fseek(fp,CHUNK_SIZE*(chunk_no-1),SEEK_SET);
			string s;

			

            if(chunk_no==1){
                cout << "chunk_no:1" << endl;
                int j=1;
                char c;
                while((j <= CHUNK_SIZE) && ((c=fgetc(fp))!=EOF)){
                    // cout << "char: " << c << endl;
                    // printf("char:%c\n",c);
                    s.push_back(c);
                    ++j;
                }
                // cout << "j " << j << endl;
                // cout << "first chunk: " << s[0] << endl;
            }
            else{
                for(int j=1; j<=CHUNK_SIZE; j++){
				    s.push_back(fgetc(fp));
			    }    
            }
            // cout << "current chunk size " << s.size() << endl;
            int iter = s.size()/(32*1024); // checking the sub chunks of a chunk as per the buffer size.
            if(s.size()%(32*1024)){
                ++iter;
            }
			// cout << "number of iter for chunk " << iter  << endl;
            int j=0;
            for(j=0; j<iter-1; j++){
                string s_chunk = s.substr((j)*32*1024, 32*1024);
                send(new_socket, s_chunk.c_str(), strlen(s_chunk.c_str()), 0);
                // cout << "sent chunk size in for loop(should be 32K): " << s_chunk.size() << endl;
                // cout << "sent chunk: " << s_chunk << endl;
            }
            
            // j-=1;

            string last_chunk = s.substr(j*32*1024);
            send(new_socket, last_chunk.c_str(), strlen(last_chunk.c_str()), 0);
            // cout << "sent_last chunk_size: " << last_chunk.size() << endl;
            string done = "done";
			send(new_socket , done.c_str() , strlen(done.c_str()) , 0 );
			s.clear();

		}
		cout << endl;

		// last chunk.
		cout << "last chunk starts " << endl;
		memset(buffer, 0, sizeof(buffer));
		recv(new_socket,buffer,32*1024,0);	
		printf("chunk_requested: %s\n", buffer);
		// cout << "chunk requested: " << buffer << endl;
		int chunk_no = stoi(buffer);
		fseek(fp,CHUNK_SIZE*(chunk_no-1),SEEK_SET);
		string s;
		s.clear();
		int j=1;
		char c;
		while((j <= CHUNK_SIZE) && ((c=fgetc(fp))!=EOF)){
			// cout << "char: " << c << endl;
			// printf("char:%c\n",c);
			s.push_back(c);
			++j;
		}
		cout << "last entire chunk size " << s.size() << endl;
		int iter = s.size()/(32*1024); // checking the sub chunks of a chunk as per the buffer size.
		if(s.size()%(32*1024)){
			++iter;
		}
		cout << "number of iter " << iter << endl;
		j=0;
		for(j=0; j<iter-1; j++){
			string s_chunk = s.substr((j)*32*1024, 32*1024);
			send(new_socket, s_chunk.c_str(), strlen(s_chunk.c_str()), 0);
			recv(new_socket,buffer,32*1024,0);	 // dummy
			// cout << "sent chunk size in for loop(should be 32K): " << s_chunk.size() << endl;
			// cout << "sent chunk: " << s_chunk << endl;
		}
		string done = "done";
        send(new_socket , done.c_str() , strlen(done.c_str()) , 0 );
            // j-=1;
		cout << "j last sub chunk " << j << endl;
		string last_chunk = s.substr(j*32*1024); // till end
		string last_chunk_size = to_string(last_chunk.size()); // last sub chunk of the last chunk.

		cout << "last sub chunk size " << last_chunk_size << endl;

		send(new_socket, last_chunk_size.c_str(), strlen(last_chunk_size.c_str()), 0);

		cout << "size sent " << endl;

		memset(buffer, 0, sizeof(buffer));
		recv(new_socket,buffer,32*1024,0);	 // dummy
		send(new_socket, last_chunk.c_str(), strlen(last_chunk.c_str()), 0);
		

		cout << "last chunk sent " << endl;

		memset(buffer, 0, sizeof(buffer));
		recv(new_socket,buffer,32*1024,0);	 // dummy

		// send(new_socket , done.c_str() , strlen(done.c_str()) , 0 );
		fclose(fp);

		
	}
    ///////////////////////FOR PROVIDING THE REQUIRED CHUNK
	
    // FILE* fp = fopen("myfile","r");


    // for(int i=1; i<= chunks-1; i++){
    //     fseek(fp,(i-1)*CHUNK_SIZE,SEEK_SET);
    //     string s;
    //     for(int j=1; j<=CHUNK_SIZE; j++){
    //         s.push_back(fgetc(fp));
    //     }
    //     s = to_string(i) + s;
    //     send(new_socket , s.c_str() , strlen(s.c_str()) , 0 ); 
    // }
    // fclose(fp);
	//////////////////////////////////
    // while(1){
	// char *hello = "Hello from server"; 
	// int valread = recv(new_socket,buffer,1024,0);
	// pthread_mutex_lock(&mutex_lock);
	// printf("%s\n",buffer ); 
	// send(new_socket , hello , strlen(hello) , 0 ); 
	// printf("Hello message sent\n"); 
	// memset(buffer, 0, sizeof(buffer));
	// pthread_mutex_unlock(&mutex_lock);
    // }
    
}
void* client_as_server(void* arg){
    int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	// char *hello = "Hello from server"; 
	

    pthread_t tid[3];
    int i=0;
	// Creating socket file descriptor 
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
	{ 
		perror("socket failed"); 
		exit(EXIT_FAILURE); 
	} 
	
	// Forcefully attaching socket to the port 8080 
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
												&opt, sizeof(opt))) 
	{ 
		perror("setsockopt"); 
		exit(EXIT_FAILURE); 
	} 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = INADDR_ANY; 
	address.sin_port = htons( CSPORT ); 
	
	// Forcefully attaching socket to the port 8080 
	if (bind(server_fd, (struct sockaddr *)&address, 
								sizeof(address))<0) 
	{ 
		perror("bind failed"); 
		exit(EXIT_FAILURE); 
	} 
	if (listen(server_fd, 3) < 0) 
	{ 
		perror("listen"); 
		exit(EXIT_FAILURE); 
	} 
	//while(1)

    while(1){
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0) 
        { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        } 
        if(pthread_create(&tid[i++],NULL,handler, &new_socket)!=0){
            cout << "failed to create thread" << endl;
        }
		// sleep(5);

        if(i>=3){
            i = 0;
            while(i<3){
               pthread_join(tid[i++], NULL);
            }
            i=0;
        }

    }
}

void* chat_for_peer(void* arg){
	cout << "in chat handler client" << endl;
	char buffer[32*1024] = {0}; 
	int sock = *((int *) arg);
	int valread;

	while(1){
		// string inp;
		// cin >> inp;
		fflush(stdin);
		char inp[1024];
		// char str[MAX_LIMIT]; 
   		fgets(inp, 1024, stdin); 
		// scanf("%s",inp);


        // char* connect = "connect";
		// if(strcpy(inp,connect)==0){
		// 	cout << "equal" << endl;
		// 	pthread_t client_connecter;
		// 	int port = 6490;
		// 	if(pthread_create(&client_connecter,NULL,client_as_client_for_peer, (void *) &port)!=0){
        //     	cout << "failed to create thread" << endl;
        // 	}
		// 	pthread_join(client_connecter,NULL);
		// }

		send(sock , inp , strlen(inp) , 0 ); 
		// printf(" message sent\n"); 
		// valread = read( sock , buffer, 1024); 
		recv(sock,buffer,32*1024,0);
		printf("%s\n",buffer ); 
		memset(buffer, 0, sizeof(buffer));
	}
}


void* get_chunk_info(void* arg){
	cout << "in get chunk info" << endl;
	char buffer[32*1024] = {0}; 
	int valread;

	//while(1);


	// vector<string> arg_parsed;
	// parse_cmd(*(string *)arg, arg_parsed, ' ');

	// struc sock = *((int *) arg);

	struct sock_serv *args = (struct sock_serv *)arg;
	// int sock = stoi(arg_parsed[1]);
	// int serv_port = stoi(arg_parsed[0]);


	int sock = args->sock;
	int serv_port = args->serv_port;

	cout <<"in get chunk info : sock : " << sock << " serv_port: " << serv_port << endl;

	fflush(stdin);
	string chunk_request = "chunk_info " + requested_file;
	cout << chunk_request << endl;
	// char inp[1024]={0};
	// inp=chunk_request.c_str();
	// cin.getline(inp,sizeof(inp));



	send(sock , chunk_request.c_str() , strlen(chunk_request.c_str()) , 0 ); 
	cout << "request sent for chunks "  << endl;
	memset(buffer, 0, sizeof(buffer));
	cout << "waiting for response " << endl;
	// cout << "recv return val" << recv(sock,buffer,1024,0) << endl;
	recv(sock,buffer,32*1024,0);
	printf("Chunk vector received from %d :",serv_port); 
	printf("%s\n",buffer);
	vector<string> response_parsed;
	parse_cmd(buffer, response_parsed, ',');

	// if(file_to_chunks.find(requested_file)==file_to_chunks.end()){
	// 	vector<int> temp(stoi(requested_file_size));
	// 	file_to_chunks[requested_file] = temp;
	// }

	// vector<int> temp(stoi(requested_file_size));
	vector<int> temp(response_parsed.size()-1);
	for(int i=0; i<response_parsed.size()-1; i++){
		// peerport_to_chunks[serv_port].push_back(stoi(response_parsed[i]));
		temp[i] = (stoi(response_parsed[i]));
	}
	peerport_to_chunks[serv_port] = temp;


	cout << "peerport_to_chunks: " << endl;
	for(auto it : peerport_to_chunks){
		cout << it.first << ": ";
		for(auto jt : it.second){
			cout << jt << ",";
		}
		cout << endl;
	}

	///////////////////////
	// for(int i=0; i<response_parsed.size()-1;i++){
		// peerport_to_chunks[serv_port].push_back(stoi(response_parsed[i]));
		// file_to_chunks[requested_file][stoi(response_parsed[i])] = 1;
	// }

	memset(buffer, 0, sizeof(buffer));
}

void* get_chunks(void* arg){
	// cout << "in get chunks " << endl;
	char buffer[32*1024] = {0};

	struct sock_serv *args = (struct sock_serv*)arg;

	int sock = args->sock;
	int serv_port = args->serv_port;
	fflush(stdin);
	

	string chunk_request_init = "give_chunk " + to_string(peerport_to_chunks[serv_port].size()) + " " + requested_file;
	// FILE* fp = fopen()
	cout << "chunk_request_init:" << chunk_request_init << endl;
	send(sock, chunk_request_init.c_str(), strlen(chunk_request_init.c_str()), 0);

	
	recv(sock,buffer,1024,0);//dummy
	memset(buffer, 0, sizeof(buffer));

	cout << "peerport_to_chunks[serv_port].size() " << peerport_to_chunks[serv_port].size() << endl;
	for(int i=1; i<=(peerport_to_chunks[serv_port].size()-1); i++){
		printf("%d ",i);
		// char emptybuff[1024] = {0};
		// send(sock, emptybuff, strlen(emptybuff),0);
		string chunk_request = to_string(peerport_to_chunks[serv_port][i-1]);
		// cout << "ASKED :" << chunk_request << endl;
		send(sock, chunk_request.c_str(), strlen(chunk_request.c_str()), 0);
		// cout << "request sent for getting chunks " << endl;
		memset(buffer, 0, sizeof(buffer));
		recv(sock,buffer,32*1024,0);
        // printf("received first: %s\n",buffer);
        string response = buffer;
        string chunk_response;
        while(response.compare("done")!=0){
            chunk_response += response;
            memset(buffer, 0, sizeof(buffer));
		    recv(sock,buffer,32*1024,0);
			response.clear();
            response = buffer;  
            // printf("received: %s\n",buffer);
            // cout << "received in while: " << response << endl;
        }
        fseek(fp,(stoi(chunk_request)-1)*CHUNK_SIZE, SEEK_SET);
		fputs(chunk_response.c_str(),fp);
		response.clear();
		chunk_response.clear();
        // printf("chunk received: %s\n",chunk_response.c_str());
		// cout << "chunk received..." << chunk_response << endl;
	}


	// last chunk.
	cout << "last chunk starts " << endl;
	string chunk_request = to_string(peerport_to_chunks[serv_port][peerport_to_chunks[serv_port].size()-1]);
	send(sock, chunk_request.c_str(), strlen(chunk_request.c_str()), 0);

	memset(buffer, 0, sizeof(buffer));
	recv(sock,buffer,32*1024,0);

	// printf("received %s\n",buffer);

	string response = buffer;
    string chunk_response;
	string dummy = "dummy";
    while(response.compare("done")!=0){
		// printf("received %s\n",buffer);
		chunk_response += response;
		send(sock, dummy.c_str(), strlen(dummy.c_str()), 0);
		memset(buffer, 0, sizeof(buffer));
		recv(sock,buffer,32*1024,0);
		response = buffer;  
		// printf("received: %s\n",buffer);
		// cout << "received in while: " << response << endl;
	}
	cout << "out from while";
	fseek(fp,(stoi(chunk_request)-1)*CHUNK_SIZE, SEEK_SET);

	fputs(chunk_response.c_str(),fp);
	response.clear();
	chunk_response.clear();
	
	memset(buffer, 0, sizeof(buffer));
	cout << "receiving last chunk size " << endl;
	recv(sock,buffer,32*1024,0); // last sub chunk size.
	int last_chunk_size = stoi(buffer);
	printf("last chunk size %d\n", last_chunk_size);

	send(sock, dummy.c_str(), strlen(dummy.c_str()), 0);

	recv(sock,buffer,last_chunk_size,0); // last chunk.
	send(sock, dummy.c_str(), strlen(dummy.c_str()), 0);

	int i=0;
	// cout << "last chunk size " << last_chunk_size << endl;
	while(i<last_chunk_size){
		fputc(buffer[i],fp);
		++i;
	}
	

	cout << endl << "done receving." << endl;
	


}

void* client_as_client_for_peer(void* arg){
    int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char *hello = "Hello from client"; 
    pthread_t thread;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		// return; 
	} 


	struct serv_fun *args = (struct serv_fun *)arg;

	// vector<string> arg_parsed;
	// parse_cmd(*(string *)arg, arg_parsed, ' ');
	int function = args->fun;
    // int serv_port = *((int *) arg);
	int serv_port = args->serv;

	cout << "sever port in cps2 for client to client peer: " << serv_port << " function: " << function << endl;
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(serv_port); 
	

	// string servport_sock = arg_parsed[0]+" "+to_string(sock);

	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		// return; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		// return; 
	} 
	if(function==1){
		// ask for chunks.
		sock_serv sock_serv_arg;
		sock_serv_arg.serv_port =  serv_port;
		sock_serv_arg.sock = sock;

		if(pthread_create(&thread, NULL, get_chunk_info, (void *) &sock_serv_arg)!=0){
			cout << "failed to create client thread" << endl;
		}
		pthread_join(thread, NULL);
	}
	else{
		sock_serv sock_serv_arg;
		sock_serv_arg.serv_port =  serv_port;
		sock_serv_arg.sock = sock;

		if(pthread_create(&thread, NULL, get_chunks, (void *) &sock_serv_arg)!=0){
			cout << "failed to create client thread" << endl;
		}
		pthread_join(thread, NULL);
		//download;
		// if(pthread_create(&thread, NULL, chat_for_peer, (void *) &sock )!=0){
		// 	cout << "failed to create client thread" << endl;
		// // }
		// if(pthread_create(&thread, NULL, chat_for_peer, (void *) &sock_serv_arg)!=0){
		// 	cout << "failed to create client thread" << endl;
		// }
	}
	
	// sleep(1);
	// pthread_join(thread,NULL);
}


// chat with tracker.
void* chat(void* arg){
	cout << "in chat handler client" << endl;
	// vector<string> arg_parsed;
	// parse_cmd(*(string *)arg, arg_parsed, ' ');
	char buffer[32*1024] = {0}; 
	int sock = *((int *) arg);
	// int sock = stoi(arg_parsed[0]);
	// int function = stoi(arg_parsed[1]);
	int valread;
	fflush(stdin);
	string client_port = to_string(CSPORT);
	send(sock , client_port.c_str() , strlen(client_port.c_str()) , 0 ); //sending port information.
	while(1){
		fflush(stdin);
		char inp[1024]={0};
		cin.getline(inp,sizeof(inp));
	
        // string connect = "connect";
		string input=inp;
		// if(connect==input){
		// 	cout << "equal" << endl;
		// 	pthread_t client_connecter;
		// 	int port = 6490; // will be given by tracker.
		// 	if(pthread_create(&client_connecter,NULL,client_as_client_for_peer, (void *) &port)!=0){
        //     	cout << "failed to create thread" << endl;
        // 	}
		// 	pthread_join(client_connecter,NULL);
		// }

		send(sock , inp , strlen(inp) , 0 ); //send command
		// recv(sock,buffer,1024,0); // response/

		vector<string> parsed;
		parse_cmd(input, parsed,' ');

		if(parsed[0]=="upload_file"){
			if(file_exists(parsed[1])){
				int file_size = get_file_size(parsed[1]);
				cout << "file size calculated: " << file_size << endl;
				int chunks = file_size/CHUNK_SIZE;
				cout << "chunks before" << chunks << endl;
				if(file_size % CHUNK_SIZE){
					cout << "in if" << endl;
					cout << "file size " << file_size << endl;
					cout << "mod " << file_size % CHUNK_SIZE << endl; 
					++chunks;
				}
				cout << "chunks after" << chunks << endl;
				// check if mapping not exists. create new vector.
  				fill(file_to_chunks[parsed[1]].begin(),file_to_chunks[parsed[1]].begin()+file_to_chunks[parsed[1]].size(),1);
				vector<int> temp(chunks);
				for(int i=0; i<temp.size(); i++) temp[i] = i+1; // this peers has all the chunks from 1...n.
				
				file_to_chunks[parsed[1]] = temp; // in this system which file has which chunks


				string chunks_string = to_string(chunks);
				string file_size_string = to_string(file_size);
				// printf("%s\n",buffer ); 
				// memset(buffer, 0, sizeof(buffer));
				send(sock,file_size_string.c_str(),strlen(file_size_string.c_str()), 0);
				cout << "file_size string sent for file size " << endl;
				recv(sock,buffer,32*1024,0); // response/
				cout << buffer << endl;
			}
			else{
				cout << "File " << parsed[1] << " not found in the system." << endl;
			}
		}

		// if(parsed[0]=="download_file​"){

		else if(parsed[0].compare("download_file")==0){
			////////////have to check in every command successful or not from tracker.

			cout << "Getting file size of requested file..." << endl;
			memset(buffer, 0, sizeof(buffer));
			recv(sock,buffer,32*1024,0); // requested file size

			string dummy = "dummy";
			send(sock , dummy.c_str() , strlen(dummy.c_str()) , 0 );

			cout << "Download: received the file size : " << buffer << endl;
			requested_file_size = buffer;
			string dnwld_file = parsed[2]+"_dwnld";
			fp = fopen(dnwld_file.c_str(),"w");
			ftruncate(fileno(fp), stoll(requested_file_size)); 

			cout << "Getting list of peers..." << endl;
			memset(buffer, 0, sizeof(buffer));
			recv(sock,buffer,32*1024,0); // getting the list of peers.
			// printf("%s\n",buffer ); 
			string response = buffer;
			vector<string> peer_ports;
			response.pop_back();
			parse_cmd(response,peer_ports,',');

			for(auto it : peer_ports) cout << it << " ";
			cout << endl;

			int no_of_peers = peer_ports.size();
			requested_file = parsed[2];
			/////////create the mapping of which peer will provide which chunk.
			// recv(sock,buffer,1024,0); // receiving the file size for the file.

			pthread_t  *downloaders;	
			downloaders = (pthread_t*)malloc(sizeof(pthread_t)*no_of_peers); 
			

			// vector<pthread_t> downloaders(no_of_peers);


			// int port = 6490; // will be given by tracker.

			for(int i=0; i<no_of_peers; i++){
				cout << "i " << i << " " << peer_ports[i] << endl;
				// first getting the chunks info from the peers.
				
				serv_fun serv_fun_arg;
				serv_fun_arg.fun = 1;
				serv_fun_arg.serv = stoi(peer_ports[i]);

				// if(pthread_create(&downloaders[i], NULL, get_chunk_info, (void *) &sock_serv_arg)!=0){
				// 	cout << "Downloader thread creation failed." << endl;
				// }
				if(pthread_create(&downloaders[i], NULL, client_as_client_for_peer, (void *) &serv_fun_arg)!=0){
					cout << "Downloader thread creation failed." << endl;
				}
	
				// if(pthread_create(&client_connecter,NULL,client_as_client_for_peer, (void *) &port)!=0){
            	// 	cout << "failed to create thread" << endl;
        		// }
			}
			for(int i=0; i<no_of_peers; i++){
				pthread_join(downloaders[i], NULL);
			}
			//////////////CHUNK SELECTION ALGORITHM//////////////

			current_file_chunks.resize(stoi(requested_file_size));

			unordered_map<int,int> indices;	
			unordered_map<int, vector<int>> chunks_selected;//for deciding which peer will give what chunks.

			for(int iter=0; iter<stoi(requested_file_size); iter++){
				bool selected = false;
				for(auto it : peerport_to_chunks){
					// cout << "in selection: " << it.first << endl;
					// for(int j=0; j<it.second.size(); j++){
					while(current_file_chunks[it.second[indices[it.first]]]==1){
						++indices[it.first];
					}
					// cout << "index after while " << indices[it.first] << endl;
					if(indices[it.first]< it.second.size()){
						selected = true;
						current_file_chunks[it.second[indices[it.first]]] = 1;
						chunks_selected[it.first].push_back(it.second[indices[it.first]]);
						++indices[it.first];
					}
				// }
				}
        		if(!selected) break;
    		}
			cout << "chunk selection result: " << endl;
			for(auto it : chunks_selected){
				cout << it.first << ": ";
				for(auto jt : it.second){
					cout << jt << " ";
				}
				cout << endl;
			}


			///////////////// ASKING FOR THE CHUNKS DATA FROM THE PEERS/////////////
			for(int i=0; i<no_of_peers; i++){
				// first getting the chunks info from the peers.
				
				serv_fun serv_fun_arg;
				serv_fun_arg.fun = 0; // 0 function for asking the actual data.
				serv_fun_arg.serv = stoi(peer_ports[i]);

				// if(pthread_create(&downloaders[i], NULL, get_chunk_info, (void *) &sock_serv_arg)!=0){
				// 	cout << "Downloader thread creation failed." << endl;
				// }
				if(pthread_create(&downloaders[i], NULL, client_as_client_for_peer, (void *) &serv_fun_arg)!=0){
					cout << "Downloader thread for chunk data creation failed." << endl;
				}



				// if(pthread_create(&client_connecter,NULL,client_as_client_for_peer, (void *) &port)!=0){
            	// 	cout << "failed to create thread" << endl;
        		// }
			}
			for(int i=0; i<no_of_peers; i++){
				pthread_join(downloaders[i], NULL);
			}
			fclose(fp);
			cout << "file closed " << endl;
			// pthread_join(client_connecter,NULL);


		}
		// cout <<parsed[0].compare("download_file") << endl;
		// printf("%s\n",buffer ); 
		// string response = buffer;
		// while(buffer!="END"){
		// 	printf("%s\n",buffer ); 
		// 	recv(sock, buffer, 1024, 0);
		// }
		else{
			recv(sock,buffer,32*1024,0); // response/
			printf("%s\n",buffer ); 
		}
		memset(buffer, 0, sizeof(buffer)); // check if elsewhere needed in the program.
	}
}

// connect with tracker.
void* client_as_client(void* arg){
    int sock = 0, valread; 
	struct sockaddr_in serv_addr; 
	char *hello = "Hello from client"; 
    pthread_t thread;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		// return; 
	} 
    // int serv_port = *arg;
    int serv_port = *((int *) arg);
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(serv_port); 
	cout << "serv port for cps2 client as client for tracker " << serv_port << endl;
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		// return; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		// return; 
	} 
	if(pthread_create(&thread, NULL, chat, (void *) &sock )!=0){
		cout << "failed to create client thread" << endl;
	}
	sleep(5);
	pthread_join(thread,NULL);
}




int main(int argc, char const *argv[]) 
{ 	CSPORT = stoi(argv[1]);
    pthread_t tid[2];
    if(pthread_create(&tid[1],NULL, client_as_server,NULL)!=0){
        cout << "client as server thread creation failed" << endl;
    }
    sleep(5);
    int port = PORT;
    if(pthread_create(&tid[0],NULL, client_as_client,(void *) &port)!=0){
        cout << "client as client thread creation failed" << endl;
    }
    sleep(5);
    
    for(int i=0; i<2; i++){
        pthread_join(tid[i],NULL);
    }

    // client as sserver, client.
	// int sock = 0, valread; 
	// struct sockaddr_in serv_addr; 
	// char *hello = "Hello from client"; 
    // pthread_t thread;
    // pthread_t tid[3];
	// if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	// { 
	// 	printf("\n Socket creation error \n"); 
	// 	return -1; 
	// } 

	// serv_addr.sin_family = AF_INET; 
	// serv_addr.sin_port = htons(PORT); 
	
	// // Convert IPv4 and IPv6 addresses from text to binary form 
	// if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) 
	// { 
	// 	printf("\nInvalid address/ Address not supported \n"); 
	// 	return -1; 
	// } 

	// if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	// { 
	// 	printf("\nConnection Failed \n"); 
	// 	return -1; 
	// } 
	// if(pthread_create(&thread, NULL, chat, (void *) &sock )!=0){
	// 	cout << "failed to create client thread" << endl;
	// }
	// sleep(5);
	// pthread_join(thread,NULL);
	// cout << "hi there?" << endl;	
	return 0; 
} 
