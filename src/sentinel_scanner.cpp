#include "core/sentinel_scanner.h"

SentinelScanner::SentinelScanner(std::string sentinel) 
    : sentinel_(std::move(sentinel)) {}

SentinelScanner::Out SentinelScanner::feed(std::string_view chunk) {
    Out result;
    result.sentinel_found = false;
    
    // Combine existing pending characters with the incoming chunk
    pending_ += chunk;
    
    // Check if the sentinel is fully present in this combined text
    std::size_t found_pos = pending_.find(sentinel_);
    
    if (found_pos != std::string::npos) {
        // Sentinel found: everything before it is safe
        result.safe_text = pending_.substr(0, found_pos);
        result.sentinel_found = true;
        pending_.clear(); 
    } else {
        // Sentinel not found: hold back the maximum possible partial match length
        std::size_t max_hold_back = sentinel_.size() - 1;
        
        if (pending_.size() > max_hold_back) {
            std::size_t safe_length = pending_.size() - max_hold_back;
            result.safe_text = pending_.substr(0, safe_length);
            // Retain only the trailing characters that might form a sentinel with the next chunk
            pending_ = pending_.substr(safe_length);
        }
        // If pending_ is shorter than max_hold_back, safe_text remains empty.
    }
    
    return result;
}

SentinelScanner::Out SentinelScanner::flush() {
    // Release any text still held back after the stream ends
    Out result;
    result.safe_text = pending_;
    result.sentinel_found = false;
    pending_.clear();
    return result;
}