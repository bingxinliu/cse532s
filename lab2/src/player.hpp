#pragma once

#include <thread>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include "play.hpp"

class Player
{
    std::map<unsigned int, Structured_line> lines;
    std::string character;
    std::thread mythread;
    std::ifstream input_file_stream;
    Play& play;

public:

    // Update the Player class constructor, member variables, and methods so that they are compatible with the HS/HA or L/F design approach you take per the instructions above. In particular, you should consider whether the following changes are compatible with your approach (and if not should modify them accordingly). Again, please make sure to document your design decisions and rationale for this part as well, in your project report.

    // Update its constructor and member variables so that it only keeps track of a reference to the Play in which it is performing (and does not store the name of a character or a character part file stream).
    Player(Play& play_, std::string const name_, std::ifstream& input_file_stream_) :
        input_file_stream(std::move(input_file_stream_)), play(play_)
    {
        this->character = name_;
    }

    Player(Player&& right) :
           lines(std::move(right.lines)), mythread(std::move(right.mythread)),
           input_file_stream(std::move(right.input_file_stream)),  play(right.play)
    {
        this->character = right.character;
    }

    ~Player() {this->exit();}

    void read();
    void act();
    void enter();
    void exit();
};