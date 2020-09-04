#include <Rbo/Data.hpp>

#include <cassert>

namespace Rbo {

Data::Data() {
    assert(LENGTH_SIZE <= std::numeric_limits<byte>::max());

    buffer_.fill(0);
    bytes_ += LENGTH_SIZE;
}

Data::Data(const std::vector<byte>& data) : Data {} {
    if (data.size() > MAX_LENGTH - LENGTH_SIZE)
        throw BufferOverflow {};

    std::size_t i { 0 };
    for (const byte b : data)
        buffer_[LENGTH_SIZE + i++] = b;

    bytes_ += data.size();
}

bool Data::operator==(const Data& rhs) const {
    return count() == rhs.count() && buffer() == rhs.buffer();
}

void Data::add(const byte b) {
    if (count() + 1 > MAX_LENGTH)
        throw BufferOverflow {};

    buffer_[bytes_++] = b;
}

void Data::put(const std::string& str) {
    if (count() + str.length() + sizeof(word) > MAX_LENGTH)
        throw BufferOverflow {};

    putNumeric<word>(str.length());
    for (const std::size_t init { bytes_ }; bytes_ < (str.length() + init); bytes_++)
        buffer_[bytes_] = str[bytes_ - init];
}

const Data& DataFactory::dataWithLength() {
    data_.refreshLength();

    return data_;
}

} // namespace Rbo
