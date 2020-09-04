#define BOOST_TEST_MODULE Data

#include "boost/test/unit_test.hpp"
#include "boost/test/data/test_case.hpp"
#include "Rbo/Data.hpp"

using namespace Rbo; // Common.hpp nécessite d'include Player.hpp

namespace dataset = boost::unit_test::data;

BOOST_TEST_DONT_PRINT_LOG_VALUE(DataBuffer)

BOOST_AUTO_TEST_SUITE(Ctor)

BOOST_AUTO_TEST_CASE(Default) {
    const DataBuffer expected_buffer { 0 };

    const Data data;

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE);
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(BytesVector) {
    const DataBuffer expected_buffer { 0, 0, 33, 57, 128, 255, 124, 0 };
    const std::vector<byte> arg { 33, 57, 128, 255, 124, 0 };

    const Data data { arg };

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE + arg.size());
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(Overflow) {
    std::vector<byte> arg;
    arg.resize(10000, 0);

    BOOST_CHECK_THROW(Data { arg }, BufferOverflow);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Add)

BOOST_DATA_TEST_CASE(Byte, dataset::xrange(25))
{
    const byte value { static_cast<byte>(sample) };
    const DataBuffer expected_buffer { 0, 0, value };

    Data data;
    data.add(value);

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE + 1);
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(Overflow) {
    Data data;

    bool thrown { false };
    try {
        for (int i { 0 }; i < 10000000; i++)
            data.add(0);
    } catch (const BufferOverflow&) {
        thrown = true;
    }

    BOOST_CHECK(thrown);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Put)

BOOST_AUTO_TEST_CASE(EmptyString) {
    const DataBuffer expected_buffer { 0, 0, 0, 0 };
    const std::string arg { "" };

    Data data;
    data.put(arg);

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE + arg.size() + 2);
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(String) {
    const DataBuffer expected_buffer {
        0, 0, 0, 12, 'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd', '!'
    };
    const std::string arg { "Hello world!" };

    Data data;
    data.put(arg);

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE + arg.size() + 2);
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(StringOverflow) {
    const std::string big_str(10000, ' '); // Syntaxe C++14 pour éviter l'initializer_list constructor

    Data data;
    BOOST_CHECK_THROW(data.put(big_str), BufferOverflow);
}

BOOST_AUTO_TEST_CASE(Int16) {
    const DataBuffer expected_buffer { 0, 0, 0x89, 0xA0 };
    const short arg { -30304 };

    Data data;
    data.putNumeric(arg);

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE + sizeof(short));
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(UInt32) {
    const DataBuffer expected_buffer { 0, 0, 0x00, 0x01, 0x11, 0x70 };
    const uint arg { 70000 };

    Data data;
    data.putNumeric(arg);

    BOOST_CHECK_EQUAL(data.count(), LENGTH_SIZE + sizeof(uint));
    BOOST_CHECK_EQUAL(data.buffer(), expected_buffer);
}

BOOST_AUTO_TEST_CASE(NumericOverflow, *boost::unit_test::depends_on { "Add/Byte" }) {
    Data data;
    for (std::size_t i { 0 }; i < MAX_LENGTH - 3; i++)
        data.add(0);

    BOOST_CHECK_THROW(data.putNumeric<int>(9999999), BufferOverflow);
}

BOOST_AUTO_TEST_SUITE_END()
