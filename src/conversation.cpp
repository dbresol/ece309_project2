
#include "core/conversation.h"
#include <stdexcept>

Conversation::Conversation() : data_(nullptr), size_(0), capacity_(0) {} //base constructor

Conversation::~Conversation() //destructor
{
    delete[] data_; 
}
Conversation::Conversation(const Conversation& other) : size_(other.size_), capacity_(other.capacity_) //copy constructor
{    
    if (capacity_ > 0) //its a copy constructor so there is no need to make space for the data copied
    { 
        data_ = new Message[capacity_];
        for (std::size_t i = 0; i < size_; ++i)  //copy the data
        {
            data_[i] = other.data_[i];
        }
    } 
    else 
    {
        data_ = nullptr;
    }
}
Conversation& Conversation::operator=(const Conversation& other) //copy assignment
{
    if (this != &other) 
    {
        delete[] data_; //make room for the data to be copied
        capacity_ = other.capacity_;
        size_ = other.size_; //copy non-array data to the region which just got deleted
        
        
        if (capacity_ > 0) 
        { //copy the array data from "other" into the free space if there is actual data to be copied over
            data_ = new Message[capacity_];
            for (std::size_t i = 0; i < size_; ++i) 
            {
                data_[i] = other.data_[i];
            }
        } 
        else
        {
            data_ = nullptr; //if there is no message, just ignore
        }
    }
    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) //move constructor
{
    other.size_ = 0; //remove data from old data
    other.capacity_ = 0;
    other.data_ = nullptr; 
}

Conversation& Conversation::operator=(Conversation&& other) noexcept //move asignment
{
    if (this != &other) 
    {
        delete[] data_; //get rid of old data
        
        size_ = other.size_;
        capacity_ = other.capacity_;
        data_ = other.data_; //move old data into new location without copying per element
        
        other.size_ = 0;
        other.capacity_ = 0;
        other.data_ = nullptr; //remove data from the old object
        
    }

    return *this;
}

void Conversation::append(Message m)  //try to append a message given
{ 
    if (size_ == capacity_)
    {
        std::size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2; //double capacity or make it 1 if currently 0
        
        Message* new_data = new Message[new_capacity];
        for (std::size_t i = 0; i < size_; ++i) { //copy over each char without removing old or anything like that
            new_data[i] = data_[i];
        }
        
        delete[] data_; //delete old stuff and update the properties
        data_ = new_data;
        capacity_ = new_capacity;
    }

    data_[size_] = m; //insert the message at the end of data_ and increment
    ++size_;
}

std::size_t Conversation::size() const noexcept { //get size of conversation
    return size_;
}

const Message& Conversation::at(std::size_t i) const { //get an element at a certain index
    if (i >= size_) 
    {
        //if not found, throw exception. the test suite in the design document says empty conversations should not have out of bounds access,
        //and here the program creates an error in that case
        throw std::out_of_range("Conversation::at - index out of bounds"); 
    }
    return data_[i];
}

const Message* Conversation::begin() const noexcept { //return first message
    return data_;
}

const Message* Conversation::end() const noexcept { //return the final message
    return data_ + size_;
}