#ifndef ALIFE2018_EXPERIMENT_H
#define ALIFE2018_EXPERIMENT_H

// @includes
#include <iostream>
#include <string>
#include <utility>
#include <fstream>
#include <sys/stat.h>
#include <algorithm>
#include <functional>
#include <deque>
#include <unordered_set>

#include "base/Ptr.h"  
#include "base/vector.h"
#include "control/Signal.h"
#include "Evolve/World.h"
#include "Evolve/Resource.h"
#include "Evolve/SystematicsAnalysis.h"
#include "Evolve/World_output.h"
#include "hardware/EventDrivenGP.h"
#include "hardware/InstLib.h"
#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/math.h"
#include "tools/string_utils.h"

#include "consensus-config.h"
#include "SGPDeme.h"

constexpr size_t RUN_ID__EXP = 0;
constexpr size_t RUN_ID__ANALYSIS = 1;

constexpr size_t TAG_WIDTH = 16;

constexpr uint32_t MIN_UID = 1;
constexpr uint32_t MAX_UID = 1000000000;

constexpr size_t TRAIT_ID__DEME_ID = 0;
constexpr size_t TRAIT_ID__UID = 1;
constexpr size_t TRAIT_ID__DIR = 2;
constexpr size_t TRAIT_ID__OPINION = 3;

constexpr size_t MSG_ID__DELAY = 512;

constexpr uint32_t NO_VOTE = 0;

/// Class to manage ALIFE2018 changing environment (w/logic 9) experiments.
class Experiment {
public:
  // Forward declarations.
  struct Agent;
  class ConsensusDeme;

  // Hardware/agent aliases.
  using hardware_t = emp::EventDrivenGP_AW<TAG_WIDTH>;
  using program_t = hardware_t::Program;
  using state_t = hardware_t::State;
  using inst_t = hardware_t::inst_t;
  using inst_lib_t = hardware_t::inst_lib_t;
  using event_t = hardware_t::event_t;
  using event_lib_t = hardware_t::event_lib_t;
  using memory_t = hardware_t::memory_t;
  using tag_t = hardware_t::affinity_t;
  using exec_stk_t = hardware_t::exec_stk_t;
  // Deme alias
  using deme_t = ConsensusDeme;
  // World alias
  using world_t = emp::World<Agent>;
    // Task aliases
  using task_io_t = uint32_t;

  /// Agent to be evolved.
  struct Agent {
    size_t agent_id;
    program_t program;

    Agent(const program_t & _p) : agent_id(0), program(_p) { ; }
    Agent(const Agent && in) : agent_id(in.GetID()), program(in.program) { ; }
    Agent(const Agent & in): agent_id(in.GetID()), program(in.program) { ; }

    size_t GetID() const { return agent_id; }
    void SetID(size_t id) { agent_id = id; }

    program_t & GetGenome() { return program; }

  };

  /// Wrapper around SGPDeme that includes useful propagule/activation functions.
  class ConsensusDeme : public SGPDeme {
  public:
    using grid_t = SGPDeme::grid_t;
    using hardware_t = SGPDeme::hardware_t;
    using SGPDeme::random;
    using SGPDeme::grid;
    using SGPDeme::on_deme_single_advance_sig;

  protected:

    size_t phen_id;
    std::unordered_set<uint32_t> uids;
    std::unordered_multiset<uint32_t> valid_votes; ///< Maintains the number of valid votes present in the deme at the end of the most recent deme-update.
    size_t max_vote_cnt;                           ///< Highest vote count for any single valid UID.
    size_t valid_vote_cnt;                         ///< Maintains total number of valid votes.

    uint32_t min_uid;
    uint32_t max_uid;
    uint32_t leader_uid;

  public:
    ConsensusDeme(size_t _w, size_t _h, emp::Ptr<emp::Random> _rnd, emp::Ptr<inst_lib_t> _ilib, emp::Ptr<event_lib_t> _elib)
    : SGPDeme(_w, _h, _rnd, _ilib, _elib), phen_id(0), uids(), valid_votes(), max_vote_cnt(0), min_uid(0), max_uid(0)
    {
      for (size_t i = 0; i < grid.size(); ++i) {
        grid[i].SetTrait(TRAIT_ID__DEME_ID, i);
        grid[i].SetTrait(TRAIT_ID__UID, i+1);
        uids.emplace(i+1);
      }

      on_deme_single_advance_sig.AddAction([this]() {
        valid_votes.clear();
        valid_vote_cnt = 0;
        max_vote_cnt = 0;
      });
    }

    size_t GetPhenID() const { return phen_id; }
    void SetPhenID(size_t id) { phen_id = id; }

    size_t GetMaxVoteCnt() const { return max_vote_cnt; }
    size_t GetValidVoteCnt() const { return valid_vote_cnt; }
    uint32_t GetLargestUID() const { return max_uid; }
    uint32_t GetSmallestUID() const { return min_uid; }
    uint32_t GetLeaderUID() const { return leader_uid; }

    void TallyVote(uint32_t vote) {
       if (emp::Has(uids, vote)) {
        valid_vote_cnt++;
        // If so, add i's vote.
        valid_votes.emplace(vote);
        size_t cnt = valid_votes.count(vote);
        if (cnt > max_vote_cnt) {
          max_vote_cnt = cnt;
          leader_uid = vote;
        }
      }
    }

    /// Randomize unique identifiers for each agent (range of each: [MIN_UID:MAX_UID]). Function
    /// ensures uniqueness.
    void RandomizeUIDS() {
      emp_assert(MAX_UID - MIN_UID > grid.size());
      uids.clear();
      valid_votes.clear();
      max_vote_cnt = 0;
      valid_vote_cnt = 0;
      min_uid = MAX_UID;
      max_uid = 0;
      leader_uid = 0;
      for (size_t i = 0; i < grid.size(); ++i) {
        size_t val = random->GetUInt(MIN_UID, MAX_UID);
        while (emp::Has(uids, val)) { val = random->GetUInt(MIN_UID, MAX_UID); }
        grid[i].SetTrait(TRAIT_ID__UID, val);
        uids.emplace(val);
        if (val < min_uid) min_uid = val;
        if (val > max_uid) max_uid = val;
      }
    }
  };


  /// Struct used to keep track of deme phenotype information.
  struct Phenotype {
    double score;
    size_t total_full_consensus_time;  ///< Number of time steps that full consensus is maintained.
    size_t mr_full_consensus_time;
    size_t max_consensus_size;
    size_t valid_vote_cnt;        ///< How many valid votes at end of evaluation?
    size_t msgs_exchanged;
    uint32_t min_uid;
    uint32_t max_uid;
    uint32_t leader_uid;


    Phenotype()
      : score(0)
    { ; }

    double GetScore() const { return score; }

