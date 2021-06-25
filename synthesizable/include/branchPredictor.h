#ifndef INCLUDE_BRANCHPREDICTOR_H_
#define INCLUDE_BRANCHPREDICTOR_H_

#define DEBUG 1

#include "logarithm.h"
#include "portability.h"

#ifdef DEBUG
#include <assert.h>
#include <iostream>
#include <queue>
#endif

template <class T> class BranchPredictorWrapper {
#ifdef DEBUG
  int missPredictions = 0;
  int processCount    = 0;
  int updateCount     = 0;
  bool predictions[2];
  int front = 0, back = 0;
#endif

public:
  void update(CORE_UINT(32) pc, bool isBranch)
  {
    static_cast<T*>(this)->_update(pc, isBranch);
#ifdef DEBUG
    updateCount++;
    assert(updateCount + 1 >= processCount);
    bool pred = predictions[front];
    front ^= 1;
    missPredictions += isBranch == pred ? 0 : 1;
    std::cout << "pc: " << pc << "\n"
              << "branch: " << isBranch << "\n"
              << "predict: " << pred << "\n"
              << "miss rate: " << (float)missPredictions / updateCount << "\n";

#endif
  }

  void process(CORE_UINT(32) pc, bool& isBranch)
  {
    static_cast<T*>(this)->_process(pc, isBranch);
#ifdef DEBUG
    processCount++;
    predictions[back] = isBranch;
    back ^= 1;
#endif
  }

  void undo()
  {
#ifdef DEBUG
    if (processCount > updateCount) {
      processCount--;
      front ^= 1;
    }
#endif
  }
};

template <int BITS, int ENTRIES>
class BitBranchPredictor : public BranchPredictorWrapper<BitBranchPredictor<BITS, ENTRIES> > {
  static const int LOG_ENTRIES = log2const<ENTRIES>::value;
  static const int NT_START    = (1 << BITS) - 1;
  static const int NT_FINAL    = (1 << BITS) >> 1;
  static const int T_START     = 0;
  static const int T_FINAL     = NT_FINAL - 1;

  CORE_UINT(BITS) table[ENTRIES];

public:
  BitBranchPredictor()
  {
    for (int i = 0; i < ENTRIES; i++) {
      table[i] = T_START;
    }
  }

  void _update(CORE_UINT(32) pc, bool isBranch)
  {
    CORE_UINT(LOG_ENTRIES) index = pc.SLC(LOG_ENTRIES, 2);

    if (isBranch) {
      table[index] -= table[index] != T_START ? 1 : 0;
    } else {
      table[index] += table[index] != NT_START ? 1 : 0;
    }
  }

  void _process(CORE_UINT(32) pc, bool& isBranch)
  {
    CORE_UINT(LOG_ENTRIES) index = pc.SLC(LOG_ENTRIES, 2);
    isBranch                         = table[index] <= T_FINAL;
  }
};

template<int SIZE, int BITS, int ENTRIES, int THRESHOLD, int LR>
class PerceptronBranchPredictor : public BranchPredictorWrapper<PerceptronBranchPredictor<SIZE, BITS, ENTRIES, THRESHOLD, LR> > {
    static const int LOG_ENTRIES = log2const<ENTRIES>::value;
    static const int PERC_MAX    = (1 << (BITS - 1)) - 1;
    static const int PERC_MIN    = -(PERC_MAX + 1);
    static const int PERC_INC_TH = PERC_MAX - LR + 1;
    static const int PERC_DEC_TH = PERC_MIN + LR - 1;
    
    CORE_INT(BITS) perceptron[ENTRIES][SIZE+1];
    bool bht[SIZE];    // branch history table
    int  dp;           // dot product
    bool pd;           // prediction

public:
    PerceptronBranchPredictor() {
        for (int i = 0; i < ENTRIES; i++) {
            for (int j = 0; j < SIZE+1; j++) {
                perceptron[i][j] = 0;
            }
        }
        for (int i = 0; i < SIZE; i++) {
            bht[i] = 0;
        }
    }
    
    void _update(CORE_UINT(32) pc, bool isBranch) {
        if (pd == isBranch && dp > THRESHOLD) return;
        CORE_UINT(LOG_ENTRIES) index = pc.SLC(LOG_ENTRIES, 0);
        if (isBranch) {
            if (perceptron[index][SIZE] < PERC_INC_TH) perceptron[index][SIZE] += LR;
        } else {
            if (perceptron[index][SIZE] > PERC_DEC_TH) perceptron[index][SIZE] -= LR;
        }
        
        update:for (int i = 0; i < SIZE; i++) {
            if (bht[i] == isBranch) {
                if (perceptron[index][i] < PERC_INC_TH) perceptron[index][i] += LR;
            } else {
                if (perceptron[index][i] > PERC_DEC_TH) perceptron[index][i] -= LR;
            }
        }

        shift_reg:for (int i = 0; i < SIZE-1; i++) {
            bht[i] = bht[i+1];
        }
        bht[SIZE-1] = isBranch;
    }

