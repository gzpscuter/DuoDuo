//
// Created by gzp on 2021/3/5.
//

#ifndef DUODUO_BUFFER_H
#define DUODUO_BUFFER_H

#include <memory>
#include <algorithm>
#include "PoolByteBuf.h"
#include "PoolByteBufAllocator.h"

namespace buffer {
    class Buffer {
        static constexpr char kCRLF[] = "\r\n";

    public:
        Buffer();
        explicit Buffer(int size);
        ~Buffer();

        inline const int readableBytes() const
        {
            return m_internalByteBuf->readableBytes();
        }

        inline const int writableBytes()
        {
            return m_internalByteBuf->writableBytes();
        }

        void swap(Buffer&& buffer)
        {
//            m_internalByteBuf.swap(buffer.m_internalByteBuf);
            std::swap(m_internalByteBuf, buffer.m_internalByteBuf);
        }

        inline const char * peek() const
        {
            return m_internalByteBuf->peek();
        }

        inline char * begin() const
        {
            return m_internalByteBuf->tmpBuf();
        }

        inline const int capacity() const
        {
            return m_internalByteBuf->capacity();
        }

        void retrieve(size_t len)
        {
            m_internalByteBuf->retrieve(len);
        }

        void retrieveAll()
        {
            m_internalByteBuf->retrieveAll();
        }

        std::string retrieveAllAsString()
        {
            return m_internalByteBuf->retrieveAllAsString();
        }

        std::string retrieveAsString(size_t len)
        {
            m_internalByteBuf->retrieveAsString(len);

        }

        Buffer * writeBytes(const char* data, int length) {
            m_internalByteBuf->writeBytes(data, length);
            return this;
        }

        Buffer * writeBytes(const std::string& str ) {
            m_internalByteBuf->writeBytes(str.data(), str.size());
            return this;
        }

        inline char* beginWrite()
        { return begin() + m_internalByteBuf->m_writerIndex; }

        inline const char* beginWrite() const
        { return begin() + m_internalByteBuf->m_writerIndex; }


        const char* findCRLF() const
        {
            const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
            return crlf == beginWrite() ? NULL : crlf;
        }

        const char* findCRLF(const char* start) const
        {
            assert(peek() <= start);
            assert(start <= beginWrite());
            const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
            return crlf == beginWrite() ? NULL : crlf;
        }

        void retrieveUntil(const char* end)
        {
            assert(peek() <= end);
            assert(end <= beginWrite());
            retrieve(end - peek());
        }

        ssize_t readFd(int fd, int* savedErrno);

    private:
//        using poolByteBufPtr = std::unique_ptr<PoolByteBuf>;

        PoolByteBuf* m_internalByteBuf;
    };

}
#endif //DUODUO_BUFFER_H
