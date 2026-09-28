
#include "core/sentinel_scanner.h"

SentinelScanner::SentinelScanner(std::string sentinel) : sentinel_(std::move(sentinel)) {} //base constructor

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk)
{
    Out res;
    res.sentinel_found = false;
    
    pending_ += chunk; //append the incoming chunk to the data already pending
    std::size_t foundPosition = pending_.find(sentinel_); //find the sentinel in the total text
    
    if (foundPosition != std::string::npos) //if the sentinel was found (std::string::npos is returned by find if search failed)
    {
        res.safe_text = pending_.substr(0, foundPosition); //clear the pending
        res.sentinel_found = true;
        pending_.clear(); 
    } 
    else 
    {
        //std::size_t maxHoldBack = sentinel_.size() - 1;
        std::size_t maxHoldBack = sentinel_.size() - 1; //if sentinel not found, retain an amount equal to the largest partial match length
        
        if (pending_.size() > maxHoldBack) 
        {
            std::size_t safeLength = pending_.size() - maxHoldBack;
            res.safe_text = pending_.substr(0, safeLength);
            pending_ = pending_.substr(safeLength); //making sure that we didnt cut of part of the sentinel
        }
    }
    
    return res;
}

SentinelScanner::Out SentinelScanner::flush() //release data remaining in pending_
{
    Out res;

    res.safe_text = pending_;
    pending_.clear();

    res.sentinel_found = false;

    return res;
}