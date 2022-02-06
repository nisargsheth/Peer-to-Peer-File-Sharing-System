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

using namespace std;
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;

struct user{
    string user_id;
    string passwd;
	unordered_map<string,bool> member_of_groups;
	bool is_logged_in = false;
	int port;//dynamic
};

// vector<user> users;


struct group{
	string group_id;
	string admin;
	unordered_map<string, user> members; //user_id to user
	unordered_map<string,long long> file_to_size; // storing chunks of the file.
	unordered_map<string,unordered_map<string,bool>> file_to_user; //file_name to list of users having it.
	unordered_map<string, bool> pending_request; //user_id to true/false;
};





unordered_map<string,user> users; // user_id : user struct mapping.
unordered_map<string,group> groups; // group_id : group

bool find_user(string user_id){
    if(users.find(user_id)==users.end()){
        return false;
    }
    return true;
}

bool find_group(string group_id){
    if(groups.find(group_id)==groups.end()){
        return false;
    }
    return true;
}



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

void* handler(void* arg){
	char buffer[32*1024] = {0}; 
	int new_socket = *((int *) arg);
	string current_user = "";
	recv(new_socket,buffer,32*1024,0);
	int client_port = stoi(buffer);///// check for login logout
	cout << "client with port " <<  client_port << " connected." << endl;
    while(1){
        // char *hello = "Hello from server"; 
		recv(new_socket,buffer,32*1024,0);
        pthread_mutex_lock(&mutex_lock);

        string command = buffer;

        vector<string> command_parsed;
        parse_cmd(command, command_parsed, ' ');

        cout << "Command requested: " << command_parsed[0] << endl;

        if(command_parsed[0]=="create_user"){
            if(command_parsed.size()!=3){
				string message = "Usage : create_account <username> <passwd>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
            }
            else{
                if(find_user(command_parsed[1])){
					string message = "User already exists.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
                }

                else{
                    struct user new_user;
                    new_user.user_id = command_parsed[1];
                    new_user.passwd = command_parsed[2];
                    users[command_parsed[1]] = new_user;
					string message = "User created.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
                }

            }
        }
        else if(command_parsed[0]=="login"){
			if(find_user(command_parsed[1])){
				if(current_user.size()){
					string message = "User " + current_user + " is currently logged in. Only one user can login from a host at a time.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
				}
				else if(users[command_parsed[1]].passwd==command_parsed[2]){

					if(users[command_parsed[1]].is_logged_in){
						string message = "User: " + command_parsed[1] + " already logged in.";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
					}
					else{
						users[command_parsed[1]].is_logged_in = true;
						users[command_parsed[1]].port = client_port;
						string message = "User: " + command_parsed[1] + " logged in successfully.";
						current_user=command_parsed[1];
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
					
				}
				else{
					string message = "Invalid user password.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Invalid user_id.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else if(command_parsed[0]=="create_group"){
			if(command_parsed.size()!=2){
				string message = "Usage : create_group <group_id>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}

			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[1])){
						string message = "Group with this id already exists.";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
					else{
						struct group new_group;
						users[current_user].member_of_groups[command_parsed[1]]=true;
						new_group.admin = current_user;
						new_group.group_id = command_parsed[1];
						new_group.members[current_user] = users[current_user];
						groups[command_parsed[1]] = new_group;				
						string message = "Group - " + command_parsed[1] + "created.";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else if(command_parsed[0]=="leave_group"){
			if(command_parsed.size()!=2){
				string message = "Usage : leave_group <group_id>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[1])){
						if(groups[command_parsed[1]].members.find(current_user)==groups[command_parsed[1]].members.end()){
							string message = "You are not a part of this group.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
						}
						else{
							groups[command_parsed[1]].members.erase(current_user);
							users[current_user].member_of_groups.erase(command_parsed[1]);
							string message = "You left the " + command_parsed[1] + " group";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}
		else if(command_parsed[0]=="join_group"){
			if(command_parsed.size()!=2){
				string message = "Usage : join_group <group_id>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[1])){
						if(groups[command_parsed[1]].members.find(current_user)==groups[command_parsed[1]].members.end()){
							groups[command_parsed[1]].pending_request[current_user]=true;
							string message = "Join request for group" + command_parsed[1] +"sent.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
						else{
							string message = "You are already a part of this group";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else if(command_parsed[0]=="list_requests"){
			if(command_parsed.size()!=2){
				string message = "Usage : list_requests <group_id>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}	
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[1])){
						if(groups[command_parsed[1]].admin==current_user){
							string message = "Pending requests list::\n";
							// send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 

							for(auto it : groups[command_parsed[1]].pending_request){
								message += it.first + ", ";
								// send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
							}
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
						else{
							string message = "This operation is only allowed for admin.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else if(command_parsed[0]=="accept_request"){
			if(command_parsed.size()!=3){
				string message = "Usage : accept_request <group_id> <user_id>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[1])){
						if(groups[command_parsed[1]].admin==current_user){
							if(groups[command_parsed[1]].pending_request.find(command_parsed[2])==groups[command_parsed[1]].pending_request.end()){
								string message = "No pending request found for user " + command_parsed[2];
								send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
							}
							else{
								groups[command_parsed[1]].pending_request.erase(command_parsed[2]);
								users[command_parsed[2]].member_of_groups[command_parsed[1]]=true;
								groups[command_parsed[1]].members[command_parsed[2]]=users[command_parsed[2]];
								string message = "User " + command_parsed[2] + "added to group " + command_parsed[1];
								send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
							}
						}
						else{
							string message = "This operation is only allowed for admin.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}
		else if(command_parsed[0]=="list_groups"){
			//////////check.
			if(command_parsed.size()!=1){
				string message = "Usage : list_groups";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					string message = "Following groups are available in the network:\n";
					// send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					for(auto it : groups){
						message += it.first + ", ";
					}
					cout << message << endl;
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}


		else if(command_parsed[0]=="list_files"){
			if(command_parsed.size()!=2){
				string message = "Usage : list_files <group_id>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[1])){
						if(groups[command_parsed[1]].members.find(current_user)==groups[command_parsed[1]].members.end()){
							string message = "You are not a member of this group.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
						else{
							string message = "List of files in group " + command_parsed[1] + "\n";
							// send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
							for(auto it : groups[command_parsed[1]].file_to_user){
								message += it.first + ", ";
							}
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else if(command_parsed[0]=="upload_file"){
			if(command_parsed.size()!=3){
				string message = "Usage : upload_file​ <file_path> <group_id​>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[2])){
						if(groups[command_parsed[2]].members.find(current_user)==groups[command_parsed[2]].members.end()){
							string message = "You are not a member of this group.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
						else{
							//send,recv,file_size.
							memset(buffer, 0, sizeof(buffer));
							recv(new_socket,buffer,32*1024,0); // receving the file size.
							cout << "File size received: : " << buffer << endl;
							if(groups[command_parsed[2]].file_to_user.find(command_parsed[1])==groups[command_parsed[2]].file_to_user.end()){
								unordered_map<string,bool> temp;
								temp[current_user] = true;
								groups[command_parsed[2]].file_to_user[command_parsed[1]] = temp;
							}
							else{
								groups[command_parsed[2]].file_to_user[command_parsed[1]][current_user] = true;
							}
							// cout << "File size received: " << buffer << endl;
							groups[command_parsed[2]].file_to_size[command_parsed[1]] = stoll(buffer);//errrorrr
							string message = "File uploaded.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}	
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else if(command_parsed[0]=="download_file"){
			if(command_parsed.size()!=4){
				string message = "download_file​ <group_id> <file_name> <destination_path>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else{
				if(current_user.size()){
					if(users[current_user].is_logged_in){
						if(find_group(command_parsed[1])){
							if(groups[command_parsed[1]].members.find(current_user)==groups[command_parsed[1]].members.end()){
								string message = "You are not a member of this group.";
								send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
							}
							else{
								if(groups[command_parsed[1]].file_to_user.find(command_parsed[2])==groups[command_parsed[1]].file_to_user.end()){
									string message = "No such file exists in the group " + command_parsed[1];
									send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
								}
								else{
									string message;
									message = to_string(groups[command_parsed[1]].file_to_size[command_parsed[2]]);
									memset(buffer, 0, sizeof(buffer));
									cout << "file size from tracker sent: " << message.c_str() << endl;
									send(new_socket , message.c_str() , strlen(message.c_str()) , 0 );  // sending the file size.
									recv(new_socket,buffer,32*1024,0);
									memset(buffer, 0, sizeof(buffer));
									// send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
									message.clear();
									for(auto it : groups[command_parsed[1]].file_to_user[command_parsed[2]]){
										message += to_string(users[it.first].port)+",";
									}
									
									send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
								}
							}
						}
						else{
							string message = "No such group exists";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
					}	
					else{
						string message = "Plesae login first to use the services.";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
				}
			}
		}

		else if(command_parsed[0]=="logout"){
			if(current_user.size()){
				users[current_user].is_logged_in = false;
				current_user = "";
				string message = "Logged out successfully.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
			}
			else{
				string message = "No user is currently logged in.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
			}
		}

		else if(command_parsed[0]=="stop_share"){
			if(command_parsed.size()!=3){
				string message = "Usage : stop_share ​ <group_id> <file_name>";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
			else if(current_user.size()){
				if(users[current_user].is_logged_in){
					if(find_group(command_parsed[2])){
						if(groups[command_parsed[2]].members.find(current_user)==groups[command_parsed[2]].members.end()){
							string message = "You are not a member of this group.";
							send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
						}
						else{
							if(groups[command_parsed[1]].file_to_user.find(command_parsed[2])==groups[command_parsed[1]].file_to_user.end()){
								string message = "No such file exists in the group";
								send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
							}
							else{
								if(groups[command_parsed[1]].file_to_user[command_parsed[2]].find(current_user)==groups[command_parsed[1]].file_to_user[command_parsed[2]].end()){
									string message = "You have not uploaded this file in this group.";
									send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
								}
								else{
									groups[command_parsed[1]].file_to_user[command_parsed[2]].erase(current_user);
									string message = "Unshared successfully.";
									send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
								}
							}
						}
					}
					else{
						string message = "No such group exists";
						send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
					}
				}
				else{
					string message = "Plesae login first to use the services.";
					send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
				}
			}
			else{
				string message = "Plesae login first to use the services.";
				send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 
			}
		}

		else{
			string message = "Invalid command.";
			send(new_socket , message.c_str() , strlen(message.c_str()) , 0 ); 	
		}


        // send(new_socket , hello , strlen(hello) , 0 ); 
        // printf("Hello message sent\n"); 
        memset(buffer, 0, sizeof(buffer));
		pthread_mutex_unlock(&mutex_lock);
    }
    
}


int main(int argc, char const *argv[]) 
{ 	
    int server_fd, new_socket, valread; 
	struct sockaddr_in address; 
	int opt = 1; 
	int addrlen = sizeof(address); 
	char *hello = "Hello from server"; 
	

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
	address.sin_port = htons( PORT ); 
	
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
		sleep(5);

        if(i>=3){
            i = 0;
            while(i<3){
               pthread_join(tid[i++], NULL);
            }
            i=0;
        }

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
