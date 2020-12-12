#include "../include/StationProcess.hpp"

/**
 * @author Borislav Sabotinov
 * Implementation for the StationProcess.hpp file
 */ 

/**
 * Constructor for our SP object, where we intialize some variables
 */ 
StationProcess::StationProcess()
{
    cout << "Creating SP object." << endl;
    this->station_num = 0;
    this->recvFrames[RECVFRAME] = { -1 };
}


/**
 * Destructor for SP, where we close the socket and the log file
 */ 
StationProcess::~StationProcess()
{
    cout << "Deleting SP object." << endl;
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);
	fclose(this->write_station_log_file);
}

/**
 * if wait is required, we pause reading data from the SP data file
 */ 
bool StationProcess::wait(char* buff)
{
    char s1[10], s2[10], s3[10], s4[10];
    sscanf(buff, "%s %s %s %d %s", s1, s2, s3, &frame_need, s4);
    
    // check if the frame was correctly received
    for(int i = 0; i < RECVFRAME; i++)
        if(frame_need == recvFrames[i])
            return false;
    
    return true;
}

/**
 * process the SP file by reading the frames from it
 */ 
void StationProcess::read_file(char* buff)
{
    char comparison_word[10] = "Frame";
    char word[10];
    bzero(word, sizeof(word));
    sscanf(buff, "%s", word);
    
    
    if(strncasecmp(word, comparison_word, strlen(comparison_word)) == 0)
    {
        // Send out the frame
        char s1[10], s2[10], s3[10], s4[10];
        int seq_num, dest;
        sscanf(buff, "%s %d %s %s %s %d", s1, &seq_num, s2, s3, s4, &dest);
        

        char temp[FRAME_SIZE];
        bzero(temp, sizeof(temp));
        char word[10];
        bzero(word, sizeof(word));
        memcpy(word, REQUEST, sizeof(word));
        
        
        save_buff(seq_num, station_num, dest, word, temp);
        
        if(write(client_fd, temp, sizeof(temp)) < 0) 
            err_sys("Write error", -1);
            
    
        fprintf(write_station_log_file, "Send a request to CSP to send data frame with sequence # %d to SP # %d\n", seq_num, dest);
        fflush(write_station_log_file);
    }
    else
    {
        if_wait = wait(buff);
    }
}

/** 
 * SP uses this function to respond to CSP according to different situations
 */ 
