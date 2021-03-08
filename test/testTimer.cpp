//
// Created by gzp on 2021/3/7.
//

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include <future>
#include <pthread.h>
#include "../TcpServer.h"
#include "../EventLoop.h"
#include "../InetAddress.h"
#include "../buffer/Buffer.h"
#include "../TcpConnection.h"

int numThreads = 0;
using namespace duoduo;
using namespace std;
using namespace std::placeholders;

void printTime(std::promise<int>* promiseForInsert) {
    time_entry<> tmp;
    tmp.getTimeOfDay();
    cout <<"threadId: "<< std::this_thread::get_id() <<" "<< tmp.toString() << endl;
//    auto f = promiseForInsert.get_future();
    promiseForInsert->set_value(1);
}

void insertTime(EventLoop& loop) {

    while (true){
        std::promise<int> promiseFor;
        auto bf = bind(printTime, &promiseFor);

        loop.runAfter(time_entry<>({0, 2000000}), bf);
        auto f = promiseFor.get_future();
        (void)f.get();
//        promiseForInsert.set_value(0);
    }
}

int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        numThreads = atoi(argv[1]);
    }
    numThreads = 4;
    bool ipv6 = argc > 2;
    EventLoop loop;
    InetAddress listenAddr(2000, false, false);
    TcpServer server(&loop, listenAddr, "EchoServer");
//    EchoServer server(&loop, listenAddr);

    pthread_t th_a, th_b;
    /* 创建生产者和消费者线程*/
    pthread_create(&th_a, NULL, reinterpret_cast<void *(*)(void *)>(insertTime), &loop);
    pthread_create(&th_a, NULL, reinterpret_cast<void *(*)(void *)>(insertTime), &loop);
    /* 等待两个线程结束*/
    pthread_join(th_a, nullptr);
    pthread_join(th_b, nullptr);

//    server.start();
//    loop.runAfter(time_entry<>({0, 200000}), printTime);
    loop.loop();
//    insertTime(loop);

}