    void Reset() {
      score = 0;
      total_full_consensus_time = 0;
      mr_full_consensus_time = 0;
      max_consensus_size = 0;
      valid_vote_cnt = 0;
      msgs_exchanged = 0;
      min_uid = 0;
      max_uid = 0;
      leader_uid = 0;
    }
  };


protected:
  // == Configurable experiment parameters ==
  size_t RUN_MODE;
  int RANDOM_SEED;
  size_t POP_SIZE;
  size_t GENERATIONS;
  size_t EVAL_TIME;
  size_t TRIAL_CNT;
  std::string ANCESTOR_FPATH;
  size_t DEME_WIDTH;
  size_t DEME_HEIGHT;
  size_t INBOX_CAPACITY;
  size_t TOURNAMENT_SIZE;
  size_t SELECTION_METHOD;
  size_t ELITE_SELECT__ELITE_CNT;
  size_t SGP_PROG_MAX_FUNC_CNT;
  size_t SGP_PROG_MIN_FUNC_CNT;
  size_t SGP_PROG_MAX_FUNC_LEN;
  size_t SGP_PROG_MIN_FUNC_LEN;
  size_t SGP_PROG_MAX_TOTAL_LEN;
  bool SGP_HW_EVENT_DRIVEN;
  size_t SGP_HW_ED_MSG_DELAY;
  bool SGP_HW_FORK_ON_MSG;
  size_t SGP_HW_MAX_CORES;
  size_t SGP_HW_MAX_CALL_DEPTH;
  double SGP_HW_MIN_BIND_THRESH;
  size_t SGP__PROG_MAX_ARG_VAL;
  double SGP__PER_BIT__TAG_BFLIP_RATE;
  double SGP__PER_INST__SUB_RATE;
  double SGP__PER_INST__INS_RATE;
  double SGP__PER_INST__DEL_RATE;
  double SGP__PER_FUNC__SLIP_RATE;
  double SGP__PER_FUNC__FUNC_DUP_RATE;
  double SGP__PER_FUNC__FUNC_DEL_RATE;
  size_t SYSTEMATICS_INTERVAL;
  size_t FITNESS_INTERVAL;
  size_t POP_SNAPSHOT_INTERVAL;
  std::string DATA_DIRECTORY;

  size_t DEME_SIZE;

  emp::Ptr<emp::Random> random;
  emp::Ptr<world_t> world;

  emp::Ptr<inst_lib_t> inst_lib;
  emp::Ptr<event_lib_t> event_lib;
  emp::Ptr<deme_t> eval_deme;
  
  using inbox_t = std::deque<event_t>;
  emp::vector<inbox_t> inboxes;

  size_t update;
  size_t eval_time;

  size_t dom_agent_id;

  emp::vector<Phenotype> agent_phen_cache;

  // Run signals.
  emp::Signal<void(void)> do_begin_run_setup_sig;   ///< Triggered at begining of run. Shared between AGP and SGP
  emp::Signal<void(void)> do_pop_init_sig;          ///< Triggered during run setup. Defines way population is initialized.
  emp::Signal<void(void)> do_evaluation_sig;        ///< Triggered during run step. Should trigger population-wide agent evaluation.
  emp::Signal<void(void)> do_selection_sig;         ///< Triggered during run step. Should trigger selection (which includes selection, reproduction, and mutation).
  emp::Signal<void(void)> do_world_update_sig;      ///< Triggered during run step. Should trigger world->Update(), and whatever else should happen right before/after population turnover.
  emp::Signal<void(void)> do_analysis_sig;
  // Systematics signals.
  emp::Signal<void(size_t)> do_pop_snapshot_sig;    ///< Triggered if we should take a snapshot of the population (as defined by POP_SNAPSHOT_INTERVAL). Should call appropriate functions to take snapshot.
  // Agent signals.
  emp::Signal<void(Agent &)> begin_agent_eval_sig;
  
  std::function<double(Agent &)> calc_score;


  // Some inbox utilities.
  void ResetInboxes() {
    for (size_t i = 0; i < inboxes.size(); ++i) inboxes[i].clear();
  }

  void ResetInbox(size_t id) {
    emp_assert(id < inboxes.size());
    inboxes[id].clear();
  }

  inbox_t & GetInbox(size_t id) {
    emp_assert(id < inboxes.size());
    return inboxes[id];
  }

  bool InboxFull(size_t id) const { 
    emp_assert(id < inboxes.size());
    return inboxes[id].size() >= INBOX_CAPACITY; 
  }

  bool InboxEmpty(size_t id) const {
    emp_assert(id < inboxes.size());
    return inboxes[id].empty();
  }

  // Deliver message (event) to specified inbox. 
  // Make room by clearing out old messages (back of deque). 
  void DeliverToInbox(size_t id, const event_t & event) {
    emp_assert(id < inboxes.size());
    while (InboxFull(id)) inboxes[id].pop_back();
    inboxes[id].emplace_front(event);
  }  

  /// NOTE: Re-use inbox for costly event-driven messaging.
  void DelayDelivery(size_t id, const event_t & event) {
    emp_assert(id < inboxes.size());
    inboxes[id].emplace_back(event);
    event_t & mut_event = inboxes[id].back();
    mut_event.msg[MSG_ID__DELAY] = SGP_HW_ED_MSG_DELAY;
  }

  void DoDelayDeliver(size_t id) {
    emp_assert(id < inboxes.size());
    // 1) Consume all msgs ready for delivery.
    while (!inboxes[id].empty()) {
      event_t & event = inboxes[id].front();
      if (event.msg[MSG_ID__DELAY] == 0) {
        // Queue event.
        eval_deme->GetHardware(id).QueueEvent(event);
        // Pop front. 
        inboxes[id].pop_front();
      } else {
        break;
      }
    }
    // 2) Update timers for rest of msgs. 
    for (event_t & event : inboxes[id]) { // Loop over rest of msgs, subtracting one from timer.
      event.msg[MSG_ID__DELAY] -= 1;
    }
  }

  /// Evaluate agent on deme. 
  void Evaluate(Agent & agent) {
    const size_t id = agent.GetID();
    begin_agent_eval_sig.Trigger(agent);
    size_t full_consensus_time = 0; 
    size_t mr_full_consensus_time = 0;
    for (eval_time = 0; eval_time < EVAL_TIME; ++eval_time) {
      eval_deme->SingleAdvance();
      if (eval_deme->GetMaxVoteCnt() == DEME_SIZE) {
        ++full_consensus_time;
        ++mr_full_consensus_time;
      } else {
        mr_full_consensus_time = 0;
      }
    }
    // Record phenotype information.
    Phenotype & phen = agent_phen_cache[id];
    phen.total_full_consensus_time = full_consensus_time;  ///< Number of time steps that full consensus is maintained.
    phen.mr_full_consensus_time = mr_full_consensus_time;
    phen.max_consensus_size = eval_deme->GetMaxVoteCnt();
    phen.valid_vote_cnt = eval_deme->GetValidVoteCnt();        ///< How many valid votes at end of evaluation?
    phen.min_uid = eval_deme->GetSmallestUID();
    phen.max_uid = eval_deme->GetLargestUID();
    phen.leader_uid = eval_deme->GetLeaderUID();
    phen.score = calc_score(agent);
  }

