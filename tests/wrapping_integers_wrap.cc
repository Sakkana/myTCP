#include "test_should_be.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main() {
    try {
        // 绝对序号 = 3 * 1 << 32, ISN = 0 
        //         ---> seqno = (绝对序号 + ISN) % (1<<32) 
        //         ---> seqno = 0 + 0 = 0
        test_should_be(wrap(3 * (1ll << 32), WrappingInt32(0)), WrappingInt32(0));

        // 绝对序号 = 3 * 1 << 32 + 17, ISN = 15 
        //         ---> seqno = (绝对序号 + ISN) % (1<<32)
        //         ---> seqno = 17 + 15 = 32 
        test_should_be(wrap(3 * (1ll << 32) + 17, WrappingInt32(15)), WrappingInt32(32));

        // 绝对序号 = 7 * 1 << 32 - 2, ISN = 15 
        //         ---> seqno = (绝对序号 + ISN) % (1<<32) 
        //         ---> seqno = (8 * 1 << 32 + 13) % (1<<32)
        //         ---> seqno = 13
        test_should_be(wrap(7 * (1ll << 32) - 2, WrappingInt32(15)), WrappingInt32(13));
    } catch (const exception &e) {
        cerr << e.what() << endl;
        return 1;
    }

    return EXIT_SUCCESS;
}
