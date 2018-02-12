#ifndef LANCHAT_INPUTOUTPUTHANDLER_H
#define LANCHAT_INPUTOUTPUTHANDLER_H

#include <string>

namespace UI
{
class InputOutputHandler
{
public:
    InputOutputHandler(const std::string & _handle);
    virtual ~InputOutputHandler();
    virtual void
    SetUserHandle();
    virtual bool
    GetChatText();
    virtual void
    ShowChatText(const std::string & _sender, const std::string & _chat_text);

private:
    std::string m_handle;
};
}

#endif //LANCHAT_INPUTOUTPUTHANDLER_H
