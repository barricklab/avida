/*
 *  cHardwareExperimental.h
 *  Avida
 *
 *  Created by David on 2/10/07 based on cHardwareCPU.h
 *  Copyright 1999-2011 Michigan State University. All rights reserved.
 *  Copyright 1999-2003 California Institute of Technology.
 *
 *
 *  This file is part of Avida.
 *
 *  Avida is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 *  Avida is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with Avida.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef cHardwareExperimental_h
#define cHardwareExperimental_h

#include "avida/hardware/InstLib.h"
#include "avida/util/NopSequence.h"


class cHardwareExperimental : public cHardwareBase
{
public:
  typedef bool (cHardwareExperimental::*tMethod)(Context& ctx);

private:
  // --------  Structure Constants  --------
  static const int NUM_REGISTERS = 8;
  static const int NUM_HEADS = NUM_REGISTERS < 4 ? 4 : NUM_REGISTERS;
  enum tRegisters { rAX = 0, rBX, rCX, rDX, rEX, rFX, rGX, rHX, rIX, rJX, rKX, rLX, rMX, rNX, rOX, rPX};
  enum { HEAD_IP = 0, HEAD_READ, HEAD_WRITE, HEAD_FLOW };
  static const int NUM_NOPS = NUM_REGISTERS;
  
  
  // --------  Static Variables  --------
  static StaticTableInstLib<cHardwareExperimental::tMethod>* s_inst_slib;
  static StaticTableInstLib<cHardwareExperimental::tMethod>* initInstLib(void);
  
  
  // --------  Define Internal Data Structures  --------
  struct DataValue
  {
    int value;
    
    // Actual age of this value
    unsigned int originated:14;
    unsigned int from_env:1;
    unsigned int from_sensor:1;
    
    // Age of the oldest component used to create this value
    unsigned int oldest_component:14;
    unsigned int env_component:1;
    unsigned int sensor_component:1;
    
    inline DataValue() { Clear(); }
    inline void Clear() { value = 0; originated = 0; from_env = 0, from_sensor = 0, oldest_component = 0; env_component = 0, sensor_component = 0; }
    inline DataValue& operator=(const DataValue& i);
  };
  
  
  class Stack
  {
  private:
    int m_sz;
    DataValue* m_stack;
    int m_sp;
    
  public:
    Stack() : m_sz(0), m_stack(NULL), m_sp(0) { ; }
    inline Stack(const Stack& is) : m_sp(is.m_sp) { Clear(is.m_sz); for (int i = 0; i < m_sz; i++) m_stack[i] = is.m_stack[i]; }
    ~Stack() { delete [] m_stack; }
    
    inline void operator=(const Stack& is) { m_sp = is.m_sp; Clear(is.m_sz); for (int i = 0; i < m_sz; i++) m_stack[i] = is.m_stack[i]; }
    
    inline void Push(const DataValue& value) { if (--m_sp < 0) m_sp = m_sz - 1; m_stack[(int)m_sp] = value; }
    inline DataValue Pop() { DataValue v = m_stack[(int)m_sp]; m_stack[(int)m_sp].Clear(); if (++m_sp == m_sz) m_sp = 0; return v; }
    inline DataValue& Peek() { return m_stack[(int)m_sp]; }
    inline const DataValue& Peek() const { return m_stack[(int)m_sp]; }
    inline const DataValue& Get(int d = 0) const { assert(d >= 0); int p = d + m_sp; return m_stack[(p >= m_sz) ? (p - m_sz) : p]; }
    inline void Clear(int sz) { delete [] m_stack; m_sz = sz; m_stack = new DataValue[sz]; }
  };
  
  
  struct cLocalThread
  {
  private:
    int m_id;
    

    cLocalThread(const cLocalThread&);
    
  public:
    DataValue reg[NUM_REGISTERS];
    cHeadCPU heads[NUM_HEADS];
    Stack stack;
    unsigned char cur_stack;              // 0 = local stack, 1 = global stack.
    unsigned char cur_head;
    
    struct {
      bool reading_label:1;
      bool reading_seq:1;
      bool active:1;
      bool wait_greater:1;
      bool wait_equal:1;
      bool wait_less:1;
      unsigned int wait_reg:4;
      unsigned int wait_dst:4;
    };
    int wait_value;
    
    Util::NopSequence read_label;
    Util::NopSequence read_seq;
    Util::NopSequence next_label;
    
    inline cLocalThread() { ; }
    cLocalThread(cHardwareExperimental* in_hardware, int in_id = -1) { Reset(in_hardware, in_id); }
    ~cLocalThread() { ; }
    
    void operator=(const cLocalThread& in_thread);
    void Reset(cHardwareExperimental* in_hardware, int in_id);
    inline int GetID() const { return m_id; }
    inline void SetID(int in_id) { m_id = in_id; }
  };
  
  
  // --------  Member Variables  --------
  const tMethod* m_functions;
  
  InstMemSpace m_memory;          // Memory...
  Stack m_global_stack;     // A stack that all threads share.
  
  Apto::Array<cLocalThread, Apto::ManagedPointer> m_threads;
  int m_thread_id_chart;
  int m_cur_thread;
  
  int m_use_avatar;
  bool m_from_sensor;
  
  struct {
    unsigned int m_cycle_count:16;
    unsigned int m_last_output:16;
  };
  
  // Flags
  struct {
    bool m_mal_active:1;         // Has an allocate occured since last divide?
    bool m_advance_ip:1;         // Should the IP advance after this instruction?
    bool m_executedmatchstrings:1;	// Have we already executed the match strings instruction?
    bool m_spec_die:1;
    
    bool m_thread_slicing_parallel:1;
    bool m_no_cpu_cycle_time:1;
    
    bool m_slip_read_head:1;
    
    bool m_io_expire:1;
    
    unsigned int m_waiting_threads:4;
  };
  
  
  
  cHardwareExperimental(const cHardwareExperimental&); // @not_implemented
  cHardwareExperimental& operator=(const cHardwareExperimental&); // @not_implemented
  
  
public:
  cHardwareExperimental(cAvidaContext& ctx, cWorld* world, cOrganism* in_organism, cInstSet* in_inst_set);
  ~cHardwareExperimental() { ; }
  
  static InstLib* InstructionLibrary() { return s_inst_slib; }
  static cString GetDefaultInstFilename() { return "instset-experimental.cfg"; }
  
  
  // --------  Core Execution Methods  --------
  bool SingleProcess(cAvidaContext& ctx, bool speculative = false);
  void ProcessBonusInst(cAvidaContext& ctx, const Instruction& inst);

  
  // --------  Helper Methods  --------
  int GetType() const { return HARDWARE_TYPE_CPU_EXPERIMENTAL; }  
  bool SupportsSpeculative() const { return true; }
  void PrintStatus(std::ostream& fp);
  
  // --------  Stack Manipulation  --------
  inline int GetStack(int depth=0, int stack_id = -1, int in_thread = -1) const;
  inline int GetNumStacks() const { return 2; }
  
  
  // --------  Head Manipulation (including IP)  --------
  const cHeadCPU& GetHead(int head_id) const { return m_threads[m_cur_thread].heads[head_id]; }
  cHeadCPU& GetHead(int head_id) { return m_threads[m_cur_thread].heads[head_id];}
  const cHeadCPU& GetHead(int head_id, int thread) const { return m_threads[thread].heads[head_id]; }
  cHeadCPU& GetHead(int head_id, int thread) { return m_threads[thread].heads[head_id];}
  int GetNumHeads() const { return NUM_HEADS; }
  
  const cHeadCPU& IP() const { return m_threads[m_cur_thread].heads[HEAD_IP]; }
  cHeadCPU& IP() { return m_threads[m_cur_thread].heads[HEAD_IP]; }
  const cHeadCPU& IP(int thread) const { return m_threads[thread].heads[HEAD_IP]; }
  cHeadCPU& IP(int thread) { return m_threads[thread].heads[HEAD_IP]; }
  
  
  // --------  Memory Manipulation  --------
  const InstMemSpace& GetMemory() const { return m_memory; }
  InstMemSpace& GetMemory() { return m_memory; }
  int GetMemSize() const { return m_memory.GetSize(); }
  const InstMemSpace& GetMemory(int) const { return m_memory; }
  InstMemSpace& GetMemory(int) { return m_memory; }
  int GetMemSize(int) const { return  m_memory.GetSize(); }
  int GetNumMemSpaces() const { return 1; }
  
  
  // --------  Register Manipulation  --------
  int GetRegister(int reg_id) const { return m_threads[m_cur_thread].reg[reg_id].value; }
  int GetNumRegisters() const { return NUM_REGISTERS; }
  bool FromSensor(int reg_id) const { return m_threads[m_cur_thread].reg[reg_id].from_sensor; }
  
  
  // --------  Thread Manipulation  --------
  inline void ThreadPrev(); // Shift the current thread in use.
  Systematics::UnitPtr ThreadGetOwner() { m_organism->AddReference(); return Systematics::UnitPtr(m_organism); }
  
  int GetNumThreads() const     { return m_threads.GetSize(); }
  int GetCurThread() const      { return m_cur_thread; }
  

  
private:
  
  // --------  Core Execution Methods  --------
  bool SingleProcess_ExecuteInst(cAvidaContext& ctx, const Instruction& cur_inst);
  void internalReset();
  void internalResetOnFailedDivide();
  
  
  // --------  Stack Manipulation  --------
  inline DataValue stackPop();
  inline Stack& getStack(int stack_id);
  inline void switchStack();
  
  
  // --------  Head Manipulation (including IP)  --------
  void AdjustHeads();
  
  
  // --------  Label Manipulation  -------
  const Util::NopSequence& GetLabel() const { return m_threads[m_cur_thread].next_label; }
  Util::NopSequence& GetLabel() { return m_threads[m_cur_thread].next_label; }
  void ReadLabel();
  cHeadCPU FindLabelStart(bool mark_executed);
  cHeadCPU FindLabelForward(bool mark_executed);
  cHeadCPU FindLabelBackward(bool mark_executed);
  cHeadCPU FindNopSequenceStart(bool mark_executed);
  cHeadCPU FindNopSequenceForward(bool mark_executed);
  cHeadCPU FindNopSequenceBackward(bool mark_executed);
  inline const Util::NopSequence& GetReadLabel() const { return m_threads[m_cur_thread].read_label; }
  inline const Util::NopSequence& GetReadSequence() const { return m_threads[m_cur_thread].read_seq; }
  inline Util::NopSequence& GetReadLabel() { return m_threads[m_cur_thread].read_label; }
  inline Util::NopSequence& GetReadSequence() { return m_threads[m_cur_thread].read_seq; }
  
  
  // --------  Thread Manipulation  -------
  bool ForkThread(); // Adds a new thread based off of m_cur_thread.
  bool ThreadCreate(const cHeadCPU& start_pos); // Adds a new thread starting at the flow head
  bool ExitThread(); // Kill the current thread!
  
  
  // ---------- Instruction Helpers -----------
  int FindModifiedRegister(int default_register);
  int FindModifiedNextRegister(int default_register);
  int FindModifiedPreviousRegister(int default_register);
  int FindModifiedHead(int default_head);
  int FindNextRegister(int base_reg);
  
  bool Allocate_Necro(const int new_size);
  bool Allocate_Random(cAvidaContext& ctx, const int old_size, const int new_size);
  bool Allocate_Default(const int new_size);
  bool Allocate_Main(cAvidaContext& ctx, const int allocated_size);
  
  int calcCopiedSize(const int parent_size, const int child_size);
  
  inline const cHeadCPU& getHead(int head_id) const { return m_threads[m_cur_thread].heads[head_id]; }
  inline cHeadCPU& getHead(int head_id) { return m_threads[m_cur_thread].heads[head_id];}
  inline const cHeadCPU& getHead(int head_id, int thread) const { return m_threads[thread].heads[head_id]; }
  inline cHeadCPU& getHead(int head_id, int thread) { return m_threads[thread].heads[head_id];}
  
  inline const cHeadCPU& getIP() const { return m_threads[m_cur_thread].heads[HEAD_IP]; }
  inline cHeadCPU& getIP() { return m_threads[m_cur_thread].heads[HEAD_IP]; }
  inline const cHeadCPU& getIP(int thread) const { return m_threads[thread].heads[HEAD_IP]; }
  inline cHeadCPU& getIP(int thread) { return m_threads[thread].heads[HEAD_IP]; }

  // --------  Division Support  -------
  bool Divide_Main(cAvidaContext& ctx, const int divide_point, const int extra_lines=0, double mut_multiplier=1);
  

  // ---------- Utility Functions -----------
  inline unsigned int BitCount(unsigned int value) const;
  inline void setInternalValue(int reg_num, int value, bool from_env = false, bool from_sensor = false);
  inline void setInternalValue(int reg_num, int value, const DataValue& src);
  inline void setInternalValue(int reg_num, int value, const DataValue& op1, const DataValue& op2);
  void checkWaitingThreads(int cur_thread, int reg_num);

  void ReadInst(Instruction in_inst);
  
  
  
  // ---------- Instruction Library -----------
  // Multi-threading
  bool Inst_ForkThread(cAvidaContext& ctx);
  bool Inst_ThreadCreate(cAvidaContext& ctx);
  bool Inst_ExitThread(cAvidaContext& ctx);
  bool Inst_IdThread(cAvidaContext& ctx);
  
  // Flow Control
  bool Inst_IfNEqu(cAvidaContext& ctx);
  bool Inst_IfLess(cAvidaContext& ctx);
  bool Inst_IfNotZero(cAvidaContext& ctx);
  bool Inst_IfEqualZero(cAvidaContext& ctx);
  bool Inst_IfGreaterThanZero(cAvidaContext& ctx);
  bool Inst_IfLessThanZero(cAvidaContext& ctx);
  bool Inst_IfGtrX(cAvidaContext& ctx);
  bool Inst_IfEquX(cAvidaContext& ctx);
  bool Inst_IfConsensus(cAvidaContext& ctx);
  bool Inst_IfConsensus24(cAvidaContext& ctx);
  bool Inst_IfLessConsensus(cAvidaContext& ctx);
  bool Inst_IfLessConsensus24(cAvidaContext& ctx);
  bool Inst_IfStackGreater(cAvidaContext& ctx);
  bool Inst_IfNest(cAvidaContext& ctx);
  bool Inst_Label(cAvidaContext& ctx);

  // Stack and Register Operations
  bool Inst_Pop(cAvidaContext& ctx);
  bool Inst_Push(cAvidaContext& ctx);
  bool Inst_PopAll(cAvidaContext& ctx);
  bool Inst_PushAll(cAvidaContext& ctx);
  bool Inst_SwitchStack(cAvidaContext& ctx);
  bool Inst_SwapStackTop(cAvidaContext& ctx);
  bool Inst_Swap(cAvidaContext& ctx);

  // Single-Argument Math
  bool Inst_ShiftR(cAvidaContext& ctx);
  bool Inst_ShiftL(cAvidaContext& ctx);
  bool Inst_Inc(cAvidaContext& ctx);
  bool Inst_Dec(cAvidaContext& ctx);
  bool Inst_Zero(cAvidaContext& ctx);
  bool Inst_One(cAvidaContext& ctx);
  bool Inst_Rand(cAvidaContext& ctx);
  bool Inst_Mult100(cAvidaContext& ctx);
  
  // Double Argument Math
  bool Inst_Add(cAvidaContext& ctx);
  bool Inst_Sub(cAvidaContext& ctx);
  bool Inst_Mult(cAvidaContext& ctx);
  bool Inst_Div(cAvidaContext& ctx);
  bool Inst_Mod(cAvidaContext& ctx);
  bool Inst_Nand(cAvidaContext& ctx);

  // I/O and Sensory
  bool Inst_TaskIO(cAvidaContext& ctx);
  bool Inst_TaskInput(cAvidaContext& ctx);
  bool Inst_TaskOutput(cAvidaContext& ctx);
  bool Inst_TaskOutputZero(cAvidaContext& ctx);

  // Head-based Instructions
  bool Inst_HeadAlloc(cAvidaContext& ctx);
  bool Inst_MoveHead(cAvidaContext& ctx);
  bool Inst_MoveHeadIfNEqu(cAvidaContext& ctx);
  bool Inst_MoveHeadIfLess(cAvidaContext& ctx);
  bool Inst_Goto(cAvidaContext& ctx);
  bool Inst_GotoIfNEqu(cAvidaContext& ctx);
  bool Inst_GotoIfLess(cAvidaContext& ctx);
  bool Inst_GotoConsensus(cAvidaContext& ctx);
  bool Inst_GotoConsensus24(cAvidaContext& ctx);
  bool Inst_JumpHead(cAvidaContext& ctx);
  bool Inst_GetHead(cAvidaContext& ctx);
  bool Inst_IfCopiedCompLabel(cAvidaContext& ctx);
  bool Inst_IfCopiedDirectLabel(cAvidaContext& ctx);
  bool Inst_IfCopiedCompSeq(cAvidaContext& ctx);
  bool Inst_IfCopiedDirectSeq(cAvidaContext& ctx);
  bool Inst_HeadDivide(cAvidaContext& ctx);
  bool Inst_HeadDivideSex(cAvidaContext& ctx);
  bool Inst_HeadRead(cAvidaContext& ctx);
  bool Inst_HeadWrite(cAvidaContext& ctx);
  bool Inst_HeadCopy(cAvidaContext& ctx);
  bool Inst_Search_Label_Comp_S(cAvidaContext& ctx);
  bool Inst_Search_Label_Comp_F(cAvidaContext& ctx);
  bool Inst_Search_Label_Comp_B(cAvidaContext& ctx);
  bool Inst_Search_Label_Direct_S(cAvidaContext& ctx);
  bool Inst_Search_Label_Direct_F(cAvidaContext& ctx);
  bool Inst_Search_Label_Direct_B(cAvidaContext& ctx);
  bool Inst_Search_Seq_Comp_S(cAvidaContext& ctx);
  bool Inst_Search_Seq_Comp_F(cAvidaContext& ctx);
  bool Inst_Search_Seq_Comp_B(cAvidaContext& ctx);
  bool Inst_Search_Seq_Direct_S(cAvidaContext& ctx);
  bool Inst_Search_Seq_Direct_F(cAvidaContext& ctx);
  bool Inst_Search_Seq_Direct_B(cAvidaContext& ctx);
  bool Inst_SetFlow(cAvidaContext& ctx);
  
  // Thread Execution Control
  bool Inst_WaitCondition_Equal(cAvidaContext& ctx);
  bool Inst_WaitCondition_Less(cAvidaContext& ctx);
  bool Inst_WaitCondition_Greater(cAvidaContext& ctx);
  

  // Bit Consensus
  bool Inst_BitConsensus(cAvidaContext& ctx);
  bool Inst_BitConsensus24(cAvidaContext& ctx);
  
  // Replication
  bool Inst_Repro(cAvidaContext& ctx);
  bool Inst_Die(cAvidaContext& ctx);
  
  // Movement and Navigation 
  bool Inst_Move(cAvidaContext& ctx);
  bool Inst_JuvMove(cAvidaContext& ctx);
  bool Inst_GetCellPosition(cAvidaContext& ctx);
  bool Inst_GetCellPositionX(cAvidaContext& ctx);
  bool Inst_GetCellPositionY(cAvidaContext& ctx);
  bool Inst_GetNorthOffset(cAvidaContext& ctx);  
  bool Inst_GetPositionOffset(cAvidaContext& ctx);  
  bool Inst_GetNortherly(cAvidaContext& ctx); 
  bool Inst_GetEasterly(cAvidaContext& ctx);
  bool Inst_ZeroEasterly(cAvidaContext& ctx);
  bool Inst_ZeroNortherly(cAvidaContext& ctx);
  bool Inst_ZeroPosOffset(cAvidaContext& ctx);
  
  // Rotation
  bool Inst_RotateLeftOne(cAvidaContext& ctx);
  bool Inst_RotateRightOne(cAvidaContext& ctx);
  bool Inst_RotateUphill(cAvidaContext& ctx);
  bool Inst_RotateUpFtHill(cAvidaContext& ctx);
  bool Inst_RotateHome(cAvidaContext& ctx);
  bool Inst_RotateUnoccupiedCell(cAvidaContext& ctx);
  bool Inst_RotateX(cAvidaContext& ctx);
  bool Inst_RotateDir(cAvidaContext& ctx);
  bool Inst_RotateOrgID(cAvidaContext& ctx);
  bool Inst_RotateAwayOrgID(cAvidaContext& ctx);

  
  // Resource and Topography Sensing
  bool Inst_SenseResourceID(cAvidaContext& ctx); 
  bool Inst_SenseResQuant(cAvidaContext& ctx); 
  bool Inst_SenseNest(cAvidaContext& ctx); 
  bool Inst_SenseResDiff(cAvidaContext& ctx); 
  bool Inst_SenseFacedHabitat(cAvidaContext& ctx);
  bool Inst_LookAhead(cAvidaContext& ctx);
  bool Inst_LookAheadIntercept(cAvidaContext& ctx);
  bool Inst_LookAround(cAvidaContext& ctx);
  bool Inst_LookAroundIntercept(cAvidaContext& ctx);
  bool Inst_LookFT(cAvidaContext& ctx);
  bool Inst_LookAroundFT(cAvidaContext& ctx);
  bool Inst_SetForageTarget(cAvidaContext& ctx);
  bool Inst_SetForageTargetOnce(cAvidaContext& ctx);
  bool Inst_SetRandForageTargetOnce(cAvidaContext& ctx);
  bool Inst_SetRandPFTOnce(cAvidaContext& ctx);
  bool Inst_GetForageTarget(cAvidaContext& ctx);
  bool Inst_ShowForageTarget(cAvidaContext& ctx);
  bool Inst_GetLocOrgDensity(cAvidaContext& ctx);
  bool Inst_GetFacedOrgDensity(cAvidaContext& ctx);
  
  bool DoActualCollect(cAvidaContext& ctx, int bin_used, bool unit);
  bool FakeActualCollect(cAvidaContext& ctx, int bin_used, bool unit);
  bool Inst_CollectEdible(cAvidaContext& ctx);
  bool Inst_CollectSpecific(cAvidaContext& ctx);
  bool Inst_DepositResource(cAvidaContext& ctx);
  bool Inst_DepositSpecific(cAvidaContext& ctx);
  bool Inst_DepositAllAsSpecific(cAvidaContext& ctx);
  bool Inst_NopDepositResource(cAvidaContext& ctx);
  bool Inst_NopDepositSpecific(cAvidaContext& ctx);    
  bool Inst_NopDepositAllAsSpecific(cAvidaContext& ctx);
  bool Inst_Nop2DepositAllAsSpecific(cAvidaContext& ctx);
  bool Inst_NopCollectEdible(cAvidaContext& ctx);
  bool Inst_Nop2CollectEdible(cAvidaContext& ctx);
  bool Inst_GetResStored(cAvidaContext& ctx);
  bool Inst_GetSpecificStored(cAvidaContext& ctx);

  // Org Interactions
  bool Inst_GetFacedOrgID(cAvidaContext& ctx);
  bool Inst_AttackPrey(cAvidaContext& ctx); 
  bool Inst_AttackPreyGroup(cAvidaContext& ctx);
  bool Inst_AttackPreyShare(cAvidaContext& ctx);
  bool Inst_AttackPreyNoShare(cAvidaContext& ctx);
  bool Inst_AttackPreyFakeShare(cAvidaContext& ctx);
  bool Inst_AttackPreyFakeGroupShare(cAvidaContext& ctx);
  bool Inst_AttackPreyGroupShare(cAvidaContext& ctx);
  bool Inst_AttackSpecPrey(cAvidaContext& ctx);
  bool Inst_AttackPreyArea(cAvidaContext& ctx);
  bool Inst_AttackFTPrey(cAvidaContext& ctx); 
  bool Inst_AttackPoisonPrey(cAvidaContext& ctx);
  bool Inst_AttackPoisonFTPrey(cAvidaContext& ctx);
  bool Inst_AttackPoisonFTPreyGenetic(cAvidaContext& ctx);
  bool Inst_AttackPoisonFTMixedPrey(cAvidaContext& ctx);
  bool Inst_FightMeritOrg(cAvidaContext& ctx); 
  bool Inst_FightBonusOrg(cAvidaContext& ctx); 
  bool Inst_GetMeritFightOdds(cAvidaContext& ctx); 
  bool Inst_FightOrg(cAvidaContext& ctx); 
  bool Inst_AttackPred(cAvidaContext& ctx); 
  bool Inst_KillPred(cAvidaContext& ctx); 
  bool Inst_FightPred(cAvidaContext& ctx); 
  bool Inst_MarkCell(cAvidaContext& ctx); 
  bool Inst_MarkGroupCell(cAvidaContext& ctx); 
  bool Inst_MarkPredCell(cAvidaContext& ctx); 
  bool Inst_ReadFacedCell(cAvidaContext& ctx); 
  bool Inst_ReadFacedPredCell(cAvidaContext& ctx); 
  bool Inst_TeachOffspring(cAvidaContext& ctx);
  bool Inst_LearnParent(cAvidaContext& ctx);
  
  bool Inst_SetGuard(cAvidaContext& ctx);
  bool Inst_SetGuardOnce(cAvidaContext& ctx);
  bool Inst_GetNumGuards(cAvidaContext& ctx);
  bool Inst_GetNumJuvs(cAvidaContext& ctx);
  
  bool Inst_ActivateDisplay(cAvidaContext& ctx);
  bool Inst_UpdateDisplay(cAvidaContext& ctx);
  bool Inst_ModifyDisplay(cAvidaContext& ctx);
  bool Inst_ReadLastSeenDisplay(cAvidaContext& ctx);
  bool Inst_KillDisplay(cAvidaContext& ctx);
  
  bool Inst_ModifySimpDisplay(cAvidaContext& ctx);
  bool Inst_ReadLastSimpDisplay(cAvidaContext& ctx);

  // Control-type Instructions
  bool Inst_ScrambleReg(cAvidaContext& ctx);
  
  bool Inst_GetFacedEditDistance(cAvidaContext& ctx);  

private:
  std::pair<bool, int> m_last_cell_data; // If cell data has been previously collected, and it's value
  
  // ---------- Some Instruction Helpers -----------
  inline const Apto::String& GetCurInstName() { return m_inst_set->GetName(m_threads[m_cur_thread].heads[HEAD_IP].GetInst()); }
  
  struct sLookRegAssign {
    int habitat;
    int distance;
    int search_type;
    int id_sought;
    int count;
    int value;
    int group;
    int ft;
  };
  
  struct sAttackReg {
    int success_reg;
    int bonus_reg;
    int bin_reg;
  };
  
  struct sAttackResult {
    unsigned int inst:1;     // 0 == solo attack inst, 1 == group attack inst
    unsigned int share:2;    // 0 == no, 1 == yes, 2 == fake
    unsigned int success:2;  // 0 == sucess, 1 == no prey failure, 2 == no friends failure, 3 == chance failure
    unsigned int size:3;     // potential group size, not including self
  };
  
  bool GoLook(cAvidaContext& ctx, const int look_dir, const int cell_id, bool use_ft = false);
  Features::VisualSensor::LookResults InitLooking(cAvidaContext& ctx, sLookRegAssign& lookin_defs, int facing, int cell_id, bool use_ft = false);
  void LookResults(cAvidaContext& ctx, sLookRegAssign& lookin_defs, Features::VisualSensor::LookResults& look_results);
  
  void InjureOrg(cOrganism* target);
  void MakePred(cAvidaContext& ctx);
  void MakeTopPred(cAvidaContext& ctx);
  bool TestAttack(cAvidaContext& ctx);
  bool TestAttackPred(cAvidaContext& ctx);
  cOrganism* GetPreyTarget(cAvidaContext& ctx);
  bool TestPreyTarget(cOrganism* target);
  void SetAttackReg(sAttackReg& reg);
  bool ExecuteAttack(cAvidaContext& ctx, cOrganism* target, sAttackReg& reg, double odds = -1);
  bool ExecuteShareAttack(cAvidaContext& ctx, cOrganism* target, sAttackReg& reg, Apto::Array<cOrganism*>& pack, double odds = -1);
  bool ExecuteFakeShareAttack(cAvidaContext& ctx, cOrganism* target, sAttackReg& reg, double share, double odds = -1);
  bool ExecutePoisonPreyAttack(cAvidaContext& ctx, cOrganism* target, sAttackReg& reg, double odds = -1);
  
  bool TestAttackResultsOut(sAttackResult& results);
  bool TestAttackChance(cAvidaContext& ctx, cOrganism* target, sAttackReg& reg, double odds = -1);
  void ApplyKilledPreyMerit(cOrganism* target, double effic);
  void ApplyKilledPreyReactions(cOrganism* target);
  void ApplyKilledPreyBonus(cOrganism* target, sAttackReg& reg, double effic);
  void ApplyKilledPreyResBins(cOrganism* target, sAttackReg& reg, double effic);

  void ApplySharedKilledPreyMerit(cOrganism* target, double effic, cOrganism* org, double share);
  void ApplySharedKilledPreyBonus(cOrganism* target, sAttackReg& reg, double effic, cOrganism* org, double share);
  void ApplySharedKilledPreyResBins(cOrganism* target, sAttackReg& reg, double effic, cOrganism* org, double share);

  Apto::Array<cOrganism*> GetPredGroupAttackNeighbors();
  Apto::Array<cOrganism*> GetPredSameGroupAttackNeighbors();
  void TryPreyClone(cAvidaContext& ctx);
  void UpdateGroupAttackStats(const cString& inst, sAttackResult& result, bool get_size = true);
  void TryWriteGroupAttackBits(unsigned char raw_bits);
  void TryWriteGroupAttackString(cString& string);
};

inline cHardwareExperimental::DataValue& cHardwareExperimental::DataValue::operator=(const DataValue& i)
{
  value = i.value;
  originated = i.originated;
  from_env = i.from_env;
  oldest_component = i.oldest_component;
  env_component = i.env_component;
  return *this;
}

inline void cHardwareExperimental::ThreadPrev()
{
  if (m_cur_thread == 0) m_cur_thread = m_threads.GetSize() - 1;
  else m_cur_thread--;
}

inline cHardwareExperimental::DataValue cHardwareExperimental::stackPop()
{
  if (m_threads[m_cur_thread].cur_stack == 0) {
    return m_threads[m_cur_thread].stack.Pop();
  } else {
    return m_global_stack.Pop();
  }
}


inline cHardwareExperimental::Stack& cHardwareExperimental::getStack(int stack_id)
{
  if (stack_id == 0) {
    return m_threads[m_cur_thread].stack;
  } else {
    return m_global_stack;
  }
}


inline void cHardwareExperimental::switchStack()
{
  m_threads[m_cur_thread].cur_stack++;
  if (m_threads[m_cur_thread].cur_stack > 1) m_threads[m_cur_thread].cur_stack = 0;
}


inline int cHardwareExperimental::GetStack(int depth, int stack_id, int in_thread) const
{
  DataValue value;

  if(in_thread >= m_threads.GetSize() || in_thread < 0) in_thread = m_cur_thread;

  if (stack_id == -1) stack_id = m_threads[in_thread].cur_stack;

  if (stack_id == 0) value = m_threads[in_thread].stack.Get(depth);
  else if (stack_id == 1) value = m_global_stack.Get(depth);

  return value.value;
}

inline void cHardwareExperimental::setInternalValue(int reg_num, int value, bool from_env, bool from_sensor)
{
  DataValue& dest = m_threads[m_cur_thread].reg[reg_num];
  dest.value = value;
  dest.from_env = from_env;
  dest.originated = m_cycle_count;
  dest.oldest_component = m_cycle_count;
  dest.env_component = from_env;
  dest.from_sensor = from_sensor;
  dest.sensor_component = from_sensor;
  if (m_waiting_threads) checkWaitingThreads(m_cur_thread, reg_num);
}


inline void cHardwareExperimental::setInternalValue(int reg_num, int value, const DataValue& src)
{
  DataValue& dest = m_threads[m_cur_thread].reg[reg_num];
  dest.value = value;
  dest.from_env = false;
  dest.from_sensor = false;
  dest.originated = m_cycle_count;
  dest.oldest_component = src.oldest_component;
  dest.env_component = src.env_component;
  dest.sensor_component = src.sensor_component;
  if (m_waiting_threads) checkWaitingThreads(m_cur_thread, reg_num);
}


inline void cHardwareExperimental::setInternalValue(int reg_num, int value, const DataValue& op1, const DataValue& op2)
{
  DataValue& dest = m_threads[m_cur_thread].reg[reg_num];
  dest.value = value;
  dest.from_env = false;
  dest.from_sensor = false;
  dest.originated = m_cycle_count;
  dest.oldest_component = (op1.oldest_component < op2.oldest_component) ? op1.oldest_component : op2.oldest_component;
  dest.env_component = (op1.env_component || op2.env_component);
  dest.sensor_component = (op1.sensor_component || op2.sensor_component);
  if (m_waiting_threads) checkWaitingThreads(m_cur_thread, reg_num);
}


#endif