  /// Test function.
  /// Exists to test features as I add them.
  void Test() {
    std::cout << "Running tests!" << std::endl;
    // Test propragule activation.
    // 1) Load an ancestor program.
    do_pop_init_sig.Trigger();
    Agent & agent = world->GetOrg(0);
    std::cout << "---- TEST PROGRAM ----" << std::endl;
    agent.GetGenome().PrintProgramFull();
    std::cout << "----------------------" << std::endl;

    agent.SetID(0);
    eval_deme->SetProgram(agent.GetGenome());
    eval_deme->SetPhenID(0);
    Phenotype & phen = agent_phen_cache[0];
    phen.Reset();
    
    std::cout << "Before begin-agent-eval signal!" << std::endl;
    eval_deme->PrintState();
    begin_agent_eval_sig.Trigger(agent);
    std::cout << "Post begin-agent-eval signal!" << std::endl;
    eval_deme->PrintState();
    std::cout << "------ RUNNING! ------" << std::endl;

    size_t full_consensus_time = 0; 
    size_t mr_full_consensus_time = 0;
    for (eval_time = 0; eval_time < EVAL_TIME; ++eval_time) {
      eval_deme->SingleAdvance();
      if (eval_deme->GetMaxVoteCnt() == DEME_SIZE) {
        ++full_consensus_time;
        ++mr_full_consensus_time;
      } else {
        mr_full_consensus_time = 0;
      }
      std::cout << "=========================== TIME: " << eval_time << " ===========================" << std::endl;
            
      // Print inbox sizes
      std::cout << "Inbox cnts: [";
      for (size_t i = 0; i < inboxes.size(); ++i) {
        std::cout << " " << i << ":" << inboxes[i].size();
      } std::cout << "]" << std::endl;
      
      // Print Phenotype info
      std::cout << "PHENOTYPE INFORMATION" << std::endl;
      std::cout << "Max consensus size: " << eval_deme->GetMaxVoteCnt() << std::endl;
      std::cout << "Valid votes: " << eval_deme->GetValidVoteCnt() << std::endl;

      eval_deme->PrintState();
    }
    // Record phenotype information.
    phen.total_full_consensus_time = full_consensus_time;  ///< Number of time steps that full consensus is maintained.
    phen.mr_full_consensus_time = mr_full_consensus_time;
    phen.max_consensus_size = eval_deme->GetMaxVoteCnt();
    phen.valid_vote_cnt = eval_deme->GetValidVoteCnt();        ///< How many valid votes at end of evaluation?
    phen.min_uid = eval_deme->GetSmallestUID();
    phen.max_uid = eval_deme->GetLargestUID();
    phen.leader_uid = eval_deme->GetLeaderUID();
    phen.score = calc_score(agent);
    std::cout << "DONE EVALUATING DEME" << std::endl;

    // Print Phenotype info
    std::cout << "PHENOTYPE INFORMATION" << std::endl;
    std::cout << "Score: " << phen.score << std::endl;
    std::cout << "Total time at consensus: " << phen.total_full_consensus_time << std::endl;
    std::cout << "Most recent consensus streak: " << phen.mr_full_consensus_time << std::endl;
    std::cout << "Max consensus size (at end): " << phen.max_consensus_size << std::endl;
    std::cout << "Valid votes: " << phen.valid_vote_cnt << std::endl;
    std::cout << "Min UID: " << phen.min_uid << std::endl;
    std::cout << "Max UID: " << phen.max_uid << std::endl;
    std::cout << "Majority UID: " << phen.leader_uid << std::endl;
    std::cout << "Message count: " << phen.msgs_exchanged << std::endl;

    exit(-1);
  }

public:
  Experiment(const ConsensusConfig & config)
    : DEME_SIZE(0), inboxes(0),
      update(0), eval_time(0), dom_agent_id(0)
  {
    RUN_MODE = config.RUN_MODE();
    RANDOM_SEED = config.RANDOM_SEED();
    POP_SIZE = config.POP_SIZE();
    GENERATIONS = config.GENERATIONS();
    EVAL_TIME = config.EVAL_TIME();
    TRIAL_CNT = config.TRIAL_CNT();
    DEME_WIDTH = config.DEME_WIDTH();
    DEME_HEIGHT = config.DEME_HEIGHT();
    INBOX_CAPACITY = config.INBOX_CAPACITY();
    ANCESTOR_FPATH = config.ANCESTOR_FPATH();
    TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
    SELECTION_METHOD = config.SELECTION_METHOD();
    ELITE_SELECT__ELITE_CNT = config.ELITE_SELECT__ELITE_CNT();
    SGP_PROG_MAX_FUNC_CNT = config.SGP_PROG_MAX_FUNC_CNT();
    SGP_PROG_MIN_FUNC_CNT = config.SGP_PROG_MIN_FUNC_CNT();
    SGP_PROG_MAX_FUNC_LEN = config.SGP_PROG_MAX_FUNC_LEN();
    SGP_PROG_MIN_FUNC_LEN = config.SGP_PROG_MIN_FUNC_LEN();
    SGP_PROG_MAX_TOTAL_LEN = config.SGP_PROG_MAX_TOTAL_LEN();
    SGP_HW_EVENT_DRIVEN = config.SGP_HW_EVENT_DRIVEN();
    SGP_HW_ED_MSG_DELAY = config.SGP_HW_ED_MSG_DELAY();
    SGP_HW_FORK_ON_MSG = config.SGP_HW_FORK_ON_MSG();
    SGP_HW_MAX_CORES = config.SGP_HW_MAX_CORES();
    SGP_HW_MAX_CALL_DEPTH = config.SGP_HW_MAX_CALL_DEPTH();
    SGP_HW_MIN_BIND_THRESH = config.SGP_HW_MIN_BIND_THRESH();
    SGP__PROG_MAX_ARG_VAL = config.SGP__PROG_MAX_ARG_VAL();
    SGP__PER_BIT__TAG_BFLIP_RATE = config.SGP__PER_BIT__TAG_BFLIP_RATE();
    SGP__PER_INST__SUB_RATE = config.SGP__PER_INST__SUB_RATE();
    SGP__PER_INST__INS_RATE = config.SGP__PER_INST__INS_RATE();
    SGP__PER_INST__DEL_RATE = config.SGP__PER_INST__DEL_RATE();
    SGP__PER_FUNC__SLIP_RATE = config.SGP__PER_FUNC__SLIP_RATE();
    SGP__PER_FUNC__FUNC_DUP_RATE = config.SGP__PER_FUNC__FUNC_DUP_RATE();
    SGP__PER_FUNC__FUNC_DEL_RATE = config.SGP__PER_FUNC__FUNC_DEL_RATE();
    SYSTEMATICS_INTERVAL = config.SYSTEMATICS_INTERVAL();
    FITNESS_INTERVAL = config.FITNESS_INTERVAL();
    POP_SNAPSHOT_INTERVAL = config.POP_SNAPSHOT_INTERVAL();
    DATA_DIRECTORY = config.DATA_DIRECTORY();

    DEME_SIZE = DEME_WIDTH*DEME_HEIGHT;

    // Make the random number generator.
    random = emp::NewPtr<emp::Random>(RANDOM_SEED);

    // Make the world!
    world = emp::NewPtr<world_t>(random, "World");

    // Build phenotype cache.
    agent_phen_cache.resize(POP_SIZE);
    for (size_t i = 0; i < agent_phen_cache.size(); ++i) {
      Phenotype & phen = agent_phen_cache[i];
      phen.Reset();
    }

    // Make inst/event libraries.
    inst_lib = emp::NewPtr<inst_lib_t>();
    event_lib = emp::NewPtr<event_lib_t>();

    // Configure hardware/instruction libs.
    Config_HW();

    // Configure given run mode.
    switch (RUN_MODE) {
      case RUN_ID__EXP:
        Config_Run();
        break;
      case RUN_ID__ANALYSIS:
        Config_Analysis();
        break;
    }
    // Test();
  }

