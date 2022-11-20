#include "sender_harness.hh"
#include "tcp_config.hh"
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

            TCPSenderTestHarness test{"FIN sent test", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(Close{});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectBytesInFlight{1});
            test.execute(ExpectSegment{}.with_fin(true).with_seqno(isn + 1));
            test.execute(ExpectNoSegment{});
        }
        cout << "\033[33m" << "=== Test 1 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 2 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"FIN acked test", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(Close{});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectSegment{}.with_fin(true).with_seqno(isn + 1));
            test.execute(AckReceived{WrappingInt32{isn + 2}});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_ACKED});
            test.execute(ExpectBytesInFlight{0});
            test.execute(ExpectNoSegment{});
        }
        cout << "\033[33m" << "=== Test 2 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 3 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"FIN not acked test", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(Close{});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectSegment{}.with_fin(true).with_seqno(isn + 1));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectBytesInFlight{1});
            test.execute(ExpectNoSegment{});
        }
        cout << "\033[33m" << "=== Test 3 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 4 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"FIN retx test", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(Close{});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectSegment{}.with_fin(true).with_seqno(isn + 1));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectBytesInFlight{1});
            test.execute(ExpectNoSegment{});
            test.execute(Tick{TCPConfig::TIMEOUT_DFLT - 1});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectBytesInFlight{1});
            test.execute(ExpectNoSegment{});
            test.execute(Tick{1});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectBytesInFlight{1});
            test.execute(ExpectSegment{}.with_fin(true).with_seqno(isn + 1));
            test.execute(ExpectNoSegment{});
            test.execute(Tick{1});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_SENT});
            test.execute(ExpectBytesInFlight{1});
            test.execute(ExpectNoSegment{});
            test.execute(AckReceived{WrappingInt32{isn + 2}});
            test.execute(ExpectState{TCPSenderStateSummary::FIN_ACKED});
            test.execute(ExpectBytesInFlight{0});
            test.execute(ExpectNoSegment{});
        }
        cout << "\033[33m" << "=== Test 4 OK ===" << "\033[0m" << endl;

    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
