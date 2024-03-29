
#include "producer.hpp"
#include <thread>

uint producer::director_id = 1;

// set self acceptor
// launch a ui service, and register it into event loop
producer::producer(ACE_SOCK_Acceptor acceptor)
    : acceptor(acceptor)
{
    this->ui_srv = new ui_service(this);
    if (this->ui_srv->register_service() < 0)
    {
        *safe_io << "Error: UI_service register failed.";
        safe_io->flush();
    }
}

producer::~producer()
{
    *safe_io << "Release producer", safe_io->flush();
}

// send message to a specific director by its id
void
producer::send_msg(uint id, const std::string str)
{
    ACE_SOCK_Stream& ass = *(this->id_socket_map[id]);
    ass.send_n(str.c_str(), str.length() + 1);
}

// send QUIT message to all directors.
void
producer::send_quit_all()
{
    std::string str(QUIT_COMMAND);
    for (std::map<uint, ACE_SOCK_Stream*>::iterator it = this->id_socket_map.begin();
    it != this->id_socket_map.end();
    ++it)
    {
        it->second->send_n(str.c_str(), str.length() + 1);
    }
}

// waiting for all directors quit
void 
producer::wait_for_quit()
{
    if (DEBUG)
        *safe_io << "WAIT FOR QUIT", safe_io->flush();
    while(!(this->id_socket_map.empty()))
    {
        std::this_thread::yield();
    }
    if (DEBUG)
        *safe_io << "SAFE TO QUIT", safe_io->flush();
}

ACE_HANDLE 
producer::get_handle() const
{
    return this->acceptor.get_handle();
}

// handle connection request
// construct a new reader services for new connector
// register in record of director id and ace_sock_stream map
int
producer::handle_input(ACE_HANDLE h)
{
    if (DEBUG)
        *safe_io << "listener handle connect", safe_io->flush();

    ACE_SOCK_Stream* ace_sock_stream = new ACE_SOCK_Stream;

    if (this->acceptor.accept(*ace_sock_stream) < 0)
    {
        *safe_io << "Error: Can not accept the ace_sock_stream", safe_io->flush();
        return EISCONN;
    }

    uint id = producer::director_id++;

    this->id_socket_map[id] = ace_sock_stream;

    reader_service* reader = new reader_service(*this, ace_sock_stream, id);

    ACE_Reactor::instance()->register_handler(reader, ACE_Event_Handler::READ_MASK);
    
    if (DEBUG)
        *safe_io << "handle connect done, hand over to reader_service.", safe_io->flush();

    return EXIT_SUCCESS;
}

// handle close
int
producer::handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask)
{
    if ( (mask & ACCEPT_MASK) && (mask & SIGNAL_MASK) )
        if (DEBUG)
            *safe_io << "handle close with ACCEPT_MASK and SIGNAL_MASK";
    if ( !(mask & ACCEPT_MASK) && (mask & SIGNAL_MASK) )
        if (DEBUG)
        *safe_io << "handle close with SIGNAL_MASK";
    if ( (mask & ACCEPT_MASK) && !(mask & SIGNAL_MASK) )
        if (DEBUG)
            *safe_io << "handle close with ACCEPT_MASK";
    
    if (DEBUG)
        safe_io->flush();
    return SUCCESS;
}

// handle signal
// 1st send quit message to are directors
// 2nd wait for all directors confirm message in a new thread
// if all directors quited, stop event loop and close self.
int
producer::handle_signal(int signal, siginfo_t* sig, ucontext_t* ucontx)
{
    this->send_quit_all();
    std::thread t = std::thread([this](){
        this->wait_for_quit();
        int ret;
        ret = ACE_Reactor::instance()->end_event_loop();

        if (ret < 0)
        {
            *(threadsafe_io::get_instance()) << "Error in ACE_Reactor::instance()->end_reactor_event_loop() with error code: " << ret, threadsafe_io::get_instance()->flush();
        }
        ret = ACE_Reactor::instance()->close();
        if (ret < 0)
        {
            *(threadsafe_io::get_instance()) << "Error in ACE_Reactor::instance()->close() with error code: " << ret, threadsafe_io::get_instance()->flush();
        }
    });
    t.detach();
    return EXIT_SUCCESS;
}