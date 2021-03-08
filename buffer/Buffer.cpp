//
// Created by gzp on 2021/3/5.
//

#include <sys/uio.h>
#include <netinet/in.h>
#include "Buffer.h"


namespace buffer {

    Buffer::Buffer()
    :m_internalByteBuf(nullptr)
    {
    }

    Buffer::Buffer(int size)
    {
        assert(size >= 0);
        m_internalByteBuf = PoolByteBufAllocator::ALLOCATOR()->poolBuffer(size);
//        m_internalByteBuf.reset(PoolByteBufAllocator::ALLOCATOR()->poolBuffer(size));
    }

    Buffer::~Buffer()
    {
//        assert(m_internalByteBuf.get() != nullptr);
//        m_internalByteBuf.reset();
        if(m_internalByteBuf != nullptr)
            m_internalByteBuf->deallocate();
    }

    ssize_t Buffer::readFd(int fd, int* savedErrno)
    {
        // saved an ioctl()/FIONREAD call to tell how much to read
        char extrabuf[65536];
        struct iovec vec[2];
        const size_t writable = writableBytes();
        vec[0].iov_base = begin() + m_internalByteBuf->m_writerIndex;
        vec[0].iov_len = writable;
        vec[1].iov_base = extrabuf;
        vec[1].iov_len = sizeof extrabuf;
        // when there is enough space in this buffer, don't read into extrabuf.
        // when extrabuf is used, we read 128k-1 bytes at most.
        const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
        const ssize_t n = ::readv(fd, vec, iovcnt);
        if (n < 0)
        {
            *savedErrno = errno;
        }
        else if (static_cast<size_t>(n) <= writable)
        {
            m_internalByteBuf->m_writerIndex += n;
        }
        else
        {
            m_internalByteBuf->m_writerIndex = capacity();
            writeBytes(extrabuf, n - writable);
        }

        return n;
    }

}