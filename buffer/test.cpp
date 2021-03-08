//
// Created by gzp on 2021/2/11.
//

#include <iostream>
#include <algorithm>
#include <string.h>
#include <chrono>
#include <cstdlib> // Header file needed to use srand and rand
#include <ctime> // Header file needed to use time
#include <pthread.h>
#include<ctime>
#include "PoolByteBuf.h"
#include "PoolByteBufAllocator.h"

using namespace std;
using namespace buffer;

char* toWriteSent = new char[81920];

void testFunc() {
    unsigned seed;  // Random generator seed
    // Use the time function to get a "seed” value for srand
    seed = time(0);
    srand(seed);
    PoolByteBufAllocator* tt = PoolByteBufAllocator::ALLOCATOR();
    for(int i = 0; i < 50000; i++) {
        int size = rand() % 32768;
//        cout << "allocation size "<< size << endl;
        PoolByteBuf * toTest = tt->poolBuffer(size);
        toTest->writeBytes(toWriteSent, size );
        toTest->deallocate();
    }
}

int main(int argc, char * argv[]) {

//    PoolByteBuf * tmp = tt.poolBuffer(25);
//    PoolByteBuf * tmp1 = tt.poolBuffer(20);
//    const char* firstSentence = "this is a test!";
//    const char* secondSentence = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx55xxxx.com";
//    const char* thirdSentence = "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOYYYYYYYYYYYYYYYYY.com";

    unsigned seed;
    seed = time(0);
    srand(seed);

    char* forthSentence = new char[8192];
    fill_n(forthSentence, 8192, '9');
    auto start = std::chrono::high_resolution_clock::now();
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tm;

    vector<pthread_t> threadArray;
    fill_n(toWriteSent, 65536, 'X');

    for(int i = 0; i < 4; i++) {
        pthread_t th;
        int ret = pthread_create(&th, nullptr, reinterpret_cast<void *(*)(void *)>(testFunc), nullptr );
        if(ret != 0) {

            perror("create thread fail");
            exit(1);
        }
        threadArray.push_back(th);
    }

    start = std::chrono::high_resolution_clock::now();//计时开始
    for(auto& thread : threadArray) {
        pthread_join(thread, nullptr);
    }

    end = std::chrono::high_resolution_clock::now();//计时结束
    tm = end - start;
    cout << "The run time of PoolByteBUf-multiTHread is:" << tm.count() <<  endl;

    start = std::chrono::high_resolution_clock::now();//计时开始
//    PoolByteBuf * toTest = tt.poolBuffer(16384);

    for(int i = 1; i < 50000*4; i++) {
//        PoolByteBuf * toTest = tt.poolBuffer(i);
//        toTest->writeBytes(forthSentence,  i);
//        toTest->deallocate();
        int size = rand() % 32768;
        vector<char> kk( size );
        std::copy(toWriteSent, toWriteSent +  size , kk.begin());
        kk.clear();
    }
    end = std::chrono::high_resolution_clock::now();//计时结束
    tm = end - start;
    cout << "The run time of vector is:" << tm.count() <<  endl;

    start = std::chrono::high_resolution_clock::now();//计时开始
    for(int i = 0; i < 50000*4 ; i++) {
        int size = rand() % 32768;
        char* test2 = new char[size];
        std::copy(toWriteSent, toWriteSent + size, test2);
        delete[] test2;
    }
    end = std::chrono::high_resolution_clock::now();//计时结束
    tm = end - start;
    cout << "The run time of newChar is:" <<tm.count() <<  endl;

    for(int i = 0; i < 200000; i++) {
        PoolByteBuf* testSequence =  PoolByteBufAllocator::ALLOCATOR()->poolBuffer(2048);
        testSequence->writeBytes(toWriteSent, 128);
        cout <<"peek " << testSequence->peek() <<" tmpBuf "<< testSequence->tmpBuf() <<" readableBytes " <<testSequence->readableBytes()
        <<" offset "<< testSequence->offset()<<endl;
    }

////    cout << "to deallocate "<<endl;
//    tmp->deallocate();
//    tmp1->writeBytes(secondSentence, strlen(secondSentence));
//    tmp1->deallocate();
//    PoolByteBuf * tttmp = tt.poolBuffer(8192*5);
//    tttmp->writeBytes(thirdSentence, strlen(thirdSentence));
////    tmp1->writeBytes(thirdSentence, strlen(thirdSentence));
//
//    cout <<tmp->maxWritableBytes() << endl;
//    cout << tmp->writableBytes() << endl;
////    std::vector<char>* kk = tmp->internalBuffer(0, 128);
////    for(auto& item : *kk) {
////        item = '8';
////    }
////    for(auto& item : *kk) {
////        cout << item << endl;
////    }
////    PoolByteBuf * jj = new PoolByteBuf(0);
}