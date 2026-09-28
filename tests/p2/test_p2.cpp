// tests/p2/test_p2.cpp
//
// YOUR test suite goes here. At least 12 assert-based test cases — see
// spec §5 for the required categories and the sample test for the
// expected level of rigor.
//
// This file is a stub so the project builds out of the box; replace the
// body of main() with your own tests.

#include "core/conversation.h"
#include "core/message.h"
#include "core/sentinel_scanner.h"
#include "harness/harness.h"
#include "model/replay_client.h"
#include "model/scripted_client.h"

#include <cassert>


#include <iostream>
#include <stdexcept>
#include <fstream>



//mock classes for harness integration for the last tests here
class MockInput : public InputSource // creates fake inputs for use in the harness
{
    int current_turn = 0;
public:
    std::string read_line() override { current_turn++; return "user input"; }
    bool is_eof() const override { return false; }
};


class MockOutput : public OutputSink // creates fake outputs for when we need to
{
public:
    void write(std::string_view) override {} // dont output
};


class DummyModel : public ModelClient // mock model
{ 
public:
    void generate(const Conversation&, TokenSink& sink) override 
    {
        sink.on_chunk("dummy reply"); //dummy reply for generate
        sink.on_complete();
    }
};


class SentinelModel : public ModelClient // mock sentinel model
{
public:
    void generate(const Conversation&, TokenSink& sink) override
    {
        sink.on_chunk("done<|end_conversation|>"); //common sentinel
        sink.on_complete();
    }
};