  ~Experiment() {
    world.Delete();
    eval_deme.Delete();
    inst_lib.Delete();
    event_lib.Delete();
    random.Delete();
  }

  void Run() {
    switch (RUN_MODE) {
      case RUN_ID__EXP:
        do_begin_run_setup_sig.Trigger();
        for (update = 0; update <= GENERATIONS; ++update) {
          RunStep();
          if (update % POP_SNAPSHOT_INTERVAL == 0) do_pop_snapshot_sig.Trigger(update);
        }
        break;
      case RUN_ID__ANALYSIS:
        std::cout << "Analysis mode not implemented yet..." << std::endl;
        exit(-1);
        do_analysis_sig.Trigger();
        break;
      default:
        std::cout << "Unrecognized run mode! Exiting..." << std::endl;
        exit(-1);
    }
  }

  void RunStep() {
    do_evaluation_sig.Trigger();
    do_selection_sig.Trigger();
    do_world_update_sig.Trigger();
  }

  void Config_HW();
  void Config_Run();
  void Config_Analysis();

  size_t Mutate(Agent & agent, emp::Random & rnd);
  double CalcFitness(Agent & agent) { return agent_phen_cache[agent.GetID()].GetScore(); } ;

  void InitPopulation_FromAncestorFile();
  void Snapshot_SingleFile(size_t update);

  emp::DataFile & AddDominantFile(const std::string & fpath="dominant.csv");

  // Instructions
  // (execution control)
  static void Inst_Fork(hardware_t & hw, const inst_t & inst);
  static void Inst_Terminate(hardware_t & hw, const inst_t & inst);
  static void Inst_Nand(hardware_t & hw, const inst_t & inst);

  //   - Orientation
  static void Inst_RotCW(hardware_t & hw, const inst_t & inst);
  static void Inst_RotCCW(hardware_t & hw, const inst_t & inst);
  static void Inst_RandomDir(hardware_t & hw, const inst_t & inst);
  static void Inst_GetDir(hardware_t & hw, const inst_t & inst);
  //   - Messaging
  static void Inst_SendMsgFacing(hardware_t & hw, const inst_t & inst);
  static void Inst_BroadcastMsg(hardware_t & hw, const inst_t & inst);
  void Inst_RetrieveMsg(hardware_t & hw, const inst_t & inst);
  //   - Voting
  static void Inst_GetUID(hardware_t & hw, const inst_t & inst);
  static void Inst_GetOpinion(hardware_t & hw, const inst_t & inst);
  static void Inst_SetOpinion(hardware_t & hw, const inst_t & inst);
  
  // Events
  void EventDriven__DispatchMessage_Send(hardware_t & hw, const event_t & event);
  void EventDriven__DispatchMessage_Broadcast(hardware_t & hw, const event_t & event);
  void EventDriven_Delay__DispatchMessage_Send(hardware_t & hw, const event_t & event);
  void EventDriven_Delay__DispatchMessage_Broadcast(hardware_t & hw, const event_t & event);
  void Imperative__DispatchMessage_Send(hardware_t & hw, const event_t & event);
  void Imperative__DispatchMessage_Broadcast(hardware_t & hw, const event_t & event);
  static void HandleEvent__Message_Forking(hardware_t & hw, const event_t & event);
  static void HandleEvent__Message_NonForking(hardware_t & hw, const event_t & event);
};

// --- Instruction implementations ---
void Experiment::Inst_GetUID(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetTrait(TRAIT_ID__UID));
}

void Experiment::Inst_GetOpinion(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetTrait(TRAIT_ID__OPINION));
}

void Experiment::Inst_SetOpinion(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  double val = state.AccessLocal(inst.args[0]);
  if (val > 0) hw.SetTrait(TRAIT_ID__OPINION, (uint32_t)val);
}

/// Instruction: Fork
/// Description: Fork thread with local memory as new thread's input buffer.
void Experiment::Inst_Fork(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  hw.SpawnCore(inst.affinity, hw.GetMinBindThresh(), state.local_mem);
} 

void Experiment::Inst_Terminate(hardware_t & hw, const inst_t & inst) {
  // Pop all the call states from current core.
  exec_stk_t & core = hw.GetCurCore();
  core.resize(0);
}

void Experiment::Inst_Nand(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  const task_io_t a = (task_io_t)state.GetLocal(inst.args[0]);
  const task_io_t b = (task_io_t)state.GetLocal(inst.args[1]);
  state.SetLocal(inst.args[2], ~(a&b));
}

void Experiment::Inst_RotCW(hardware_t & hw, const inst_t & inst) {
  hw.SetTrait(TRAIT_ID__DIR, emp::Mod(hw.GetTrait(TRAIT_ID__DIR) - 1, deme_t::NUM_DIRS));
}

void Experiment::Inst_RotCCW(hardware_t & hw, const inst_t & inst) {
  hw.SetTrait(TRAIT_ID__DIR, emp::Mod(hw.GetTrait(TRAIT_ID__DIR) + 1, deme_t::NUM_DIRS));
}

void Experiment::Inst_RandomDir(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetRandom().GetUInt(0, deme_t::NUM_DIRS));
}

void Experiment::Inst_GetDir(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  state.SetLocal(inst.args[0], hw.GetTrait(TRAIT_ID__DIR));
}

void Experiment::Inst_SendMsgFacing(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  hw.TriggerEvent("SendMessage", inst.affinity, state.output_mem);
}

void Experiment::Inst_BroadcastMsg(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  hw.TriggerEvent("BroadcastMessage", inst.affinity, state.output_mem);
}

void Experiment::Inst_RetrieveMsg(hardware_t & hw, const inst_t & inst) {
  const size_t loc_id = (size_t)hw.GetTrait(TRAIT_ID__DEME_ID);
  if (!InboxEmpty(loc_id)) {
    inbox_t & inbox = GetInbox(loc_id);
    hw.HandleEvent(inbox.front());
    inbox.pop_front(); // Remove!
  }
}

void Experiment::EventDriven__DispatchMessage_Send(hardware_t & hw, const event_t & event) {
  const size_t facing_id = eval_deme->GetNeighborID((size_t)hw.GetTrait(TRAIT_ID__DEME_ID), (size_t)hw.GetTrait(TRAIT_ID__DIR));
  hardware_t & rHW = eval_deme->GetHardware(facing_id);
  rHW.QueueEvent(event);
  agent_phen_cache[eval_deme->GetPhenID()].msgs_exchanged++;
}

