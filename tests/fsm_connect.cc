#include "tcp_config.hh"
#include "tcp_expectation.hh"
#include "tcp_fsm_test_harness.hh"
#include "tcp_header.hh"
#include "tcp_segment.hh"
#include "util.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace std;
using State = TCPTestHarness::State;

int main() {
    try {
        TCPConfig cfg{};
        auto rd = get_random_generator();

        // NB: In Riad's version of this test, the ACK was not ignored, but per
        // pg 68 of the RFC, I think that it should be.
        //
        // test #1: START -> SYN_SENT -> ACK (ignored) -> SYN -> SYN_RECV
        {
            TEST(1);

            TCPTestHarness test_1(cfg);

            // tell the FSM to connect, make sure we get a SYN
            // 1. 建立连接，发送 ack
            test_1.execute(Connect{});
            test_1.execute(Tick(1));
            TCPSegment seg1 = test_1.expect_seg(ExpectOneSegment{}.with_syn(true).with_ack(false),
                                                "test 1 failed: could not parse SYN segment or invalid flags");

            test_1.execute(ExpectState{State::SYN_SENT});

            // send ACK only (no SYN yet)
            test_1.send_ack(WrappingInt32{0}, seg1.header().seqno + 1);
            test_1.execute(Tick(1));
            test_1.execute(ExpectState{State::SYN_SENT});

            // now send SYN
            const WrappingInt32 isn(rd());
            test_1.send_syn(isn);
            test_1.execute(Tick(1));
            test_1.execute(ExpectState{State::SYN_RCVD});

            test_1.execute(ExpectOneSegment{}.with_ack(true).with_syn(false).with_ackno(isn + 1));

            test_1.execute(ExpectBytesInFlight{1UL});

            OK(1);
        }

        // test #2: START -> SYN_SENT -> SYN -> ACK -> ESTABLISHED
        {
            TEST(2);

            TCPTestHarness test_2(cfg);

            test_2.execute(Connect{});
            test_2.execute(Tick(1));

            TCPSegment seg = test_2.expect_seg(ExpectOneSegment{}.with_syn(true).with_ack(false),
                                               "test 2 failed: could not parse SYN segment or invalid flags");
            auto &seg_hdr = seg.header();

            test_2.execute(ExpectState{State::SYN_SENT});

            // send SYN (no ACK yet)
            const WrappingInt32 isn(rd());
            test_2.send_syn(isn);
            test_2.execute(Tick(1));

            test_2.expect_seg(ExpectOneSegment{}.with_syn(false).with_ack(true).with_ackno(isn + 1),
                              "test 2 failed: bad ACK for SYN");

            test_2.execute(ExpectState{State::SYN_RCVD});

            // now send ACK
            test_2.send_ack(isn + 1, seg_hdr.seqno + 1);
            test_2.execute(Tick(1));
            test_2.execute(ExpectNoSegment{}, "test 2 failed: got spurious ACK after ACKing SYN");
            test_2.execute(ExpectState{State::ESTABLISHED});

            OK(2);
        }

        // test #3: START -> SYN_SENT -> SYN/ACK -> ESTABLISHED
        {
            TEST(1);

            TCPTestHarness test_3(cfg);

            test_3.execute(Connect{});
            test_3.execute(Tick(1));

            TCPSegment seg = test_3.expect_seg(ExpectOneSegment{}.with_syn(true).with_ack(false),
                                               "test 3 failed: could not parse SYN segment or invalid flags");
            auto &seg_hdr = seg.header();

            test_3.execute(ExpectState{State::SYN_SENT});

            // send SYN (no ACK yet)
            const WrappingInt32 isn(rd());
            test_3.send_syn(isn, seg_hdr.seqno + 1);
            test_3.execute(Tick(1));

            test_3.execute(ExpectOneSegment{}.with_ack(true).with_ackno(isn + 1).with_syn(false),
                           "test 3 failed: bad ACK for SYN");

            test_3.execute(ExpectState{State::ESTABLISHED});

            OK(3);
        }
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
