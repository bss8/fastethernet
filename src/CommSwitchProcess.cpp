#include "../include/CommSwitchProcess.hpp"



CommSwitchProcess::CommSwitchProcess()
{
	cout << "Creating CSP object." << endl;
}

CommSwitchProcess::~CommSwitchProcess() 
{
	cout << "Deleting CSP object." << endl;
	shutdown(sock_fd, SHUT_RDWR);
	close(sock_fd);
	fclose(this->write_csp_log_file);
}

bool CommSwitchProcess::is_dataQueue_full() // checke if the data queue is full or not
{
    for(int i = 0; i < QUEUE_SIZE; i++)
    {
        if(dataQueue[i].sequence_number == -1)
            return false;
    }
    return true;
}

bool CommSwitchProcess::is_reqQueue_full() // check if the request queue is full or not
{
    for(int i = 0; i < QUEUE_SIZE; i++)
    {
        if(requestQueue[i].sequence_number == -1)
            return false;
    }
    return true;
}


// process frames from SP
void CommSwitchProcess::process_frame(int client_socket_fd, char* buf)
{
    struct Frame data;
    read_buffer(&data, buf);
    int seq_num = data.sequence_number;
    int src = data.source_address;
    int dest = data.destination_address;

    char word[MAX];
    bzero(word, sizeof(word));
    memcpy(word, data.data, sizeof(data.data));

	// if no data then done
    if(word[0] == '\0') 
		return;
	
	cout << "Receiving data frame (seq_num, src, dest, word): " << seq_num << ", " << src << ", " << dest << ", " << word << endl;

    if(strncasecmp(word, REQUEST, strlen(word)) == 0) // get requests from SP
    {
        if(is_dataQueue_full() && is_reqQueue_full())
        {
            char data[MAX];
            bzero(data, sizeof(data));
            memcpy(data, NEGATIVE, sizeof(data));
            char sent[FRAME_SIZE];
            bzero(sent, sizeof(sent));
            save_buff(seq_num, src, dest, data, sent);

            if(write(client_socket_fd, sent, sizeof(sent)) < 0)
                err_sys("Write error", -1);
        }
        else if(!is_dataQueue_full()) // check dataQueue
        {
            char data[MAX];
            bzero(data, sizeof(data));
            memcpy(data, POSITIVE, sizeof(data));
            char sent[FRAME_SIZE];
            bzero(sent, sizeof(sent));
            save_buff(seq_num, src, dest, data, sent);

            // printf("Send data : %d %d %d %s\n", seq_num, src, dest, info);
            if(write(client_socket_fd, sent, sizeof(sent)) < 0)
                err_sys("Write error", -1);
			cout << "Send data (seq, src, dest, data): " << seq_num << ", " << src << ", " << dest << ", " << data << endl;

            fprintf(write_csp_log_file, "Send approval to SP %d \n", src);
            fflush(write_csp_log_file);

        }
        else if(!is_reqQueue_full()) // check request_queue
        {
            // take the request into queue
            for(int i = 0; i < QUEUE_SIZE; i++)
            {
                if(requestQueue[i].sequence_number == -1)
                {
                    requestQueue[i] = data;
                    break;
                }
            }
        }
    }
    else if(strncasecmp(word, NEW, strlen(word)) == 0)
    {
        // data frame from SP
        // printf("Save the frame to dataQueue\n");
		cout << "Saving frame in data queue....." << endl;

        for(int i = 0; i < QUEUE_SIZE; i++)
        {
            if(dataQueue[i].sequence_number == -1)
            {
                dataQueue[i] = data;
                break;
            }
        }

        fprintf(write_csp_log_file, "Receive the data frame from (src) SP %d to (dest) SP %d) \n", src, dest);
        fflush(write_csp_log_file);
    }
}

// partition the buffer to solve the TCP sticky packet problem
void CommSwitchProcess::partition_buffer(char* buf, int len, int client_socket_fd)
{
    int k = 0;
    for(int i = 0; i < len; i += FRAME_SIZE)
    {
        char* sub_buf = (char*)malloc(FRAME_SIZE * sizeof(char));
        for(int j = 0; j < FRAME_SIZE && k < len; j++)
            sub_buf[j] = buf[k++];
        process_frame(client_socket_fd, sub_buf);
        free(sub_buf);
    }
}