void StationProcess::process_data(char* buff)
{
    struct Frame frame;
    read_buffer(&frame, buff);
    
    int seq_num = frame.sequence_number;
    int src = frame.source_address;
    int dest = frame.destination_address;

    if (seq_num < 0 || seq_num > 10) 
	{
		frame.sequence_number = 0;
		seq_num = 0;
	}

	if (src < 0 || src > 10) 
	{
		frame.source_address = 0;
		src = 0;
	}

	if (dest < 0 || dest > 10) 
	{
		frame.destination_address = 0;
		dest = 0;
	}

    char word[10];
    bzero(word, sizeof(word));
	if (src < 0 || src > 10 ) 
		memcpy(word, NULL, 0);
	else 
    	memcpy(word, frame.data, sizeof(frame.data));
    
    
    cout << "Receiving data frame (seq, src, dest, data): " << seq_num << ", " << src << ", " << dest << ", "
         << word << endl;
    
    char word[10];
    bzero(word, sizeof(word));
    memcpy(word, frame.data, sizeof(word));
    
    if(strncasecmp(word, NEW, strlen(word)) == 0)
    {
        // receive a frame from other SP
        for(int i = 0; i < RECVFRAME; i++)
        {
            if(recvFrames[i] == -1)
            {
                recvFrames[i] = seq_num;
                break;
            }
        }
        
        if(if_wait && frame_need == seq_num)
        {
            if_wait = false;
            frame_need = -1;
        }
        
        fprintf(write_station_log_file, "Received data frame from SP %d\n", src);
    }
    else if(strncasecmp(word, POSITIVE, strlen(word)) == 0)
    {
        fprintf(write_station_log_file, "Received approval from CSP to send data frame with sequence # %d to SP # %d\n", seq_num, dest);
        fflush(write_station_log_file);
        
        // send out the data frame
        char info[10];
        bzero(info, sizeof(info));
        memcpy(info, NEW, sizeof(info));
        char sent[FRAME_SIZE];
        bzero(sent, sizeof(sent));
        save_buff(seq_num, src, dest, info, sent);
        
        cout << "Send data (seq, src, dest, info): " << seq_num << ", " << src << ", " << dest << ", " << info << endl;
        if(write(client_fd, sent, sizeof(sent)) < 0) 
            err_sys("ERROR writing on socket!", -1);
    
    }
    else if(strncasecmp(word, SEND, strlen(word)) == 0)
    {
        fprintf(write_station_log_file, "Send data frame with sequence # %d to SP # %d\n", seq_num, dest);
        fflush(write_station_log_file);
    }
    else if(strncasecmp(word, NEGATIVE, strlen(word)) == 0)
    {
        fprintf(write_station_log_file, "Receive refuse from CSP to send data frame with sequence # %d to SP # %d\n", seq_num, dest);
        fflush(write_station_log_file);
        
        // we need to resend because CSP gave us a NEGATIVE reply
        char info[10];
        bzero(info, sizeof(info));
        memcpy(info, REQUEST, sizeof(info));
        char sent[FRAME_SIZE];
        bzero(sent, sizeof(sent));
        save_buff(seq_num, src, dest, info, sent);
        
        if(write(client_fd, sent, sizeof(sent)) < 0)
            err_sys("Write error", -1);
    
        fprintf(write_station_log_file, "Resend the request to CSP to send data frame with sequence # %d to SP # %d\n", seq_num, dest);
    }
    fflush(write_station_log_file);
}


/* TCP connection is a long connection and data may be sent multiple times
*  "The client sends too many packets in a period of time, and the server 
*  does not complete all processing. Then the data will be overstocked, resulting in stickiness."
*  https://developpaper.com/solutions-to-the-problem-of-packet-sticking-and-unpacking-in-golang-tcp/
*/
void StationProcess::partition_buffer(char* buff, int len)
{
    // printf("Client receive: %d\n", len);
    cout << "Client received length: " << len << endl;
    int k = 0;
    for(int i = 0; i < len; i += FRAME_SIZE)
    {
        char* sub_buf = (char*)malloc(FRAME_SIZE * sizeof(char));
        for(int j = 0; j < FRAME_SIZE && k < len; j++)
            sub_buf[j] = buff[k++];
        process_data(sub_buf);
        free(sub_buf);
    }
}

/**
 * We perform initialization by creating a socket and open a connection on it
 */ 
