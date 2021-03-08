//
// Created by zjw on 2021/1/22.
//

#include "SizeClass.h"
using namespace std;
using namespace buffer;

namespace buffer {


    SizeClass::SizeClass(int pageSize, int pageShifts, int chunkSize)
            : m_pageSize(pageSize),
              m_pageShifts(pageShifts), //13
              m_chunkSize(chunkSize),
              m_searchMaxSize(0),
              m_nSubpages(0),
              m_nPSizes(0),
              m_smallMaxSizeIdx(0){
        int group = MathUtils::log2(chunkSize) + 1 - LOG2_QUANTUM;

        //group四个一组
        m_sizeClasses.resize(group << LOG2_SIZE_CLASS_GROUP);
        m_nSizes = sizeClasses();

        m_sizeIdx2sizeTab.reset(new int[m_nSizes]);
        m_pageIdx2sizeTab.reset(new int[m_nPSizes]);
        idx2SizeTab(m_sizeIdx2sizeTab.get(), m_pageIdx2sizeTab.get());

        m_size2idxTab.reset(new int[m_searchMaxSize >> LOG2_QUANTUM]);
        size2idxTab(m_size2idxTab.get());
    }

//    SizeClass::~SizeClass() {
//    }

    int SizeClass::sizeClasses() {
        int normalMaxSize = -1;

        int index = 0;
        int size = 0;

        int log2Group = LOG2_QUANTUM;
        int log2Delta = LOG2_QUANTUM;
        int ndeltaLimit = 1 << LOG2_SIZE_CLASS_GROUP;

        int nDelta = 0;

        while (nDelta < ndeltaLimit) {
            size = sizeClass(index++, log2Group, log2Delta, nDelta++);
        }
        log2Group += LOG2_SIZE_CLASS_GROUP;

        //不达到huge的内存块,normal最大size应当等于chunksize
        while (size < m_chunkSize) {
            nDelta = 1;
            while (nDelta <= ndeltaLimit && size < m_chunkSize) {
                size = sizeClass(index++, log2Group, log2Delta, nDelta++);
                normalMaxSize = size;
            }

            log2Delta++;
            log2Group++;
        }
        assert(normalMaxSize == m_chunkSize);

        return index;
    }

    int SizeClass::sizeClass(int index, int log2Group, int log2Delta, int nDelta) {
        unsigned short isMultiPageSize;
        int size = (1 << log2Group) + (1 << log2Delta) * nDelta;
        //组内内存块增量大小大于8192,必定是normal内存
        if (log2Delta >= m_pageShifts) {
            isMultiPageSize = yes;
        } else {
            int pageSize = 1 << m_pageShifts;

            isMultiPageSize = (size & pageSize - 1) == 0 ? yes : no;
        }

        int log2nDelta = nDelta == 0 ? 0 : MathUtils::log2(nDelta);
        //mark每组第3个元素才是yes
        unsigned char mark = (1 << log2nDelta) < nDelta ? yes : no;
        int log2size = log2nDelta + log2Delta == log2Group ? log2Group + 1 : log2Group;
        if (log2size == log2Group)
            mark = yes;

        unsigned short isSubPage = log2size < m_pageShifts + LOG2_SIZE_CLASS_GROUP ? yes : no;

        int log2searchDelta = log2size < LOG2_MAX_SEARCH_SIZE ||
                              log2size == LOG2_MAX_SEARCH_SIZE && mark == no
                              ? log2Delta : no;

        unsigned short *sizeTab = new unsigned short[7]{
                static_cast<unsigned short>(index), static_cast<unsigned short>(log2Group),
                static_cast<unsigned short>(log2Delta),
                static_cast<unsigned short>(nDelta), isMultiPageSize, isSubPage,
                static_cast<unsigned short>(log2searchDelta)
        };

        m_sizeClasses[index].reset(sizeTab);
//    int size = (1 << log2Group) + (nDelta << log2Delta);

        if (sizeTab[PAGESIZE_IDX] == yes)
            m_nPSizes++;
        if (sizeTab[SUBPAGE_IDX] == yes) {
            m_nSubpages++;
            m_smallMaxSizeIdx = index;
        }
        if (sizeTab[LOG2_DELTA_SEARCH_IDX] != no)
            m_searchMaxSize = size;
        return size;
    }

    void SizeClass::idx2SizeTab(int *sizeIdx2sizeTab, int *pageIdx2sizeTab) {
        int pageIdx = 0;
        for (int i = 0; i < m_nSizes; i++) {
            unsigned short *sizeClass = m_sizeClasses[i].get();

            int log2group = sizeClass[LOG2GROUP_IDX];
            int log2delta = sizeClass[LOG2DELTA_IDX];
            int nDelata = sizeClass[NDELTA_IDX];

            int size = (1 << log2group) + (1 << log2delta) * nDelata;
            sizeIdx2sizeTab[i] = size;

            //大于页normal大小的另外
            if (sizeClass[PAGESIZE_IDX] == yes)
                pageIdx2sizeTab[pageIdx++] = size;
        }
    }

