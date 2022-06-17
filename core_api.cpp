/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <iostream>
#include <stdio.h>

using namespace std;
Instruction* curr_inst;
void CORE_BlockedMT() {
}

void CORE_FinegrainedMT() {
	uint32_t line = 0;
	int threads = SIM_GetThreadsNum();
	SIM_MemInstRead(line, curr_inst, 1);
	// cout << curr_inst->opcode << endl;


}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
