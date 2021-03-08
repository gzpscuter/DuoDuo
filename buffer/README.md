### <h3 align='center'>jemalloc4原理的内存池</h3>

#### 1.环境依赖

* c++ 11
* 64bit linux

#### 2.组件

##### 2.1 PoolArena

* 负责内存分配和内存回收。
* m_smallSubpagePools包含小于28KB的内存分配区域，不同元素根据SizeClass的m_sizeClasses表格储存不同规格的内存块。
* 包含m_smallSubpagePools数组和不同内存使用率的PoolChunkList，m_smallSubpagePools长度是chunkSize / pageSize，
* 为申请内存的线程分配内存的主要组件，如果申请内存小于28KB，则按照page分配，超过则按照run分配，一个run包含多个page。
* 小内存释放后，如果小于40960B，则由thread_local类型的PoolThreadCache对象缓存，PoolThreadCache对象包含一条Entry队列，Entry存放chunk，handle，buffer信息，指示PoolChunk的一块区域。如果是大内存释放，则根据PoolChunk的内存使用率存入对应内存使用率区间的PoolChunkList，从而在下次分配时可以根据不同内存使用率的PoolChunkList查找对应PoolChunk进行分配。

##### 2.2 PoolThreadCache

* 包含small缓存数组和normal缓存数组，small和normal缓存继承MemoryRegionCache，对初始化buffer由不同实现。

* 缓存小内存，当缓存发生时，从Entry回收站生成或取得Entry结构体保存PoolChunk的内存信息，保存进m_queue。
* 进行分配时，弹出m_queue元素，并将Entry放回回收站用于下次缓存其他内存，Entry回收站可重复利用分配出去的Entry，减少下次缓存其他内存需要new Entry的开销。

##### 2.3 PoolChunk

* 一块PoolChunk大小时16MB，包含有BestFitQueue组成的数组m_runsAvail，
* 申请内存小于28KB，则按照subpage分配，根据申请内存大小查找PoolArena的m_smallSubpagePools数组，得到足够分配的PoolSubpage对象池
* PoolChunk分配内存时，如果小于28KB，则分配pageSize和elemSize最小公倍数的内存runSize，这样能保证在该大小的run上能正好分配runSize/elemSize个PoolSubpage内存单元。如果大于28KB，则根据分配内存大小获取对应的BestFitQueue，然后弹出offset最小的元素，根据需要的pages数切分run，将多余的pages放回m_runsAvail和m_runAvailMap，用于下次分配。
* PoolChunk释放时，先释放PoolSubpage，然后释放run，释放run的时候会查找该run前后是否有可以合并的空闲run，与之一起合并成更大的run再释放。

##### 2.4 PoolSubpage

* m_bitmap用long的二进制表示记录page所有可分配的内存单元，由于SizeClass表格最小分配内存是16B（QUANTUM），所以m_bitmap长度最多为8192/16/64 = 8。初次创建PoolSubpage时，会将PoolSubpage加入PoolSubpage池作为头元素。
* PoolChunk分配subpage内存时，根据申请内存大小对应的分配大小，将page分成ceil(pageSize/elemSize)份，进行分配时获取m_nextAvail，分配完成设置m_nextAvail为-1，这样下次分配就通过findNextAvail查找下一个空闲内存块，空闲内存块表现为m_bitmap数组的该元素二进制存在0。当一个subpage分配完全，就会从subpage池中移除。
* PoolSubpage释放时，会根据其释放区域的bitmap将对应m_bitmap对应元素对应位置为0，如果该PoolSubpage不处于PoolSubpage池，就将其加入。

##### 2.5 PoolByteBufAllocator

* 维护m_arena的PoolArena数组，该数组初始化时设定为cpu虚拟核数目长度
* 提供静态函数ALLOCATOR，工作与单例模式，提供用于分配内存池缓冲PoolByteBuf的功能。对于每个线程都分配一个thread_local的m_poolThreadCache，并通过负载均衡将该线程绑定到分配线程数目最少的PoolArena。

##### 2.6 PoolByteBuf

* 包含m_writerIndex和m_readerIndex，写数据时移动m_writerIndex，读时移动m_readerIndex

  ```c++
  /// +-------------------+------------------+------------------+
  /// | discardable bytes |  readable bytes  |  writable bytes  |
  /// |                   |     (CONTENT)    |                  |
  /// +-------------------+------------------+------------------+
  /// |                   |                  |                  |
  /// 0      <=      m_readerIndex   <=   m_writerIndex    <=   capacity
  ```

  

##### 2.6 Buffer

* 主要对外工作的类，用于申请一定大小的内存池缓冲，并提供相关接口进行Buffer的读写。

#### 3. 实验结果

* 单线程进行分配和释放4000000次，总共用时5.61702秒；4线程分配释放4000000次，总共用时2.52292。多线程分配释放显著高于单线程，证明在多线程下效果更佳。

  | 线程数 | 总分配释放数 | 用时    |
  | ------ | ------------ | ------- |
  | 1      | 4000000      | 5.61702 |
  | 4      | 4000000      | 2.52292 |

* 线程对PoolArena负载均衡结果，下面左侧时线程地址，右侧时线程绑定的PoolArena地址，可见在虚拟核心为8的时候，八个不同线程分别绑定到不同的PoolArena，实现了负载均衡。

  | 线程地址       | PoolArena地址  |
  | -------------- | -------------- |
  | 0x7f431c017750 | 0x7f431c000bd0 |
  | 0x7f430c000b60 | 0x7f431c0066b0 |
  | 0x7f4310000b60 | 0x7f431c009420 |
  | 0x7f4304000b60 | 0x7f431c00c190 |
  | 0x7f4314000b60 | 0x7f431c00ef00 |
  | 0x7f4300000b60 | 0x7f431c00ef00 |
  | 0x7f42f8000b60 | 0x7f431c011c70 |
  | 0x7f42f0000b60 | 0x7f431c0149e0 |

* 单线程下，进行1000000次1-32768字节内存随机分配并释放，与vector和new/delete对比，可见该内存池分配效率显著优于vector，接近于new/delete操作。

  | 类型                      | 分配次数 | 分配字节数（随机） | 用时     |
  | ------------------------- | -------- | ------------------ | -------- |
  | PoolByteBuf（内存池缓冲） | 1000000  | 1-32768            | 1.68461  |
  | vector                    | 1000000  | 1-32768            | 4.26066  |
  | new/delete                | 1000000  | 1-32768            | 0.930197 |

  