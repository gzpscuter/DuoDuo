## <h1 align='center'>高性能原子计数类</h1>

### 1. 环境依赖

* c++ 11
* 64bit linux
* 64位及以下长度的cacheline

### 2. 功能

* 提供Cell类型的结构体，通过扩展Cell结构体至少包含64字节，确保不会触发cacheline伪共享机制

* 定义了提供高性能原子变量操作支持的MetaConcurrent类，该类作用如下：
  * 包含m_base和m_cells两个原子成员变量，m_base用于并发计数不密集的情况，m_cells用于大并发计数场景下，隔离不同slot的线程，极大避免多线程cas锁争用
  * 包含m_nthArray原子变量，用于标识m_cells不同slot的线程数量，结合**轮询算法可以为新线程查找负载最小的slot（也就是线程数最少）**，避免过多线程聚集在一个slot中
  * 包含thread_local回调成员，在每个线程结束后触发回调，将该线程所在slot的线程数量减1
  * 直接在线程开始计数的时候就绑定线程至最小负载的slot，避免重复哈希运算查找slot的开销
  * **便于扩展**，通过复制LongAddr和DoubleAddr类可以构造很多不同类型的高性能原子计数类
* 基于MetaConcurrent构造LongAddr的long类型计数类，具备高并发下原子性的特点，且**性能明显高于atomic<long>的原子变量**
* 基于MetaConcurrent构造DoubleAddr的double类型计数类，提供**c++不支持的针对浮点数的原子算术操作**

### 3. 结果

* 实验测试函数

  ```c++
  LongAddr nums;
  atomic<long> numss;
  DoubleAddr dnums;
  atomic<double> dnumss;
  
  void addNum() {
      for(int i = 0; i < 1000000; i++) {
          dnums.add(2);
      }
  }
  
  void addNumss() {
      for(int i = 0; i < 1000000; i++) {
  //        numss += 2;
          double ret = dnumss.load();
          dnumss.compare_exchange_strong(ret, ret + 2);
      }
  }
  ```

  测试函数测试了LongAddr，atomic<long>，DoubleAddr和atomic<double>四种变量，一共开50个线程，每个线程执行1000000次计数任务，最后打印时间差。这里只展示50线程的情景，其他测试情况也是差不多的，不多展示。其余代码可见test.cpp。

* LongAddr  和 atomic<long>

  | 类型         | 线程数量 | 单线程计数 | 预期结果  | 最终结果  | 用时/s  |
  | :----------- | -------- | ---------- | --------- | --------- | ------- |
  | LongAddr     | 50       | 1000000    | 100000000 | 100000000 | 3.60851 |
  | atomic<long> | 50       | 1000000    | 100000000 | 100000000 | 6.66951 |

* DoubleAddr 和 atomic<double>（**c++不支持浮点原子算数）**

  | 类型           | 线程数量 | 单线程计数 | 预期结果  | 最终结果    | 用时/s  |
  | :------------- | -------- | ---------- | --------- | ----------- | ------- |
  | DoubleAddr     | 50       | 1000000    | 100000000 | 1e+08       | 4.45137 |
  | atomic<double> | 50       | 1000000    | 100000000 | 2.48596e+07 | 9.67852 |

* 线程负载均衡情况

  | slot1 | slot2 | slot3 | slot4 | slot5 | slot6 | slot7 | slot8 |
  | ----- | ----- | ----- | ----- | ----- | ----- | ----- | ----- |
  | 6     | 6     | 6     | 6     | 6     | 6     | 7     | 7     |

  可以看出50线程被负载到不同的slot上，每个slot承接线程数目基本一样，负载均衡功能正常

  