void Experiment::EventDriven__DispatchMessage_Broadcast(hardware_t & hw, const event_t & event) {
  const size_t loc_id = (size_t)hw.GetTrait(TRAIT_ID__DEME_ID);
  const size_t uid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_UP);
  const size_t did = eval_deme->GetNeighborID(loc_id, deme_t::DIR_DOWN);
  const size_t lid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_LEFT);
  const size_t rid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_RIGHT);
  eval_deme->GetHardware(uid).QueueEvent(event);  
  eval_deme->GetHardware(did).QueueEvent(event);
  eval_deme->GetHardware(lid).QueueEvent(event);
  eval_deme->GetHardware(rid).QueueEvent(event);  
  agent_phen_cache[eval_deme->GetPhenID()].msgs_exchanged += 4;
}

void Experiment::EventDriven_Delay__DispatchMessage_Send(hardware_t & hw, const event_t & event) {
  const size_t facing_id = eval_deme->GetNeighborID((size_t)hw.GetTrait(TRAIT_ID__DEME_ID), (size_t)hw.GetTrait(TRAIT_ID__DIR));
  DelayDelivery(facing_id, event);
  agent_phen_cache[eval_deme->GetPhenID()].msgs_exchanged++;
}

void Experiment::EventDriven_Delay__DispatchMessage_Broadcast(hardware_t & hw, const event_t & event) {
  const size_t loc_id = (size_t)hw.GetTrait(TRAIT_ID__DEME_ID);
  const size_t uid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_UP);
  const size_t did = eval_deme->GetNeighborID(loc_id, deme_t::DIR_DOWN);
  const size_t lid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_LEFT);
  const size_t rid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_RIGHT);
  DelayDelivery(uid, event);
  DelayDelivery(did, event);
  DelayDelivery(lid, event);
  DelayDelivery(rid, event);
  agent_phen_cache[eval_deme->GetPhenID()].msgs_exchanged += 4;
}

void Experiment::Imperative__DispatchMessage_Send(hardware_t & hw, const event_t & event) {
  const size_t facing_id = eval_deme->GetNeighborID(hw.GetTrait(TRAIT_ID__DEME_ID), hw.GetTrait(TRAIT_ID__DIR));
  DeliverToInbox(facing_id, event);
  agent_phen_cache[eval_deme->GetPhenID()].msgs_exchanged++;
}

void Experiment::Imperative__DispatchMessage_Broadcast(hardware_t & hw, const event_t & event) {
  const size_t loc_id = (size_t)hw.GetTrait(TRAIT_ID__DEME_ID);
  const size_t uid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_UP);
  const size_t did = eval_deme->GetNeighborID(loc_id, deme_t::DIR_DOWN);
  const size_t lid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_LEFT);
  const size_t rid = eval_deme->GetNeighborID(loc_id, deme_t::DIR_RIGHT);
  DeliverToInbox(uid, event);
  DeliverToInbox(did, event);
  DeliverToInbox(lid, event);
  DeliverToInbox(rid, event);
  agent_phen_cache[eval_deme->GetPhenID()].msgs_exchanged += 4;
}

void Experiment::HandleEvent__Message_Forking(hardware_t & hw, const event_t & event) {
  // Spawn a new core.
  hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg);
}

void Experiment::HandleEvent__Message_NonForking(hardware_t & hw, const event_t & event) {
  // Instead of spawning a new core, load event data into input buffer of current call state.
  state_t & state = hw.GetCurState();
  // Loop through event memory... 
  for (auto mem : event.msg) { state.SetInput(mem.first, mem.second); }
}

// --- Utilities ---
void Experiment::InitPopulation_FromAncestorFile() {
  std::cout << "Initializing population from ancestor file!" << std::endl;
  // Configure the ancestor program.
  program_t ancestor_prog(inst_lib);
  std::ifstream ancestor_fstream(ANCESTOR_FPATH);
  if (!ancestor_fstream.is_open()) {
    std::cout << "Failed to open ancestor program file(" << ANCESTOR_FPATH << "). Exiting..." << std::endl;
    exit(-1);
  }
  ancestor_prog.Load(ancestor_fstream);
  std::cout << " --- Ancestor program: ---" << std::endl;
  ancestor_prog.PrintProgramFull();
  std::cout << " -------------------------" << std::endl;
  world->Inject(ancestor_prog, 1);    // Inject a bunch of ancestors into the population.
}

void Experiment::Snapshot_SingleFile(size_t update) {
  std::string snapshot_dir = DATA_DIRECTORY + "pop_" + emp::to_string((int)update);
  mkdir(snapshot_dir.c_str(), ACCESSPERMS);
  // For each program in the population, dump the full program description in a single file.
  std::ofstream prog_ofstream(snapshot_dir + "/pop_" + emp::to_string((int)update) + ".pop");
  for (size_t i = 0; i < world->GetSize(); ++i) {
    if (i) prog_ofstream << "===\n";
    Agent & agent = world->GetOrg(i);
    agent.program.PrintProgramFull(prog_ofstream);
  }
  prog_ofstream.close();
}

emp::DataFile & Experiment::AddDominantFile(const std::string & fpath) {
  auto & file = world->SetupFile(fpath);

  std::function<size_t(void)> get_update = [this](){ return world->GetUpdate(); };
  file.AddFun(get_update, "update", "Update");

  std::function<double(void)> get_score = [this]() {
    const Phenotype & phen = agent_phen_cache[dom_agent_id];
    return phen.GetScore();
  };
  file.AddFun(get_score, "score", "Dominant score");

  // total_full_consensus_time
  std::function<size_t(void)> get_full_consensus_time = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.total_full_consensus_time; 
  };
  file.AddFun(get_full_consensus_time, "full_consensus_time", "Total time steps deme was at consensus.");
  // mr_full_consensus_time
  std::function<size_t(void)> get_full_consensus_streak = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.mr_full_consensus_time; 
  };
  file.AddFun(get_full_consensus_streak, "most_recent_consensus_streak", "Most recent streak of consensus.");
  // max_consensus_size
  std::function<size_t(void)> get_max_consensus_size = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.max_consensus_size; 
  };
  file.AddFun(get_max_consensus_size, "max_consensus_size", "Vote count for majority vote getter");
  // valid_vote_cnt
  std::function<size_t(void)> get_valid_vote_cnt = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.valid_vote_cnt; 
  };
  file.AddFun(get_valid_vote_cnt, "valid_vote_cnt", "How many votes were valid at end of evaluation?");
  // msgs_exchanged
  std::function<size_t(void)> get_msgs_exchanged = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.msgs_exchanged; 
  };
  file.AddFun(get_msgs_exchanged, "msgs_sent", "How many messages were sent?");
  // min_uid
  std::function<uint32_t(void)> get_min_uid = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.min_uid; 
  };
  file.AddFun(get_min_uid, "min_uid", "What was the min UID for this evaluation?");
  // max_uid
  std::function<uint32_t(void)> get_max_uid = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.max_uid; 
  };
  file.AddFun(get_max_uid, "max_uid", "What was the max UID for this evaluation?");
  // leader_uid
  std::function<uint32_t(void)> get_leader_uid = [this]() { 
    const Phenotype & phen = agent_phen_cache[dom_agent_id];  
    return phen.leader_uid; 
  };
  file.AddFun(get_max_uid, "leader_uid", "Leader UID for this evaluation?");

  file.PrintHeaderKeys();
  return file;
}

