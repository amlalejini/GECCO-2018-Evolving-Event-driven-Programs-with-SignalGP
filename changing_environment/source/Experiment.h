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

#include "base/Ptr.h"
#include "base/vector.h"
#include "control/Signal.h"
#include "Evolve/World.h"
#include "Evolve/Resource.h"
#include "Evolve/SystematicsAnalysis.h"
#include "Evolve/World_output.h"
#include "games/Othello.h"
#include "hardware/EventDrivenGP.h"
#include "hardware/InstLib.h"
#include "tools/BitVector.h"
#include "tools/Random.h"
#include "tools/random_utils.h"
#include "tools/math.h"
#include "tools/string_utils.h"

#include "l9_chg_env-config.h"
#include "TaskSet.h"

// == Notes ==
// Things I want to configure:
//  ENVIRONMENT
//    * environment change type (regular, random)
//    * environment count (number of environment states)
//    * environment change probability (probability of changing if in random change mode)
//    * environment change rate (rate in timesteps of environment change if in regular mode)

constexpr size_t RUN_ID__EXP = 0;
constexpr size_t RUN_ID__ANALYSIS = 1;

constexpr size_t ENV_TAG_GEN_ID__RANDOM = 0;
constexpr size_t ENV_TAG_GEN_ID__LOAD = 1;

constexpr size_t ENV_CHG_ID__RANDOM = 0;
constexpr size_t ENV_CHG_ID__REGULAR = 1;

constexpr size_t SELECTION_METHOD_ID__TOURNAMENT = 0;
constexpr size_t SELECTION_METHOD_ID__LEXICASE = 1;
constexpr size_t SELECTION_METHOD_ID__ECOEA = 2;
constexpr size_t SELECTION_METHOD_ID__MAPELITES = 3;
constexpr size_t SELECTION_METHOD_ID__ROULETTE = 4;

constexpr size_t TAG_WIDTH = 16;

constexpr size_t TRAIT_ID__STATE = 0;

constexpr uint32_t MIN_TASK_INPUT = 0;
constexpr uint32_t MAX_TASK_INPUT = 1000000000;
constexpr size_t MAX_TASK_NUM_INPUTS = 2;

constexpr size_t TASK_CNT = 9;

/// Class to manage ALIFE2018 changing environment (w/logic 9) experiments.
class Experiment {
public:
  // Forward declarations.
  struct Agent;
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

  class Phenotype {
  protected:
    emp::vector<size_t> env_match_score_by_trial;          ///< By trial.
    emp::vector<size_t> time_all_tasks_credited_by_trial;  ///< By trial.
    emp::vector<size_t> total_wasted_completions_by_trial; ///< By trial.
    emp::vector<size_t> unique_tasks_credited_by_trial;    ///< By trial.
    emp::vector<size_t> unique_tasks_completed_by_trial;   ///< By trial.
    emp::vector<double> scores_by_trial;          ///< By trial.

    emp::vector<size_t> wasted_completions; ///< By trial & by task (special indexing) Completions before credited (for each task).
    emp::vector<size_t> credited; ///< By trial & by task (special indexing)
    emp::vector<size_t> completed; ///< By trial & by task

    size_t min_trial;

    size_t GetByTaskIdx(size_t trial_id, size_t task_id) const {
      return (trial_id * TASK_CNT) + task_id;
    }

  public:
    Phenotype(size_t trial_cnt=1)
      : env_match_score_by_trial(trial_cnt),
        time_all_tasks_credited_by_trial(trial_cnt),
        total_wasted_completions_by_trial(trial_cnt),
        unique_tasks_credited_by_trial(trial_cnt),
        unique_tasks_completed_by_trial(trial_cnt),
        scores_by_trial(trial_cnt),
        wasted_completions(trial_cnt * TASK_CNT),
        credited(trial_cnt * TASK_CNT),
        completed(trial_cnt * TASK_CNT),
        min_trial(0)
    { emp_assert(trial_cnt > 0); }

    /// Zero out everything we're tracking.
    void Reset() {
      min_trial = 0;
      for (size_t i = 0; i < scores_by_trial.size(); ++i) {
        env_match_score_by_trial[i] = 0;
        time_all_tasks_credited_by_trial[i] = 0;
        total_wasted_completions_by_trial[i] = 0;
        unique_tasks_credited_by_trial[i] = 0;
        unique_tasks_completed_by_trial[i] = 0;
        scores_by_trial[i] = 0;
      }
      for (size_t j = 0; j < completed.size(); ++j) {
        wasted_completions[j] = 0;
        credited[j] = 0;
        completed[j] = 0;
      }
    }

    void SetTrialCnt(size_t trial_cnt) {
      env_match_score_by_trial.resize(trial_cnt);
      time_all_tasks_credited_by_trial.resize(trial_cnt);
      total_wasted_completions_by_trial.resize(trial_cnt);
      unique_tasks_credited_by_trial.resize(trial_cnt);
      unique_tasks_completed_by_trial.resize(trial_cnt);
      scores_by_trial.resize(trial_cnt);
      wasted_completions.resize(trial_cnt * TASK_CNT);
      credited.resize(trial_cnt * TASK_CNT);
      completed.resize(trial_cnt * TASK_CNT);
    }

    size_t GetMinTrial() const { return min_trial; }

    size_t GetEnvMatchScore(size_t trialID) const { return env_match_score_by_trial[trialID]; }
    size_t GetTimeAllTasksCredited(size_t trialID) const { return time_all_tasks_credited_by_trial[trialID]; }
    size_t GetTotalWastedCompletions(size_t trialID) const { return total_wasted_completions_by_trial[trialID]; }
    size_t GetUniqueTasksCredited(size_t trialID) const { return unique_tasks_credited_by_trial[trialID]; }
    size_t GetUniqueTasksCompleted(size_t trialID) const { return unique_tasks_completed_by_trial[trialID]; }
    double GetScore(size_t trialID) const { return scores_by_trial[trialID]; }

    size_t GetTaskWastedCompletions(size_t trialID, size_t taskID) const { return wasted_completions[GetByTaskIdx(trialID, taskID)]; }
    size_t GetTaskCredited(size_t trialID, size_t taskID) const { return credited[GetByTaskIdx(trialID, taskID)]; }
    size_t GetTaskCompleted(size_t trialID, size_t taskID) const { return completed[GetByTaskIdx(trialID, taskID)]; }

