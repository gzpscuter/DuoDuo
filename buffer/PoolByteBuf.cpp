//
// Created by gzp on 2021/2/13.
//


#include "PoolByteBuf.h"
#include "PoolChunk.h"
#include "PoolByteBufAllocator.h"
#include "PoolThreadCache.h"
#include "PoolArena.h"

namespace buffer {

    PoolByteBuf::PoolByteBuf(int maxCapacity)
    : m_allocator(nullptr), m_maxCapacity(maxCapacity), m_chunk(nullptr), m_handle(0), m_memory(nullptr),m_offset(0),m_length(0),
      m_maxLength(0), m_cache(nullptr),m_tmpBuf(nullptr),m_readerIndex(0),m_writerIndex(0)
    {
        assert(maxCapacity >= 0);
    }

    void PoolByteBuf::init(PoolChunk * chunk, const char * buffer, long handle, int offset, int length, int maxLength, PoolThreadCache * cache) {
        init0(chunk, buffer, handle, offset, length, maxLength, cache);
    }

    void PoolByteBuf::initUnpool(PoolChunk * chunk, int length) {
        init0(chunk, nullptr, 0, chunk->m_offset, length, length, nullptr);
    }

    void PoolByteBuf::init0(PoolChunk * chunk, const char * buffer, long handle, int offset, int length, int maxLength, PoolThreadCache * cache) {
        assert(handle >= 0);
        assert(chunk != nullptr);
        m_chunk = chunk;
        m_memory = chunk->m_memory;
//        m_tmpBuf = const_cast<char*>(buffer);
        m_allocator = chunk->m_arena->m_parent;
        m_cache = cache;
        m_handle = handle;
        m_offset = offset;
        m_length = length;
        m_maxLength = maxLength;
        if(buffer == nullptr)
            buffer = m_memory + m_offset;
        m_tmpBuf = const_cast<char*>(buffer);
    }

//    int PoolByteBuf::capacity() {
//        return m_length;
//    }

    PoolByteBufAllocator * PoolByteBuf::alloc() {
        return m_allocator;
    }

    void PoolByteBuf::deallocate() {
        if(m_handle >= 0) {
            long handle = m_handle;
            m_handle = -1;
            m_memory = nullptr;
            m_chunk->m_arena->free(m_chunk, m_tmpBuf, handle, m_maxLength, m_cache);
            m_tmpBuf = nullptr;
            m_chunk = nullptr;
            recycle();
        }
    }

    void PoolByteBuf::recycle() {
        m_recycler->recycle(this);
    }

//    int PoolByteBuf::idx(int index) {
//        return m_offset + index;
//    }
//
//    int PoolByteBuf::maxCapacity() {
//        return m_maxCapacity;
//    }
//
//    int PoolByteBuf::length() {
//        return m_length;
//    }
//
//    char * PoolByteBuf::memory() {
//        return m_memory;
//    }
//
//    long PoolByteBuf::handle() {
//        return m_handle;
//    }
//
//    int PoolByteBuf::offset() {
//        return m_offset;
//    }
//
//    int PoolByteBuf::maxLength() {
//        return m_maxLength;
//    }
//
//    char * PoolByteBuf::tmpBuf() {
//        return m_tmpBuf;
//    }
//
//    PoolChunk * PoolByteBuf::chunk() {
//        return m_chunk;
//    }
//
//    PoolThreadCache * PoolByteBuf::cache() {
//        return m_cache;
//    }

    char * PoolByteBuf::internalBuffer() {
//        char * tmpBuf = m_tmpBuf;
        if(m_tmpBuf == nullptr) {
            m_tmpBuf = newInternalBuffer(m_memory);
        } else {
//            m_tmpBuf->clear();
//            reuse(m_maxCapacity);
        }
        return m_tmpBuf;
    }

    char * PoolByteBuf::internalBuffer0(int index, int length, bool duplicate) {
        index = idx(index);
//        char * buffer = duplicate? newInternalBuffer(m_memory) : internalBuffer();
//        m_length = length;
        m_tmpBuf =  m_memory + index;
        return m_tmpBuf;
    }

    char * PoolByteBuf::duplicateInternalBuffer(int index, int length) {
        assert(index + length <= m_length);
        return internalBuffer0(index, length, true);
    }

    char * PoolByteBuf::internalBuffer(int index, int length) {
        assert(index + length <= m_length);
        return internalBuffer0(index, length, false);
    }

    void PoolByteBuf::reuse(int maxCapacity) {
        setMaxCapacity(maxCapacity);
        setIndex0(0, 0);
    }

    void PoolByteBuf::setIndex0(int readerIndex, int writerIndex) {
        m_readerIndex = readerIndex;
        m_writerIndex = writerIndex;
    }

    void PoolByteBuf::setMaxCapacity(int maxCapacity) {
        m_maxCapacity = maxCapacity;
    }


    PoolByteBuf * PoolByteBuf::newInstance(int maxCapacity) {
        PoolByteBuf * buf = m_recycler->get();
        buf->reuse(maxCapacity);
        return buf;
    }

//    PoolByteBuf::DefaultPoolByteBuf(Handle<DefaultPoolByteBuf *> * recyclerHandle, int maxCapacity)
//    : PoolByteBuf(recyclerHandle, maxCapacity){
//
//    }

    PoolByteBuf * PoolByteBufRecycler::newObject() {
        return new PoolByteBuf(0);
    }


    Recycler<PoolByteBuf> * PoolByteBuf::m_recycler = new PoolByteBufRecycler();

