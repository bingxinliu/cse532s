all:
	g++ -L$(ACE_ROOT)/ace -Wall -std=c++11 -pthread -DTEMPLATE_HEADERS_INCLUDE_SOURCE -o main main.cpp producer.cpp ui_service.cpp reader_service.cpp threadsafe_menu.cpp ../utilities/const.hpp ../utilities/threadsafe_io.cpp ../utilities/utilities.cpp -I$(ACE_ROOT) -lACE

clean:
	rm main *~


