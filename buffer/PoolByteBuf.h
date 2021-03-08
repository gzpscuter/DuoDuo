//
// Created by gzp on 2021/2/13.
//

#ifndef LONGADDR_POOLBYTEBUF_H
#define LONGADDR_POOLBYTEBUF_H

#include <vector>
#include "commons.h"
#include "Recycler.h"


namespace buffer {

    class PoolByteBufAllocator;
    class PoolChunk;
    class PoolThreadCache;
    class PoolArena;
    class PoolByteBufRecycler;


    class PoolByteBuf {

        void init0(PoolChunk *chunk, const char * buffer, long handle, int offset, int length, int maxLength,
                   PoolThreadCache *cache);
        void setIndex0(int readerIndex, int writerIndex);

//        Handle<PoolByteBuf *> *m_recyclerHandle;
        PoolByteBufAllocator *m_allocator;
        int m_maxCapacity;
        // m_recycler是PoolByteBuf的回收站，第一次创建PoolByteBuf时需要New一片区域，后面释放该PoolByteBuf后会进入回收站，
        // 当再次申请PoolByteBuf时会从该回收站直接返回，避免再次new一片新区域，减少new系统调用次数
        static Recycler<PoolByteBuf> * m_recycler;

    protected:
        //m_chunk指向申请该PoolByteBuf的chunk，用于该PoolByteBuf的内存分配
        PoolChunk *m_chunk;
        long m_handle;
        //m_memory指向m_chunk的m_memory成员，也就是m_chunk全部空间，16mb
        char *m_memory;
        //m_offset是m_tmpBuf在m_memory指针后面的距离，是PoolByteBuf可读写区域的最开始，writerIndex和readerIndex都是指m_tmpBuf为0时候后面的索引
        int m_offset;
        //m_length是写入m_tmpBuf的有效字节数
        int m_length;
        //m_maxLength是PoolByteBuf分配到的内存单元，也是PoolByteBuf(num)中num所申请的字节数
        int m_maxLength;
        PoolThreadCache *m_cache;
        char *m_tmpBuf;

        char *internalBuffer();

        char * newInternalBuffer(const char *memory);

//        void deallocate();

        inline int idx(int index) {
            return m_offset + index;
        }

        char *internalBuffer0(int index, int length, bool duplicate);

        void reuse(int maxCapacity);
        inline void setMaxCapacity(int maxCapacity);

        void checkIndexBounds(const int& readerIdx, const int& writerIdx, const int& capacity);
        void checkNewCapacity(int newCapacity);
        void checkReadableBytes(PoolByteBuf * src, int length);

    public:
        int m_readerIndex;
        int m_writerIndex;

        explicit PoolByteBuf(int maxCapacity);
        ~PoolByteBuf() {
            deallocate();
        };

        void init(PoolChunk *chunk, const char *buffer, long handle, int offset, int length, int maxLength,
                  PoolThreadCache *cache);

        void initUnpool(PoolChunk *chunk, int length);

        inline const int capacity() const{
            return m_length;
        }

        inline PoolByteBufAllocator *alloc();

        void recycle();

        char *duplicateInternalBuffer(int index, int length);

        char *internalBuffer(int index, int length);

        inline int maxCapacity() {
            return m_maxCapacity;
        }

        inline int length() {
            return m_length;
        }

        inline char * memory() {
            return m_memory;
        }

        inline long handle() {
            return m_handle;
        }

        inline int offset() {
            return m_offset;
        }

        inline int maxLength() {
            return m_maxLength;
        }

        inline char * tmpBuf() {
            return m_tmpBuf;
        }

        inline PoolChunk * chunk() {
            return m_chunk;
        }

        inline PoolThreadCache * cache() {
            return m_cache;
        }

        static PoolByteBuf * newInstance(int maxCapacity);

        char _getByte(int index);

        PoolByteBuf * setReaderIdx(const int& readerIdx);

        inline const int readableBytes() const{
            return m_writerIndex - m_readerIndex;
        }

        inline const int writableBytes() const {
            return capacity() - m_writerIndex;
        }

        inline const char * peek() const {
            return m_tmpBuf + m_readerIndex;
        }

        int maxWritableBytes();

        void retrieve(size_t len);

        void retrieveAll();

        std::string retrieveAllAsString();

        std::string retrieveAsString(size_t len);


//        PoolByteBuf * writeBytes(const char* data, int srcIndex, int length);
        PoolByteBuf * writeBytes(const char* data, int srcIndex, int length);
        PoolByteBuf * writeBytes(const char* data, int length) ;
//        PoolByteBuf * writeBytes(const PoolByteBuf* byteBuf);
//        PoolByteBuf * writeBytes(PoolByteBuf * src, int length);
//        PoolByteBuf * writeBytes(PoolByteBuf * src, int srcIndex, int length);


        PoolByteBuf * ensureWritable(int minWritableBytes);
//        void ensureWritable0(int minWritableBytes);
        PoolByteBuf * capacity(int newCapacity);

        PoolByteBuf * setBytes(int index, const char* src, int srcIndex, int length);
        PoolByteBuf * clear();
        void deallocate();
        void trimIndicesToCapacity(int newCapacity);


    };

    class PoolByteBufRecycler : public Recycler<PoolByteBuf> {
        PoolByteBuf * newObject() override;
    };

}
#endif //LONGADDR_POOLBYTEBUF_H
