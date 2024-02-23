#pragma once
#include <iostream>
namespace guard::utils{





class Log{
public:
    class Debug{
public:
    template <typename T>
    const Debug& operator<<(const T& val) const{
            std::cerr << val ;    
            return *this;
    }

    Debug(){
         std::cerr<< "[DEBUG] ";
    }
    ~Debug(){
        std::cerr<<"\n";
    }
};

class Info{
public:    

    template <typename T>
    const Info& operator<<(const T& val) const{
            std::cerr << val ;    
            return *this;
    }
    Info(){
         std::cerr<< "[INFO] ";
    }
    ~Info(){
        std::cerr<<"\n";
    }

};
class Warning{
public:

    template <typename T>
    const Warning& operator<<(const T& val) const{
            std::cerr<< val;    
            return *this;
    }
    Warning(){
         std::cerr<< "[WARINING] ";
    }
    ~Warning(){
        std::cerr<<"\n";
    }

};
class Error{
public:

    template <typename T>
    const Error& operator<<(const T& val) const{
            std::cerr<< val;    
            return *this;
    }
    Error(){
         std::cerr<< "[ERROR] ";
    }
    ~Error(){
        std::cerr<<"\n";
    }

};

class Test{
public:
    template <typename T>
    const Test& operator<<(const T& val) const{
            std::cerr<< val;    
            return *this;
    }
    Test(){
         std::cerr<< "[TEST] ";
    }
    ~Test(){
        std::cerr<<"\n";
    }

};
  
};



}