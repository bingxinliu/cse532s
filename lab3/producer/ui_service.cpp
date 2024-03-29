#include "ui_service.hpp"

#include <thread>

#include "ace/Reactor.h"
#include "ace/Thread_Manager.h"

#include "../utilities/const.hpp"
#include "../utilities/threadsafe_io.hpp"

// construct a ui service
ui_service::ui_service(producer* producer_ptr) : producer_ptr(producer_ptr) {}

ui_service::~ui_service() 
{
    *safe_io << "Realese UI SREVICE", safe_io->flush();
}

// register self as a stdin handler
int
ui_service::register_service()
{
    if (ACE_Event_Handler::register_stdin_handler(this, ACE_Reactor::instance(), ACE_Thread_Manager::instance()) < 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

// parse users command
void
ui_service::parse_command(const std::string str)
{
    std::stringstream ss(str);
    std::string command;

    if (ss >> command)
    {
        // user require start a play and refresh the menu
        if (command == START_COMMAND)
        {
            uint offset;
            if (ss >> offset)
            {
                if (DEBUG)
                    *safe_io << command << " " << offset, safe_io->flush();
                std::string playname = this->producer_ptr->menu[offset];
                if (playname.size() == 0)
                {
                    *safe_io << "Sorry, unavailable.", safe_io->flush();
                    return;
                } 
                uint id = this->producer_ptr->menu.pop_avaliable(playname);
                if (id == 0)
                {
                    *safe_io << "SORRY, NOT AVAILABLE", safe_io->flush();
                    return;
                }
                *safe_io << this->producer_ptr->menu.str();
                safe_io->flush();
                std::stringstream send_ss;
                send_ss << START_COMMAND << " " << playname;
                this->producer_ptr->send_msg(id, send_ss.str());
                if (DEBUG)
                {
                    *safe_io << "SEND [" << send_ss.str() << "]";
                    safe_io->flush();
                }
                return;
            }
            *safe_io << "Sorry, unavailable.", safe_io->flush();
            return;
        }

        // user require to stop a play
        if (command == STOP_COMMAND)
        {
            uint offset;
            if (ss >> offset)
            {
                if (DEBUG)
                    *safe_io << command << " " << offset, safe_io->flush();
                std::string playname = this->producer_ptr->menu[offset];
                if (playname.size() == 0) return;
                uint id = this->producer_ptr->menu.pop_busy_play(playname);
                if (id == 0)
                {
                    *safe_io << "SORRY, NOT AVAILABLE", safe_io->flush();
                    return;
                }

                std::stringstream send_ss;
                send_ss << STOP_COMMAND << " " << playname;
                this->producer_ptr->send_msg(id, send_ss.str());
                if (DEBUG)
                {
                    *safe_io << "SEND [" << send_ss.str() << "]";
                    safe_io->flush();
                }


            }
            return;
        }

        // user require to quit
        if (command == QUIT_COMMAND)
        {
            std::string str;
            if (ss >> str && str.length() > 0)
            {
                *safe_io << "WARNING: Remaining command: " << str, safe_io->flush();
            }
            else
            {
                // remove self from reactor
                ACE_Reactor::instance()->remove_handler(this, ACE_Event_Handler::NULL_MASK);
                this->producer_ptr->send_quit_all();

                // waiting for quited in another thread
                // if all quit, stop event loop and close self
                std::thread t = std::thread([this](){
                    this->producer_ptr->wait_for_quit();
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
            }
            return;
        }

    }
    *safe_io << "ERROR: Cannot parse command: " << ss.str(), safe_io->flush();
}

// listenning on user's input
int
ui_service::handle_input(ACE_HANDLE h)
{
    if (h == ACE_STDIN)
    {
        char buffer[BUFFER_SIZE];
        size_t recv_len = ACE_OS::read(h, buffer, BUFFER_SIZE);
        if (recv_len <= 0) return FAILURE;
        // remove new line character
        if (DEBUG)
            *(threadsafe_io::get_instance()) << "UI RECV [" << std::string(buffer).substr(BEGINNING, std::string(buffer).length() - 1) << "]";
        threadsafe_io::get_instance()->flush();

        this->parse_command(std::string(buffer));

        memset(buffer, 0, BUFFER_SIZE);

        return SUCCESS;
    }
    *(threadsafe_io::get_instance()) << "WARNING: UI_SERVICE handle input, but not from stdin.";
    threadsafe_io::get_instance()->flush();
    return SUCCESS;
}

// close the ui service and clean self
int
ui_service::handle_close(ACE_HANDLE h, ACE_Reactor_Mask mask)
{
    *(threadsafe_io::get_instance()) << "Closing UI_SERVICE...";
    threadsafe_io::get_instance()->flush();
    delete this;
    return SUCCESS;
}