void StationProcess::init()
{
    this->if_wait = false;
	this->frame_need = -1;
	this->station_num = station;
    cout << "station_number: " << this->station_num << endl;
	
    // extern int socket (int __domain, int __type, int __protocol) __THROW;
	// if protocol is 0 one is auto-chosen; we want TCP
	if((this->client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) 
        err_sys("ERROR creating socket!", -4);
		
	
	struct sockaddr_in client_addr;
	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(server);
	client_addr.sin_port = htons(PORT); 

	
	if(connect(this->client_fd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0)
	{
		err_sys("ERROR connecting on socket!\n", -5);
	}	
	else
    {
        cout << "Client connected successfully!" << endl;	
    }

    // process clients as they come
    process_station();
}

/**
 * we process station logic such as sending requests to the switch
 * and processing the data files which contain our data 
 * driving the execution of this program (sending frames or waiting to receve them)
 */ 
void StationProcess::process_station()
{
    // we loop continuously; since the outer function init() which calls this
    // is wrapped up in a try/catch, we will catch the SIGINT 
    // interrupt the exit gracefully
    while(true)
    {
        bzero(buf, sizeof(buf));
        FD_ZERO(&rset);
        int inputf = fileno(this->read_station_data_file);
        FD_SET(inputf, &rset);      
        FD_SET(this->client_fd, &rset);
        int maxfd;

        if (inputf < this->client_fd)
            maxfd = this->client_fd;
        else 
            maxfd = inputf;
        
        //Returns the number of ready descriptors, or -1 for errors.
        int num_ready_descr = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (num_ready_descr == -1) 
        {
            err_sys("ERROR on invoking select()!", -10);
        }

        // we look at the socket and check if wait is true and FD_ISSET
        if(!this->if_wait && FD_ISSET(inputf, &rset)) 
        {
            bzero(buf, sizeof(buf));
            
            if(!feof(this->read_station_data_file))
            {
                if(fgets(buf, sizeof(buf) - 1, this->read_station_data_file) != NULL)
                    this->read_file(buf);
            }
        }
        
        // if our connection changes read the data from our socket 
        if(FD_ISSET(this->client_fd, &rset)) 
        {
            bzero(buf, sizeof(buf));
            if((this->read_return_value = read(this->client_fd, buf, sizeof(buf) - 1)) < 0)
            {
                cout << "Error reading!" << endl;
                continue;
            }
            else
            {
                this->partition_buffer(buf, this->read_return_value);
            }
        }
    } // end while
}


/**
 * Main driver of the SP / Client
 * We keep it short and readable. 
 * Create an instance of StationProcess, initialize it, and process
 * the data file, sending frames or waiting as described in the station files.
 * Standard arguments: 
 * @arg int argc to count how many arguments are provided by user on launch
 * @arg char* argv[] array to hold the arguments passed via cmdline
 */ 
int main(int argc, char* argv[])
{
	if(argc < 2 || argc > 3) 
    { 
        err_sys("Enter the station # (required) and server IP address (optional). \nUsage: ./SP <STATION_NUM> <SVR_IP> &", -9); 
    }

    cout << "Initializing client....." << endl;
    print_art();

    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sig_to_exception;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    StationProcess* SP = new StationProcess();
	
    // second argument is the station number, so 1 is S1.txt
	SP->station = atoi(argv[1]);
    
    if (argc == 3)
    {
        // server/host is our 3rd (optional) argument
        SP->server = argv[2];
    }
    else
    { 
        SP->server = ZEUS_IP;  
        cout << "Using ZEUS_IP for connecting to the server....." << endl;
    }

    cout << "Launching client for Station #" << SP->station << " connection to host: " << SP->server <<endl;
	
    // if the file does not exist, we want to create it
	// https://linux.die.net/man/3/fopen
	SP->write_station_log_file = fopen("./logs/SP_out.log", "w+");
    if(SP->write_station_log_file == NULL) 
        err_sys("Open file error\n", -3);
	
	// read data frames and waits from the input files
    // since we have 10 stations, files are S0 thru S9 with a .txt extension
	char s1[2] = "S";
	char s2[5] = ".txt";
	char *path = (char*)malloc(strlen(s1) + strlen(s2) + 1);

	sprintf(path, "./resources/data/%s%d%s", s1, SP->station, s2);

    cout << "Data file path is: " << path << endl;
	
    // we want to read only
    SP->read_station_data_file = fopen(path, "r");
	if(SP->read_station_data_file == NULL)
    {
        err_sys("Error opening SP file!", -3);
    }
	
    // wrap in try to catch interrupt and gracefully exit
    try 
    {
        SP->init();
    }
    catch(InterruptException &e)
	{
		cerr << "Interrupt SIGINT exception caught - will destroy SP object. \n Goodbye! :)" << endl;
		// destructor for CSP object is automatically invoked

		print_art();
	}

    // We may not reach this if SIGINT interrupt is provided on cmdline 
	// but need to close socket and file in case we do
	// No more receptions or transmissions.  
	close(SP->client_fd);
	fclose(SP->read_station_data_file);
	fclose(SP->write_station_log_file);
    
    return EXIT_SUCCESS; //0
}