void CommSwitchProcess::init()
{
	int opt_val = 1;
	
	sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_fd == -1)
		err_sys("Error creating socket!", -4);
		
	// setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
	// extern int socket (int __domain, int __type, int __protocol) __THROW;
	// if protocol is 0 one is auto-chosen; we want TCP
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(int));
	
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);

	bind(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr));

	listen(sock_fd, BACKLOG);

	max_fd = sock_fd;
	max_i = -1;
	for (int i = 0; i < FD_SETSIZE; i++)
		this->client[i] = -1;
	
	this->write_csp_log_file = fopen("./logs/CSP_out.log", "a+");
    if(this->write_csp_log_file == NULL) 
		err_sys("open file", -3);
	
	// UCOMMENT BELOW FOR DEBUGGING
	// If using a different compiler, may need to verify struct variables are initialized 

	// for(int i = 0; i < QUEUE_SIZE; i++) 
	// {
	// 	cout << CSP->dataQueue[i].frame;
	// 	cout << CSP->requestQueue[i].key;
	// }
	
	FD_ZERO(&allset);
	FD_SET(sock_fd, &allset);
}

void CommSwitchProcess::process_connections()
{
	struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_to_exception;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

	//wrap in try to catch interrupt and gracefully exit
	try {
		while(true)
		{
			FD_ZERO(&rset);
			rset = allset;		

			// Returns the number of ready descriptors, or -1 for errors.
			num_ready_descriptors = select(max_fd + 1, &rset, NULL, NULL, NULL);

			if (num_ready_descriptors == -1)
			{
				err_sys("Error on switch() call!", -10);
			}

			if (FD_ISSET(sock_fd, &rset)) 
			{	
				client_len = sizeof(client_addr);
				conn_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_len);

				if (conn_fd == -1) 
				{
					err_sys("Error accepting connection!", -7);
				}
				/*
				* 	"inet_ntop does the reverse conversion, from numeric (addrptr) to presentation
				*	(strptr). The len argument is the size of the destination, to prevent the function from
				*	overflowing the caller's buffer." (Network Programming pg. 122)
				*/
				cout << "New client accepted: " << inet_ntop(AF_INET, (void *)&client_addr.sin_addr, ipv4, sizeof(client_addr)) 
					<< " on port " << ntohs(client_addr.sin_port) << endl;
				int i;
				for (i = 0; i < FD_SETSIZE; i++)
				{
					if (this->client[i] < 0) 
					{
						this->client[i] = conn_fd;	

						cout << "Client: " << i << ", client file descriptor: " << conn_fd << endl;
						break;
					}
				}
					
				if (i == FD_SETSIZE)
					err_sys("ERROR - too many clients", -6);

				FD_SET(conn_fd, &allset);	
				if (conn_fd > max_fd) max_fd = conn_fd;
				if (i > max_i) max_i = i;

			}

			process_other_file_descriptors();
			
			process_data_queue();
			
			// record the empty position in the dataQueue
			int pos[QUEUE_SIZE];
			int j = 0;
			for(int i = 0; i < QUEUE_SIZE; i++)
			{
				if(this->dataQueue[i].sequence_number == -1)
					pos[j++] = i;
			}
			
			int k = 0;
			for(int i = 0; i < QUEUE_SIZE; i++)
			{
				if(this->requestQueue[i].sequence_number != -1)
				{
					if(k < j)
					{
						
						this->dataQueue[pos[k]] = this->requestQueue[i];
						k++;
						
						// sending feedback to src, src will send frame to CSP
						int seq_num = this->requestQueue[i].sequence_number;
						int src = this->requestQueue[i].source_address;
						int dest = this->requestQueue[i].destination_address;
						
						char sent[FRAME_SIZE];
						bzero(sent, sizeof(sent));
						char word[MAX];
						bzero(word, sizeof(word));
						memcpy(word, POSITIVE, sizeof(word));
						save_buff(seq_num, src, dest, word, sent);
						
						write(this->client[src - 1], sent, sizeof(sent));
						
						fprintf(this->write_csp_log_file, "Send approval to SP %d \n", src);
						fflush(this->write_csp_log_file);
						
						this->requestQueue[i].sequence_number = -1;
					}	
				}
			}

		} // end while
	} // end try
	catch(InterruptException &e)
	{
		cerr << "Interrupt SIGINT exception caught - will destroy CSP object. \n Goodbye! :)" << endl;
		// destructor for CSP object is automatically invoked
		print_art();
	}

	// No more receptions or transmissions.  
	shutdown(sock_fd, SHUT_RDWR);
	close(sock_fd);
	fclose(this->write_csp_log_file);
}

