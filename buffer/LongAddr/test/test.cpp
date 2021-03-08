//
// Created by gzp on 2021/1/28.
//

#include "LongAddr.h"
#include "DoubleAddr.h"
#include <iostream>
#include <thread>
#include <atomic>
#include<ctime>
using namespace buffer;
using namespace std;

LongAddr nums;
atomic<long> numss;
DoubleAddr dnums;
atomic<float> dnumss;

void addNum() {
    for(int i = 0; i < 1000000; i++) {
        dnums.add(3.5);
    }
}

void addNumss() {
    for(int i = 0; i < 1000000; i++) {
//        numss += 2;
        float ret = dnumss.load();
        dnumss.compare_exchange_strong(ret, ret + 2);
    }
}
int main() {
//    nums.add(100);
    clock_t startTime,endTime;
    startTime = clock();//计时开始



    thread t1(addNum);
    thread t2(addNum);
    thread t3(addNum);
    thread t4(addNum);
    thread t5(addNum);
    thread t6(addNum);
    thread t7(addNum);
    thread t8(addNum);
    thread t9(addNum);
    thread t10(addNum);
    thread t11(addNum);
    thread t12(addNum);
    thread t13(addNum);
    thread t14(addNum);
    thread t15(addNum);
    thread t16(addNum);
    thread t17(addNum);
    thread t18(addNum);
    thread t19(addNum);
    thread t20(addNum);
    thread t21(addNum);
    thread t22(addNum);
    thread t23(addNum);
    thread t24(addNum);
    thread t25(addNum);
    thread t26(addNum);
    thread t27(addNum);
    thread t28(addNum);
    thread t29(addNum);
    thread t30(addNum);
    thread t31(addNum);
    thread t32(addNum);
    thread t33(addNum);
    thread t34(addNum);
    thread t35(addNum);
    thread t36(addNum);
    thread t37(addNum);
    thread t38(addNum);
    thread t39(addNum);
    thread t40(addNum);
    thread t41(addNum);
    thread t42(addNum);
    thread t43(addNum);
    thread t44(addNum);
    thread t45(addNum);
    thread t46(addNum);
    thread t47(addNum);
    thread t48(addNum);
    thread t49(addNum);
    thread t50(addNum);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    t6.join();
    t7.join();
    t8.join();
    t9.join();
    t10.join();
    t11.join();
    t12.join();
    t13.join();
    t14.join();
    t15.join();
    t16.join();
    t17.join();
    t18.join();
    t19.join();
    t20.join();
    t21.join();
    t22.join();
    t23.join();
    t24.join();
    t25.join();
    t26.join();
    t27.join();
    t28.join();
    t29.join();
    t30.join();
    t31.join();
    t32.join();
    t33.join();
    t34.join();
    t35.join();
    t36.join();
    t37.join();
    t38.join();
    t39.join();
    t40.join();
    t41.join();
    t42.join();
    t43.join();
    t44.join();
    t45.join();
    t46.join();
    t47.join();
    t48.join();
    t49.join();
    t50.join();


    endTime = clock();//计时结束
    cout << "The run time of doubleaddr is:" <<(double)(endTime - startTime) / CLOCKS_PER_SEC << "s  nums: " << dnums.sum()<< endl;
}
