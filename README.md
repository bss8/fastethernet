# TCP Client/Server Fast Ethernet Simulation
Author: Borislav S. Sabotinov (bss64)
CS5310 Network

View in a browser for best results so markdown and HTML may be parsed. 
Code and README on GitHub: 

## Overview 

No user input is required for the server (CSP).     
Port is predefined as 4410 (I selected it at random).

User may specify hostname in the form of IP address when launching client (SP).  
If they do not, ZEUS.CS.TXSTATE.EDU (147.26.231.156) will be used for the server by default. 
In that case, client should be ran on EROS.CS.TXSTATE.EDU (147.26.231.153). 

### Why C++11?

A small aside but worth mentioning. Why use c++11 compiler?
C++11 now supports: 
- lambda expressions, 
- automatic type deduction of objects, 
- uniform initialization syntax, 
- delegating constructors, 
- deleted and defaulted function declarations, 
- nullptr,  
- rvalue references

"The C++11 Standard Library was also revamped with new algorithms, new container classes, atomic operations, type traits, regular expressions, new smart pointers, async() facility, and of course a multithreading library."
https://smartbear.com/blog/develop/the-biggest-changes-in-c11-and-why-you-should-care/

## Build
From the project root level directory, run this command:    
`make all`

## Run Server
On **ZEUS** - From the project root level directory, run this command:     
`./CSP &`

## Run Client
On **EROS** - From the project root level directory, run this command:     
`./SP <HOST_IP> <STATION_NUM> &`

## <span style="color:red">**Terminate Processes (Important!)**</span>

**The processes will not stop on their own. The user MUST terminate them 
using one of the two methods outlined below for GRACEFUL exit!**

Helper Script Method *(Preferred)*: 

To terminate the CSP (single) and SP (multiple) processes, please use the 
`kill_pids.sh` helper script. It will terminate processes with SP in the name, so 
ensure there are no other processes running. It is a graceful kill using kill -s SIGINT, not -9. The interrupt is caught in C++ and handled to 1) gracefully close the socket and 2) invoke the CSP object destructor.

Manual Method: 

Alternatively, one may manually terminate these processes:     
1. ps -ef | grep SP
2. Should see 1 or more PIDs for CSP and SP
3. kill -s SIGINT \<PID> where PID is the process ID you wish to terminate

It is strongly recommended to kill SP processes FIRST, then terminate CSP. Also use -s SIGINT, not -9.  

## Simulation Output / Logs

Logs will be available under the `logs` directory, located in the project root level directory. 
There will be `CSP_out.log` and `SP_out.log` files for a full run. 

## Example Run:    
Let us run the server on ZEUS (147.26.231.156) and the client on EROS (147.26.231.153). 
1. Run Server (ZEUS): `./CSP &`
2. Run Client:  `./SP 1 147.26.231.156 &`
3. Observe output printed to console
4. Kill processes (gracefully): `./kill_pids.sh`
5. Navitgate to logs: `cd logs`
6. View logs: `vi CSP_out.log` or `vi SP_out.log`


# References

I am documenting sources and websites I referenced for this project. 
Just like in an essay when we reference another text, we give credit where it's due.  
I also want to refer back to these for future studies and refreshers. 

1.  A.S.Tanenbaum and D.J.Wetherall, Computer Networks (5th ed.). Prentice-Hall, 2011. ISBN13: 978-0-13-212695-3.
2.  W. R. Stevens, Bill Fenner, and Andrew M. Rudoff. UNIX   Network Programming – Networking APIs: Sockets and XTI (3rd ed.). Addison-Wesley, 2004. ISBN: 0-13-141155-1.
3.  Stevens, Richard. "UNIX Network Programming: The sockets networking API".
4.  https://www.bogotobogo.com/cplusplus/sockets_server_client.php
5.  https://stackoverflow.com/questions/6973749/checking-whether-include-is-already-declared
6.  https://stackoverflow.com/questions/7058779/where-is-function-err-sys-defined
7.  https://stackoverflow.com/questions/21113919/difference-between-r-and-w-in-fopen 
8.  https://stackoverflow.com/questions/16782103/initializing-default-values-in-a-struct 
9.  https://smartbear.com/blog/develop/the-biggest-changes-in-c11-and-why-you-should-care/ 
10. https://stackoverflow.com/questions/388242/the-definitive-c-book-guide-and-list 
11. https://stackoverflow.com/questions/24331687/is-it-possible-to-print-the-awk-output-in-the-same-line
12. https://stackoverflow.com/questions/19366503/in-c-when-interrupted-with-ctrl-c-call-a-function-with-arguments-other-than-s