#include "ninmagic_http_policy.hpp"
#include <cassert>
#include <iostream>

int main() {
    using namespace ninmagic::http;
    assert(AcceptRange(200, 0, {}));
    assert(!AcceptRange(200, 1, {}));
    assert(!AcceptRange(200, 4294967296LL, {}));
    assert(!AcceptRange(206, 0, {}));
    assert(AcceptRange(206, 2147483648LL, ParseContentRange("bytes 2147483648-2147483651/15032385536")));
    assert(AcceptRange(206, 4294967296LL, ParseContentRange("bytes 4294967296-15032385535/15032385536")));
    assert(!AcceptRange(206, 4294967296LL, ParseContentRange("bytes 0-99/15032385536")));
    assert(!AcceptRange(302, 0, {}));
    assert(!AcceptRange(416, 0, {}));
    assert(!AcceptRange(200, -1, {}));
    for (const auto value : {"", "bytes 1-0/2", "bytes 0-2/2", "bytes 0-0/0", "bytes 0-1/*",
        "bytes -1-2/3", "bytes 0-1/18446744073709551616", "bytes 0-1/9223372036854775808",
        "bytes 0-1/2junk", "bytes 0 -1/2", "bytes 0-1/2/3", "items 0-1/2"}) assert(!ParseContentRange(value));
    std::cout << "NinMagicFoil HTTP range policy: passed\n";
}
