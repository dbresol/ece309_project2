#include "core/message.h"

#include <utility>

Message::Message() : role_(Role::System), content_("") {} //constructor

Message::Message(Role role, std::string content) : role_(role), content_(std::move(content)) {} //another constructor where you specify initial values

Role Message::role() const noexcept  //get the current role
{
    return role_;
}

const std::string& Message::content() const noexcept //get the content of the current message
{
    return content_;
}