void CommSwitchProcess::process_other_file_descriptors()
{
	for (int i = 0; i <= max_i; i++) // read data from different fd
		{	
			char buf[MAX];
			int client_socket_fd; 

			bzero(buf, sizeof(buf));
			if ((client_socket_fd = this->client[i]) < 0)
				continue;
			if (FD_ISSET(client_socket_fd, &rset)) 
			{
				cout << "client socket file descriptor = " << client_socket_fd << endl;

				bzero(buf, sizeof(buf));
				if((n = read(client_socket_fd, buf, sizeof(buf) - 1)) == 0) 
				{
					// close client
					close(client_socket_fd);
					FD_CLR(client_socket_fd, &allset);
					this->client[i] = -1;

					cout << "READ ERROR while processing fd!" << endl;
				} 
				else
				{
					cout << "Server received ssize_t: " << n << endl;

					this->partition_buffer(buf, n, client_socket_fd);
				}			
			}
		}
}

void CommSwitchProcess::process_data_queue()
{
	for(int i = 0; i < QUEUE_SIZE; i++) 
	{
		if(this->dataQueue[i].sequence_number != -1)
		{
			int seq_num = this->dataQueue[i].sequence_number;
			int src = this->dataQueue[i].source_address;
			int dest = this->dataQueue[i].destination_address;
			int fd = this->client[dest - 1];
			
			// if fd does not exist, keep going
			if(fd <= 0) 
				continue;
			
			char sent[FRAME_SIZE];
			bzero(sent, sizeof(sent));
			char word[MAX];
			bzero(word, sizeof(word));
			memcpy(word, NEW, sizeof(word));
			save_buff(seq_num, src, dest, word, sent);
			
			cout << "Sending data frame to SP (seq, src, dest, word): " << seq_num << ", " << src << ", " << dest << ", " 
				 << word << endl;

			write(fd, sent, sizeof(sent));
			this->dataQueue[i].sequence_number = -1;
			
			fprintf(this->write_csp_log_file, "Forward the data frame from SP%d to SP%d \n", src, dest);
			fflush(this->write_csp_log_file);
			
			char data[FRAME_SIZE];
			bzero(data, sizeof(data));
			char info[MAX];
			bzero(info, sizeof(info));
			memcpy(info, SEND, sizeof(info));
			save_buff(seq_num, src, dest, info, data);
			
			cout << "Send response data (seq, src, dest, info) " << seq_num << ", " << src << ", " << dest << ", " << word 
				 << " to socket: " << this->client[src-1] << endl;
			
			write(this->client[src - 1], data, sizeof(data));	
		}
	}
}

void CommSwitchProcess::record_empty_pos_in_data_q(int (&position)[QUEUE_SIZE], int* j)
{

}

void CommSwitchProcess::process_request_queue(int (&position)[QUEUE_SIZE], int* j)
{

}


int main(int argc, char* argv[])
{
	if(argc != 1)
	{ 
		err_sys("No arguments are required. Usage: ./CSP", -9);
	}

	cout << "Welcome! Initializing server....." << endl;
	print_art();

	CommSwitchProcess* CSP = new CommSwitchProcess();
	CSP->init();
	CSP->process_connections();
	
    
    return EXIT_SUCCESS;
}