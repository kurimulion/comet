/* vim: set ts=4 ai nu: */
#include "elfFile.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <map>
#include <iostream>
#include <string.h>
#include "core.h"
#include "portability.h"
#include "syscall.h"
//#include "sds_lib.h"

#ifdef __VIVADO__
	#include "DataMemory.h"
#else 
	#include "DataMemoryCatapult.h"
#endif

using namespace std;

class Simulator{

	private:
		//counters
		map<CORE_UINT(32), CORE_UINT(8) > ins_memorymap;
		map<CORE_UINT(32), CORE_UINT(8) > data_memorymap;

		struct Core core;
		bool exitFlag;

	public:
		CORE_INT(32)* ins_memory;
		CORE_INT(32)* data_memory;
		DataMemory dataMemory;

		Simulator(){
			ins_memory = (CORE_INT(32) *)malloc(8192 * sizeof(CORE_INT(32)));
			data_memory = (CORE_INT(32) *)malloc(8192 * sizeof(CORE_INT(32)));
			core.im = ins_memory;
			for(int i =0;i<8192;i++){
				ins_memory[i]=0;
				data_memory[i]=0;
			}
		}

		~Simulator(){
			free(ins_memory);
			free(data_memory);
		}

		void fillMemory(){
			//Check whether data memory and instruction memory from program will actually fit in processor.
			// cout << ins_memorymap.size()<<endl;

			if(ins_memorymap.size() / 4 > 8192){
				printf("Error! Instruction memory size exceeded");
				exit(-1);
			}
			if(data_memorymap.size() / 4 > 8192){
				printf("Error! Data memory size exceeded");
				exit(-1);
			}

			//fill instruction memory
			for(map<CORE_UINT(32), CORE_UINT(8) >::iterator it = ins_memorymap.begin(); it!=ins_memorymap.end(); ++it){
				ins_memory[(it->first/4) % 8192].SET_SLC(((it->first)%4)*8,it->second);
			}

			//fill data memory
			for(map<CORE_UINT(32), CORE_UINT(8) >::iterator it = data_memorymap.begin(); it!=data_memorymap.end(); ++it){
				//data_memory.set_byte((it->first/4)%8192,it->second,it->first%4);
				data_memory[(it->first%8192)/4].SET_SLC(((it->first%8192)%4)*8,it->second);
			}
		}

		void setInstructionMemory(CORE_UINT(32) addr, CORE_INT(8) value){
			ins_memorymap[addr] = value;
		}

		void setDataMemory(CORE_UINT(32) addr, CORE_INT(8) value){
			if((addr % 8192 )/4 == 0){
				//cout << addr << " " << value << endl;
			}
			data_memorymap[addr] = value;
		}

		CORE_INT(32)* getInstructionMemory(){
			return ins_memory;
		}


		CORE_INT(32)* getDataMemory(){
			return data_memory;
		}

		void setPC(CORE_UINT(32) pc){
			this->core.pc = pc;
		}

