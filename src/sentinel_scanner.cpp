//Credit to Stack Overflow for resources for help while doing this project.




#include "core/sentinel_scanner.h"

SentinelScanner::SentinelScanner(std::string sentinel) 
    : sentinel_(std::move(sentinel)) {}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    Out result;
    result.sentinel_found = false;
    
    //add new chunk to pending_
    pending_ += chunk;
    
    //find location of sentinel if it is there
    std::size_t found_pos = pending_.find(sentinel_);
    
    if (found_pos != std::string::npos) {
        //if sentinel is found, everything before is good
        result.safe_text = pending_.substr(0, found_pos);
        result.sentinel_found = true;
        pending_.clear(); 
    } else {
        // if sentinel is not found, hold back the maximum possible match length
        std::size_t max_hold_back = sentinel_.size() - 1;
        
        if (pending_.size() > max_hold_back) {
            std::size_t safe_length = pending_.size() - max_hold_back;
            result.safe_text = pending_.substr(0, safe_length);
            //keep only trailing chars
            pending_ = pending_.substr(safe_length);
        }
        //if pending is less than the limit at max_hold_back, safe_text is still empty
    }
    
    return result;
}

SentinelScanner::Out SentinelScanner::flush() {
    //release any text left after everything else is done
    Out result;
    result.safe_text = pending_;
    result.sentinel_found = false;
    pending_.clear();
    return result;
}