    size_t GetMinEnvMatchScore() const { return env_match_score_by_trial[min_trial]; }
    size_t GetMinTimeAllTasksCredited() const { return time_all_tasks_credited_by_trial[min_trial]; }
    size_t GetMinTotalWastedCompletions() const { return total_wasted_completions_by_trial[min_trial]; }
    size_t GetMinUniqueTasksCredited() const { return unique_tasks_credited_by_trial[min_trial]; }
    size_t GetMinUniqueTasksCompleted() const { return unique_tasks_completed_by_trial[min_trial]; }
    double GetMinScore() const { return scores_by_trial[min_trial]; }

    size_t GetMinTaskWastedCompletions(size_t taskID) const { return wasted_completions[GetByTaskIdx(min_trial, taskID)]; }
    size_t GetMinTaskCredited(size_t taskID) const { return credited[GetByTaskIdx(min_trial, taskID)]; }
    size_t GetMinTaskCompleted(size_t taskID) const { return completed[GetByTaskIdx(min_trial, taskID)]; }

    void SetMinTrial(size_t mt) { emp_assert(mt < scores_by_trial.size()); min_trial = mt; }
    void SetMinTrial() {
      double val = scores_by_trial[0];
      size_t mt = 0;
      for (size_t i = 0; i < scores_by_trial.size(); ++i) {
        const double trial_score = scores_by_trial[i];
        if (trial_score < val) { val = trial_score; mt = i; }
      }
      min_trial = mt;
    }

    void SetEnvMatchScore(size_t trialID, size_t val) { env_match_score_by_trial[trialID] = val; }
    void SetTimeAllTasksCredited(size_t trialID, size_t val) { time_all_tasks_credited_by_trial[trialID] = val; }
    void SetTotalWastedCompletions(size_t trialID, size_t val) { total_wasted_completions_by_trial[trialID] = val; }
    void SetUniqueTasksCredited(size_t trialID, size_t val) { unique_tasks_credited_by_trial[trialID] = val; }
    void SetUniqueTasksCompleted(size_t trialID, size_t val) { unique_tasks_completed_by_trial[trialID] = val; }
    void SetScore(size_t trialID, double val) { scores_by_trial[trialID] = val; }

    void SetTaskWastedCompletions(size_t trialID, size_t taskID, size_t val) { wasted_completions[GetByTaskIdx(trialID, taskID)] = val; }
    void SetTaskCredited(size_t trialID, size_t taskID, size_t val) { credited[GetByTaskIdx(trialID, taskID)] = val; }
    void SetTaskCompleted(size_t trialID, size_t taskID, size_t val) { completed[GetByTaskIdx(trialID, taskID)] = val; }

    void IncEnvMatchScore(size_t trialID, size_t amt=1) { env_match_score_by_trial[trialID] += amt; }
  };


protected:
  // == Configurable experiment parameters ==
  size_t RUN_MODE;
  int RANDOM_SEED;
  size_t POP_SIZE;
  size_t GENERATIONS;
  size_t EVAL_TIME;
  size_t TRIAL_CNT;
  bool TASKS_ON;
  std::string ANCESTOR_FPATH;
  size_t SELECTION_METHOD;
  size_t ELITE_SELECT__ELITE_CNT;
  size_t TOURNAMENT_SIZE;
  size_t ENVIRONMENT_STATES;
  size_t ENVIRONMENT_TAG_GENERATION_METHOD;
  std::string ENVIRONMENT_TAG_FPATH;
  size_t ENVIRONMENT_CHANGE_METHOD;
  double ENVIRONMENT_CHANGE_PROB;
  size_t ENVIRONMENT_CHANGE_INTERVAL;
  size_t SGP_PROG_MAX_FUNC_CNT;
  size_t SGP_PROG_MIN_FUNC_CNT;
  size_t SGP_PROG_MAX_FUNC_LEN;
  size_t SGP_PROG_MIN_FUNC_LEN;
  size_t SGP_PROG_MAX_TOTAL_LEN;
  bool SGP_ENVIRONMENT_SIGNALS;
  bool SGP_ACTIVE_SENSORS;
  size_t SGP_HW_MAX_CORES;
  size_t SGP_HW_MAX_CALL_DEPTH;
  double SGP_HW_MIN_BIND_THRESH;
  int SGP__PROG_MAX_ARG_VAL;
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

  size_t ANALYSIS;
  std::string ANALYZE_AGENT_FPATH;
  std::string ANALYSIS_OUTPUT_FNAME;

  emp::Ptr<emp::Random> random;
  emp::Ptr<world_t> world;

  emp::Ptr<inst_lib_t> inst_lib;
  emp::Ptr<event_lib_t> event_lib;

  emp::Ptr<hardware_t> eval_hw;

  emp::vector<tag_t> env_state_tags;  ///< Tags associated with each environment state.

  using taskset_t = TaskSet<std::array<task_io_t,MAX_TASK_NUM_INPUTS>,task_io_t>;
  taskset_t task_set;
  std::array<task_io_t,MAX_TASK_NUM_INPUTS> task_inputs; ///< Current task inputs.
  size_t input_load_id;

  size_t update;
  size_t eval_trial;
  size_t eval_time;
  size_t env_state;

  size_t dom_agent_id;

  emp::vector<Phenotype> agent_phen_cache;
  emp::vector<double> env_match_proportion_lookup;

  emp::vector<std::function<double(Agent &)>> lexicase_fit_set; ///< Fit set for SGP lexicase selection.

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
  emp::Signal<void(Agent &)> begin_trial_sig;
  emp::Signal<void(void)> env_advance_sig;
  emp::Signal<void(Agent &)> agent_advance_sig;
  emp::Signal<void(Agent &)> record_cur_phenotype_sig;

  // Functors!
  std::function<double(Agent &)> calc_score;

  size_t GetCacheIndex(size_t agent_id, size_t trial_id) {
    return (agent_id * TRIAL_CNT) + trial_id;
  }

