#include <exception>
#include <signal.h>

/**
 * @author Borislav Sabotinov
 * Class definition and helper method (which is not associated with an instance of the Interrupt class).
 * Allows us to catch an interrupt signal SIGINT from the command line
 * and raise an exception. We can then handle closing the socket and destroying the CSP or SP instances gracefully. 
 */ 
class InterruptException : public std::exception
{
    public:
        InterruptException(int signal) : Signal(signal)
        {
        }
    private:
        int Signal;
};

void sig_to_exception(int signal)
{
  throw InterruptException(signal);
}