// --- Configuration/setup function implementations ---
void Experiment::Config_Run() {
  // Make data directory.
  mkdir(DATA_DIRECTORY.c_str(), ACCESSPERMS);
  if (DATA_DIRECTORY.back() != '/') DATA_DIRECTORY += '/';
  
  // Configure the world.
  world->Reset();
  world->SetWellMixed(true);
  world->SetFitFun([this](Agent & agent) { return this->CalcFitness(agent); });
  world->SetMutFun([this](Agent &agent, emp::Random &rnd) { return this->Mutate(agent, rnd); }, ELITE_SELECT__ELITE_CNT);

  // === Setup signals! ===
  // On population initialization:
  do_pop_init_sig.AddAction([this]() {
    this->InitPopulation_FromAncestorFile();
  });

  // On run setup:
  do_begin_run_setup_sig.AddAction([this]() {
    std::cout << "Doing initial run setup." << std::endl;
    // Setup systematics/fitness tracking.
    auto & sys_file = world->SetupSystematicsFile(DATA_DIRECTORY + "systematics.csv");
    sys_file.SetTimingRepeat(SYSTEMATICS_INTERVAL);
    auto & fit_file = world->SetupFitnessFile(DATA_DIRECTORY + "fitness.csv");
    fit_file.SetTimingRepeat(FITNESS_INTERVAL);
    this->AddDominantFile(DATA_DIRECTORY+"dominant.csv").SetTimingRepeat(SYSTEMATICS_INTERVAL);
    do_pop_init_sig.Trigger();
  });

  begin_agent_eval_sig.AddAction([this](Agent & agent) {
    // Do agent setup at the beginning of its evaluation.
    // - Here, the eval_deme has been reset.
    // Randomize UIDS
    eval_deme->RandomizeUIDS();
  });

  // On evaluation:
  do_evaluation_sig.AddAction([this]() {
    double best_score = -32767;
    dom_agent_id = 0;
    for (size_t id = 0; id < world->GetSize(); ++id) {
      Agent & our_hero = world->GetOrg(id);
      our_hero.SetID(id);
      eval_deme->SetProgram(our_hero.GetGenome());
      eval_deme->SetPhenID(id);
      agent_phen_cache[id].Reset();
      this->Evaluate(our_hero);
      if (agent_phen_cache[id].GetScore() > best_score) { best_score = agent_phen_cache[id].GetScore(); dom_agent_id = id; }
    }
    std::cout << "Update: " << update << " Max score: " << best_score << std::endl;
  });
  
  do_selection_sig.AddAction([this]() {
    emp::EliteSelect(*world, ELITE_SELECT__ELITE_CNT, 1);
    emp::TournamentSelect(*world, TOURNAMENT_SIZE, POP_SIZE - ELITE_SELECT__ELITE_CNT);
  });
  
  // Do world update action
  do_world_update_sig.AddAction([this]() {
    world->Update();
  });

  // Do population snapshot action
  do_pop_snapshot_sig.AddAction([this](size_t update) { this->Snapshot_SingleFile(update); }); 

  calc_score = [this](Agent & agent) {
    Phenotype & phen = agent_phen_cache[agent.GetID()];
    return (double)(phen.valid_vote_cnt + phen.max_consensus_size + (phen.total_full_consensus_time * DEME_SIZE));
  };

}

void Experiment::Config_Analysis() { ; }