    void _predict(CORE_UINT(LOG_ENTRIES) index) {
        dp = perceptron[index][SIZE];
        predict:for (int i = 0; i < SIZE; i++) {
            if (bht[i]) {
                dp += perceptron[index][i];
            } else {
                dp -= perceptron[index][i];
            }
        }
        pd = dp >= 0;
        if (!pd) dp = -dp;
    }
    
    void _process(CORE_UINT(32) pc, bool& isBranch) {
        CORE_UINT(LOG_ENTRIES) index = pc.SLC(LOG_ENTRIES, 0);
        _predict(index);
        isBranch = pd;
    }
};

template<int SIZE, int BITS, int ENTRIES, int THRESHOLD, int LR>
class PerceptronBranchPredictorV2 : public BranchPredictorWrapper<PerceptronBranchPredictorV2<SIZE, BITS, ENTRIES, THRESHOLD, LR> > {
    static const int LOG_ENTRIES = log2const<ENTRIES>::value;
    static const int PERC_MAX    = (1 << (BITS - 1)) - 1;
    static const int PERC_MIN    = -(PERC_MAX + 1);
    static const int DOUBLE_LR   = LR * 2;
    static const int PERC_INC_TH = PERC_MAX - DOUBLE_LR + 1;
    static const int PERC_DEC_TH = PERC_MIN + DOUBLE_LR - 1;
    
    CORE_INT(BITS) perceptron[ENTRIES][SIZE+1];     // w0' = -w0 + w1, w1' = -w0 - w1
    bool bht[SIZE];    // branch history table
    int  dp;           // dot product
    bool pd;           // prediction

public:
    PerceptronBranchPredictorV2() {
        for (int i = 0; i < ENTRIES; i++) {
            for (int j = 0; j < SIZE+1; j++) {
                perceptron[i][j] = 0;
            }
        }
        for (int i = 0; i < SIZE; i++) {
            bht[i] = 0;
        }
    }
    
    void _update(CORE_UINT(32) pc, bool isBranch) {
        if (pd == isBranch && dp > THRESHOLD) return;
        CORE_UINT(LOG_ENTRIES) index = pc.SLC(LOG_ENTRIES, 0);
        if (isBranch) {
            if (perceptron[index][SIZE] < PERC_INC_TH) perceptron[index][SIZE] += LR;
        } else {
            if (perceptron[index][SIZE] > PERC_DEC_TH) perceptron[index][SIZE] -= LR;
        }
        
//        update:for (int i = 0; i < SIZE; i++) {
//            if (bht[i] == isBranch) {
//                if (perceptron[index][i] < PERC_INC_TH) perceptron[index][i] += LR;
//            } else {
//                if (perceptron[index][i] > PERC_DEC_TH) perceptron[index][i] -= LR;
//            }
//        }
        
        update:for (int i1 = 0, i2 = 1; i1 < SIZE; i1 += 2, i2 += 2) {
            if (bht[i1] == bht[i2]) {
                if (bht[i1] == isBranch) {
                    if (perceptron[index][i2] > PERC_DEC_TH) perceptron[index][i2] -= DOUBLE_LR;
                } else {
                    if (perceptron[index][i2] < PERC_INC_TH) perceptron[index][i2] += DOUBLE_LR;
                }
            } else {
                if (bht[i1] == isBranch) {
                    if (perceptron[index][i1] > PERC_DEC_TH) perceptron[index][i1] -= DOUBLE_LR;
                } else {
                    if (perceptron[index][i1] < PERC_INC_TH) perceptron[index][i1] += DOUBLE_LR;
                }
            }
        }

        shift_reg:for (int i = 0; i < SIZE-1; i++) {
            bht[i] = bht[i+1];
        }
        bht[SIZE-1] = isBranch;
    }

    void _predict(CORE_UINT(LOG_ENTRIES) index) {
        dp = perceptron[index][SIZE];
        
//        predict:for (int i = 0; i < SIZE; i++) {
//            if (bht[i]) {
//                dp += perceptron[index][i];
//            } else {
//                dp -= perceptron[index][i];
//            }
//        }
        
        predict:for (int i = 0; i < SIZE; i += 2) {
            if (bht[i] == bht[i+1]) {
                if (bht[i]) {
                    dp -= perceptron[index][i+1];
                } else {
                    dp += perceptron[index][i+1];
                }
            } else {
                if (bht[i]) {
                    dp -= perceptron[index][i];
                } else {
                    dp += perceptron[index][i];
                }
            }
        }
        
        pd = dp >= 0;
        if (!pd) dp = -dp;
    }
    
    void _process(CORE_UINT(32) pc, bool& isBranch) {
        CORE_UINT(LOG_ENTRIES) index = pc.SLC(LOG_ENTRIES, 0);
        _predict(index);
        isBranch = pd;
    }
};

using BranchPredictor = BitBranchPredictor<2, 4>;

#endif /* INCLUDE_BRANCHPREDICTOR_H_ */