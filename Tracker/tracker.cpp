// Tracker Side
// Time: 9:30 PM, Nov 8, 2021

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
unordered_map<string, string> credentials;
unordered_map<string, vector<string>> groups;
// group id, members where first member is admin
unordered_map<string, vector<string>> groupPendingRequest;
unordered_map<string, unordered_map<string, vector<pair<string, string>>>> seederList;
// group name, filename, uploadedby, path
unordered_map<string, int> portNo; //username corresponding port no
// if port no is not avaiable or logout of the system then it will be -1
unordered_map<string,vector<string>> down;
// group id, files downloaded

void errorLog(string msg)
{
    std::cerr << msg << " " << strerror(errno) << "\n";
    exit(0);
}

void initalizeTracker(int argc, char *argv[])
{
    fstream openFile;
    openFile.open(argv[1], ios::in);
    if (openFile.is_open())
    {
        string input;

        if (strcmp(argv[2], "2") == 0)
        {
            getline(openFile, input);
            getline(openFile, input);
            getline(openFile, input);
            tracker2IP = input;
            getline(openFile, input);
            tracker2Port = stoi(input);
        }
        else
        {
            getline(openFile, input);
            tracker1IP = input;
            getline(openFile, input);
            tracker1Port = stoi(input);
        }

        openFile.close();
    }
    else
    {
        errorLog("Tracker info file not present in current"
                 "working directory\n");
    }
}

