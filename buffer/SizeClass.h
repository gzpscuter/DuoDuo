//
// Created by zjw on 2021/1/22.
//

#ifndef DUODUO_SIZECLASS_H
#define DUODUO_SIZECLASS_H

#include <vector>
#include <memory>
#include <assert.h>
#include "MathUtils.h"

namespace buffer {

    class SizeClass {


        static const int LOG2_SIZE_CLASS_GROUP = 2;

        static const int LOG2_MAX_SEARCH_SIZE = 12;

        static const int INDEX_IDX = 0;
        static const int LOG2GROUP_IDX = 1;
        static const int LOG2DELTA_IDX = 2;
        static const int NDELTA_IDX = 3;
        static const int PAGESIZE_IDX = 4;
        static const int SUBPAGE_IDX = 5;
        static const int LOG2_DELTA_SEARCH_IDX = 6;

        static const unsigned char no = 0, yes = 1;

        int m_searchMaxSize;

        std::vector<std::unique_ptr<unsigned short[]> > m_sizeClasses;

        std::unique_ptr<int[]> m_pageIdx2sizeTab;

        // lookup table for sizeIdx <= smallMaxSizeIdx
        std::unique_ptr<int[]> m_sizeIdx2sizeTab;

        // lookup table used for size <= lookupMaxclass
        // spacing is 1 << LOG2_QUANTUM, so the size of array is lookupMaxclass >> LOG2_QUANTUM
        std::unique_ptr<int[]> m_size2idxTab;

        int sizeClasses();
        int sizeClass(int index, int log2Group, int log2Delta, int nDelta);
        void idx2SizeTab(int* sizeIdx2sizeTab, int* pageIdx2sizeTab);
        void size2idxTab(int* size2idxTab);
        static int normalizeSizeCompute(int size);
        int pages2pageIdxCompute(int pages, bool floor);

    protected:




    public:
        int m_nSizes;
        //m_nSubpages是subpage型sizeclass，一共38
        int m_nSubpages;
        //m_nPSizes是容量为page整数倍的sizeclass数量，一共40
        int m_nPSizes;

        int m_pageSize;
        int m_pageShifts;
        int m_chunkSize;
        //m_smallmaxSizeIdx是subpage的最大值，这里是38
        int m_smallMaxSizeIdx;
        SizeClass(int pageSize, int pageShifts, int chunkSize);
        ~SizeClass() = default;

        int size2sizeIdx(int size);
        int sizeIdx2size(int sizeIdx);
        int sizeIdx2sizeCompute(int sizeIdx);
        long pageIdx2size(int pageIdx);
        long pageIdx2sizeCompute(int pageIdx);
        int page2pageIdx(int pages);
        int page2pageIdxFloor(int pages);
        int normalizeSize(int size);

        static const int LOG2_QUANTUM = 4;


    };
}



#endif //DUODUO_SIZECLASS_H
