//
// Created by ASA3 on 07-07-2017.
//

#include "InputOutputHandler.h"
#include <iostream>

UI::InputOutputHandler::InputOutputHandler(const std::string &_handle)
    : m_handle(_handle)
{

}

UI::InputOutputHandler::~InputOutputHandler()
{
// Write to a file
}

void
UI::InputOutputHandler::SetUserHandle()
{
    std::string handle = m_handle;
    do
    {
        std::cout << "Enter your handle: ";
        std::cin >> handle;
    } while (handle_list.contains(handle));
    m_handle = handle;
}

bool
UI::InputOutputHandler::GetChatText()
{
    std::string chat_text;
    std::cin >> chat_text;
    if (chat_text == "bye")
    {
        return false;
    }

    if (chat_text[0] == '@' && chat_text.size() > 3)
    {
        size_t pos = chat_text.find(" ");
        std::string receiver = chat_text.substr(1, pos);
        std::string chat_text = chat_text.substr(pos, std::string::npos);
        // Send to receiver
    }
    else
    {
        // Send to server
    }
}

void
UI::InputOutputHandler::ShowChatText(const std::string & _sender, const std::string & _chat_text)
{
    std::cout << _sender << ": " << _chat_text;
}
