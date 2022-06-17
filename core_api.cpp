/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <iostream>
#include <stdio.h>
#include <set>
#include <vector>
#include <tuple>
#define IDLE -1

using namespace std;

class simulation {
	protected:
		int cycle;
		int threads_num;
		set<int> threads_pool;
		set <int> finished_t;
		set <tuple<int,int>> wait_t; // (thread_id, return_cycle)
		vector<int> line_num;
	public:
		vector<tcontext> thread_regs;
		simulation(int threads_num) : threads_num(threads_num), cycle(0) {
			tcontext init_regs;
			for(int i = 0; i < REGS_COUNT; i++) {
				init_regs.reg[i] = 0;
			}
			for (int i = 0; i< threads_num; i++){
				threads_pool.insert(i);
				line_num.push_back(0);
				thread_regs.push_back(init_regs);
			}
		}
		int getCycle() {return cycle;}
		int getThreadsNum() {return threads_num;}
		void wait(int tid, int wait_cycle) {
			threads_pool.erase(tid);
			int return_cycle = cycle + wait_cycle + 1;
			wait_t.insert(make_tuple(tid, return_cycle));
		}
		bool idle() {return threads_pool.empty();}
		void threadEnded(int tid) {
			finished_t.insert(tid);
			threads_pool.erase(tid);
		}
		bool simEnded() {return finished_t.size() == threads_num;}
		int getNextLine(int tid) {return line_num[tid];}
		void aritAct(Instruction inst, int tid) {
			if(inst.opcode == CMD_ADD) {
				thread_regs[tid].reg[inst.dst_index] = thread_regs[tid].reg[inst.src1_index] + thread_regs[tid].reg[inst.src2_index_imm];
			}
			if(inst.opcode == CMD_ADDI && inst.isSrc2Imm) {
				thread_regs[tid].reg[inst.dst_index] = thread_regs[tid].reg[inst.src1_index] + inst.src2_index_imm;
			}
			if(inst.opcode == CMD_SUB) {
				thread_regs[tid].reg[inst.dst_index] = thread_regs[tid].reg[inst.src1_index] - thread_regs[tid].reg[inst.src2_index_imm];
			}
			if(inst.opcode == CMD_SUBI && inst.isSrc2Imm) {
				thread_regs[tid].reg[inst.dst_index] = thread_regs[tid].reg[inst.src1_index] - inst.src2_index_imm;
			}
		}
		void memAct(Instruction inst, int tid) {
			int sec_op;
			if(inst.isSrc2Imm) {
				sec_op = inst.src2_index_imm;
			}
			else {
				sec_op = thread_regs[tid].reg[inst.src2_index_imm];
			}
			if(inst.opcode == CMD_STORE) {
				SIM_MemDataWrite(thread_regs[tid].reg[inst.dst_index] + sec_op,thread_regs[tid].reg[inst.src1_index]);
			}
			if(inst.opcode == CMD_LOAD) {
				SIM_MemDataRead(thread_regs[tid].reg[inst.src1_index] + sec_op, &thread_regs[tid].reg[inst.dst_index]);
			}
		}
		void endCycle(int tid) {
			cycle ++;
			set <tuple<int,int>>::iterator itr;
			for(itr = wait_t.begin(); itr != wait_t.end(); itr++) {
				if(cycle == get<1>(*itr)){
					int return_t = get<0>(*itr);
					threads_pool.insert(return_t);
				}
			}
			if(tid != IDLE) {
				line_num[tid]++;
			}

		}
};

class fine_grained: public simulation {
	private:
		int inst_num;
	public:
		fine_grained(int threads_num) : simulation(threads_num), inst_num(0) {}
		int nextThread(int tid) {
			
			int next_tid = IDLE;
			if(idle()){
				return next_tid;
			}
			for(int i = tid + 1; i < getThreadsNum(); i++) {
				if(threads_pool.count(i) > 0){
					return i;
				}
			}
			for(int i = 0; i < tid + 1; i++) {
				if(threads_pool.count(i) > 0){
					return i;
				}
			}
			return next_tid;
			
		}

};


vector<tcontext> THREADS_REGS;
void CORE_BlockedMT() {
}

void CORE_FinegrainedMT() {
	int threads_num = SIM_GetThreadsNum();
	if (threads_num == 0){
		return;
	}
	int curr_tid = 0;
	fine_grained curr_sim(SIM_GetThreadsNum());
	Instruction curr_inst;
	while(!curr_sim.simEnded()) {
		cout << "curr_tid: "<< curr_tid << endl;
		if(curr_tid != IDLE) {
			SIM_MemInstRead(curr_sim.getNextLine(curr_tid), &curr_inst, curr_tid);
			if(curr_inst.opcode == CMD_HALT) {
				curr_sim.threadEnded(curr_tid);
			}
			else {
				if(curr_inst.opcode < CMD_LOAD && curr_inst.opcode > CMD_NOP) {
					curr_sim.aritAct(curr_inst, curr_tid);
				}
				if(curr_inst.opcode >= CMD_LOAD){
					curr_sim.memAct(curr_inst, curr_tid);
					int wait_cycle;
					if(curr_inst.opcode == CMD_LOAD) {
						wait_cycle = SIM_GetLoadLat();
					}
					else{
						wait_cycle = SIM_GetStoreLat();
					}
					curr_sim.wait(curr_tid, wait_cycle);
				}	
			}
		}
		curr_sim.endCycle(curr_tid);
		curr_tid = curr_sim.nextThread(curr_tid);
	}
	THREADS_REGS = curr_sim.thread_regs;
	tcontext i = THREADS_REGS.front();
	cout << i.reg[0] << endl;
	// vector<tcontext>::iterator itr;
	// for(itr = curr_sim.thread_regs.begin(); itr != curr_sim.thread_regs.end();  itr++) {
	// 	THREADS_REGS.push_back(*itr);
	// 	cout << THREADS_REGS.front() << endl;

	// }
}

double CORE_BlockedMT_CPI(){
	return 0;
}

double CORE_FinegrainedMT_CPI(){
	return 0;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	cout << THREADS_REGS[0].reg[0] << endl;
	// context[threadid] = THREADS_REGS[threadid];
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
}
