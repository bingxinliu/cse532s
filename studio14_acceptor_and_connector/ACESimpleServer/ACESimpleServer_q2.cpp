#include <iostream>
#include <string>
#include <sstream>
#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include "ace/SOCK_Acceptor.h"

#include "ace/Event_Handler.h"

#include "ace/Reactor.h"

using namespace std;

#define SUCCESS 0
#define BUFFER_SIZE 256

static size_t client_counter = 0;

class server_read : public ACE_Event_Handler
{
    ACE_SOCK_Stream* ace_sock_stream = nullptr;
public:

    ~server_read()
    {
        this->ace_sock_stream = nullptr;
    }
    void set(ACE_SOCK_Stream* ss)
    {
        this->ace_sock_stream = ss;
    }

    virtual ACE_HANDLE get_handle() const
    {
        return this->ace_sock_stream->get_handle();
    }

    virtual int handle_input (ACE_HANDLE h = ACE_INVALID_HANDLE)
    {
        char buffer[BUFFER_SIZE];
        size_t recv_len = 0;

        recv_len = this->ace_sock_stream->recv(&buffer, BUFFER_SIZE);
        if ( recv_len < 0 )
        {
            cout << "The socket is closed." << endl;
            ACE_Reactor::instance()->remove_handler(this, ACE_Event_Handler::NULL_MASK);
            return this->ace_sock_stream->close();

        }
        if ( recv_len == 0 )
        {
            //cout << "Empty Message" << endl;
            return 0;
        }
        cout << "RECV [" << string(buffer) << "]" << endl;
        if ( recv_len == 2 && stoi(string(buffer)) == 0 )
        {
            cout << "Receive new client" << endl;
            cout << "Client Counter: " << client_counter << endl;
            string send_str = to_string(client_counter);
            cout << "send: " << send_str << endl;
            if ( this->ace_sock_stream->send_n(&client_counter, sizeof(client_counter)) < 0 )
                cout << "Error: Can not send client id" << endl; 
            client_counter++;
        }

        return recv_len;
    }

    virtual int handle_close (ACE_HANDLE handle, ACE_Reactor_Mask mask)
    {
        cout << "handle close in reader "; 
        if ( mask | READ_MASK )
            cout << "with READ_MASK";
        cout << endl;
        this->ace_sock_stream->close();
        return 0;
    }



};

class server_accept : public ACE_Event_Handler
{
    ACE_SOCK_Acceptor* acceptor;
public:
    server_accept(ACE_SOCK_Acceptor* acceptor) : acceptor(acceptor) {}

    virtual ACE_HANDLE get_handle() const
    {
        cout << "get_handle" << endl;
        return acceptor->get_handle();
    }

    virtual int handle_input (ACE_HANDLE h = ACE_INVALID_HANDLE)
    {

        ACE_SOCK_Stream ace_sock_stream;
        if ( acceptor->accept(ace_sock_stream) < 0 )
        {
            cout << "Error: Can not accept the ace_sock_stream" << endl;
            return EISCONN;
        }

        server_read server_read;
        server_read.set(&ace_sock_stream);

        ACE_Reactor::instance()->register_handler(&server_read, ACE_Event_Handler::READ_MASK);
        
        // this step is neccessary
        ACE_Reactor::instance()->run_reactor_event_loop();

        return 0;
    }

    virtual int handle_signal (int signal, siginfo_t* = 0, ucontext_t* = 0)
    {
        cout << "handle signal" << endl;
        ACE_Reactor::instance()->end_reactor_event_loop();
        ACE_Reactor::instance()->close();
        cout << "end and close done" << endl;
        return 0;
    }

    virtual int handle_close (ACE_HANDLE handle, ACE_Reactor_Mask mask)
    {
        if ( (mask | ACCEPT_MASK) && (mask | SIGNAL_MASK) )
            cout << "handle close with ACCEPT_MASK and SIGNAL_MASK" << endl;
        if ( !(mask | ACCEPT_MASK) && (mask | SIGNAL_MASK) )
            cout << "handle close with SIGNAL_MASK" << endl;
        if ( (mask | ACCEPT_MASK) && !(mask | SIGNAL_MASK) )
            cout << "handle close with ACCEPT_MASK" << endl;

        return 0;
    }
};

// class server_signal : public ACE_Event_Handler
// {
//     virtual int handle_signal (int signal, siginfo_t* = 0, ucontext_t* = 0)
//     {
//         cout << "handle signal in server signal" << endl;
//         ACE_Reactor::instance()->end_reactor_event_loop();
//         return ACE_Reactor::instance()->close();
//     }
// };



int 
main(int argc, char* argv[])
{
    if (argc != 1)
    {
        cout << "Error: Too many command line arguments." << endl;
        return EINVAL;
    }
     
    string arg(argv[argc - 1]);
    cout << arg << endl;

    ACE_TCHAR buffer[BUFFER_SIZE];
    ACE_INET_Addr address(8086, ACE_LOCALHOST);
    ACE_SOCK_Acceptor acceptor;
    if ( acceptor.open(address, 1) < 0)
    {
        address.addr_to_string(buffer, BUFFER_SIZE, 1);
        cout << "Error: Can not listen on " << buffer << endl;
        return EINVAL;
    }
    address.addr_to_string(buffer, BUFFER_SIZE, 1);
    cout << "Start listening on " << buffer << endl;

    server_accept server(&acceptor);
    ACE_Reactor::instance()->register_handler(&server, ACE_Event_Handler::ACCEPT_MASK);
    ACE_Reactor::instance()->register_handler(SIGINT, &server);
    ACE_Reactor::instance()->run_reactor_event_loop();

    cout << "Server Stopped" << endl;

    return SUCCESS;
}