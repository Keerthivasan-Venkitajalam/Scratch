#include <gtest/gtest.h>
#include "parser/fsm_fix_parser.hpp"
#include "common/tick.hpp"
#include <vector>
#include <string>

using namespace feedhandler::parser;
using namespace feedhandler::common;

// Test Fixture for FSM Parser
class FSMParserTest : public ::testing::Test {
protected:
    FSMFixParser parser;
    std::vector<Tick> ticks;
    
    void SetUp() override {
        parser.reset();
        ticks.clear();
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
};

// ============================================================================
// Test 1: Full Message Parsing
// ============================================================================

TEST_F(FSMParserTest, ParseCompleteMessage) {
    const char* message = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    ASSERT_GT(ticks.size(), 0);
    
    const auto& tick = ticks[0];
    EXPECT_EQ(tick.symbol, "AAPL");
    EXPECT_EQ(tick.price, 1502500);  // 150.25 * 10000
    EXPECT_EQ(tick.qty, 500);
    EXPECT_EQ(tick.side, 'B');
    EXPECT_TRUE(tick.is_valid());
}

TEST_F(FSMParserTest, ParseMultipleCompleteMessages) {
    const char* buffer = 
        "8=FIX.4.4|35=D|55=MSFT|44=123.45|38=1000|54=1|10=001|\n"
        "8=FIX.4.4|35=D|55=GOOGL|44=2750.80|38=100|54=2|10=002|\n"
        "8=FIX.4.4|35=D|55=TSLA|44=245.67|38=750|54=1|10=003|\n";
    
    size_t consumed = parser.parse(buffer, strlen(buffer), ticks);
    
    EXPECT_EQ(consumed, strlen(buffer));
    EXPECT_EQ(ticks.size(), 3);
    
    // Verify first tick
    EXPECT_EQ(ticks[0].symbol, "MSFT");
    EXPECT_EQ(ticks[0].price, 1234500);
    EXPECT_EQ(ticks[0].qty, 1000);
    EXPECT_EQ(ticks[0].side, 'B');
    
    // Verify second tick
    EXPECT_EQ(ticks[1].symbol, "GOOGL");
    EXPECT_EQ(ticks[1].price, 27508000);
    EXPECT_EQ(ticks[1].qty, 100);
    EXPECT_EQ(ticks[1].side, 'S');
    
    // Verify third tick
    EXPECT_EQ(ticks[2].symbol, "TSLA");
    EXPECT_EQ(ticks[2].price, 2456700);
    EXPECT_EQ(ticks[2].qty, 750);
    EXPECT_EQ(ticks[2].side, 'B');
}

// ============================================================================
// Test 2: Fragmented Message Parsing
// ============================================================================

TEST_F(FSMParserTest, ParseFragmentedMessage_TwoChunks) {
    // Fragment 1: First half of message
    const char* fragment1 = "8=FIX.4.4|35=D|55=AAPL|44=150";
    size_t consumed1 = parser.parse(fragment1, strlen(fragment1), ticks);
    
    EXPECT_EQ(consumed1, strlen(fragment1));
    EXPECT_EQ(ticks.size(), 0);  // No complete message yet
    EXPECT_TRUE(parser.is_parsing());  // Parser should be mid-message
    
    // Fragment 2: Second half of message
    const char* fragment2 = ".25|38=500|54=1|10=123|\n";
    size_t consumed2 = parser.parse(fragment2, strlen(fragment2), ticks);
    
    EXPECT_EQ(consumed2, strlen(fragment2));
    EXPECT_EQ(ticks.size(), 1);  // Now we have a complete message
    EXPECT_FALSE(parser.is_parsing());  // Parser should be idle
    
    const auto& tick = ticks[0];
    EXPECT_EQ(tick.symbol, "AAPL");
    EXPECT_EQ(tick.price, 1502500);
    EXPECT_EQ(tick.qty, 500);
    EXPECT_EQ(tick.side, 'B');
}

TEST_F(FSMParserTest, ParseFragmentedMessage_MultipleChunks) {
    // Simulate highly fragmented TCP stream
    const char* fragment1 = "8=FIX.4.4|35=D|55=GO";
    const char* fragment2 = "OGL|44=2750.";
    const char* fragment3 = "80|38=100|54=";
    const char* fragment4 = "2|10=456|\n";
    
    parser.parse(fragment1, strlen(fragment1), ticks);
    EXPECT_EQ(ticks.size(), 0);
    EXPECT_TRUE(parser.is_parsing());
    
    parser.parse(fragment2, strlen(fragment2), ticks);
    EXPECT_EQ(ticks.size(), 0);
    EXPECT_TRUE(parser.is_parsing());
    
    parser.parse(fragment3, strlen(fragment3), ticks);
    EXPECT_EQ(ticks.size(), 0);
    EXPECT_TRUE(parser.is_parsing());
    
    parser.parse(fragment4, strlen(fragment4), ticks);
    EXPECT_EQ(ticks.size(), 1);
    EXPECT_FALSE(parser.is_parsing());
    
    const auto& tick = ticks[0];
    EXPECT_EQ(tick.symbol, "GOOGL");
    EXPECT_EQ(tick.price, 27508000);
    EXPECT_EQ(tick.qty, 100);
    EXPECT_EQ(tick.side, 'S');
}

TEST_F(FSMParserTest, ParseFragmentedMessage_ByteByByte) {
    // Extreme case: one byte at a time
    std::string message = "8=FIX.4.4|35=D|55=BTC|44=45000.00|38=10|54=1|10=999|\n";
    
    for (size_t i = 0; i < message.size(); ++i) {
        parser.parse(&message[i], 1, ticks);
        
        // Parser may complete on tag 10 (checksum) or newline
        // Don't check is_parsing() state as it depends on message structure
    }
    
    EXPECT_EQ(ticks.size(), 1);
    
    const auto& tick = ticks[0];
    EXPECT_EQ(tick.symbol, "BTC");
    EXPECT_EQ(tick.price, 450000000);
    EXPECT_EQ(tick.qty, 10);
    EXPECT_EQ(tick.side, 'B');
}

// ============================================================================
// Test 3: Corrupt Message Handling
// ============================================================================

TEST_F(FSMParserTest, HandleMissingRequiredFields) {
    // Message missing symbol (tag 55)
    const char* message = "8=FIX.4.4|35=D|44=150.25|38=500|54=1|10=123|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    // Parser design: invalid messages are silently dropped, not emitted
    EXPECT_EQ(ticks.size(), 0);  // No tick emitted for invalid message
}

TEST_F(FSMParserTest, HandleInvalidPriceFormat) {
    // Price with invalid characters
    const char* message = "8=FIX.4.4|35=D|55=AAPL|44=ABC.XYZ|38=500|54=1|10=123|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    
    if (ticks.size() > 0) {
        // Parser should handle gracefully, price might be 0
        EXPECT_EQ(ticks[0].price, 0);
    }
}

TEST_F(FSMParserTest, HandleInvalidSideValue) {
    // Side value not 1 or 2
    const char* message = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=9|10=123|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    
    if (ticks.size() > 0) {
        // Side should be invalid ('\0')
        EXPECT_EQ(ticks[0].side, '\0');
        EXPECT_FALSE(ticks[0].is_valid());
    }
}

TEST_F(FSMParserTest, HandleEmptyMessage) {
    const char* message = "";
    
    size_t consumed = parser.parse(message, 0, ticks);
    
    EXPECT_EQ(consumed, 0);
    EXPECT_EQ(ticks.size(), 0);
    EXPECT_FALSE(parser.is_parsing());
}

TEST_F(FSMParserTest, HandleMessageWithOnlyDelimiters) {
    const char* message = "|||||\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    // Parser should handle gracefully, might produce invalid tick or no tick
}

// ============================================================================
// Test 4: Edge Cases
// ============================================================================

TEST_F(FSMParserTest, HandleVeryLongSymbol) {
    // Symbol longer than typical
    const char* message = "8=FIX.4.4|35=D|55=VERYLONGSYMBOLNAME123456|44=100.00|38=10|54=1|10=999|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    
    if (ticks.size() > 0) {
        EXPECT_EQ(ticks[0].symbol, "VERYLONGSYMBOLNAME123456");
    }
}

TEST_F(FSMParserTest, HandleVeryLargePriceValue) {
    // Very large price
    const char* message = "8=FIX.4.4|35=D|55=BTC|44=99999.9999|38=1|54=1|10=999|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    
    if (ticks.size() > 0) {
        EXPECT_EQ(ticks[0].price, 999999999);  // 99999.9999 * 10000
    }
}

TEST_F(FSMParserTest, HandleZeroQuantity) {
    const char* message = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=0|54=1|10=123|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    
    if (ticks.size() > 0) {
        EXPECT_EQ(ticks[0].qty, 0);
        EXPECT_FALSE(ticks[0].is_valid());  // Zero quantity should be invalid
    }
}

TEST_F(FSMParserTest, HandleNegativePrice) {
    const char* message = "8=FIX.4.4|35=D|55=AAPL|44=-150.25|38=500|54=1|10=123|\n";
    
    size_t consumed = parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(consumed, strlen(message));
    EXPECT_EQ(ticks.size(), 1);
    
    if (ticks.size() > 0) {
        // Negative price should be handled (might be 0 or negative)
        EXPECT_LE(ticks[0].price, 0);
    }
}

// ============================================================================
// Test 5: Parser State Management
// ============================================================================

TEST_F(FSMParserTest, ResetParserState) {
    // Parse partial message
    const char* fragment = "8=FIX.4.4|35=D|55=AAPL|44=150";
    parser.parse(fragment, strlen(fragment), ticks);
    
    EXPECT_TRUE(parser.is_parsing());
    
    // Reset parser
    parser.reset();
    
    EXPECT_FALSE(parser.is_parsing());
    EXPECT_EQ(parser.get_state(), FSMFixParser::State::WAIT_TAG);
    
    // Parse new complete message
    const char* message = "8=FIX.4.4|35=D|55=MSFT|44=200.00|38=100|54=2|10=456|\n";
    parser.parse(message, strlen(message), ticks);
    
    EXPECT_EQ(ticks.size(), 1);
    EXPECT_EQ(ticks[0].symbol, "MSFT");
}

TEST_F(FSMParserTest, MultipleMessagesWithFragmentation) {
    // First message complete
    const char* msg1 = "8=FIX.4.4|35=D|55=AAPL|44=150.25|38=500|54=1|10=123|\n";
    parser.parse(msg1, strlen(msg1), ticks);
    EXPECT_EQ(ticks.size(), 1);
    
    // Second message fragmented
    const char* msg2_part1 = "8=FIX.4.4|35=D|55=GOOGL|44=27";
    parser.parse(msg2_part1, strlen(msg2_part1), ticks);
    EXPECT_EQ(ticks.size(), 1);  // Still only first message
    
    const char* msg2_part2 = "50.80|38=100|54=2|10=456|\n";
    parser.parse(msg2_part2, strlen(msg2_part2), ticks);
    EXPECT_EQ(ticks.size(), 2);  // Now both messages
    
    // Third message complete
    const char* msg3 = "8=FIX.4.4|35=D|55=TSLA|44=245.67|38=750|54=1|10=789|\n";
    parser.parse(msg3, strlen(msg3), ticks);
    EXPECT_EQ(ticks.size(), 3);
    
    // Verify all ticks
    EXPECT_EQ(ticks[0].symbol, "AAPL");
    EXPECT_EQ(ticks[1].symbol, "GOOGL");
    EXPECT_EQ(ticks[2].symbol, "TSLA");
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}