    char * PoolByteBuf::newInternalBuffer(const char * memory) {
//        std::vector<char> * tmpBuffer = new std::vector<char>(memory->begin(), memory->end());
//        return tmpBuffer;
        return m_memory;
    }

    char PoolByteBuf::_getByte(int index) {
        return m_memory[idx(index)];
    }

    void PoolByteBuf::checkIndexBounds(const int& readerIdx, const int& writerIdx, const int& capacity){
        if(readerIdx < 0 || readerIdx > writerIdx || writerIdx > capacity)
            throw std::exception();
    }

    PoolByteBuf * PoolByteBuf::setReaderIdx(const int& readerIdx) {
        checkIndexBounds(readerIdx, m_writerIndex, capacity());
        m_readerIndex = readerIdx;
        return this;
    }

    int PoolByteBuf::maxWritableBytes() {
        return maxCapacity() - writableBytes();
    }

    void PoolByteBuf::retrieve(size_t len) {
        assert(len <= readableBytes());
        if(len < readableBytes())
        {
            m_readerIndex += len;
        }
        else
        {
            retrieveAll();
        }
    }

    std::string PoolByteBuf::retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string PoolByteBuf::retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        std::string result(peek(), len);
        retrieve(len);
        return result;
    }

    void PoolByteBuf::retrieveAll()
    {
        m_readerIndex = 0;
        m_writerIndex = 0;
    }


    void PoolByteBuf::checkReadableBytes(PoolByteBuf * src, int length) {
        assert(length <= src->readableBytes());
    }

    PoolByteBuf * PoolByteBuf::writeBytes(const char* data, int srcIndex, int length) {
        ensureWritable(length);
        setBytes(m_writerIndex, data, srcIndex, length);
        m_writerIndex += length;
        return this;
    }

    PoolByteBuf * PoolByteBuf::writeBytes(const char* data, int length) {
        writeBytes(data, 0, length);
        return this;
    }

//    PoolByteBuf * PoolByteBuf::writeBytes(PoolByteBuf * src, int length) {
//        checkReadableBytes(src, length);
//        writeBytes(src, src->m_readerIndex, length);
//        src->setReaderIdx(src->m_readerIndex + length);
//        return this;
//    }

//    PoolByteBuf * PoolByteBuf::writeBytes(PoolByteBuf * src, int srcIndex, int length) {
//        ensureWritable(length);
//        setBytes(m_writerIndex, src, srcIndex, length);
//        m_writerIndex += length;
//        return this;
//    }


    PoolByteBuf * PoolByteBuf::setBytes(int index, const char* src, int srcIndex, int length) {
        index = idx(index);
//        char * buffer = duplicate? newInternalBuffer(m_memory) : internalBuffer();
//        m_length = length;
//        m_tmpBuf =  m_memory + index;
        char * toWriteBuf = m_memory + index;
        std::copy(src + srcIndex, src + srcIndex + length, toWriteBuf);
        return this;
    }



    PoolByteBuf * PoolByteBuf::ensureWritable(int minWritableBytes) {
        assert(minWritableBytes >= 0);
        const int targetCapacity = m_writerIndex + minWritableBytes;
        assert(targetCapacity >= 0 && targetCapacity <= maxCapacity());
        if(targetCapacity > capacity()) {
            int writable = writableBytes();
            int newCapacity = writable >= minWritableBytes ? m_writerIndex + writable
                                                           : m_allocator->calculateNewCapacity(targetCapacity, m_maxCapacity);
            capacity(newCapacity);
        }
//        ensureWritable0(minWritableBytes);
        return this;
    }

//    void PoolByteBuf::ensureWritable0(int minWritableBytes) {
//        int writerIndex = m_writerIndex;
//        const int targetCapacity = writerIndex + minWritableBytes;
//        if(targetCapacity >= 0 && targetCapacity <= capacity()) {
//            //ensureAccess for refCnt
//            return;
//        }
//        if(targetCapacity < 0 || targetCapacity > m_maxCapacity) {
//            throw std::exception();
//        }
//        int writable = writableBytes();
//        int newCapacity = writable >= minWritableBytes ? m_writerIndex + writable
//                                                       : m_allocator->calculateNewCapacity(targetCapacity, m_maxCapacity);
//
//        capacity(newCapacity);
//    }

    //确保newCapacity是有效的，且不大于首次分配的maxCapacity
    void PoolByteBuf::checkNewCapacity(int newCapacity) {
        //ensureAccessiabel()
        if(newCapacity < 0 || newCapacity > maxCapacity()) {
            throw std::exception();
        }
    }

    PoolByteBuf * PoolByteBuf::capacity(int newCapacity) {
        if(newCapacity == m_length) {
            return this;
        }
        assert(newCapacity >= 0 && newCapacity <= maxCapacity());
//        checkNewCapacity(newCapacity);
        if(newCapacity > m_length) {
            if(newCapacity <= m_maxLength) {
                m_length = newCapacity;
                return this;
            }
        } else if(newCapacity > (m_maxLength >> 1) && (m_maxLength > 512 || newCapacity > m_maxLength - 16)) {
            m_length = newCapacity;
            trimIndicesToCapacity(newCapacity);
            return this;
        }
        m_chunk->m_arena->reallocate(this, newCapacity, true);
        return this;
    }

    void PoolByteBuf::trimIndicesToCapacity(int newCapacity) {
        if(m_writerIndex > newCapacity) {
            m_readerIndex = std::min(m_readerIndex, newCapacity);
            m_writerIndex = newCapacity;
        }
    }




    PoolByteBuf * PoolByteBuf::clear() {
        m_readerIndex = m_writerIndex = 0;
        return this;
    }

}