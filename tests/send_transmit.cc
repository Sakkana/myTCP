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

            TCPSenderTestHarness test{"Three short writes", cfg};
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));

            // ---

            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});

            // ---
            
            test.execute(WriteBytes{"ab"});
            test.execute(ExpectSegment{}
                        .with_data("ab")
                        .with_seqno(isn + 1));
            // ---
            test.execute(WriteBytes{"cd"});
            test.execute(ExpectSegment{}
                        .with_data("cd")
                        .with_seqno(isn + 3));
            // ---
            test.execute(WriteBytes{"abcd"});
            test.execute(ExpectSegment{}
                        .with_data("abcd")
                        .with_seqno(isn + 5));
            // ---
            test.execute(ExpectSeqno{WrappingInt32{isn + 9}});
            test.execute(ExpectBytesInFlight{8});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
        }
        cout << "\033[33m" << "=== Test 1 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 2 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"Many short writes, continuous acks", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}});
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            uint32_t max_block_size = 10;
            uint32_t n_rounds = 10000;
            size_t bytes_sent = 0;
            for (uint32_t i = 0; i < n_rounds; ++i) {
                string data;
                uint32_t block_size = uniform_int_distribution<uint32_t>{1, max_block_size}(rd);
                for (uint8_t j = 0; j < block_size; ++j) {
                    uint8_t c = 'a' + ((i + j) % 26);
                    data.push_back(c);
                }
                test.execute(ExpectSeqno{WrappingInt32{isn + uint32_t(bytes_sent) + 1}});
                test.execute(WriteBytes(string(data)));
                bytes_sent += block_size;
                test.execute(ExpectBytesInFlight{block_size});
                test.execute(
                    ExpectSegment{}.with_seqno(isn + 1 + uint32_t(bytes_sent - block_size)).with_data(move(data)));
                test.execute(ExpectNoSegment{});
                test.execute(AckReceived{WrappingInt32{isn + 1 + uint32_t(bytes_sent)}});
            }
        }
        cout << "\033[33m" << "=== Test 2 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 3 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"Many short writes, ack at end", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(65000));
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            uint32_t max_block_size = 10;
            uint32_t n_rounds = 1000;
            size_t bytes_sent = 0;
            for (uint32_t i = 0; i < n_rounds; ++i) {
                string data;
                uint32_t block_size = uniform_int_distribution<uint32_t>{1, max_block_size}(rd);
                for (uint8_t j = 0; j < block_size; ++j) {
                    uint8_t c = 'a' + ((i + j) % 26);
                    data.push_back(c);
                }
                test.execute(ExpectSeqno{WrappingInt32{isn + uint32_t(bytes_sent) + 1}});
                test.execute(WriteBytes(string(data)));
                bytes_sent += block_size;
                test.execute(ExpectBytesInFlight{bytes_sent});
                test.execute(
                    ExpectSegment{}.with_seqno(isn + 1 + uint32_t(bytes_sent - block_size)).with_data(move(data)));
                test.execute(ExpectNoSegment{});
            }
            test.execute(ExpectBytesInFlight{bytes_sent});
            test.execute(AckReceived{WrappingInt32{isn + 1 + uint32_t(bytes_sent)}});
            test.execute(ExpectBytesInFlight{0});
        }
        cout << "\033[33m" << "=== Test 3 OK ===" << "\033[0m" << endl;

        {
            // LOG_OPEN = true;
            cout << "\033[33m" << "=== Test 4 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            // 1. SYN_SENT
            TCPSenderTestHarness test{"Window filling", cfg};
            test.execute(ExpectSegment{}
                        .with_no_flags()
                        .with_syn(true)
                        .with_payload_size(0)
                        .with_seqno(isn));

            // cout << "test ack: " << isn + 1 - isn << endl << endl;
            test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(3));
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});

            test.execute(WriteBytes("01234567"));
            test.execute(ExpectBytesInFlight{3});
            test.execute(ExpectSegment{}.with_data("012"));
            test.execute(ExpectNoSegment{});

            test.execute(ExpectSeqno{WrappingInt32{isn + 1 + 3}});
            // cout << "test ack: " << isn + 1 + 3 - isn << endl << endl;
            // cout << "fuck 1" << endl;
            test.execute(AckReceived{WrappingInt32{isn + 1 + 3}}.with_win(3));
            // cout << "fuck 2" << endl;
            test.execute(ExpectBytesInFlight{3});
            // cout << "fuck 3" << endl;
            test.execute(ExpectSegment{}.with_data("345"));
            // cout << "fuck 4" << endl;
            test.execute(ExpectNoSegment{});
            // cout << "fuck 5" << endl;


            test.execute(ExpectSeqno{WrappingInt32{isn + 1 + 6}});
            // cout << "fuck 6" << endl;
            // cout << "test ack: " << isn + 1 + 6 - isn << endl << endl;
            // cout << "fuck 7" << endl;
            test.execute(AckReceived{WrappingInt32{isn + 1 + 6}}.with_win(3));
            test.execute(ExpectBytesInFlight{2});
            test.execute(ExpectSegment{}.with_data("67"));
            test.execute(ExpectNoSegment{});

            test.execute(ExpectSeqno{WrappingInt32{isn + 1 + 8}});
            // cout << "test ack: " << isn + 1 + 8 - isn << endl << endl;
            test.execute(AckReceived{WrappingInt32{isn + 1 + 8}}.with_win(3));
            test.execute(ExpectBytesInFlight{0});
            test.execute(ExpectNoSegment{});
        }
        // LOG_OPEN = false;
        cout << "\033[33m" << "=== Test 4 OK ===" << "\033[0m" << endl;

        {
            cout << "\033[33m" << "=== Test 5 ===" << "\033[0m" << endl;

            TCPConfig cfg;
            WrappingInt32 isn(rd());
            cfg.fixed_isn = isn;

            TCPSenderTestHarness test{"Immediate writes respect the window", cfg};
            test.execute(ExpectSegment{}.with_no_flags().with_syn(true).with_payload_size(0).with_seqno(isn));
            test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(3));
            test.execute(ExpectState{TCPSenderStateSummary::SYN_ACKED});
            test.execute(WriteBytes("01"));
            test.execute(ExpectBytesInFlight{2});
            test.execute(ExpectSegment{}.with_data("01"));
            test.execute(ExpectNoSegment{});
            test.execute(ExpectSeqno{WrappingInt32{isn + 1 + 2}});
            test.execute(WriteBytes("23"));
            test.execute(ExpectBytesInFlight{3});
            test.execute(ExpectSegment{}.with_data("2"));
            test.execute(ExpectNoSegment{});
            test.execute(ExpectSeqno{WrappingInt32{isn + 1 + 3}});
        }
        cout << "\033[33m" << "=== Test 5 OK ===" << "\033[0m" << endl;

    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