void *quit(void *inp)
{
    string line;
    while (1)
    {
        getline(cin, line);
        if (strcmp(line.c_str(), "quit") == 0)
        {
            break;
        }
    }
    exit(0);
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

string extractFileName(string path)
{
    string name;
    for (int i = path.length() - 1; i >= 0 && path[i] != '/'; --i)
        name += path[i];
    reverse(name.begin(), name.end());
    return name;
}

bool isInGroup(string userName, string groupName)
{
    if (groups.find(groupName) == groups.end())
        return false;
    for (int i = 0; i < groups[groupName].size(); ++i)
    {
        if (groups[groupName][i] == userName)
            return true;
    }
    return false;
}

void *process(void *input)
{
    //Get the socket descriptor
    int read_size;
    char *message;
    char client_message[1024];
    int sock = *(int *)input;
    bool login = false;
    string user;
    string msg;

    while ((read_size = read(sock, client_message, sizeof(client_message))) > 0)
    {
        string query(client_message);
        query.resize(read_size);
        string command = fetch(query, 1);

        std::cout <<"Query Passed to Tracker: "<< query << endl;

        if (strcmp(command.c_str(), "create_user") == 0)
        {
            //create_user <user_id> <passwd>

            string username = fetch(query, 2);
            string password = fetch(query, 3);
            //cout<<username<<" "<<password<<endl;
            if (credentials.find(username) == credentials.end())
            {
                credentials[username] = password;
                msg = "User Created Successfully";
            }
            else
            {
                msg = "User already exists, try different name";
            }
        }
        else if (strcmp(command.c_str(), "login") == 0)
        {
            //login <user_id> <passwd>
            string username = fetch(query, 2);
            string password = fetch(query, 3);
            read_size = read(sock, client_message, sizeof(client_message));
            // taking port number of the client

            if (login)
            {
                msg = "Log out current user then login";
            }
            else if (credentials.find(username) == credentials.end())
            {
                msg = "User Doesn't exists";
            }
            else if (credentials[username] != password)
            {
                msg = "Invalid credentials";
            }
            else
            {
                login = true;
                user = username;
                msg = "Logged in successfully";
                string query(client_message);
                query.resize(read_size);
                portNo[username] = atoi(query.c_str());
                //std::cout << portNo[username]<<std::endl;
            }
        }
        else if (strcmp(command.c_str(), "create_group") == 0)
        {
            // create_group <group_id>
            string groupid = fetch(query, 2);

            if (!login)
            {
                msg = "Log in first for this operation";
            }
            else if (groups.find(groupid) == groups.end())
            {
                msg = "Group created successfully";
                groups[groupid].push_back(user);
            }
            else
            {
                msg = "Group Already exists. Try different name";
            }
        }
        else if (strcmp(command.c_str(), "join_group") == 0)
        {
            // join_group <group_id>
            string groupid = fetch(query, 2);

            if (!login)
            {
                msg = "Log in first for this operation";
            }
            else if (groups.find(groupid) == groups.end())
            {
                msg = "Group of this name doesn't exist";
            }
            else
            {
                msg = "Group Join request sent";
                bool found = false;
                for (int i = 0; i < groups[groupid].size(); ++i)
                {
                    if (groups[groupid][i] == user)
                    {
                        found = true;
                        msg = "You are already part of this group";
                        break;
                    }
                }
                if (!found)
                {
                    for (int i = 0; i < groupPendingRequest[groupid].size(); ++i)
                    {
                        if (groupPendingRequest[groupid][i] == user)
                        {
                            found = true;
                            msg = "You are already sent the join request earlier"
                                  " Please wait for the admin to approve";
                            break;
                        }
                    }
                    if (!found)
                    {
                        groupPendingRequest[groupid].push_back(user);
                        msg = "Group join request sent";
                    }
                }
            }
        }
        else if (strcmp(command.c_str(), "leave_group") == 0)
        {
            // leave_group <group_id>
            string groupid = fetch(query, 2);

            if (!login)
            {
                msg = "Log in first for this operation";
            }
            else
            {
                bool found = false;
                for (int i = 0; i < groups[groupid].size(); ++i)
                {
                    if (groups[groupid][i] == user)
                    {
                        groups[groupid].erase(groups[groupid].begin() + i);
                        found = true;
                        break;
                    }
                }
                if (found)
                {
                    msg = "Leave group successfully executed";
                }
                else
                {
                    msg = "You are not the member of this group";
                }
            }
        }
        else if (strcmp(command.c_str(), "requests") == 0)
        {
            // requests list_requests <group_id>
            string groupid = fetch(query, 3);

            if (!login)
            {
                msg = "Log in first for this operation";
            }
            else if (groups.find(groupid) == groups.end())
            {
                msg = "Group of this name doesn't exist";
            }
            else if (groups[groupid][0] == user)
            {
                msg = "Pending List to join: ";
                for (int i = 0; i < groupPendingRequest[groupid].size(); ++i)
                {
                    msg += groupPendingRequest[groupid][i] + " ";
                }
            }
            else
            {
                msg = "Only admin have the right to see pending joining list";
            }
        }

        else if (strcmp(command.c_str(), "accept_request") == 0)
        {
            // accept_request <group_id> <user_id>
            string groupid = fetch(query, 2);
            string userid = fetch(query, 3);

            if (!login)
            {
                msg = "Log in first for this operation";
            }
            else if (groups.find(groupid) == groups.end())
            {
                msg = "Group of this name doesn't exist";
            }
            else if (groups[groupid][0] == user)
            {
                //msg = "Pending List to join: ";
                bool found = false;
                for (int i = 0; i < groupPendingRequest[groupid].size(); ++i)
                {
                    if (groupPendingRequest[groupid][i] == userid)
                    {
                        found = true;
                        groupPendingRequest[groupid].erase(groupPendingRequest[groupid].begin() + i);
                        break;
                    }
                }
                if (found)
                {
                    groups[groupid].push_back(userid);
                    msg = "User requests accepted";
                }
                else
                {
                    msg = "No request exists of given groupid and userid";
                }
            }
            else
            {
                msg = "Only admin have the right to accept pending joining list";
            }
        }
        else if (strcmp(command.c_str(), "list_groups") == 0)
        {
            // lists_groups
            msg = "List of all groups in networks are: ";
            for (pair<string, vector<string>> name : groups)
            {
                msg += name.first + " ";
            }
        }
        else if (strcmp(command.c_str(), "list_files") == 0)
        {
            // list_files <group_id>

            string groupID = fetch(query, 2);
            if (isInGroup(user, groupID))
            {
                msg = "List of files are: ";
                for (pair<string, vector<pair<string, string>>> ele : seederList[groupID])
                {
                    msg += ele.first + ", ";
                }
                if(msg.length()>0)msg.pop_back();
            }
            else
            {
                msg = "You are not the memeber of this group, so you can't see files";
            }
        }
        else if (strcmp(command.c_str(), "upload_file") == 0)
        {
            // upload_file <file_path> <group_id >
            //unordered_map<string, unordered_map<string, vector<pair<string,string>>>> seederList;
            // group name, filename, uploadedby, path
            if (!login)
            {
                msg = "Login required for this operations";
            }
            else
            {
                string path = fetch(query, 2);
                string groupid = fetch(query, 3);
                string fileName = extractFileName(path);
                if (isInGroup(user, groupid))
                {
                    bool found = false;
                    for (int i = 0; i < seederList[groupid][fileName].size(); ++i)
                    {
                        pair<string, string> &curr = seederList[groupid][fileName][i];
                        if (curr.first == user)
                        {
                            curr.second = path;
                            found = true;
                            msg = "File updated in the tracker";
                            break;
                        }
                    }
                    if (!found)
                    {
                        msg = "File Added";
                        seederList[groupid][fileName].push_back(make_pair(user, path));
                    }
                }
                else
                    msg = "You are not the memeber of this group so you can't upload";
            }
        }
        else if (strcmp(command.c_str(), "download_file") == 0)
        {
            // download_file <group_id> <file_name> <destination_path>
            //unordered_map<string, unordered_map<string, vector<pair<string,string>>>> seederList;
            // group name, filename, uploadedby, path
            string groupid = fetch(query, 2);
            string fileName = fetch(query, 3);
            if (isInGroup(user, groupid))
            {
                msg="";
                for (int i = 0; i < seederList[groupid][fileName].size(); ++i)
                {
                    pair<string, string> &curr = seederList[groupid][fileName][i];
                    msg+= to_string(portNo[curr.first]) + " " + curr.second + " ";
                }
                if(msg.length()>0){
                    msg.pop_back();
                    down[groupid].push_back(fileName);
                }

            }
            else
                msg = "";
        }
        else if (strcmp(command.c_str(), "logout") == 0)
        {
            // logout
            login = false;
            user = "";
            msg = "Logged Out successfully";
            portNo[user] = -1;
            // add functionality if logged out stop any sharing
        }
        else if (strcmp(command.c_str(), "show_downloads") == 0){
            msg="";
            for(pair<string,vector<string>> ele:down){
                msg="[C] [" + ele.first + "] ";

                for(string files:ele.second){
                    msg+=files+" ";
                }
                msg+="\n";
                msg+="D(Downloading), C(Complete) ";
            }
        }
        else
        {
            // stop_share <group_id> <file_name> or other wrong cmd;
            msg="Wrong command entered";
        }
        write(sock, msg.c_str(), msg.length() + 1);
    }
    /*
        //server side
	int fin= open(client_message,O_RDONLY);


	//Receive a message from client
	while( (read_size = read(fin,client_message , sizeof(client_message))) > 0 )
	{
		//Send the message back to client
		write(sock , client_message ,read_size);
	}
	//cout<<client_message<<" ";
	
	//strcpy(client_message,"EOF-1-EOF");
	//cout<<client_message;
	//write(sock , client_message ,10);

	if(read_size == 0)
	{
		
		puts("File Downloaded successfully\n");
		fflush(stdout);
	}
	else if(read_size == -1)
	{
		perror("recv failed");
    }
    */

    std::cout.flush();
    close(sock);
    pthread_exit(NULL);
    return 0;
}

int main(int argc, char *argv[])
{
    //  ./tracker tracker_info.txt tracker_no
    if (argc != 3)
    {
        errorLog("Provide proper argument, ./tracker "
                 "tracker_info.txt 1 \n");
    }

    freopen("errorLog.txt", "a", stderr);
    time_t now = time(NULL);
    // convert now to string form
    string dt = ctime(&now);
    cerr << "\n\nThe session starts at: " << dt << "\n";

    initalizeTracker(argc, argv);
    //cout<<tracker1IP<<" "<<tracker1Port<<"\n";
    //cout<<tracker2IP<<" "<<tracker2Port<<"\n";

    pthread_t quitcheck;
    if (pthread_create(&quitcheck, NULL, quit, NULL) < 0)
    {
        // Error in creating thread
        errorLog("Failed to create quit thread\n");
    }

    socklen_t addr_size;
    struct sockaddr_storage serverStorage;

    int trackerSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (trackerSocket < 0)
        errorLog("Could not create socket");

    int opt = 1;

    if (setsockopt(trackerSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        errorLog("Not able to perfrom setsocket operation\n");
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr(tracker1IP.c_str()); //INADDR_ANY;
    serverAddr.sin_port = htons(tracker1Port);
    serverAddr.sin_family = AF_INET;

    // Bind the socket to the address and port number.
    if (bind(trackerSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        errorLog("binding has failed. Try different ip and port\n");
    }
    std::cout << "bind successful\n";

    // Listen on the socket, with 50 max connection requests queued
    while (listen(trackerSocket, 50))
    {
        std::cout << "Queue Full!!! Retrying after 10 sec\n";
        sleep(10);
    }
    std::cout << "Listening\n";

    int newSocket, i = 0;
    std::cout << "Waiting for incoming connections...\n";

    pthread_t tid[60]; //unsigned long int
    int index = 0;

    while (true)
    {
        addr_size = sizeof(serverStorage);
        // Extract the first connection in the queue
        newSocket = accept(trackerSocket,
                           (struct sockaddr *)&serverStorage, &addr_size);

        if (newSocket < 0)
            break;
        std::cout << "Connection accepted by tracker:\n";

        int *new_sock = (int *)malloc(1);
        *new_sock = newSocket;

        if (pthread_create(&tid[index], NULL, process, (void *)new_sock) < 0)
        {
            // Error in creating thread
            errorLog("Failed to create thread for the client\n");
        }
        else
            std::cout << "Handle Assigned\n";

        ++index;
        /*if (index == 60)
        {
            for (index = 0; index < 60; ++index)
            {
                // wait till all thread get freed
                pthread_join(tid[index], NULL);
            }
            index = 0;
        }
        */
    }
    if (newSocket < 0)
    {
        errorLog("Problem occured during creating accept\n");
    }

    return 0;
}