    void SizeClass::size2idxTab(int *size2idxTab) {
        int idx = 0;
        int size = 0;

        for (int i = 0; size <= m_searchMaxSize; i++) {
            unsigned short *sizeClass = m_sizeClasses[i].get();

            int log2delta = sizeClass[LOG2DELTA_IDX];
            int times = 1 << log2delta - LOG2_QUANTUM;

            while (size <= m_searchMaxSize && times--) {
                size2idxTab[idx++] = i;
                size = idx + 1 << LOG2_QUANTUM;
            }
        }
    }

    int SizeClass::size2sizeIdx(int size) {
        if (size == 0)
            return 0;
        if (size > m_chunkSize)
            return m_nSizes;
        if (size <= m_searchMaxSize)
            return m_size2idxTab[size - 1 >> LOG2_QUANTUM];

        int x = MathUtils::log2((size << 1) - 1);
        int shift = x < LOG2_QUANTUM + LOG2_SIZE_CLASS_GROUP + 1
                    ? 0 : x - (LOG2_SIZE_CLASS_GROUP + LOG2_QUANTUM);

        int group = shift << LOG2_SIZE_CLASS_GROUP;
        int log2delta = x < LOG2_SIZE_CLASS_GROUP + LOG2_QUANTUM + 1
                        ? LOG2_QUANTUM : x - LOG2_SIZE_CLASS_GROUP - 1;
        int deltaInverseMask = -1 << log2delta;
        int mod = (size - 1 & deltaInverseMask) >> log2delta &
                  (1 << LOG2_SIZE_CLASS_GROUP) - 1;

        return group + mod;
    }

    int SizeClass::sizeIdx2size(int sizeIdx) {
        return m_sizeIdx2sizeTab[sizeIdx];
    }

    int SizeClass::sizeIdx2sizeCompute(int sizeIdx) {
        int group = sizeIdx >> LOG2_SIZE_CLASS_GROUP;
        int mod = sizeIdx & (1 << LOG2_SIZE_CLASS_GROUP) - 1;

        int groupSize = group == 0 ? 0 :
                        1 << LOG2_QUANTUM + LOG2_SIZE_CLASS_GROUP - 1 << group;

        int shift = group == 0 ? 1 : group;
        int logDelta = shift + LOG2_QUANTUM - 1;
        int modSize = mod + 1 << logDelta;

        return groupSize + modSize;
    }

    long SizeClass::pageIdx2size(int pageIdx) {
        return m_pageIdx2sizeTab[pageIdx];
    }

    long SizeClass::pageIdx2sizeCompute(int pageIdx) {
        int group = pageIdx >> LOG2_SIZE_CLASS_GROUP;
        int mod = pageIdx & (1 << LOG2_SIZE_CLASS_GROUP) - 1;

        long groupSize = group == 0 ? 0 :
                         1L << m_pageShifts + LOG2_SIZE_CLASS_GROUP - 1 << group;

        int shift = group == 0 ? 1 : group;
        int log2Delta = shift + m_pageShifts - 1;
        int modSize = mod + 1 << log2Delta;

        return groupSize + modSize;
    }

    int SizeClass::page2pageIdx(int pages) {
        return pages2pageIdxCompute(pages, false);
    }

    int SizeClass::page2pageIdxFloor(int pages) {
        return pages2pageIdxCompute(pages, true);
    }

    int SizeClass::pages2pageIdxCompute(int pages, bool floor) {
        int pageSize = pages << m_pageShifts;
        if (pageSize > m_chunkSize)
            return m_nPSizes;

        //x == log2group + 1
        int x = MathUtils::log2((pageSize << 1) - 1);

        int shift = x < LOG2_SIZE_CLASS_GROUP + m_pageShifts ?
                    0 : x - (LOG2_SIZE_CLASS_GROUP + m_pageShifts);

        int group = shift << LOG2_SIZE_CLASS_GROUP;
        int log2Delta = x < LOG2_SIZE_CLASS_GROUP + m_pageShifts + 1 ?
                        m_pageShifts : x - LOG2_SIZE_CLASS_GROUP - 1;

        int deltaInverseMask = -1 << log2Delta;
        int mod = (pageSize - 1 & deltaInverseMask) >> log2Delta &
                  (1 << LOG2_SIZE_CLASS_GROUP) - 1;

        int pageIdx = group + mod;

        if (floor && m_pageIdx2sizeTab[pageIdx] > pages << m_pageShifts) {
            pageIdx--;
        }

        return pageIdx;
    }

    int SizeClass::normalizeSize(int size) {
        if (size == 0) {
            return m_sizeIdx2sizeTab[0];
        }

        if (size <= m_searchMaxSize) {
            int ret = m_sizeIdx2sizeTab[m_size2idxTab[size - 1 >> LOG2_QUANTUM]];
            assert(ret == normalizeSizeCompute(size));
            return ret;
        }
        return normalizeSizeCompute(size);
    }

//private
    int SizeClass::normalizeSizeCompute(int size) {
        int x = MathUtils::log2((size << 1) - 1);
        int log2Delta = x < LOG2_SIZE_CLASS_GROUP + LOG2_QUANTUM + 1
                        ? LOG2_QUANTUM : x - LOG2_SIZE_CLASS_GROUP - 1;
        int delta = 1 << log2Delta;
        int delta_mask = delta - 1;
        return size + delta_mask & ~delta_mask;
    }

}