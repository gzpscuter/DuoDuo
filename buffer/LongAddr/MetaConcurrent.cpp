//
// Created by gzp on 2021/1/27.
//

#include "MetaConcurrent.h"
//wolegedacao!!!! should be wrapped by namespace_brace, "using namespace buffer" is forbidden, or a undefined reference will occur
namespace buffer {
    thread_local int chosenIdx = -1;
}