  void Evaluate(Agent & agent) {
    for (eval_trial = 0; eval_trial < TRIAL_CNT; ++eval_trial) {
      env_state = (size_t)-1;
      begin_trial_sig.Trigger(agent);
      for (eval_time = 0; eval_time < EVAL_TIME; ++eval_time) {
        // 1) Advance environment.
        env_advance_sig.Trigger();
        // 2) Advance agent.
        agent_advance_sig.Trigger(agent);
      }
      // Record everything we want to store about trial phenotype:
      record_cur_phenotype_sig.Trigger(agent);
    }
  }

public:
  Experiment(const L9ChgEnvConfig & config)
    : input_load_id(0), update(0), eval_trial(0), eval_time(0), env_state(0), dom_agent_id(0)
  {
    RUN_MODE = config.RUN_MODE();
    RANDOM_SEED = config.RANDOM_SEED();
    POP_SIZE = config.POP_SIZE();
    GENERATIONS = config.GENERATIONS();
    EVAL_TIME = config.EVAL_TIME();
    TRIAL_CNT = config.TRIAL_CNT();
    TASKS_ON = config.TASKS_ON();
    ANCESTOR_FPATH = config.ANCESTOR_FPATH();
    SELECTION_METHOD = config.SELECTION_METHOD();
    ELITE_SELECT__ELITE_CNT = config.ELITE_SELECT__ELITE_CNT();
    TOURNAMENT_SIZE = config.TOURNAMENT_SIZE();
    ENVIRONMENT_STATES = config.ENVIRONMENT_STATES();
    ENVIRONMENT_TAG_GENERATION_METHOD = config.ENVIRONMENT_TAG_GENERATION_METHOD();
    ENVIRONMENT_TAG_FPATH = config.ENVIRONMENT_TAG_FPATH();
    ENVIRONMENT_CHANGE_METHOD = config.ENVIRONMENT_CHANGE_METHOD();
    ENVIRONMENT_CHANGE_PROB = config.ENVIRONMENT_CHANGE_PROB();
    ENVIRONMENT_CHANGE_INTERVAL = config.ENVIRONMENT_CHANGE_INTERVAL();
    SGP_PROG_MAX_FUNC_CNT = config.SGP_PROG_MAX_FUNC_CNT();
    SGP_PROG_MIN_FUNC_CNT = config.SGP_PROG_MIN_FUNC_CNT();
    SGP_PROG_MAX_FUNC_LEN = config.SGP_PROG_MAX_FUNC_LEN();
    SGP_PROG_MIN_FUNC_LEN = config.SGP_PROG_MIN_FUNC_LEN();
    SGP_PROG_MAX_TOTAL_LEN = config.SGP_PROG_MAX_TOTAL_LEN();
    SGP_ENVIRONMENT_SIGNALS = config.SGP_ENVIRONMENT_SIGNALS();
    SGP_ACTIVE_SENSORS = config.SGP_ACTIVE_SENSORS();
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
    ANALYSIS = config.ANALYSIS();
    ANALYZE_AGENT_FPATH = config.ANALYZE_AGENT_FPATH();
    ANALYSIS_OUTPUT_FNAME = config.ANALYSIS_OUTPUT_FNAME();

    if (RUN_MODE == RUN_ID__EXP) {
      // Make data directory.
      mkdir(DATA_DIRECTORY.c_str(), ACCESSPERMS);
      if (DATA_DIRECTORY.back() != '/') DATA_DIRECTORY += '/';
    }

    for (size_t i = 0; i < EVAL_TIME; ++i) {
      env_match_proportion_lookup.emplace_back((double)i/(double)EVAL_TIME);
    }

    // Make the random number generator.
    random = emp::NewPtr<emp::Random>(RANDOM_SEED);
    // Configure environment tags.
    switch (ENVIRONMENT_TAG_GENERATION_METHOD) {
      case ENV_TAG_GEN_ID__RANDOM:
        GenerateEnvTags_Random();
        break;
      case ENV_TAG_GEN_ID__LOAD:
        GenerateEnvTags_Load();
        break;
      default:
        std::cout << "Unrecognized tag generation method!" << std::endl;
        exit(-1);
    }
    // Make the world!
    world = emp::NewPtr<world_t>(random, "L9-CE-World");
    for (size_t i = 0; i < POP_SIZE; ++i) agent_phen_cache.emplace_back(TRIAL_CNT);
    // Make inst/event libraries.
    inst_lib = emp::NewPtr<inst_lib_t>();
    event_lib = emp::NewPtr<event_lib_t>();
    // Configure tasks.
    Config_Tasks();
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

  void Config_Tasks();
  void Config_HW();
  void Config_Run();
  void Config_Analysis();

  size_t Mutate(Agent & agent, emp::Random & rnd);
  double CalcFitness(Agent & agent) {
    return agent_phen_cache[agent.GetID()].GetMinScore();
  }

  void InitPopulation_FromAncestorFile();
  void Snapshot_SingleFile(size_t update);

  emp::DataFile & AddDominantFile(const std::string & fpath="dominant.csv");

  void GenerateEnvTags_Load();
  void GenerateEnvTags_Random();
  void SaveEnvTags();

  // events
  // Events.
  static void HandleEvent__EnvSignal_ED(hardware_t & hw, const event_t & event);
  static void HandleEvent__EnvSignal_IMP(hardware_t & hw, const event_t & event);

  static void DispatchEvent__EnvSignal_ED(hardware_t & hw, const event_t & event);
  static void DispatchEvent__EnvSignal_IMP(hardware_t & hw, const event_t & event);

  // Instructions
  static void Inst_Fork(hardware_t & hw, const inst_t & inst);
  static void Inst_Nand(hardware_t & hw, const inst_t & inst);
  static void Inst_Terminate(hardware_t & hw, const inst_t & inst);

  void Inst_Load1(hardware_t & hw, const inst_t & inst);
  void Inst_Load2(hardware_t & hw, const inst_t & inst);
  void Inst_Submit(hardware_t & hw, const inst_t & inst);

};

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

void Experiment::GenerateEnvTags_Load() {
  // Make sure number loaded in matches ENVIRONMENT STATES!
  // Load environment tags from local file.
  env_state_tags.resize(ENVIRONMENT_STATES, tag_t());
  std::ifstream tag_fstream(ENVIRONMENT_TAG_FPATH);
  if (!tag_fstream.is_open()) {
    std::cout << "Failed to open env_tags.csv. Exiting..." << std::endl;
    exit(-1);
  }
  std::string cur_line;
  emp::vector<std::string> line_components;
  std::getline(tag_fstream, cur_line); // Consume header.
  while (!tag_fstream.eof()) {
    std::getline(tag_fstream, cur_line);
    emp::remove_whitespace(cur_line);
    if (cur_line == emp::empty_string()) continue;
    emp::slice(cur_line, line_components, ',');
    int state_id = std::stoi(line_components[0]);
    if (state_id > env_state_tags.size()) {
      std::cout << "WARNING: environment state tag ID exceeds ENVIRONMENT_STATES parameter." << std::endl;
      continue;
    }
    for (size_t i = 0; i < line_components[1].size(); ++i) {
      if (i >= TAG_WIDTH) break;
      if (line_components[1][i] == '1') env_state_tags[state_id].Set(env_state_tags[state_id].GetSize() - i - 1, true);
    }
  }
  tag_fstream.close();
}

void Experiment::SaveEnvTags() {
  // Save out the environment states.
  std::ofstream envtags_ofstream(ENVIRONMENT_TAG_FPATH);
  envtags_ofstream << "env_id,tag\n";
  for (size_t i = 0; i < env_state_tags.size(); ++i) {
    envtags_ofstream << i << ","; env_state_tags[i].Print(envtags_ofstream); envtags_ofstream << "\n";
  }
  envtags_ofstream.close();
}

void Experiment::GenerateEnvTags_Random() {
  if (ENVIRONMENT_STATES > emp::Pow2(TAG_WIDTH)) {
    std::cout << "Requested environment states exceed limit! Exiting..." << std::endl;
    exit(-1);
  }
  std::cout << "Randomly generate environment tags!" << std::endl;
  std::unordered_set<int> uset; // Used to make sure all environment tags are unique.
  for (size_t i = 0; i < ENVIRONMENT_STATES; ++i) {
    env_state_tags.emplace_back(tag_t());
    env_state_tags[i].Randomize(*random);
    int tag_int = env_state_tags[i].GetUInt(0);
    while (true) {
      if (!emp::Has(uset, tag_int)) {
        uset.emplace(tag_int);
        break;
      } else {
        env_state_tags[i].Randomize(*random);
        tag_int = env_state_tags[i].GetUInt(0);
      }
    }
  }
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

/// Setup a data_file with world that records information about the dominant genotype.
emp::DataFile & Experiment::AddDominantFile(const std::string & fpath) {
    auto & file = world->SetupFile(fpath);

    std::function<size_t(void)> get_update = [this](){ return world->GetUpdate(); };
    file.AddFun(get_update, "update", "Update");

    std::function<double(void)> get_score = [this]() {
      Phenotype & phen = agent_phen_cache[dom_agent_id];
      return phen.GetMinScore();
    };
    file.AddFun(get_score, "score", "...");

    std::function<size_t(void)> get_env_match_score = [this]() {
      Phenotype & phen = agent_phen_cache[dom_agent_id];
      return phen.GetMinEnvMatchScore();
    };
    file.AddFun(get_env_match_score, "env_matches", "...");

    std::function<size_t(void)> get_time_all_tasks_credited = [this]() {
      Phenotype & phen = agent_phen_cache[dom_agent_id];
      return phen.GetMinTimeAllTasksCredited();
    };
    file.AddFun(get_time_all_tasks_credited, "time_all_tasks_credited", "...");

    std::function<size_t(void)> get_unique_tasks_completed = [this]() {
      Phenotype & phen = agent_phen_cache[dom_agent_id];
      return phen.GetMinUniqueTasksCompleted();
    };
    file.AddFun(get_unique_tasks_completed, "total_unique_tasks_completed", "...");

    std::function<size_t(void)> get_total_wasted_completions = [this]() {
      Phenotype & phen = agent_phen_cache[dom_agent_id];
      return phen.GetMinTotalWastedCompletions();
    };
    file.AddFun(get_total_wasted_completions, "total_wasted_completions", "...");

    std::function<size_t(void)> get_unique_tasks_credited = [this]() {
      Phenotype & phen = agent_phen_cache[dom_agent_id];
      return phen.GetMinUniqueTasksCredited();
    };
    file.AddFun(get_unique_tasks_credited, "total_unique_tasks_credited", "...");

    for (size_t i = 0; i < TASK_CNT; ++i) {
      std::function<size_t(void)> get_wasted = [this, i]() {
        Phenotype & phen = agent_phen_cache[dom_agent_id];
        return phen.GetMinTaskWastedCompletions(i);
      };
      file.AddFun(get_wasted, "wasted_"+task_set.GetName(i), "...");

      std::function<size_t(void)> get_completed = [this, i]() {
        Phenotype & phen = agent_phen_cache[dom_agent_id];
        return phen.GetMinTaskCompleted(i);
      };
      file.AddFun(get_completed, "completed_"+task_set.GetName(i), "...");

      std::function<size_t(void)> get_credited = [this, i]() {
        Phenotype & phen = agent_phen_cache[dom_agent_id];
        return phen.GetMinTaskCredited(i);
      };
      file.AddFun(get_credited, "credited_"+task_set.GetName(i), "...");
    }
    file.PrintHeaderKeys();
    return file;
}

void Experiment::Config_Run() {
  world->Reset();
  world->SetWellMixed(true);
  world->SetMutFun([this](Agent & agent, emp::Random & rnd) { return this->Mutate(agent, rnd); }, ELITE_SELECT__ELITE_CNT);
  world->SetFitFun([this](Agent & agent) { return this->CalcFitness(agent); });

  // Save out env tags in use if randomly generated
  if (ENVIRONMENT_TAG_GENERATION_METHOD != ENV_TAG_GEN_ID__LOAD) { SaveEnvTags(); }
  // Print tags
  std::cout << "Environment states: " << std::endl;
  for (size_t i = 0; i < env_state_tags.size(); ++i) {
    int tag_int = env_state_tags[i].GetUInt(0);
    std::cout << "[" << i << "]: "; env_state_tags[i].Print(); std::cout << "(" << tag_int << ")" << std::endl;
  }
  std::cout << "--" << std::endl;

  // score: uniques tasks credited + unique tasks completed + (eval time - time took to get full credit) + env matches
  calc_score = [this](Agent & agent) {
    double score = 0;

    score += task_set.GetUniqueTasksCredited();
    score += task_set.GetUniqueTasksCompleted();

    if (task_set.AllTasksCredited()) {
      score += (EVAL_TIME - task_set.GetAllTasksCreditedTime());
    }
    score += agent_phen_cache[agent.GetID()].GetEnvMatchScore(eval_trial);
    return score;
  };

  // ==== Setup signals! ====
  // Population initialization action
  do_pop_init_sig.AddAction([this]() {
    this->InitPopulation_FromAncestorFile();
  });

  // Begin run setup action
  do_begin_run_setup_sig.AddAction([this]() {
    std::cout << "Doing initial run setup." << std::endl;
    // Setup systematics/fitness tracking.
    auto & sys_file = world->SetupSystematicsFile(DATA_DIRECTORY + "systematics.csv");
    sys_file.SetTimingRepeat(SYSTEMATICS_INTERVAL);
    auto & fit_file = world->SetupFitnessFile(DATA_DIRECTORY + "fitness.csv");
    fit_file.SetTimingRepeat(FITNESS_INTERVAL);
    this->AddDominantFile(DATA_DIRECTORY + "dominant.csv").SetTimingRepeat(SYSTEMATICS_INTERVAL);
    // Generate the initial population.
    do_pop_init_sig.Trigger();
  });

  // Do evaluation action
  do_evaluation_sig.AddAction([this]() {
    double best_score = -32767;
    dom_agent_id = 0;
    for (size_t id = 0; id < world->GetSize(); ++id) {
      Agent & our_hero = world->GetOrg(id);
      our_hero.SetID(id);
      eval_hw->SetProgram(our_hero.GetGenome());
      // Reset cache values.
      agent_phen_cache[id].Reset();
      this->Evaluate(our_hero);
      // Find min trial.
      agent_phen_cache[id].SetMinTrial();
      // -- Keep track of worst-type phenotype & cur phenotype;
      if (agent_phen_cache[id].GetMinScore() > best_score) { best_score = agent_phen_cache[id].GetMinScore(); dom_agent_id = id; }
    }
    std::cout << "Update: " << update << " Max score: " << best_score << std::endl;
  });

  // Do world update action
  do_world_update_sig.AddAction([this]() {
    world->Update();
  });

  // Do population snapshot action
  do_pop_snapshot_sig.AddAction([this](size_t update) { this->Snapshot_SingleFile(update); });

  // Do selection on population action
  switch (SELECTION_METHOD) {
    case SELECTION_METHOD_ID__TOURNAMENT: {
      do_selection_sig.AddAction([this]() {
        emp::EliteSelect(*world, ELITE_SELECT__ELITE_CNT, 1);
        emp::TournamentSelect(*world, TOURNAMENT_SIZE, POP_SIZE - ELITE_SELECT__ELITE_CNT);
      });
      break;
    }
    case SELECTION_METHOD_ID__LEXICASE: {
      lexicase_fit_set.resize(0);
      for (size_t i = 0; i < task_set.GetSize(); ++i) {
        // Function for each task credited.
        lexicase_fit_set.push_back([i, this](Agent & agent) {
          Phenotype & phen = agent_phen_cache[agent.GetID()];
          return (phen.GetMinTaskCredited(i)) ? 1.0 : 0.0;
        });
        // Function for each task completed.
        lexicase_fit_set.push_back([i, this](Agent & agent) {
          Phenotype & phen = agent_phen_cache[agent.GetID()];
          return (phen.GetMinTaskCompleted(i)) ? 1.0 : 0.0;
        });
      }
      // Function for env match score.
      lexicase_fit_set.push_back([this](Agent & agent) {
        Phenotype & phen = agent_phen_cache[agent.GetID()];
        return phen.GetMinEnvMatchScore();
      });
      // Function for efficiency.
      lexicase_fit_set.push_back([this](Agent & agent) {
        Phenotype & phen = agent_phen_cache[agent.GetID()];
        const bool all_tasks_done = phen.GetMinUniqueTasksCredited() == task_set.GetSize();
        return (all_tasks_done) ? EVAL_TIME - phen.GetMinTimeAllTasksCredited() : 0;
      });

      do_selection_sig.AddAction([this]() {
        emp::EliteSelect(*world, ELITE_SELECT__ELITE_CNT, 1);
        emp::LexicaseSelect(*world, lexicase_fit_set, POP_SIZE - ELITE_SELECT__ELITE_CNT);
      });
      break;
    }
    case SELECTION_METHOD_ID__ECOEA: {
      std::cout << "Eco-ea not implemented. Exiting..." << std::endl;
      exit(-1);
      break;
    }
    case SELECTION_METHOD_ID__MAPELITES: {
      std::cout << "Map elites not implemented. Exiting..." << std::endl;
      exit(-1);
      break;
    }
    case SELECTION_METHOD_ID__ROULETTE: {
      std::cout << "Roulette not implemented. Exiting..." << std::endl;
      exit(-1);
      break;
    }
    default:
      std::cout << "Unrecognized resource select mode. Exiting..." << std::endl;
      exit(-1);
  }


  // Begin eval trial action
  begin_trial_sig.AddAction([this](Agent & agent) {
    // 1) Reset tasks.
    task_inputs[0] = random->GetUInt(MIN_TASK_INPUT, MAX_TASK_INPUT);
    task_inputs[1] = random->GetUInt(MIN_TASK_INPUT, MAX_TASK_INPUT);
    input_load_id = 0;
    task_set.SetInputs(task_inputs);
    // 2) Reset hardware
    eval_hw->ResetHardware();
    eval_hw->SetTrait(TRAIT_ID__STATE, -1);
    eval_hw->SpawnCore(0, memory_t(), true);
  });

  record_cur_phenotype_sig.AddAction([this](Agent & agent) {
    const size_t agent_id = agent.GetID();
    Phenotype & phen = agent_phen_cache[agent_id];
    // Record everything that can only be recorded pos-trial.
    phen.SetScore(eval_trial, calc_score(agent));
    // std::cout << "  Score: " << phen.GetScore(eval_trial) << std::endl;
    phen.SetTimeAllTasksCredited(eval_trial, task_set.GetAllTasksCreditedTime());
    phen.SetUniqueTasksCredited(eval_trial, task_set.GetUniqueTasksCredited());
    phen.SetUniqueTasksCompleted(eval_trial, task_set.GetUniqueTasksCompleted());
    phen.SetTotalWastedCompletions(eval_trial, task_set.GetTotalTasksWasted());
    for (size_t taskID = 0; taskID < task_set.GetSize(); ++taskID) {
      phen.SetTaskCredited(eval_trial, taskID, task_set.GetTask(taskID).GetCreditedCnt());
      phen.SetTaskCompleted(eval_trial, taskID, task_set.GetTask(taskID).GetCompletionCnt());
      phen.SetTaskWastedCompletions(eval_trial, taskID, task_set.GetTask(taskID).GetWastedCompletionsCnt());
    }
  });

  // Advance agent action
  agent_advance_sig.AddAction([this](Agent & agent) {
    const size_t agent_id = agent.GetID();
    eval_hw->SingleProcess();
    if ((size_t)eval_hw->GetTrait(TRAIT_ID__STATE) == env_state) {
      agent_phen_cache[agent_id].IncEnvMatchScore(eval_trial);
    }
  });

  switch (ENVIRONMENT_CHANGE_METHOD) {
    case ENV_CHG_ID__RANDOM:
      env_advance_sig.AddAction([this]() {
        if (env_state == (size_t)-1 || random->P(ENVIRONMENT_CHANGE_PROB)) {
          // Trigger change!
          // 1) Change the environment to a random state.
          env_state = random->GetUInt(ENVIRONMENT_STATES);
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env_state_tags[env_state]);
        }
      });
      break;
    case ENV_CHG_ID__REGULAR:
      env_advance_sig.AddAction([this]() {
        if ((eval_time % ENVIRONMENT_CHANGE_INTERVAL) == 0) {
          // Trigger change!
          // 1) Change the environment to a random state.
          env_state = random->GetUInt(ENVIRONMENT_STATES);
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env_state_tags[env_state]);
        }
      });
      break;
    default:
      std::cout << "Unrecognized environment change method. Exiting..." << std::endl;
      exit(-1);
  }
}