		CORE_UINT(32) getPC(){
			return this->core.pc;
		}
		void solveSyscall(){
			if ((core.extoMem.opCode == RISCV_SYSTEM) && core.extoMem.instruction.SLC(12, 20) == 0 && !core.stallSignals[2] &&
				!core.stallIm && !core.stallDm) {

				CORE_INT(32) syscallId = core.regFile[17];
				CORE_INT(32) arg1      = core.regFile[10];
				CORE_INT(32) arg2      = core.regFile[11];
				CORE_INT(32) arg3      = core.regFile[12];
				CORE_INT(32) arg4      = core.regFile[13];

				if (core.memtoWB.useRd && core.memtoWB.we && !core.stallSignals[3]) {
				if (core.memtoWB.rd == 10)
					arg1 = core.memtoWB.result;
				else if (core.memtoWB.rd == 11)
					arg2 = core.memtoWB.result;
				else if (core.memtoWB.rd == 12)
					arg3 = core.memtoWB.result;
				else if (core.memtoWB.rd == 13)
					arg4 = core.memtoWB.result;
				else if (core.memtoWB.rd == 17)
					syscallId = core.memtoWB.result;
				}

				CORE_INT(32) result = 0;

				switch (syscallId) {
				case SYS_exit:
					exitFlag = 1; // Currently we break on ECALL
					break;
				case SYS_read:
					result = doRead(arg1, arg2, arg3);
					break;
				case SYS_write:
					result = doWrite(arg1, arg2, arg3);
					break;
				case SYS_brk:
					result = doSbrk(arg1);
					break;
				case SYS_open:
					result = doOpen(arg1, arg2, arg3);
					break;
				case SYS_openat:
					result = doOpenat(arg1, arg2, arg3, arg4);
					break;
				case SYS_lseek:
					result = doLseek(arg1, arg2, arg3);
					break;
				case SYS_close:
					result = doClose(arg1);
					break;
				case SYS_fstat:
					result = doFstat(arg1, arg2);
					break;
				case SYS_stat:
					result = doStat(arg1, arg2);
					break;
				case SYS_gettimeofday:
					result = doGettimeofday(arg1);
					break;
				case SYS_unlink:
					result = doUnlink(arg1);
					break;
				case SYS_exit_group:
					fprintf(stderr, "Syscall : SYS_exit_group\n");
					exitFlag = 1;
					break;
				case SYS_getpid:
					fprintf(stderr, "Syscall : SYS_getpid\n");
					exitFlag = 1;
					break;
				case SYS_kill:
					fprintf(stderr, "Syscall : SYS_kill\n");
					exitFlag = 1;
					break;
				case SYS_link:
					fprintf(stderr, "Syscall : SYS_link\n");
					exitFlag = 1;
					break;
				case SYS_mkdir:
					fprintf(stderr, "Syscall : SYS_mkdir\n");
					exitFlag = 1;
					break;
				case SYS_chdir:
					fprintf(stderr, "Syscall : SYS_chdir\n");
					exitFlag = 1;
					break;
				case SYS_getcwd:
					fprintf(stderr, "Syscall : SYS_getcwd\n");
					exitFlag = 1;
					break;
				case SYS_lstat:
					fprintf(stderr, "Syscall : SYS_lstat\n");
					exitFlag = 1;
					break;
				case SYS_fstatat:
					fprintf(stderr, "Syscall : SYS_fstatat\n");
					exitFlag = 1;
					break;
				case SYS_access:
					fprintf(stderr, "Syscall : SYS_access\n");
					exitFlag = 1;
					break;
				case SYS_faccessat:
					fprintf(stderr, "Syscall : SYS_faccessat\n");
					exitFlag = 1;
					break;
				case SYS_pread:
					fprintf(stderr, "Syscall : SYS_pread\n");
					exitFlag = 1;
					break;
				case SYS_pwrite:
					fprintf(stderr, "Syscall : SYS_pwrite\n");
					exitFlag = 1;
					break;
				case SYS_uname:
					fprintf(stderr, "Syscall : SYS_uname\n");
					exitFlag = 1;
					break;
				case SYS_getuid:
					fprintf(stderr, "Syscall : SYS_getuid\n");
					exitFlag = 1;
					break;
				case SYS_geteuid:
					fprintf(stderr, "Syscall : SYS_geteuid\n");
					exitFlag = 1;
					break;
				case SYS_getgid:
					fprintf(stderr, "Syscall : SYS_getgid\n");
					exitFlag = 1;
					break;
				case SYS_getegid:
					fprintf(stderr, "Syscall : SYS_getegid\n");
					exitFlag = 1;
					break;
				case SYS_mmap:
					fprintf(stderr, "Syscall : SYS_mmap\n");
					exitFlag = 1;
					break;
				case SYS_munmap:
					fprintf(stderr, "Syscall : SYS_munmap\n");
					exitFlag = 1;
					break;
				case SYS_mremap:
					fprintf(stderr, "Syscall : SYS_mremap\n");
					exitFlag = 1;
					break;
				case SYS_time:
					fprintf(stderr, "Syscall : SYS_time\n");
					exitFlag = 1;
					break;
				case SYS_getmainvars:
					fprintf(stderr, "Syscall : SYS_getmainvars\n");
					exitFlag = 1;
					break;
				case SYS_rt_sigaction:
					fprintf(stderr, "Syscall : SYS_rt_sigaction\n");
					exitFlag = 1;
					break;
				case SYS_writev:
					fprintf(stderr, "Syscall : SYS_writev\n");
					exitFlag = 1;
					break;
				case SYS_times:
					fprintf(stderr, "Syscall : SYS_times\n");
					exitFlag = 1;
					break;
				case SYS_fcntl:
					fprintf(stderr, "Syscall : SYS_fcntl\n");
					exitFlag = 1;
					break;
				case SYS_getdents:
					fprintf(stderr, "Syscall : SYS_getdents\n");
					exitFlag = 1;
					break;
				case SYS_dup:
					fprintf(stderr, "Syscall : SYS_dup\n");
					exitFlag = 1;
					break;

					// Custom syscalls
				case SYS_threadstart:
					result = 0;
					break;
				case SYS_nbcore:
					result = 1;
					break;

				default:
					fprintf(stderr, "Syscall : Unknown system call, %d (%x) with arguments :\n", syscallId.to_int(),
							syscallId.to_int());
					fprintf(stderr, "%d (%x)\n%d (%x)\n%d (%x)\n%d (%x)\n", arg1.to_int(), arg1.to_int(), arg2.to_int(),
							arg2.to_int(), arg3.to_int(), arg3.to_int(), arg4.to_int(), arg4.to_int());
					exitFlag = 1;
					break;
				}

				// We write the result and forward
				core.memtoWB.result = result;
				core.memtoWB.rd     = 10;
				core.memtoWB.useRd  = 1;

				if (core.dctoEx.useRs1 && (core.dctoEx.rs1 == 10))
				core.dctoEx.lhs = result;
				if (core.dctoEx.useRs2 && (core.dctoEx.rs2 == 10))
				core.dctoEx.rhs = result;
				if (core.dctoEx.useRs3 && (core.dctoEx.rs3 == 10))
				core.dctoEx.datac = result;
			}
		}

