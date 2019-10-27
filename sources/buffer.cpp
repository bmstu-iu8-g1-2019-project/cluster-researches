// Copyright 2019 AndreevSemen semen.andreev00@mail.ru

#include <buffer.hpp>

ByteBuffer::ByteBuffer(std::size_t size)
  : buffer_{new byte[size]}
  , size_{size}
{}

ByteBuffer::~ByteBuffer() {
    delete[] buffer_;
}

const byte* ByteBuffer::Begin() const {
    return buffer_;
}

byte* ByteBuffer::Begin() {
    return const_cast<byte*>(const_cast<const ByteBuffer*>(this)->Begin());
}

const byte* ByteBuffer::End() const {
    return buffer_ + size_;
}

byte* ByteBuffer::End() {
    return const_cast<byte*>(const_cast<const ByteBuffer*>(this)->End());
}

std::size_t ByteBuffer::Size() const {
    return size_;
}
