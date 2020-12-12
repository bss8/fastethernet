#include "CommonHelpers.hpp"

/**
 * @author Borislav Sabotinov
 * Class defining a station process and the attributes and behavior a station needs
 */ 
class StationProcess 
{
    public:
        int client_fd;
        FILE* write_station_log_file;
        FILE* read_station_data_file;
        int port_num;
        int station_num;

        int recvFrames[RECVFRAME];  // 30
        bool if_wait;
        int frame_need;
        int read_return_value; 

        int station;
        const char* server;

        StationProcess();
        ~StationProcess();

        bool wait(char* buff);
        void read_file(char* buff);
        void process_data(char* buff);
        void partition_buffer(char* buff, int length);

        void init();
        void process_station();

    private: 
        fd_set rset;
	    char buf[MAX];

};