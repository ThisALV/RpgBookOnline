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

void Data::add(const bool b) {
    add(static_cast<byte>(b ? 1 : 0));
}

void Data::put(const std::string& str) {
    if (count() + str.length() + sizeof(word) > MAX_LENGTH)
        throw BufferOverflow {};

    putNumeric<word>(str.length());
    for (const std::size_t init { bytes_ }; bytes_ < (str.length() + init); bytes_++)
        buffer_[bytes_] = str[bytes_ - init];
}

void Data::putList(const OptionsList& options) {
    const std::size_t size { options.size() };

    if (size > std::numeric_limits<byte>::max())
        throw BufferOverflow {};

    const std::size_t total {
        std::accumulate(options.cbegin(), options.cend(), std::size_t { 1 }, [](const std::size_t size, const auto& o) {
            return size + 1 + STR_LENGTH_SIZE + o.second.length();
        })
    };

    if (total > MAX_LENGTH)
        throw BufferOverflow {};

    add(static_cast<byte>(size));
    for (const auto& [option, txt] : options) {
        add(option);
        put(txt);
    }
}

const Data& DataFactory::dataWithLength() {
    data_.refreshLength();

    return data_;
}

} // namespace Rbo
