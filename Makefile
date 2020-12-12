# the compiler: gcc for C program, define as g++ for C++
# cc is for compiling C files, while cxx is for compiling C++ files.
CXX = g++
PROJDIR := $(CURDIR)
SOURCEDIR := $(PROJDIR)/src
BUILDDIR := $(PROJDIR)/build
VERBOSE = true
VPATH = $(SOURCEDIR)

# compiler flags:
# we use CXXFLAGS because we are using C++; for C, we would use CFLAGS
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
#  -std=c++11 : TXST Linux hosts do not have c++14 or c++17, so we use c++11
#  Regarding why I use C++11, please refer to README.md
CXXFLAGS  = -g -std=c++11

csp: CommSwitchProcess.cpp 
	$(CXX) $(CXXFLAGS) -o CSP ./src/CommSwitchProcess.cpp
 
sp: StationProcess.cpp
	$(CXX) $(CXXFLAGS) -o SP ./src/StationProcess.cpp

all: sp csp