void Experiment::Config_Analysis() {
  // Print tags
  std::cout << "Environment states: " << std::endl;
  for (size_t i = 0; i < env_state_tags.size(); ++i) {
    int tag_int = env_state_tags[i].GetUInt(0);
    std::cout << "[" << i << "]: "; env_state_tags[i].Print(); std::cout << "(" << tag_int << ")" << std::endl;
  }
  std::cout << "--" << std::endl;

  // Advance agent action
  agent_advance_sig.AddAction([this](Agent & agent) {
    const size_t agent_id = agent.GetID();
    eval_hw->SingleProcess();
    if ((size_t)eval_hw->GetTrait(TRAIT_ID__STATE) == env_state) {
      agent_phen_cache[agent_id].IncEnvMatchScore(eval_trial);
    }
  });

  // score: uniques tasks credited + unique tasks completed + (eval time - time took to get full credit) + env matches
  calc_score = [this](Agent & agent) {
    double score = 0;
    score += task_set.GetUniqueTasksCredited();
    score += task_set.GetUniqueTasksCompleted();
    if (task_set.AllTasksCredited()) {
      score += (EVAL_TIME - task_set.GetAllTasksCreditedTime());
    }
    score += agent_phen_cache[agent.GetID()].GetEnvMatchScore(eval_trial);
    return score;
  };


  // Begin eval trial action
  begin_trial_sig.AddAction([this](Agent & agent) {
    // 1) Reset tasks.
    task_inputs[0] = random->GetUInt(MIN_TASK_INPUT, MAX_TASK_INPUT);
    task_inputs[1] = random->GetUInt(MIN_TASK_INPUT, MAX_TASK_INPUT);
    input_load_id = 0;
    task_set.SetInputs(task_inputs);
    // 2) Reset hardware
    eval_hw->ResetHardware();
    eval_hw->SetTrait(TRAIT_ID__STATE, -1);
    eval_hw->SpawnCore(0, memory_t(), true);
  });

  record_cur_phenotype_sig.AddAction([this](Agent & agent) {
    const size_t agent_id = agent.GetID();
    Phenotype & phen = agent_phen_cache[agent_id];
    // Record everything that can only be recorded pos-trial.
    phen.SetScore(eval_trial, calc_score(agent));
    // std::cout << "  Score: " << phen.GetScore(eval_trial) << std::endl;
    phen.SetTimeAllTasksCredited(eval_trial, task_set.GetAllTasksCreditedTime());
    phen.SetUniqueTasksCredited(eval_trial, task_set.GetUniqueTasksCredited());
    phen.SetUniqueTasksCompleted(eval_trial, task_set.GetUniqueTasksCompleted());
    phen.SetTotalWastedCompletions(eval_trial, task_set.GetTotalTasksWasted());
    for (size_t taskID = 0; taskID < task_set.GetSize(); ++taskID) {
      phen.SetTaskCredited(eval_trial, taskID, task_set.GetTask(taskID).GetCreditedCnt());
      phen.SetTaskCompleted(eval_trial, taskID, task_set.GetTask(taskID).GetCompletionCnt());
      phen.SetTaskWastedCompletions(eval_trial, taskID, task_set.GetTask(taskID).GetWastedCompletionsCnt());
    }
  });

  switch (ENVIRONMENT_CHANGE_METHOD) {
    case ENV_CHG_ID__RANDOM:
      env_advance_sig.AddAction([this]() {
        if (env_state == (size_t)-1 || random->P(ENVIRONMENT_CHANGE_PROB)) {
          // Trigger change!
          // 1) Change the environment to a random state.
          env_state = random->GetUInt(ENVIRONMENT_STATES);
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env_state_tags[env_state]);
        }
      });
      break;
    case ENV_CHG_ID__REGULAR:
      env_advance_sig.AddAction([this]() {
        if ((eval_time % ENVIRONMENT_CHANGE_INTERVAL) == 0) {
          // Trigger change!
          // 1) Change the environment to a random state.
          env_state = random->GetUInt(ENVIRONMENT_STATES);
          // 2) Trigger environment state event.
          eval_hw->TriggerEvent("EnvSignal", env_state_tags[env_state]);
        }
      });
      break;
    default:
      std::cout << "Unrecognized environment change method. Exiting..." << std::endl;
      exit(-1);
  }


  // Run analysis.
  do_analysis_sig.AddAction([this]() {
    program_t analysis_prog(inst_lib);
    std::ifstream analysis_fstream(ANALYZE_AGENT_FPATH);
    if (!analysis_fstream.is_open()) {
      std::cout << "Failed to open ancestor program file(" << ANALYZE_AGENT_FPATH << "). Exiting..." << std::endl;
      exit(-1);
    }

    analysis_prog.Load(analysis_fstream);
    std::cout << " --- Analysis program: ---" << std::endl;
    analysis_prog.PrintProgramFull();


    Agent our_hero(analysis_prog);
    our_hero.SetID(0);
    agent_phen_cache[our_hero.GetID()].Reset();
    eval_hw->SetProgram(our_hero.GetGenome());
    this->Evaluate(our_hero);

    // Output stuff to file.
    // Output shit.
    std::ofstream prog_ofstream("./"+ANALYSIS_OUTPUT_FNAME);
    // Fill out the header.
    prog_ofstream << "trial,fitness";
    for (size_t tID = 0; tID < TRIAL_CNT; ++tID) {
      prog_ofstream << "\n" << tID << "," << agent_phen_cache[our_hero.GetID()].GetScore(tID);
    }
    prog_ofstream.close();

  });

}

