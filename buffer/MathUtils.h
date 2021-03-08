//
// Created by zjw on 2021/1/22.
//

#ifndef DUODUO_MATHUTILS_H
#define DUODUO_MATHUTILS_H

namespace buffer{
    class MathUtils {
    public:
        MathUtils() = default;
        static const int DEFAULT_INT_SIZE = 32;
        static const int DEFAULT_INT_SIZE_MINUS_ONE = 31;

        static int numberOfLeadingZeros(int signedVal) {
            unsigned int val = static_cast<unsigned int>(signedVal);
            if (val == 0)
                return DEFAULT_INT_SIZE;
            int n = 1;
            //判断第一个非零值是否高于16位
            if (val >> 16 == 0) { n += 16; val <<= 16; }
            //判断第一个非零值是否高于24位
            if (val >> 24 == 0) { n +=  8; val <<=  8; }
            if (val >> 28 == 0) { n +=  4; val <<=  4; }
            if (val >> 30 == 0) { n +=  2; val <<=  2; }
            n -= (val >> 31);
            return n;
        }

        static int log2(int val) {
            return DEFAULT_INT_SIZE_MINUS_ONE - numberOfLeadingZeros(val);
        }


        static int nextPowerofTwo(int val) {
            val --;
            val |= val >> 1;
            val |= val >> 2;
            val |= val >> 4;
            val |= val >> 8;
            val |= val >> 16;
            val ++;
            return val;
        }
    };

}


#endif //DUODUO_MATHUTILS_H
