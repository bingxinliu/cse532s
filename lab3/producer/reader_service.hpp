#pragma once

#include "../utilities/threadsafe_io.hpp"
#include "../utilities/const.hpp"

#include "ace/SOCK_Stream.h"
#include "ace/Event_Handler.h"
#include "ace/Reactor.h"

class reader_service : public ACE_Event_Handler
{    
    ACE_SOCK_Stream* ace_sock_stream;
    
public:
    reader_service(ACE_SOCK_Stream* ss);
    ~reader_service();

    virtual ACE_HANDLE get_handle() const;
    virtual int handle_input(ACE_HANDLE h = ACE_INVALID_HANDLE);
    virtual int handle_close(ACE_HANDLE handle, ACE_Reactor_Mask mask);
};