void Experiment::Config_HW() {
  // - Setup the instruction set. -
  // Standard instructions:
  inst_lib->AddInst("Inc", hardware_t::Inst_Inc, 1, "Increment value in local memory Arg1");
  inst_lib->AddInst("Dec", hardware_t::Inst_Dec, 1, "Decrement value in local memory Arg1");
  inst_lib->AddInst("Not", hardware_t::Inst_Not, 1, "Logically toggle value in local memory Arg1");
  inst_lib->AddInst("Add", hardware_t::Inst_Add, 3, "Local memory: Arg3 = Arg1 + Arg2");
  inst_lib->AddInst("Sub", hardware_t::Inst_Sub, 3, "Local memory: Arg3 = Arg1 - Arg2");
  inst_lib->AddInst("Mult", hardware_t::Inst_Mult, 3, "Local memory: Arg3 = Arg1 * Arg2");
  inst_lib->AddInst("Div", hardware_t::Inst_Div, 3, "Local memory: Arg3 = Arg1 / Arg2");
  inst_lib->AddInst("Mod", hardware_t::Inst_Mod, 3, "Local memory: Arg3 = Arg1 % Arg2");
  inst_lib->AddInst("TestEqu", hardware_t::Inst_TestEqu, 3, "Local memory: Arg3 = (Arg1 == Arg2)");
  inst_lib->AddInst("TestNEqu", hardware_t::Inst_TestNEqu, 3, "Local memory: Arg3 = (Arg1 != Arg2)");
  inst_lib->AddInst("TestLess", hardware_t::Inst_TestLess, 3, "Local memory: Arg3 = (Arg1 < Arg2)");
  inst_lib->AddInst("If", hardware_t::Inst_If, 1, "Local memory: If Arg1 != 0, proceed; else, skip block.", emp::ScopeType::BASIC, 0, {"block_def"});
  inst_lib->AddInst("While", hardware_t::Inst_While, 1, "Local memory: If Arg1 != 0, loop; else, skip block.", emp::ScopeType::BASIC, 0, {"block_def"});
  inst_lib->AddInst("Countdown", hardware_t::Inst_Countdown, 1, "Local memory: Countdown Arg1 to zero.", emp::ScopeType::BASIC, 0, {"block_def"});
  inst_lib->AddInst("Close", hardware_t::Inst_Close, 0, "Close current block if there is a block to close.", emp::ScopeType::BASIC, 0, {"block_close"});
  inst_lib->AddInst("Break", hardware_t::Inst_Break, 0, "Break out of current block.");
  inst_lib->AddInst("Call", hardware_t::Inst_Call, 0, "Call function that best matches call affinity.", emp::ScopeType::BASIC, 0, {"affinity"});
  inst_lib->AddInst("Return", hardware_t::Inst_Return, 0, "Return from current function if possible.");
  inst_lib->AddInst("SetMem", hardware_t::Inst_SetMem, 2, "Local memory: Arg1 = numerical value of Arg2");
  inst_lib->AddInst("CopyMem", hardware_t::Inst_CopyMem, 2, "Local memory: Arg1 = Arg2");
  inst_lib->AddInst("SwapMem", hardware_t::Inst_SwapMem, 2, "Local memory: Swap values of Arg1 and Arg2.");
  inst_lib->AddInst("Input", hardware_t::Inst_Input, 2, "Input memory Arg1 => Local memory Arg2.");
  inst_lib->AddInst("Output", hardware_t::Inst_Output, 2, "Local memory Arg1 => Output memory Arg2.");
  inst_lib->AddInst("Commit", hardware_t::Inst_Commit, 2, "Local memory Arg1 => Shared memory Arg2.");
  inst_lib->AddInst("Pull", hardware_t::Inst_Pull, 2, "Shared memory Arg1 => Shared memory Arg2.");
  inst_lib->AddInst("Nop", hardware_t::Inst_Nop, 0, "No operation.");
  inst_lib->AddInst("Fork", Inst_Fork, 0, "Fork a new thread. Local memory contents of callee are loaded into forked thread's input memory.");
  // Terminate and Nand were not used in the Paper's experiments. 
  // inst_lib->AddInst("Terminate", Inst_Terminate, 0, "Kill current thread.");
  // inst_lib->AddInst("Nand", Inst_Nand, 3, "WM[ARG3]=~(WM[ARG1]&WM[ARG2])");

  // Add experiment-specific instructions.
  // - Orientation instructions
  inst_lib->AddInst("RotCW", Inst_RotCW, 0, "Rotate clockwise");
  inst_lib->AddInst("RotCCW", Inst_RotCCW, 0, "Rotate couter-clockwise");
  inst_lib->AddInst("RandomDir", Inst_RandomDir, 1, "Local memory: Arg1 => RandomUInt([0:4)");
  inst_lib->AddInst("GetDir", Inst_GetDir, 0, "WM[ARG1]=CURRENT DIRECTION");
  // - Messaging instructions
  inst_lib->AddInst("SendMsg", Inst_SendMsgFacing, 0, "Send output memory as message event to faced neighbor.", emp::ScopeType::BASIC, 0, {"affinity"});
  inst_lib->AddInst("BroadcastMsg", Inst_BroadcastMsg, 0, "Broadcast output memory as message event.", emp::ScopeType::BASIC, 0, {"affinity"});
  // - Voting instructions
  inst_lib->AddInst("GetUID", Inst_GetUID, 1, "LocalReg[Arg1] = Trait[UID]");
  inst_lib->AddInst("GetOpinion", Inst_GetOpinion, 1, "LocalReg[Arg1] = Trait[Opinion]");
  inst_lib->AddInst("SetOpinion", Inst_SetOpinion, 1, "Trait[Opinion] = LocalReg[Arg1]");

  // Configure evaluation hardware.
  // Make eval deme.
  eval_deme = emp::NewPtr<deme_t>(DEME_WIDTH, DEME_HEIGHT, random, inst_lib, event_lib);
  eval_deme->SetHardwareMinBindThresh(SGP_HW_MIN_BIND_THRESH);
  eval_deme->SetHardwareMaxCores(SGP_HW_MAX_CORES);
  eval_deme->SetHardwareMaxCallDepth(SGP_HW_MAX_CALL_DEPTH);

  eval_deme->OnHardwareReset([this](hardware_t & hw) {
    hw.SetTrait(TRAIT_ID__UID, 0);        // Reset traits. 
    hw.SetTrait(TRAIT_ID__OPINION, 0);
    hw.SetTrait(TRAIT_ID__DIR, 0);
    hw.SpawnCore(0, memory_t(), true);    // Spawn main thread.
  });


  if (SGP_HW_FORK_ON_MSG) {
    event_lib->AddEvent("SendMessage", HandleEvent__Message_Forking, "Send message event.");
    event_lib->AddEvent("BroadcastMessage", HandleEvent__Message_Forking, "Broadcast message event.");
  } else {
    event_lib->AddEvent("SendMessage", HandleEvent__Message_NonForking, "Send message event.");
    event_lib->AddEvent("BroadcastMessage", HandleEvent__Message_NonForking, "Broadcast message event.");
  }

  if (SGP_HW_EVENT_DRIVEN) { // Hardware is event-driven.
    if (SGP_HW_ED_MSG_DELAY) {
      // Configure dispatchers
      eval_deme->OnHardwareAdvance([this](hardware_t & hw) {
        const size_t hw_id = hw.GetTrait(TRAIT_ID__DEME_ID);
        DoDelayDeliver(hw_id);
      });
      event_lib->RegisterDispatchFun("SendMessage", [this](hardware_t & hw, const event_t & event) {
        this->EventDriven_Delay__DispatchMessage_Send(hw, event);
      });
      event_lib->RegisterDispatchFun("BroadcastMessage", [this](hardware_t &hw, const event_t &event) {
        this->EventDriven_Delay__DispatchMessage_Broadcast(hw, event);
      });
      // Configure inboxes.
      inboxes.resize(DEME_SIZE);
      eval_deme->OnHardwareReset([this](hardware_t & hw) {
        this->ResetInbox(hw.GetTrait(TRAIT_ID__DEME_ID));
      });
    } else {
      // Configure dispatchers
      event_lib->RegisterDispatchFun("SendMessage", [this](hardware_t & hw, const event_t & event) {
        this->EventDriven__DispatchMessage_Send(hw, event);
      });
      event_lib->RegisterDispatchFun("BroadcastMessage", [this](hardware_t &hw, const event_t &event) {
        this->EventDriven__DispatchMessage_Broadcast(hw, event);
      });
    }
  } else { // Hardware is imperative.
    // Add retrieve message instruction to instruction set.
    inst_lib->AddInst("RetrieveMsg", [this](hardware_t & hw, const inst_t & inst) {
        this->Inst_RetrieveMsg(hw, inst);
      }, 0, "Retrieve a message from message inbox.");
    // Configure dispatchers
    event_lib->RegisterDispatchFun("SendMessage", [this](hardware_t & hw, const event_t & event) {
      this->Imperative__DispatchMessage_Send(hw, event);
    });
    event_lib->RegisterDispatchFun("BroadcastMessage", [this](hardware_t &hw, const event_t &event) {
      this->Imperative__DispatchMessage_Broadcast(hw, event);
    });
    // Configure inboxes.
    inboxes.resize(DEME_SIZE);
    eval_deme->OnHardwareReset([this](hardware_t & hw) {
      this->ResetInbox(hw.GetTrait(TRAIT_ID__DEME_ID));
    });
  }

  eval_deme->OnHardwareAdvance([this](hardware_t & hw) {
    hw.SingleProcess();
    uint32_t vote = (uint32_t)hw.GetTrait(TRAIT_ID__OPINION);
    eval_deme->TallyVote(vote);
  });
}

