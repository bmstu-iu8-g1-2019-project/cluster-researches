// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#ifndef HEADERS_BUFFER_HPP_
#define HEADERS_BUFFER_HPP_

#include <cstdint>

using byte = uint8_t;

class ByteBuffer {
private:
    byte* buffer_;
    std::size_t size_;

public:
    explicit ByteBuffer(std::size_t size);
    ~ByteBuffer();

    const byte* Begin() const;
    byte* Begin();

    const byte* End() const;
    byte* End();

    std::size_t Size() const;
};

#endif // HEADERS_BUFFER_HPP_