void Experiment::Config_Tasks() {
  // Zero out task inputs.
  for (size_t i = 0; i < MAX_TASK_NUM_INPUTS; ++i) task_inputs[i] = 0;
  // Add tasks to task set.
  // NAND
  task_set.AddTask("NAND", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~(a&b));
  }, "NAND task");
  // NOT
  task_set.AddTask("NOT", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~a);
    task.solutions.emplace_back(~b);
  }, "NOT task");
  // ORN
  task_set.AddTask("ORN", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back((a|(~b)));
    task.solutions.emplace_back((b|(~a)));
  }, "ORN task");
  // AND
  task_set.AddTask("AND", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a&b);
  }, "AND task");
  // OR
  task_set.AddTask("OR", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a|b);
  }, "OR task");
  // ANDN
  task_set.AddTask("ANDN", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back((a&(~b)));
    task.solutions.emplace_back((b&(~a)));
  }, "ANDN task");
  // NOR
  task_set.AddTask("NOR", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~(a|b));
  }, "NOR task");
  // XOR
  task_set.AddTask("XOR", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(a^b);
  }, "XOR task");
  // EQU
  task_set.AddTask("EQU", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
    const task_io_t a = inputs[0], b = inputs[1];
    task.solutions.emplace_back(~(a^b));
  }, "EQU task");
  // ECHO
  // task_set.AddTask("ECHO", [this](taskset_t::Task & task, const std::array<task_io_t, MAX_TASK_NUM_INPUTS> & inputs) {
  //   const task_io_t a = inputs[0], b = inputs[1];
  //   task.solutions.emplace_back(a);
  //   task.solutions.emplace_back(b);
  // }, "ECHO task");


  // task_inputs[0] = 15;
  // task_inputs[1] = 3;
  // task_set.SetInputs(task_inputs);
  // std::cout << "Input a: " << task_inputs[0] << std::endl;
  // std::cout << "Input b: " << task_inputs[1] << std::endl;
  // for (size_t i = 0; i < task_set.GetSize(); ++i) {
  //   std::cout << "TASK: " << task_set.GetName(i) << std::endl;
  //   std::cout << "  Solutions:";
  //   for (size_t k = 0; k < task_set.GetTask(i).solutions.size(); ++k) {
  //     std::cout << "  " << task_set.GetTask(i).solutions[k];
  //   } std::cout << std::endl;
  // }

}

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
    inst_lib->AddInst("Terminate", Inst_Terminate, 0, "Kill current thread.");

    // Add experiment-specific instructions.
    if (TASKS_ON) {
      inst_lib->AddInst("Load-1", [this](hardware_t & hw, const inst_t & inst) { this->Inst_Load1(hw, inst); }, 1, "WM[ARG1] = TaskInput[LOAD_ID]; LOAD_ID++;");
      inst_lib->AddInst("Load-2", [this](hardware_t & hw, const inst_t & inst) { this->Inst_Load2(hw, inst); }, 2, "WM[ARG1] = TASKINPUT[0]; WM[ARG2] = TASKINPUT[1];");
      inst_lib->AddInst("Submit", [this](hardware_t & hw, const inst_t & inst) { this->Inst_Submit(hw, inst); }, 1, "Submit WM[ARG1] as potential task solution.");
      inst_lib->AddInst("Nand", Inst_Nand, 3, "WM[ARG3]=~(WM[ARG1]&WM[ARG2])");
    }

    // Add 1 set state instruction for every possible environment state.
    for (size_t i = 0; i < ENVIRONMENT_STATES; ++i) {
      inst_lib->AddInst("SetState-" + emp::to_string(i),
        [i](hardware_t & hw, const inst_t & inst) {
          hw.SetTrait(TRAIT_ID__STATE, i);
        }, 0, "Set internal state to " + emp::to_string(i));
    }

    if (SGP_ENVIRONMENT_SIGNALS) {
      // Use event-driven events.
      event_lib->AddEvent("EnvSignal", HandleEvent__EnvSignal_ED, "");
      event_lib->RegisterDispatchFun("EnvSignal", DispatchEvent__EnvSignal_ED);
    } else {
      // Use nop events.
      event_lib->AddEvent("EnvSignal", HandleEvent__EnvSignal_IMP, "");
      event_lib->RegisterDispatchFun("EnvSignal", DispatchEvent__EnvSignal_IMP);
    }

    if (SGP_ACTIVE_SENSORS) {
      // Add sensors to instruction set.
      for (int i = 0; i < ENVIRONMENT_STATES; ++i) {
        inst_lib->AddInst("SenseState-" + emp::to_string(i),
          [this, i](hardware_t & hw, const inst_t & inst) {
            state_t & state = hw.GetCurState();
            state.SetLocal(inst.args[0], this->env_state==i);
          }, 1, "Sense if current environment state is " + emp::to_string(i));
      }
    } else {
      // Add equivalent number of non-functional sensors.
      for (int i = 0; i < ENVIRONMENT_STATES; ++i) {
        inst_lib->AddInst("SenseState-" + emp::to_string(i),
          [this, i](hardware_t & hw, const inst_t & inst) { return; }, 0,
          "Sense if current environment state is " + emp::to_string(i));
      }
    }

    // Configure evaluation hardware.
    eval_hw = emp::NewPtr<hardware_t>(inst_lib, event_lib, random);
    eval_hw->SetMinBindThresh(SGP_HW_MIN_BIND_THRESH);
    eval_hw->SetMaxCores(SGP_HW_MAX_CORES);
    eval_hw->SetMaxCallDepth(SGP_HW_MAX_CALL_DEPTH);

}

