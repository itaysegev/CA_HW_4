/* 046267 Computer Architecture - Winter 20/21 - HW #4 */

#include "core_api.h"
#include "sim_api.h"
#include <iostream>
#include <stdio.h>
#include <set>
#include <vector>
#include <tuple>
#define IDLE -1
#define INIT_VAL 0
using namespace std;

//global var for calculation function 
vector<tcontext> THREADS_REGS;
double INST = INIT_VAL;
int CYCLE = INIT_VAL;

//main data struct all other data structures inherits from it
class simulation {
	protected:
		int cycle;
		int threads_num;
		set<int> threads_pool; // all ready to run threads
		set <int> finished_t; // all finished threads
		set <tuple<int,int>> wait_t; // (thread_id, return_cycle) for waiting state threads
		vector<int> line_num; // line number in code for each thread
	public:
		vector<tcontext> thread_regs; // regs context for each thread later copy to global 
		simulation(int threads_num) :cycle(INIT_VAL), threads_num(threads_num) {
			tcontext init_regs;
			// init all with zeros and all threads are ready to run
			for(int i = 0; i < REGS_COUNT; i++) {
				init_regs.reg[i] = INIT_VAL;
			}
			for (int i = 0; i< threads_num; i++){
				threads_pool.insert(i);
				line_num.push_back(INIT_VAL);
				thread_regs.push_back(init_regs);
			}
		}
		int getCycle() {return cycle;}
		int getThreadsNum() {return threads_num;}
		void wait(int tid, int wait_cycle) {
			threads_pool.erase(tid); // remove from ready to run state
			int return_cycle = cycle + wait_cycle + 1; // calculate return cycle number
			wait_t.insert(make_tuple(tid, return_cycle)); // insert to waiting list
		}
		bool idle() {return threads_pool.empty();}
		void threadEnded(int tid) {
			finished_t.insert(tid);
			threads_pool.erase(tid);
		}
		bool simEnded() {return (int)finished_t.size() == threads_num;} 
		int getNextLine(int tid) {return line_num[tid];}
		void aritAct(Instruction inst, int tid) { // all regs op by different opcode as define in sim_api.h
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
		void memAct(Instruction inst, int tid) { // all memory op by opcode as define in sim_api.h
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
			// return to ready to run all threads that have finished waiting time
			itr = wait_t.begin();
			while (itr != wait_t.end()) {
				if (cycle >= get<1>(*itr)) {
					int return_t = get<0>(*itr);
					threads_pool.insert(return_t);
					tuple<int, int> to_remove = *itr;
					itr++;
					wait_t.erase(to_remove);
					continue;
				}
				itr++;
			}
			if(tid != IDLE) {
				line_num[tid]++;
			}

		}
		virtual int nextThread(int tid) = 0;
};
// fine-grained architecture data structure
class fine_grained: public simulation {
	int last_thread;
	public:
		fine_grained(int threads_num) : simulation(threads_num), last_thread(IDLE) {}
		int nextThread(int tid) override { // schedular policy by fine-grained definition 
			if(idle()){
				return IDLE;
			}
			int start_id = tid;
			if(tid == IDLE) {
				start_id = last_thread;
			}
			for(int i = start_id + 1; i < getThreadsNum(); i++) {
				if(threads_pool.count(i) > 0){
					last_thread = i;
					return i;
				}
			}
			for(int i = 0; i < start_id + 1; i++) {
				if(threads_pool.count(i) > 0){
					last_thread = i;
					return i;
				}
			}
			return IDLE;
		}

};

// BlockedMT architecture data structure
class Blocked_MT : public simulation {
public:
	int last_thread;
	Blocked_MT(int threads_num) : simulation(threads_num), last_thread(0) {}
	int nextThread(int tid) override { // schedular policy by fine-grained definition 
		if (tid != IDLE) { last_thread = tid; }
		bool switched = false;
		int next = IDLE;
		if (tid == IDLE)
		{
			tid = last_thread;
		}
		if (idle()) {
			return next;
		}
		if (threads_pool.count(tid) > 0)
		{
			return tid;
		}
		for (int i = tid + 1; i < getThreadsNum(); i++) {
			if (threads_pool.count(i) > 0) {
				switched = true;
				next = i;
				break;
				
			}
		}
		if (switched == false) {
			for (int i = 0; i < tid; i++) {
				if (threads_pool.count(i) > 0) {
				switched = true;
				next = i;
				break;
				}
			}
		}
		if (switched) {
			cycle += SIM_GetSwitchCycles();
		}
		return next;
	}

};

void MT(simulation& curr_sim) {
	Instruction curr_inst;
	int curr_tid = INIT_VAL;
	while (!curr_sim.simEnded()) {
		if (curr_tid != IDLE) {
			INST++;
			SIM_MemInstRead(curr_sim.getNextLine(curr_tid), &curr_inst, curr_tid); //read next instruction by line and thread pid
			if (curr_inst.opcode == CMD_HALT) {
				curr_sim.threadEnded(curr_tid);
			}
			else {
				if (curr_inst.opcode < CMD_LOAD && curr_inst.opcode > CMD_NOP) {
					curr_sim.aritAct(curr_inst, curr_tid);
				}
				if (curr_inst.opcode >= CMD_LOAD) {
					curr_sim.memAct(curr_inst, curr_tid);
					int wait_cycle;
					if (curr_inst.opcode == CMD_LOAD) {
						wait_cycle = SIM_GetLoadLat();
					}
					else {
						wait_cycle = SIM_GetStoreLat();
					}
					curr_sim.wait(curr_tid, wait_cycle);
				}
			}
		}

		curr_sim.endCycle(curr_tid);
		curr_tid = curr_sim.nextThread(curr_tid);
	}
	// after simulation ended update global verbals
	CYCLE = curr_sim.getCycle();
	THREADS_REGS = curr_sim.thread_regs;
}

void CORE_BlockedMT() {
	int threads_num = SIM_GetThreadsNum();
	if (threads_num == 0) {
		return;
	}
	Blocked_MT curr_sim(SIM_GetThreadsNum());
	MT(curr_sim);
}

void CORE_FinegrainedMT() {
	int threads_num = SIM_GetThreadsNum();
	if (threads_num == 0) {
		return;
	}
	fine_grained curr_sim(SIM_GetThreadsNum());
	MT(curr_sim);
}

double CORE_BlockedMT_CPI(){
	double cpi = CYCLE / INST;
	CYCLE = INIT_VAL;
	INST = INIT_VAL;
	return cpi;
}

double CORE_FinegrainedMT_CPI(){
	double cpi = CYCLE / INST;
	CYCLE = INIT_VAL;
	INST = INIT_VAL; 
	return cpi;
}

void CORE_BlockedMT_CTX(tcontext* context, int threadid) {
	context[threadid] = THREADS_REGS[threadid];
	
}

void CORE_FinegrainedMT_CTX(tcontext* context, int threadid) {
	context[threadid] = THREADS_REGS[threadid];
}
