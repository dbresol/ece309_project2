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


//Credit to Stack Overflow for resources for help with some stuff. Wow!
//mock classes for harness integration
class MockInput : public InputSource {
    int current_turn = 0;
public:
    std::string read_line() override { current_turn++; return "user input"; }
    bool is_eof() const override { return false; }
};

class MockOutput : public OutputSink {
public:
    void write(std::string_view) override {} // Discard output during tests
};

class DummyModel : public ModelClient {
public:
    void generate(const Conversation&, TokenSink& sink) override {
        sink.on_chunk("dummy reply");
        sink.on_complete();
    }
};

class SentinelModel : public ModelClient {
public:
    void generate(const Conversation&, TokenSink& sink) override {
        sink.on_chunk("done<|end_conversation|>");
        sink.on_complete();
    }
};


int main() {
    std::cout << "Running P2 Tests...\n";

    //test 1: empty conversion bounds
    Conversation conv1;
    //Conversation conv;
    assert(conv1.size() == 0);
    assert(conv1.begin() == conv1.end());
    
    bool caught = false;
    try { conv1.at(0); } 
    catch (const std::out_of_range&) { caught = true; }
    assert(caught && "at() should throw out_of_range on empty conversation");


    // test 2: system message ordering
    Conversation conv2;
    conv2.append(Message(Role::System, "System prompt"));
    conv2.append(Message(Role::User, "User message"));
    
    assert(conv2.size() == 2);
    assert(conv2.at(0).role() == Role::System && "System message displaced");
    assert(conv2.at(1).role() == Role::User);


    // test 3: copy for rule of five
    Conversation c1_3;
    c1_3.append(Message(Role::User, "Hello"));
    
    Conversation c2_3(c1_3);
    assert(c1_3.size() == 1 && c2_3.size() == 1);
    assert(c1_3.at(0).content() == c2_3.at(0).content());
    assert(c1_3.begin() != c2_3.begin() && "Copy constructor did not allocate new memory");


    // test 4: move for rule of five
    Conversation c1_4;
    c1_4.append(Message(Role::User, "Hello"));
    const Message* original_ptr = c1_4.begin();
    
    Conversation c2_4(std::move(c1_4));
    assert(c2_4.begin() == original_ptr && "Move constructor failed to steal pointer");
    assert(c2_4.size() == 1);
    assert(c1_4.size() == 0 && c1_4.begin() == nullptr && "Move constructor failed to zero source");


    // test 5: growth behavior
    Conversation conv5;
    const int target_size = 100; 
    for (int i = 0; i < target_size; ++i) {
        conv5.append(Message(Role::User, std::to_string(i)));
    }
    assert(conv5.size() == target_size);
    for (int i = 0; i < target_size; ++i) {
        assert(conv5.at(i).content() == std::to_string(i) && "Data corrupted during reallocation");
    }


    // tests for sentinel scanner
    // test 6: scanner clean text
    SentinelScanner scanner6("STOP"); // max hold-back is 3 chars
    auto out = scanner6.feed("Hello "); // Length 6. Should emit 3, hold 3.
    assert(!out.sentinel_found);
    assert(out.safe_text == "Hel");
    
    auto flush_out = scanner6.flush();
    assert(flush_out.safe_text == "lo ");


    // test 7: scanner split sentinel
    const std::string sentinel7 = "<|end_conversation|>";
    const std::string text7 = "Goodbye." + sentinel7;
    
    for (std::size_t split = 0; split <= text7.size(); ++split) {
        SentinelScanner scanner7(sentinel7);
        auto out1_7 = scanner7.feed(text7.substr(0, split));
        auto out2_7 = scanner7.feed(text7.substr(split));
        
        assert((out1_7.sentinel_found || out2_7.sentinel_found) && "Sentinel missed at split boundary");
        assert(out1_7.safe_text + out2_7.safe_text == "Goodbye.");
    }


    // test 8: scanner false alarms
    SentinelScanner scanner8("<|end_conversation|>");
    auto out8 = scanner8.feed("Goodbye <|end_world|>");
    assert(!out.sentinel_found && "Triggered false alarm on partial match");


    //test 9: scanner with bounded memory
    const std::string sentinel9 = "<|stop|>";
    SentinelScanner scanner9(sentinel9);
    std::size_t max_held = sentinel9.size() - 1;
    
    std::string emitted9 = "";
    // 100000 * 8 is 8 megabytes... that's a lot, but it needs to be done!
    for (int i = 0; i < 1000000; ++i) {
        auto out9 = scanner9.feed("A");
        emitted9 += out9.safe_text;
        
        std::size_t total_fed = i + 1;
        std::size_t held_back = total_fed - emitted9.size();
        assert(held_back <= max_held && "pending_ buffer exceeded O(1) mathematical bound");
    }





    // tests for harness integration. in visual studio there are underlines down here... but it complies fine
    // case 10: harness turn limit
    HarnessConfig cfg1;
    cfg1.max_turns = 3; 
    Harness harness1(std::make_unique<DummyModel>(), cfg1);
    MockInput in1;
    MockOutput out1;
    
    StopReason reason1 = harness1.run(in1, out1);
    assert(reason1.kind == StopReason::Kind::TurnLimit && "Harness failed to stop at turn limit");


    // case 11: harness sentinel halt
    HarnessConfig cfg2;
    cfg2.max_turns = 20;
    Harness harness2(std::make_unique<SentinelModel>(), cfg2);
    MockInput in2;
    MockOutput out2;
    
    StopReason reason2 = harness2.run(in2, out2);
    assert(reason2.kind == StopReason::Kind::Sentinel && "Harness failed to halt on sentinel");


    // case 12: transcript round trip
    //create a mock transcript!
    const std::string filename3 = "temp_mock_transcript.txt";
    std::ofstream out3(filename3);
    out3 << "role: system\nBe concise.\n---\nrole: user\nhello\n---\nrole: assistant\nHi! What can I do for you today?\n";
    out3.close();

    //run it
    ReplayModelClient client(filename3);
    Conversation conv3;
    conv3.append(Message(Role::System, "Be concise."));
    conv3.append(Message(Role::User, "hello"));
    
    Message reply3 = client.generate(conv3);
    assert(reply3.role() == Role::Assistant);
    assert(reply3.content() == "Hi! What can I do for you today?" && "Round trip playback failed");
    
    std::remove(filename3.c_str()); // clean up

    // if it reaches this point, it works!!!!!!!!!!
    
    std::cout << "All 12 tests passed successfully!\n";
    return 0;
}