// Events.
void Experiment::HandleEvent__EnvSignal_ED(hardware_t & hw, const event_t & event) { hw.SpawnCore(event.affinity, hw.GetMinBindThresh(), event.msg); }
void Experiment::HandleEvent__EnvSignal_IMP(hardware_t & hw, const event_t & event) { return; }
void Experiment::DispatchEvent__EnvSignal_ED(hardware_t & hw, const event_t & event) { hw.QueueEvent(event); }
void Experiment::DispatchEvent__EnvSignal_IMP(hardware_t & hw, const event_t & event) { return; }

/// Instruction: Fork
/// Description: Fork thread with local memory as new thread's input buffer.
void Experiment::Inst_Fork(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  hw.SpawnCore(inst.affinity, hw.GetMinBindThresh(), state.local_mem);
}

void Experiment::Inst_Nand(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  const task_io_t a = (task_io_t)state.GetLocal(inst.args[0]);
  const task_io_t b = (task_io_t)state.GetLocal(inst.args[1]);
  state.SetLocal(inst.args[2], ~(a&b));
}

void Experiment::Inst_Terminate(hardware_t & hw, const inst_t & inst) {
  // Pop all the call states from current core.
  exec_stk_t & core = hw.GetCurCore();
  core.resize(0);
}

void Experiment::Inst_Load1(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  state.SetLocal(inst.args[0], task_inputs[input_load_id]); // Load input.
  input_load_id += 1;
  if (input_load_id >= task_inputs.size()) input_load_id = 0; // Update load ID.
}