int main() {
    std::cout << "Running P2 Tests...\n";

    // test 1: empty conversation bounds
    Conversation conv1;
    assert(conv1.size() == 0); //make sure that the new conversation is in fact empty
    assert(conv1.begin() == conv1.end()); //same as above
    

    bool errored = false; //try to force an error by looking up a value stored at an index in an empty list
    try { conv1.at(1); } 
    catch (const std::out_of_range&) { errored = true; }
    assert(errored && "at() should throw out_of_range on empty conversation"); //if the at function returned actual data, test failed
    //


    // test 2: system message ordering
    Conversation conv2;
    conv2.append(Message(Role::System, "System prompt")); //first message is a system message
    conv2.append(Message(Role::User, "User message")); //second is a user message

    assert(conv2.at(0).role() == Role::System && "System message displaced"); //check if first message is a system message
    assert(conv2.at(1).role() == Role::User); //check if second message is a user message
    //


    // test 3: copy for rule of five
    Conversation c1_3;
    c1_3.append(Message(Role::User, "Hello")); //send a user message
    
    Conversation c2_3(c1_3); //copy that conversation into this new one
    assert(c1_3.size()==1 && c2_3.size()==1); //make sure size copied
    assert(c1_3.at(0).content() == c2_3.at(0).content()); //make sure the message data copied
    assert(c1_3.begin() != c2_3.begin() && "Copy constructor did not allocate new memory"); //make sure the pointer wasnt just moved and the data is actually in a new location
    //


    // test 4: move for rule of five
    Conversation c1_4;
    c1_4.append(Message(Role::User, "Hello")); //send a user message
    const Message* originalPtr = c1_4.begin(); //copy pointer into new message
    
    Conversation c2_4(std::move(c1_4)); //construct a new conversation with the rvalue conversion of the old conversation
    assert(c2_4.begin() == originalPtr && "Move constructor failed to steal pointer"); //make sure the pointer is the original
    assert(c2_4.size() == 1); //make sure the size is the same
    assert(c1_4.size() == 0 && c1_4.begin() == nullptr && "Move constructor failed to zero source"); //make sure the old data was removed
    //


    // test 5: growth behavior
    // here, we are showing that the conversation grows by 1 every time a new message is added, which lets us find the time complexity. 
    // i believe the time complexity is O(N) because of the resizing time
    // i was not sure if i was supposed to find how the data stored doubles every time the limit is reached, but i don't know how i would do that
    // because the capacity_ property is a private
    Conversation conv5;

    const int targetSize = 100; 
    for (int i = 0; i < targetSize; ++i) //keep adding messages until the size is > 100, which is reasonable enough
    {
        conv5.append(Message(Role::User, std::to_string(i)));
        //std::cout << conv5.size() << std::endl;
    }
    assert((conv5.size() == targetSize) && "Conversation size and expected size are different"); 
    for (int i = 0; i < targetSize; ++i)
    {
        assert(conv5.at(i).content() == std::to_string(i) && "Data missing after the allocation"); // makes sure everything is allocated
    }
    //


    // tests for sentinel scanner
    // test 6: scanner clean text
    SentinelScanner scanner6("STOP"); // create a sentinal for "STOP"
    SentinelScanner::Out out = scanner6.feed("Hello "); // string without the sentinel

    assert(!out.sentinel_found && "Sentinel incorrectly found"); //error if sentinel was incorrectly found
    assert(out.safe_text == "Hel" && "Incorrect safe_text value"); //error if safe text was calculated incorrectly
    
    SentinelScanner::Out flushOut = scanner6.flush(); //same thing but after flush
    assert(flushOut.safe_text == "lo " && "Incorrect safe_text value");
    //


    // test 7: scanner split sentinel
    const std::string sentinel7 = "<|end_conversation|>";
    const std::string text7 = "Goodbye." + sentinel7;
    
    bool found = false;
    for (std::size_t split = 0; split <= text7.size(); ++split) //look through all possible split points
    {

        SentinelScanner scanner7(sentinel7);
        SentinelScanner::Out out1_7 = scanner7.feed(text7.substr(0, split)); //go from beginning to split index
        SentinelScanner::Out out2_7 = scanner7.feed(text7.substr(split)); //go from split index to end
        
        if (out1_7.sentinel_found || out2_7.sentinel_found) found = true;
    }
    assert(found && "Split sentinel was never found");
    //


    // test 8: scanner false alarms
    SentinelScanner scanner8("<|end_conversation|>");
    SentinelScanner::Out out8 = scanner8.feed("Goodbye <|end_world|>"); //false alarm tested here would be if the "end" in "<|end_world|>" is mistaken for the "end" in "<|end_conversation|>"
    assert(!out.sentinel_found && "Triggered false alarm because of partial match");
    //


    //test 9: scanner with bounded memory
    const std::string sentinel9 = "<|stop|>"; //sentinel its never going to hit
    SentinelScanner scanner9(sentinel9);
    std::size_t maxSize = sentinel9.size() - 1; //size of the sentinel string
    
    std::string emitted9 = "";
    for (int i = 0; i < 4000000; ++i)  //character is 1 byte, 4000000 chars is 4 megabytes
    {
        SentinelScanner::Out out9 = scanner9.feed("A"); //1 byte
        emitted9 += out9.safe_text; //keep track of fed bytes and data
        std::size_t totalFed = i + 1;

        std::size_t heldBack = totalFed - emitted9.size(); //because we can't get the stored pending value, it is calculated here through counting
        assert(heldBack <= maxSize && "pending_ buffer exceeded sentinel.size() - 1");
    }
    //





    // test 10: harness turn limit
    MockInput in10; //create mock input and output
    MockOutput out10;
    HarnessConfig cfg10;

    cfg10.max_turns = 3; 
    Harness harness10(std::make_unique<DummyModel>(), cfg10); //create harness with the config which is specifically a dummy model

    
    StopReason reason10 = harness10.run(in10, out10); //run the harness and wait for a stop reason
    assert(reason10.kind == StopReason::Kind::TurnLimit && "Harness did not error at TurnLimit");
    //


    // test 11: harness sentinel halt
    // mostly the same as test 10 but check for error with type "sentinel"
    MockInput in11; //create mock input and output
    MockOutput out11;
    HarnessConfig cfg11;

    cfg11.max_turns = 20;
    Harness harness11(std::make_unique<SentinelModel>(), cfg11); // create harness with the config

    
    StopReason reason11 = harness11.run(in11, out11); //run the harness and wait for a stop reason
    assert(reason11.kind == StopReason::Kind::Sentinel && "Harness failed to halt on sentinel");
    //


    // test 12: transcript round trip
    const std::string filename12 = "temp_mock_transcript.txt"; //create a mock transcript
    std::ofstream out12(filename12); //create output file

    out12 << "role: system\nBe concise.\n---\nrole: user\nhello\n---\nrole: assistant\nHi! What can I do for you today?\n"; //this is the mock conversation
    out12.close();

    ReplayModelClient client(filename12); //create a new client and give it messages
    Conversation conv12;
    conv12.append(Message(Role::System, "Be concise.")); //messages matching the mock messages above
    conv12.append(Message(Role::User, "hello"));
    
    Message reply12 = client.generate(conv12); //generate conversation
    assert((reply12.role() == Role::Assistant) && "Round trip test failed: Reply was not as the expected role as an assistant"); //if the reply is not as an assistant
    assert(reply12.content() == "Hi! What can I do for you today?" && "Round trip test failed: Reply message was incorrect"); //if the reply did not have the right message
    
    std::remove(filename12.c_str()); //don't want to actually keep the file
    
    
    std::cout << "All 12 tests passed successfully!\n"; //if it makes it to here, it works
    return 0;
}