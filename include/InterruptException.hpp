#include <exception>
#include <signal.h>

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