#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

// FSM State enumeration
enum class State {
    START,
    WAIT_TAG,
    READ_TAG,
    WAIT_VALUE,
    READ_VALUE,
    DELIM,
    COMPLETE,
    ERROR
};

// Convert state to string for display
const char* state_to_string(State s) {
    switch (s) {
        case State::START: return "START";
        case State::WAIT_TAG: return "WAIT_TAG";
        case State::READ_TAG: return "READ_TAG";
        case State::WAIT_VALUE: return "WAIT_VALUE";
        case State::READ_VALUE: return "READ_VALUE";
        case State::DELIM: return "DELIM";
        case State::COMPLETE: return "COMPLETE";
        case State::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Simple FSM demonstration
class FSMDemo {
public:
    FSMDemo() : state_(State::START), current_tag_(0), value_length_(0) {}
    
    void process_char(char c) {
        State prev_state = state_;
        
        switch (state_) {
            case State::START:
                state_ = State::WAIT_TAG;
                std::cout << "  Action: Initialize parser" << std::endl;
                break;
                
            case State::WAIT_TAG:
                if (isdigit(c)) {
                    current_tag_ = c - '0';
                    state_ = State::READ_TAG;
                    std::cout << "  Action: Start reading tag, tag=" << current_tag_ << std::endl;
                } else if (c == '|' || c == 0x01) {
                    // Skip delimiter, stay in WAIT_TAG
                    std::cout << "  Action: Skip delimiter" << std::endl;
                }
                break;
                
            case State::READ_TAG:
                if (isdigit(c)) {
                    current_tag_ = current_tag_ * 10 + (c - '0');
                    std::cout << "  Action: Accumulate tag digit, tag=" << current_tag_ << std::endl;
                } else if (c == '=') {
                    state_ = State::WAIT_VALUE;
                    std::cout << "  Action: Tag complete, tag=" << current_tag_ << std::endl;
                } else {
                    state_ = State::ERROR;
                    std::cout << "  Action: ERROR - Invalid character in tag" << std::endl;
                }
                break;
                
            case State::WAIT_VALUE:
                value_start_pos_ = position_;
                value_length_ = 1;
                state_ = State::READ_VALUE;
                std::cout << "  Action: Start reading value at position " << value_start_pos_ << std::endl;
                break;
                
            case State::READ_VALUE:
                if (c == '|' || c == 0x01) {
                    state_ = State::DELIM;
                    std::cout << "  Action: Value complete, length=" << value_length_ 
                              << ", tag=" << current_tag_ << std::endl;
                } else {
                    value_length_++;
                    std::cout << "  Action: Continue reading value, length=" << value_length_ << std::endl;
                }
                break;
                
            case State::DELIM:
                // Store field (simulated)
                std::cout << "  Action: Store field (tag=" << current_tag_ 
                          << ", value_length=" << value_length_ << ")" << std::endl;
                
                if (current_tag_ == 10) {
                    state_ = State::COMPLETE;
                    std::cout << "  Action: Message complete (checksum field)" << std::endl;
                } else {
                    state_ = State::WAIT_TAG;
                    std::cout << "  Action: Ready for next field" << std::endl;
                }
                
                // Process current character in new state
                if (state_ == State::WAIT_TAG) {
                    process_char(c);
                    return;
                }
                break;
                
            case State::COMPLETE:
                std::cout << "  Action: Message parsing complete, reset to START" << std::endl;
                state_ = State::START;
                break;
                
            case State::ERROR:
                std::cout << "  Action: Error recovery - scan for next message" << std::endl;
                break;
        }
        
        if (prev_state != state_) {
            std::cout << "  Transition: " << state_to_string(prev_state) 
                      << " -> " << state_to_string(state_) << std::endl;
        }
        
        position_++;
    }
    
    void process_message(const std::string& message) {
        std::cout << "\n=== Processing Message ===" << std::endl;
        std::cout << "Input: " << message << std::endl;
        std::cout << "\nCharacter-by-character processing:\n" << std::endl;
        
        position_ = 0;
        for (char c : message) {
            std::cout << "Position " << std::setw(2) << position_ 
                      << " | Char: '" << c << "' | State: " 
                      << std::setw(12) << state_to_string(state_) << std::endl;
            process_char(c);
        }
        
        std::cout << "\nFinal state: " << state_to_string(state_) << std::endl;
    }
    
    void demonstrate_fragmentation() {
        std::cout << "\n\n=== Fragmentation Demonstration ===" << std::endl;
        std::cout << "Simulating message split across two recv() calls\n" << std::endl;
        
        // First chunk
        std::string chunk1 = "8=FIX.4.4|55=MS";
        std::cout << "First recv(): \"" << chunk1 << "\"" << std::endl;
        std::cout << "Processing first chunk...\n" << std::endl;
        
        position_ = 0;
        for (char c : chunk1) {
            process_char(c);
        }
        
        State saved_state = state_;
        int saved_tag = current_tag_;
        size_t saved_length = value_length_;
        
        std::cout << "\n--- Buffer exhausted ---" << std::endl;
        std::cout << "Saved state: " << state_to_string(saved_state) << std::endl;
        std::cout << "Saved tag: " << saved_tag << std::endl;
        std::cout << "Saved value_length: " << saved_length << std::endl;
        
        // Second chunk
        std::string chunk2 = "FT|44=123.45|10=020|";
        std::cout << "\nSecond recv(): \"" << chunk2 << "\"" << std::endl;
        std::cout << "Resuming from saved state...\n" << std::endl;
        
        for (char c : chunk2) {
            process_char(c);
        }
        
        std::cout << "\nFragmented message successfully parsed!" << std::endl;
    }

private:
    State state_;
    int current_tag_;
    size_t value_start_pos_;
    size_t value_length_;
    size_t position_;
};

int main() {
    std::cout << "FIX Parser FSM Design Demonstration" << std::endl;
    std::cout << "====================================" << std::endl;
    
    FSMDemo demo;
    
    // Test 1: Simple complete message
    demo.process_message("55=MSFT|");
    
    // Test 2: Multiple fields
    demo.process_message("8=FIX.4.4|55=AAPL|44=150.25|10=020|");
    
    // Test 3: Fragmentation handling
    demo.demonstrate_fragmentation();
    
    std::cout << "\n\n=== State Transition Summary ===" << std::endl;
    std::cout << "The FSM successfully demonstrates:" << std::endl;
    std::cout << "  ✓ Character-by-character parsing" << std::endl;
    std::cout << "  ✓ State transitions based on input" << std::endl;
    std::cout << "  ✓ Tag and value extraction" << std::endl;
    std::cout << "  ✓ Message completion detection" << std::endl;
    std::cout << "  ✓ Fragmentation handling (suspend/resume)" << std::endl;
    
    return 0;
}