#ifndef DATA_HPP
#define DATA_HPP

#include "Common.hpp"

namespace Rbo {

const std::size_t MAX_LENGTH { 1300 };
const std::size_t LENGTH_SIZE { sizeof(word) };

using DataBuffer = std::array<byte, MAX_LENGTH>;

struct BufferOverflow : std::logic_error {
    BufferOverflow() : std::logic_error {
                           "Buffer dépassant " + std::to_string(MAX_LENGTH) + " octets" } {}
};

class Data {
private:
    DataBuffer buffer_;
    std::size_t bytes_ { 0 };

    template<typename NumType>
    void putNumeric(const NumType value, const std::size_t offset, const bool refresh = true) {
        if (count() + sizeof(NumType) > MAX_LENGTH)
            throw BufferOverflow {};

        std::size_t i { 0 };
        for (const byte b : decompose(value))
            buffer_[offset + i++] = b;

        if (refresh)
            bytes_ += sizeof(NumType);
    }

public:
    Data();
    Data(const std::vector<byte>&);

    std::size_t count() const { return bytes_; }
    const DataBuffer& buffer() const { return buffer_; };

    bool operator==(const Data&) const;

    void add(const byte);
    template<typename Enum> void add(const Enum e) { add(static_cast<byte>(e)); }

    void put(const std::string&);
    template<typename NumType> void putNumeric(const NumType value) { putNumeric(value, count()); }

    void refreshLength() { putNumeric<word>(static_cast<word>(count()), 0, false); }
};

class DataFactory {
protected:
    Data data_;

public:
    const Data& data() const { return data_; }
    const Data& dataWithLength();
};

} // namespace Rbo

#endif // DATA_HPP
