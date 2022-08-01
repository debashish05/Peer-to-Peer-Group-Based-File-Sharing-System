// Client Side aka. Peer
// Time: 7:40 PM, Nov 10, 2021

#include <bits/stdc++.h> // will add all C++ headers
#include <arpa/inet.h>   // inet_addr
#include <pthread.h>     // For threading, link with lpthread
#include <sys/stat.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
using namespace std;

string tracker1IP, tracker2IP;
long tracker1Port, tracker2Port;
int port;
string IP;

void errorLog(string msg)
{
    std::cerr << msg << " " << strerror(errno) << "\n";
    exit(0);
}

void initalizeTracker(int argc, char *argv[])
{
    fstream openFile;
    openFile.open(argv[2], ios::in);
    if (openFile.is_open())
    {
        string input;
        getline(openFile, input);
        tracker1IP = input;
        getline(openFile, input);
        tracker1Port = stoi(input);
        openFile.close();
    }
    else
    {
        errorLog("Tracker info file not present in current"
                 "working directory\n");
    }
}

string fetch(string input, int argNo = 1)
{
    string ans = input;
    istringstream ss(input);

    for (int i = 0; i < argNo; ++i)
    {
        ss >> ans;
    }
    return ans;
}

void download(string input,string dest)
{
    //int client_request = *((int*)args);
    if(input==""){
        std::cout<<"Download not possible at this moment no seeder available"<<endl;
        return;
    }
    //portNo path
    int portDest=atoi(fetch(input,1).c_str());
    //string fileName=dest+"/"+fetch(input,2);
    string fileName=fetch(input,2);
	// Create a stream socket
	int network_socket = socket(AF_INET,SOCK_STREAM, 0);
    if (network_socket == -1)
	{
		errorLog("Could not create socket");
	}

	// Initialise port number and address
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_address.sin_port = htons(portDest);

	// Initiate a socket connection
	int connection_status = connect(network_socket,
	(struct sockaddr*)&server_address,sizeof(server_address));

	// Check for connection error
	if (connection_status < 0) {
		errorLog("Connection not able to established\n");
	}

	cout<<"Connection estabilished\n";


	//manipulate here in the while loop

	//string fileName;
	//cout<<"Enter File Name that need to be downloaded: ";
    //provide file location
	//cin>>fileName;

	if(send(network_socket,fileName.c_str(),fileName.length(),0)<0)
	{
		errorLog("Filename not send successfully");
	}

	cout<<"Downloading started\n";

	char buffer[524288];    // 512 KB
	int bytes_read=0;
	int fout= open(fileName.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	
	while((bytes_read = read(network_socket, buffer, sizeof(buffer)))>0)
	{  
        write(fout,buffer, bytes_read); 
	}
	
	//while (bytes_read > 0);
	cout<<"Download Compelete\n";
	// Close the connection
	close(network_socket);
	//pthread_exit(NULL);
}

void client()
{
    // Create a stream socket
    int network_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (network_socket == -1)
    {
        errorLog("Could not create socket");
    }

    // Initialise port number and address
    struct sockaddr_in server_address;
    server_address.sin_port = htons(tracker1Port);
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(tracker1IP.c_str());

    // Initiate a socket connection
    int connection_status = connect(network_socket,
                                    (struct sockaddr *)&server_address, sizeof(server_address));

    // Check for connection error
    if (connection_status < 0)
    {
        errorLog("Connection not able to established\n");
    }

    char command[256];
    cout << "Connection estabilished\n";
    
    char buffer[512];
    while(true)
    {
        cout<<"Type Command: ";
        cin.getline(command,sizeof(command));
        string cmd(command);
        string cmdtype=fetch(cmd,1);

        send(network_socket, cmd.c_str(), cmd.length(), 0);
         
        if (strcmp(cmdtype.c_str(), "login") == 0){
            cmd=to_string(port);
            send(network_socket, cmd.c_str(), cmd.length(), 0); //send port number
        }
        read(network_socket, buffer, sizeof(buffer));

        if (strcmp(cmdtype.c_str(), "download_file") == 0){
            // start download
            string dest=fetch(cmd,4);
            download((string)buffer,dest);
        }
        else cout<<buffer<<"\n";
    }
    close(network_socket);
}

void* seedFile(void *socket_desc)
{
    
    //Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char *message , client_message[512];
	
	//Send some messages to the client
	//find filename which need to be download
	
	read_size = recv(sock , client_message , sizeof(client_message) , 0);
    //recieve path of file in "client_message"

	//server side
	int fin= open(client_message,O_RDONLY);

	//Receive a from file
	while( (read_size = read(fin,client_message , sizeof(client_message))) > 0 )
	{
		//Send the message back to client
		write(sock , client_message ,read_size);
	}

	if(read_size == 0)
	{
		//std::cout<<"File Downloaded successfully\n"<<endl;
	}
	else if(read_size == -1)
	{
		errorLog("Recieve failed");
	}
	close(sock);
	pthread_exit(NULL);
	return 0;
}

void *server(void *args)
{
    //return 0;
    struct sockaddr_storage serverStorage;
	socklen_t addr_size;

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket==-1)
	{
		errorLog("Could not create socket");
	}

    struct sockaddr_in serverAddr;
	serverAddr.sin_addr.s_addr = inet_addr(IP.c_str());
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

    int opt=1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR 
                    | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        errorLog("setsockopt");
    }

	// Bind the socket to the address and port number.
	if( bind(serverSocket,(struct sockaddr *)&serverAddr , sizeof(serverAddr)) < 0)
	{
		errorLog("Binding Error at server side of peer");
	}
	//cout<<"bind done for transfering the file\n";

	// Listen on the socket, with 50 max connection requests queued
    while(listen(serverSocket, 5)){
        //cout<<"Queue Full!!! Retrying after 10 sec\n";
        sleep(10);
    }
    //cout<<"Listening\n";


	int newSocket,i = 0;
    //cout<<"Waiting for incoming connections...\n";

	while (true) {
		addr_size = sizeof(serverStorage);
		// Extract the first connection in the queue
		newSocket = accept(serverSocket,
					(struct sockaddr*)&serverStorage,&addr_size);
        
        if(newSocket<0){
            errorLog("Connection failed");
        }
        //cout<<"Connection accepted:\n";

        pthread_t tid;
        int *new_sock =(int *)malloc(1);
		*new_sock = newSocket;
        if (pthread_create(&tid, NULL,seedFile, (void *)new_sock)<0)
		{
    		// Error in creating thread
			errorLog("Failed to create thread\n");
        }
        //else cout<<"Handle Assigned\n";
	}
    if (newSocket<0)
	{
		errorLog("accept failed");
	}
    return 0;
}

int main(int argc, char *argv[])
{
    // ./client <IP>:<PORT> tracker_info.txt
    pthread_t tid;
    //int client_request = 1;

    if (argc != 3)
    {
        errorLog("Provide proper argument, ./client <IP>:<PORT>"
                 "tracker_info.txt\n");
    }

    string sock(argv[1]);
    IP = sock.substr(0, sock.find(':'));
    port = atoi(sock.substr(sock.find(':') + 1, sock.length()).c_str());

    initalizeTracker(argc, argv);

    pthread_t clientThread;
    pthread_t serverThread;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&serverThread, &attr, server, NULL);
    client();
    //main waits for all thread to terminate
    pthread_join(serverThread, NULL);
    pthread_attr_destroy(&attr);
    return 0;
}
