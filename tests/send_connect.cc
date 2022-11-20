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
            cout << "\033[33m" << "=== Test 1 ===" << "\033[0m" << endl;
            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"SYN sent test", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});
        }
        cout << "\033[33m" << "=== Test 1 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 2 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"SYN acked test", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            // Sender 发送 syn 报文
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});

            // Sender 收到 ack = isn + 1
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectNoSegment{});
            test.execute(ExpectBytesInFlight{0});
        }
        cout << "\033[33m" << "=== Test 2 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 3 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"SYN -> wrong ack test", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});

            // Sender 发送 syn 报文
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});

            // Sender 收到错误 ack = isn (应该是 syn = isn + 1)
            test.execute(AckReceived{WrappingInt32{isn}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectNoSegment{});
            test.execute(ExpectBytesInFlight{1});
        }
        cout << "\033[33m" << "=== Test 3 OK ===" << "\033[0m" << endl;
        
        {
            cout << "\033[33m" << "=== Test 4 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            // State 1: CLOSED
            // waiting for stream to begin (no SYN sent)

            // ---



            TCPSenderTestHarness test{"SYN acked, data", cfg};
            test.execute(ExpectState{TCPSenderStateSummary::SYN_SENT});
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));
            test.execute(ExpectBytesInFlight{1});

            // State 2: SYN_SENT
            // stream started but nothing acknowledged

            // ---


            // cout << "Ack Received" << endl;
            // cout << "ack: " << isn + 1 << endl;
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectNoSegment{});
            test.execute(ExpectBytesInFlight{0});

            // State 3: SYN_ACKED
            // stream ongoing


            test.execute(WriteBytes{"abcdefgh"});
            test.execute(Tick{1});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectSegment{}.with_seqno(isn + 1).with_data("abcdefgh"));
            test.execute(ExpectBytesInFlight{8});
            test.execute(AckReceived{WrappingInt32{isn + 9}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(ExpectNoSegment{});
            test.execute(ExpectBytesInFlight{0});
            test.execute(ExpectSeqno{WrappingInt32{isn + 9}});

            // State3: SYN_ACKED
            // stream ongoing

            // ---

        }
        cout << "\033[33m" << "=== Test 4 OK ===" << "\033[0m" << endl;

    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
