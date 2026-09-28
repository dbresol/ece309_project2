#include "core/conversation.h"
#include <stdexcept>

Conversation::Conversation() : data_(nullptr), size_(0), capacity_(0) {}

Conversation::~Conversation() 
{
    delete[] data_; 
}
Conversation::Conversation(const Conversation& other) : size_(other.size_), capacity_(other.capacity_) //copy constructor
{    
    if (capacity_ > 0) {
        data_ = new Message[capacity_];
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    } else {
        data_ = nullptr;
    }
}
Conversation& Conversation::operator=(const Conversation& other) //copy assignment
{
    if (this != &other) {
        // Free existing memory
        delete[] data_;
        
        // Deep copy from 'other'
        size_ = other.size_;
        capacity_ = other.capacity_;
        
        if (capacity_ > 0) {
            data_ = new Message[capacity_];
            for (std::size_t i = 0; i < size_; ++i) {
                data_[i] = other.data_[i];
            }
        } else {
            data_ = nullptr;
        }
    }
    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept : data_(other.data_), size_(other.size_), capacity_(other.capacity_) //move constructor
{
    
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

Conversation& Conversation::operator=(Conversation&& other) noexcept //move asignment
{
    if (this != &other) {
        // Free existing memory
        delete[] data_;
        
        // Steal memory
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        
        // Zero out the source
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

void Conversation::append(Message m) {
    if (size_ == capacity_) {
        // Determine new capacity (e.g., double it, or start at 1 if currently 0)
        std::size_t new_capacity = (capacity_ == 0) ? 1 : capacity_ * 2;
        
        // Allocate new array
        Message* new_data = new Message[new_capacity];
        
        // Copy existing elements to the new array
        for (std::size_t i = 0; i < size_; ++i) {
            new_data[i] = data_[i];
        }
        
        // Delete the old array and update members
        delete[] data_;
        data_ = new_data;
        capacity_ = new_capacity;
    }
    
    // Insert the new message
    data_[size_] = m;
    size_++;
}

std::size_t Conversation::size() const noexcept {
    return size_;
}

const Message& Conversation::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("Conversation::at - index out of bounds");
    }
    return data_[i];
}

const Message* Conversation::begin() const noexcept {
    return data_;
}

const Message* Conversation::end() const noexcept {
    return data_ + size_;
}