void Experiment::Inst_Load2(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  state.SetLocal(inst.args[0], task_inputs[0]);
  state.SetLocal(inst.args[1], task_inputs[1]);
}

void Experiment::Inst_Submit(hardware_t & hw, const inst_t & inst) {
  state_t & state = hw.GetCurState();
  // Credit?
  const bool credit = hw.GetTrait(TRAIT_ID__STATE) == env_state;
  // Submit!
  task_set.Submit((task_io_t)state.GetLocal(inst.args[0]), eval_time, credit);
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
size_t Experiment::Mutate(Agent & agent, emp::Random & rnd) {
  program_t & program = agent.GetGenome();
  size_t mut_cnt = 0;
  size_t expected_prog_len = program.GetInstCnt();

  // Duplicate a (single) function?
  if (rnd.P(SGP__PER_FUNC__FUNC_DUP_RATE) && program.GetSize() < SGP_PROG_MAX_FUNC_CNT) {
    const uint32_t fID = rnd.GetUInt(program.GetSize());
    // Would function duplication make expected program length exceed max?
    if (expected_prog_len + program[fID].GetSize() <= SGP_PROG_MAX_TOTAL_LEN) {
      program.PushFunction(program[fID]);
      expected_prog_len += program[fID].GetSize();
      ++mut_cnt;
    }
  }

  // Delete a (single) function?
  if (rnd.P(SGP__PER_FUNC__FUNC_DEL_RATE) && program.GetSize() > SGP_PROG_MIN_FUNC_CNT) {
    const uint32_t fID = rnd.GetUInt(program.GetSize());
    expected_prog_len -= program[fID].GetSize();
    program[fID] = program[program.GetSize() - 1];
    program.program.resize(program.GetSize() - 1);
    ++mut_cnt;
  }

  // For each function...
  for (size_t fID = 0; fID < program.GetSize(); ++fID) {

    // Mutate affinity
    for (size_t i = 0; i < program[fID].GetAffinity().GetSize(); ++i) {
      tag_t & aff = program[fID].GetAffinity();
      if (rnd.P(SGP__PER_BIT__TAG_BFLIP_RATE)) {
        ++mut_cnt;
        aff.Set(i, !aff.Get(i));
      }
    }

    // Slip-mutation?
    if (rnd.P(SGP__PER_FUNC__SLIP_RATE)) {
      uint32_t begin = rnd.GetUInt(program[fID].GetSize());
      uint32_t end = rnd.GetUInt(program[fID].GetSize());
      const bool dup = begin < end;
      const bool del = begin > end;
      const int dup_size = end - begin;
      const int del_size = begin - end;
      // If we would be duplicating and the result will not exceed maximum program length, duplicate!
      if (dup && (expected_prog_len+dup_size <= SGP_PROG_MAX_TOTAL_LEN) && (program[fID].GetSize()+dup_size <= SGP_PROG_MAX_FUNC_LEN)) {
        // duplicate begin:end
        const size_t new_size = program[fID].GetSize() + (size_t)dup_size;
        hardware_t::Function new_fun(program[fID].GetAffinity());
        for (size_t i = 0; i < new_size; ++i) {
          if (i < end) new_fun.PushInst(program[fID][i]);
          else new_fun.PushInst(program[fID][i - dup_size]);
        }
        program[fID] = new_fun;
        ++mut_cnt;
        expected_prog_len += dup_size;
      } else if (del && ((program[fID].GetSize()-del_size) >= SGP_PROG_MIN_FUNC_LEN)) {
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
    for (size_t i = 0; i < program[fID].GetSize(); ++i) {
      inst_t & inst = program[fID][i];
      // Mutate affinity (even when it doesn't use it).
      for (size_t k = 0; k < inst.affinity.GetSize(); ++k) {
        if (rnd.P(SGP__PER_BIT__TAG_BFLIP_RATE)) {
          ++mut_cnt;
          inst.affinity.Set(k, !inst.affinity.Get(k));
        }
      }

      // Mutate instruction.
      if (rnd.P(SGP__PER_INST__SUB_RATE)) {
        ++mut_cnt;
        inst.id = rnd.GetUInt(program.GetInstLib()->GetSize());
      }

      // Mutate arguments (even if they aren't relevent to instruction).
      for (size_t k = 0; k < hardware_t::MAX_INST_ARGS; ++k) {
        if (rnd.P(SGP__PER_INST__SUB_RATE)) {
          ++mut_cnt;
          inst.args[k] = rnd.GetInt(SGP__PROG_MAX_ARG_VAL);
        }
      }

    }

    // Insertion/deletion mutations?
    // - Compute number of insertions.
    int num_ins = rnd.GetRandBinomial(program[fID].GetSize(), SGP__PER_INST__INS_RATE);
    // Ensure that insertions don't exceed maximum program length.
    if ((num_ins + program[fID].GetSize()) > SGP_PROG_MAX_FUNC_LEN) {
      num_ins = SGP_PROG_MAX_FUNC_LEN - program[fID].GetSize();
    }
    if ((num_ins + expected_prog_len) > SGP_PROG_MAX_TOTAL_LEN) {
      num_ins = SGP_PROG_MAX_TOTAL_LEN - expected_prog_len;
    }
    expected_prog_len += num_ins;

    // Do we need to do any insertions or deletions?
    if (num_ins > 0 || SGP__PER_INST__DEL_RATE > 0.0) {
      size_t expected_func_len = num_ins + program[fID].GetSize();
      // Compute insertion locations and sort them.
      emp::vector<size_t> ins_locs = emp::RandomUIntVector(rnd, num_ins, 0, program[fID].GetSize());
      if (ins_locs.size()) std::sort(ins_locs.begin(), ins_locs.end(), std::greater<size_t>());
      hardware_t::Function new_fun(program[fID].GetAffinity());
      size_t rhead = 0;
      while (rhead < program[fID].GetSize()) {
        if (ins_locs.size()) {
          if (rhead >= ins_locs.back()) {
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
        if (rnd.P(SGP__PER_INST__DEL_RATE) && (expected_func_len > SGP_PROG_MIN_FUNC_LEN)) {
          ++mut_cnt;
          --expected_prog_len;
          --expected_func_len;
        } else {
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