/// Mutation rules:
/// - Function Duplication (per-program rate):
///   - Result cannot allow program to exceed max function count
///   - Result cannot allow program to exceed max total instruction length.
/// - Function Deletion (per-program rate).
///   - Result cannot allow program to have no functions.
/// - Slip mutations (per-function rate)
///   - Result cannot allow function length to break the range [PROG_MIN_FUNC_LEN:PROG_MAX_FUNC_LEN]
/// - Instruction insertion/deletion mutations (per-instruction rate)
///   - Result cannot allow function length to break [PROG_MIN_FUNC_LEN:PROG_MAX_FUNC_LEN]
///   - Result cannot allow function length to exeed PROG_MAX_TOTAL_LEN
size_t Experiment::Mutate(Agent &agent, emp::Random &rnd) {
  program_t &program = agent.GetGenome();
  size_t mut_cnt = 0;
  size_t expected_prog_len = program.GetInstCnt();

  // Duplicate a (single) function?
  if (rnd.P(SGP__PER_FUNC__FUNC_DUP_RATE) && program.GetSize() < SGP_PROG_MAX_FUNC_CNT)
  {
    const uint32_t fID = rnd.GetUInt(program.GetSize());
    // Would function duplication make expected program length exceed max?
    if (expected_prog_len + program[fID].GetSize() <= SGP_PROG_MAX_TOTAL_LEN)
    {
      program.PushFunction(program[fID]);
      expected_prog_len += program[fID].GetSize();
      ++mut_cnt;
    }
  }

  // Delete a (single) function?
  if (rnd.P(SGP__PER_FUNC__FUNC_DEL_RATE) && program.GetSize() > SGP_PROG_MIN_FUNC_CNT)
  {
    const uint32_t fID = rnd.GetUInt(program.GetSize());
    expected_prog_len -= program[fID].GetSize();
    program[fID] = program[program.GetSize() - 1];
    program.program.resize(program.GetSize() - 1);
    ++mut_cnt;
  }

  // For each function...
  for (size_t fID = 0; fID < program.GetSize(); ++fID)
  {

    // Mutate affinity
    for (size_t i = 0; i < program[fID].GetAffinity().GetSize(); ++i)
    {
      tag_t &aff = program[fID].GetAffinity();
      if (rnd.P(SGP__PER_BIT__TAG_BFLIP_RATE))
      {
        ++mut_cnt;
        aff.Set(i, !aff.Get(i));
      }
    }

    // Slip-mutation?
    if (rnd.P(SGP__PER_FUNC__SLIP_RATE))
    {
      uint32_t begin = rnd.GetUInt(program[fID].GetSize());
      uint32_t end = rnd.GetUInt(program[fID].GetSize());
      const bool dup = begin < end;
      const bool del = begin > end;
      const int dup_size = end - begin;
      const int del_size = begin - end;
      // If we would be duplicating and the result will not exceed maximum program length, duplicate!
      if (dup && (expected_prog_len + dup_size <= SGP_PROG_MAX_TOTAL_LEN) && (program[fID].GetSize() + dup_size <= SGP_PROG_MAX_FUNC_LEN))
      {
        // duplicate begin:end
        const size_t new_size = program[fID].GetSize() + (size_t)dup_size;
        hardware_t::Function new_fun(program[fID].GetAffinity());
        for (size_t i = 0; i < new_size; ++i)
        {
          if (i < end)
            new_fun.PushInst(program[fID][i]);
          else
            new_fun.PushInst(program[fID][i - dup_size]);
        }
        program[fID] = new_fun;
        ++mut_cnt;
        expected_prog_len += dup_size;
      }
      else if (del && ((program[fID].GetSize() - del_size) >= SGP_PROG_MIN_FUNC_LEN))
      {
        // delete end:begin
        hardware_t::Function new_fun(program[fID].GetAffinity());
        for (size_t i = 0; i < end; ++i)
          new_fun.PushInst(program[fID][i]);
        for (size_t i = begin; i < program[fID].GetSize(); ++i)
          new_fun.PushInst(program[fID][i]);
        program[fID] = new_fun;
        ++mut_cnt;
        expected_prog_len -= del_size;
      }
    }

    // Substitution mutations? (pretty much completely safe)
    for (size_t i = 0; i < program[fID].GetSize(); ++i)
    {
      inst_t &inst = program[fID][i];
      // Mutate affinity (even when it doesn't use it).
      for (size_t k = 0; k < inst.affinity.GetSize(); ++k)
      {
        if (rnd.P(SGP__PER_BIT__TAG_BFLIP_RATE))
        {
          ++mut_cnt;
          inst.affinity.Set(k, !inst.affinity.Get(k));
        }
      }

      // Mutate instruction.
      if (rnd.P(SGP__PER_INST__SUB_RATE))
      {
        ++mut_cnt;
        inst.id = rnd.GetUInt(program.GetInstLib()->GetSize());
      }

      // Mutate arguments (even if they aren't relevent to instruction).
      for (size_t k = 0; k < hardware_t::MAX_INST_ARGS; ++k)
      {
        if (rnd.P(SGP__PER_INST__SUB_RATE))
        {
          ++mut_cnt;
          inst.args[k] = rnd.GetInt(SGP__PROG_MAX_ARG_VAL);
        }
      }
    }

    // Insertion/deletion mutations?
    // - Compute number of insertions.
    int num_ins = rnd.GetRandBinomial(program[fID].GetSize(), SGP__PER_INST__INS_RATE);
    // Ensure that insertions don't exceed maximum program length.
    if ((num_ins + program[fID].GetSize()) > SGP_PROG_MAX_FUNC_LEN)
    {
      num_ins = SGP_PROG_MAX_FUNC_LEN - program[fID].GetSize();
    }
    if ((num_ins + expected_prog_len) > SGP_PROG_MAX_TOTAL_LEN)
    {
      num_ins = SGP_PROG_MAX_TOTAL_LEN - expected_prog_len;
    }
    expected_prog_len += num_ins;

    // Do we need to do any insertions or deletions?
    if (num_ins > 0 || SGP__PER_INST__DEL_RATE > 0.0)
    {
      size_t expected_func_len = num_ins + program[fID].GetSize();
      // Compute insertion locations and sort them.
      emp::vector<size_t> ins_locs = emp::RandomUIntVector(rnd, num_ins, 0, program[fID].GetSize());
      if (ins_locs.size())
        std::sort(ins_locs.begin(), ins_locs.end(), std::greater<size_t>());
      hardware_t::Function new_fun(program[fID].GetAffinity());
      size_t rhead = 0;
      while (rhead < program[fID].GetSize())
      {
        if (ins_locs.size())
        {
          if (rhead >= ins_locs.back())
          {
            // Insert a random instruction.
            new_fun.PushInst(rnd.GetUInt(program.GetInstLib()->GetSize()),
                             rnd.GetInt(SGP__PROG_MAX_ARG_VAL),
                             rnd.GetInt(SGP__PROG_MAX_ARG_VAL),
                             rnd.GetInt(SGP__PROG_MAX_ARG_VAL),
                             tag_t());
            new_fun.inst_seq.back().affinity.Randomize(rnd);
            ++mut_cnt;
            ins_locs.pop_back();
            continue;
          }
        }
        // Do we delete this instruction?
        if (rnd.P(SGP__PER_INST__DEL_RATE) && (expected_func_len > SGP_PROG_MIN_FUNC_LEN))
        {
          ++mut_cnt;
          --expected_prog_len;
          --expected_func_len;
        }
        else
        {
          new_fun.PushInst(program[fID][rhead]);
        }
        ++rhead;
      }
      program[fID] = new_fun;
    }
  }
  return mut_cnt;
}

#endif