		void run()
		{
			exitFlag = false;
			while (!exitFlag) {
				// std::cout << std::hex << (int)core.pc << ";" << (int)core.im[(core.pc & 0x0FFFF)/4] << std::endl;
				doCycle(core, 0);
				solveSyscall();
      		}
			std::cout << "Program Exit Successfully in " << std::dec << core.cycle << " cycles" << std::endl;
		}

		void setDataMemory() {
			int i;
			for(i = 0;i<8192;i++){
				core.dataMemory.memory[i][0]=data_memory[i].SLC(8,0);
				core.dataMemory.memory[i][1]=data_memory[i].SLC(8,8);
				core.dataMemory.memory[i][2]=data_memory[i].SLC(8,16);
				core.dataMemory.memory[i][3]=data_memory[i].SLC(8,24);
			}
		}

		void loadOutputMemory(CORE_INT(32) *dm_out) {
			int i;
			for(i = 0;i<8192;i++){
				dm_out[i].SET_SLC(0,core.dataMemory.memory[i][0]);
				dm_out[i].SET_SLC(8,core.dataMemory.memory[i][1]);
				dm_out[i].SET_SLC(16,core.dataMemory.memory[i][2]);
				dm_out[i].SET_SLC(24,core.dataMemory.memory[i][3]);
			}
		}
};


int main(int argc, char **argv){
	char *binaryFile = argv[2];
	ElfFile elfFile(binaryFile);
	Simulator sim;
	int counter = 0;
	for (unsigned int sectionCounter = 0;sectionCounter<elfFile.sectionTable->size(); sectionCounter++){
        ElfSection *oneSection = elfFile.sectionTable->at(sectionCounter);
        if(oneSection->address != 0 && oneSection->getName().compare(".text")){
            //If the address is not null we place its content into memory
            unsigned char* sectionContent = oneSection->getSectionCode();
            for (unsigned int byteNumber = 0;byteNumber<oneSection->size; byteNumber++){
            	counter++;
                sim.setDataMemory(oneSection->address + byteNumber, sectionContent[byteNumber]);
            }
        }

        if (!oneSection->getName().compare(".text")){
        	unsigned char* sectionContent = oneSection->getSectionCode();
            for (unsigned int byteNumber = 0;byteNumber<oneSection->size; byteNumber++){
                sim.setInstructionMemory((oneSection->address + byteNumber) & 0x0FFFF, sectionContent[byteNumber]);
            }
    	}
    }

    for (int oneSymbol = 0; oneSymbol < elfFile.symbols->size(); oneSymbol++){
		ElfSymbol *symbol = elfFile.symbols->at(oneSymbol);
		const char* name = (const char*) &(elfFile.sectionTable->at(elfFile.indexOfSymbolNameSection)->getSectionCode()[symbol->name]);
		if (strcmp(name, "_start") == 0){
			fprintf(stderr, "%s\n", name);
			sim.setPC(symbol->offset);
		}
	}


    sim.fillMemory();
	sim.setDataMemory();

    CORE_INT(32)* dm_out;
    CORE_INT(32)* debug_out;
    dm_out = (CORE_INT(32) *)malloc(8192 * sizeof(CORE_INT(32)));
    debug_out = (CORE_INT(32) *)malloc(200 * sizeof(CORE_INT(32)));
	sim.loadOutputMemory(dm_out);
    std::cout << "dm" <<std::endl;
    for(int i = 0;i<8192;i++){
        	std::cout << std::dec << i << " : ";
        	std::cout << std::dec << dm_out[i] << std::endl;
     }
    sim.run();
    /*for(int i = 0;i<34;i++){
    	std::cout << std::dec << i << " : ";
    	std::cout << std::hex << debug_out[i] << std::endl;
    }*/
	sim.loadOutputMemory(dm_out);
    std::cout << "dm" <<std::endl;
    for(int i = 0;i<8192;i++){
        	std::cout << std::dec << i << " : ";
        	std::cout << std::dec << dm_out[i] << std::endl;
     }
    free(dm_out);
    free(debug_out);
	return 0;
}
