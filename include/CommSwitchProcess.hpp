#include "CommonHelpers.hpp"

/**
 * @author Borislav Sabotinov
 * Class definition for the communication switch process, which acts as our server. 
 */ 
class CommSwitchProcess
{
    public:
        FILE* write_csp_log_file;
        int client[FD_SETSIZE];
        struct Frame data_queue[QUEUE_SIZE];       
        struct Frame request_queue[QUEUE_SIZE];    

        CommSwitchProcess();
        ~CommSwitchProcess();

        bool is_data_queue_full();
        bool is_req_queue_full();
        void process_frame(int socket_fd, char* buf);
        void partition_buffer(char* buf, int len, int socket_fd);

        void init();
        void process_connections();
        void process_other_file_descriptors();
        void process_data_queue();
        void record_empty_pos_in_data_q(int (&position)[QUEUE_SIZE], int* j);
        void process_request_queue(int (&position)[QUEUE_SIZE], int* j);

    private:
        ssize_t	n;
        fd_set rset, allset;
        // defined in <netinet/in.h>, INET_ADDRSTRLEN is 16
        char ipv4[INET_ADDRSTRLEN];
        
        socklen_t client_len;
        struct sockaddr_in	client_addr, server_addr;
        int	sock_fd, max_i, max_fd, conn_fd, num_ready_descriptors;
};