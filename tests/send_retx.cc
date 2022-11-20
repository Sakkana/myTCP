#include "sender_harness.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main() {
    try {
        auto rd = get_random_generator();

        {
            cout << "\033[33m" << "=== Test 1 (1.1) ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());

            // 初始化 initial_rtx
            uint16_t retx_timeout = uniform_int_distribution<uint16_t>{10, 10000}(rd);
            cfg.fixed_isn = isn;
            cfg.rt_timeout = retx_timeout;
            cout << "initial RTO: " << retx_timeout << endl;

            TCPSenderTestHarness test{"Retx SYN twice at the right times, then ack", cfg};
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));            
            test.execute(ExpectNoSegment{});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});

            // 差点超时
            test.execute(Tick{retx_timeout - 1u});
            test.execute(ExpectNoSegment{});

            // 正式超时
            test.execute(Tick{1});
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));

            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectBytesInFlight{1});

            cout << "\033[33m" << "=== Test 1 (1.2) ===" << "\033[0m" << endl;

            // Wait twice as long b/c exponential back-off
            // 差点超时
            cout << "nearly tieout" << endl;
            test.execute(Tick{2 * retx_timeout - 1u});
            test.execute(ExpectNoSegment{});

            // 正式超时
            cout << "truly timeout" << endl;
            test.execute(Tick{1});
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectBytesInFlight{1});

            // 收到 ack
            cout << "Get ack: 1 for SYN" << endl;
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectBytesInFlight{0});
        }
        cout << "\033[33m" << "=== Test 1 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 2 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            uint16_t retx_timeout = uniform_int_distribution<uint16_t>{10, 10000}(rd);
            cfg.fixed_isn = isn;
            cfg.rt_timeout = retx_timeout;

            TCPSenderTestHarness test{"Retx SYN until too many retransmissions", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(ExpectNoSegment{});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            for (size_t attempt_no = 0; attempt_no < TCPConfig::MAX_RETX_ATTEMPTS; attempt_no++) {
                cout << "\033[33m" << "=== Test 2" << "(2." << attempt_no + 1 << ") ===" << "\033[0m" << endl;

                cout << "nearly timeout" << endl;
                test.execute(Tick{(retx_timeout << attempt_no) - 1u}
                            .with_max_retx_exceeded(false));
                test.execute(ExpectNoSegment{});

                cout << "truly timeout" << endl;
                test.execute(Tick{1}
                            .with_max_retx_exceeded(false));
                test.execute(ExpectSegment{}
                            .with_no_flags()
                            .with_syn(true)
                            .with_payload_size(0)
                            .with_seqno(isn));
                test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
                test.execute(ExpectBytesInFlight{1});
            }

            cout << "\033[33m" << "=== out of the loop ===" << "\033[0m" << endl;
            test.execute(Tick{(retx_timeout << TCPConfig::MAX_RETX_ATTEMPTS) - 1u}
                        .with_max_retx_exceeded(false));
            test.execute(Tick{1}
                        .with_max_retx_exceeded(true));
        }
        cout << "\033[33m" << "=== Test 2 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 3 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            uint16_t retx_timeout = uniform_int_distribution<uint16_t>{10, 10000}(rd);
            cfg.fixed_isn = isn;
            cfg.rt_timeout = retx_timeout;

            cout << "\033[33m" << "=== Test 3 (3.1) ===" << "\033[0m" << endl;
            TCPSenderTestHarness test{"Send some data, the retx and succeed, then retx till limit", cfg};
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));
            test.execute(ExpectNoSegment{});
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            cout << "\033[33m" << "=== 3.1 ack ===" << "\033[0m" << endl;    

            cout << "\033[33m" << "=== Test 3 (3.2) ===" << "\033[0m" << endl;
            test.execute(WriteBytes{"abcd"});
            test.execute(ExpectSegment{}
                        .with_payload_size(4));
            test.execute(ExpectNoSegment{});
            test.execute(AckReceived{WrappingInt32{isn + 5}});
            test.execute(ExpectBytesInFlight{0});
            cout << "\033[33m" << "=== 3.2 ack ===" << "\033[0m" << endl;    

            cout << "\033[33m" << "=== Test 3 (3.3) ===" << "\033[0m" << endl;
            test.execute(WriteBytes{"efgh"});
            test.execute(ExpectSegment{}
                        .with_payload_size(4));
            test.execute(ExpectNoSegment{});

            cout << "\033[33m" << "=== Test 3 (3.4) timeout, retransmit the recently one ===" << "\033[0m" << endl;
            test.execute(Tick{retx_timeout}
                        .with_max_retx_exceeded(false));
            test.execute(ExpectSegment{}
                        .with_payload_size(4));
            test.execute(ExpectNoSegment{});        
            test.execute(AckReceived{WrappingInt32{isn + 9}});
            test.execute(ExpectBytesInFlight{0});
            cout << "\033[33m" << "=== 3.3, 3.4 ack ===" << "\033[0m" << endl;    

            cout << "\033[33m" << "=== Test 3 (3.5) ===" << "\033[0m" << endl;    
            test.execute(WriteBytes{"ijkl"});
            test.execute(ExpectSegment{}
                        .with_payload_size(4)
                        .with_seqno(isn + 9));
            for (size_t attempt_no = 0; attempt_no < TCPConfig::MAX_RETX_ATTEMPTS; attempt_no++) {
                cout << "\033[33m" << "=== Test 3 (3.5." << attempt_no + 1 << ") ===" << "\033[0m" << endl;
                cout << "nearly timeout" << endl;
                test.execute(Tick{(retx_timeout << attempt_no) - 1u}
                            .with_max_retx_exceeded(false));
                test.execute(ExpectNoSegment{});

                cout << "truly timeout" << endl;
                test.execute(Tick{1}
                            .with_max_retx_exceeded(false));
                test.execute(ExpectSegment{}
                            .with_payload_size(4)
                            .with_seqno(isn + 9));
                test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
                test.execute(ExpectBytesInFlight{4});
            }

            cout << "\033[33m" << "=== out of the loop ===" << "\033[0m" << endl;
            test.execute(Tick{(retx_timeout << TCPConfig::MAX_RETX_ATTEMPTS) - 1u}.with_max_retx_exceeded(false));
            test.execute(Tick{1}.with_max_retx_exceeded(true));
        }
        cout << "\033[33m" << "=== Test 3 OK ===" << "\033[0m" << endl;

    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
