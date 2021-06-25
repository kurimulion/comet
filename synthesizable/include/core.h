/** Copyright 2021 INRIA, Université de Rennes 1 and ENS Rennes
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *       http://www.apache.org/licenses/LICENSE-2.0
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef __CORE_H__
#define __CORE_H__

#include "portability.h"
#include "riscvISA.h"

// all the possible memories
#include "branchPredictor.h"
#include "pipelineRegisters.h"
#include "DataMemory.h"

#ifndef MEMORY_INTERFACE
#define MEMORY_INTERFACE SimpleMemory
#endif

/******************************************************************************************
 * Stall signals enum
 * ****************************************************************************************
 */
enum StallNames { STALL_FETCH = 0, STALL_DECODE = 1, STALL_EXECUTE = 2, STALL_MEMORY = 3, STALL_WRITEBACK = 4 };

// This is ugly but otherwise with have a dependency : alu.h includes core.h
// (for pipeline regs) and core.h includes alu.h...

struct Core {
  FtoDC ftoDC;
  DCtoEx dctoEx;
  ExtoMem extoMem;
  MemtoWB memtoWB;

  // Interface size are configured with 4 bytes interface size (32 bits)
  DataMemory dataMemory;
  CORE_INT(32) *im;
  BranchPredictor bp;

  CORE_INT(32) regFile[32];
  CORE_UINT(32) pc;

  // stall
  bool stallSignals[5] = {0, 0, 0, 0, 0};
  bool stallIm, stallDm;
  unsigned long cycle;
  /// Multicycle operation

  /// Instruction cache
  // unsigned int idata[Sets][Blocksize][Associativity];   // made external for
  // modelsim

  /// Data cache
  // unsigned int ddata[Sets][Blocksize][Associativity];   // made external for
  // modelsim
};

void doCycle(struct Core& core, bool globalStall);

#endif // __CORE_H__
