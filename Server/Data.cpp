#include "Data.hpp"

#include <cassert>

namespace Rbo {

Data::Data() {
    assert(LENGTH_SIZE <= std::numeric_limits<byte>::max());

    buffer_.fill(0);
    bytes_ += LENGTH_SIZE;
}

Data::Data(const std::vector<byte>& data) : Data {} {
    assert(data.size() <= MAX_LENGTH - LENGTH_SIZE);

    std::size_t i { 0 };
    for (const byte b : data)
        buffer_[LENGTH_SIZE + i++] = b;

    bytes_ += data.size();
}

bool Data::operator==(const Data& rhs) const {
    return count() == rhs.count() && buffer() == rhs.buffer();
}

void Data::add(const byte b) {
    assert(count() + 1 <= MAX_LENGTH);

    buffer_[bytes_++] = b;
}

void Data::put(const std::string& str) {
    assert(count() + str.length() + 1 <= MAX_LENGTH);

    for (const std::size_t init { bytes_ }; bytes_ < (str.length() + init); bytes_++)
        buffer_[bytes_] = str[bytes_ - init];

    add(0);
}

const Data& DataFactory::dataWithLength() {
    data_.refreshLength();

    return data_;
}

} // namespace Rbo
