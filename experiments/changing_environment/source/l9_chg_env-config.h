#ifndef CHANGING_ENVIRONMENT_CONFIG_H
#define CHANGING_ENVIRONMENT_CONFIG_H

#include "config/config.h"
// TODO: Update configs (both what's in here and the descriptions)!
EMP_BUILD_CONFIG( L9ChgEnvConfig,
  GROUP(DEFAULT_GROUP, "General Settings"),
  VALUE(RUN_MODE, size_t, 0, "What mode are we running in? \n0: Native experiment\n1: Analyze mode"),
  VALUE(RANDOM_SEED, int, -1, "Random number seed (negative value for based on time)"),
  VALUE(POP_SIZE, size_t, 1000, "Total population size"),
  VALUE(GENERATIONS, size_t, 100, "How many generations should we run evolution?"),
  VALUE(EVAL_TIME, size_t, 256, "Agent evaluation time"),
  VALUE(TRIAL_CNT, size_t, 3, "..."),
  VALUE(TASKS_ON, bool, true, "Run with or without tasks?"),
  VALUE(ANCESTOR_FPATH, std::string, "ancestor.gp", "Ancestor program file"),
  GROUP(ENVIRONMENT_GROUP, "Environment Settings"),
  VALUE(ENVIRONMENT_STATES, size_t, 8, "Total possible number of environment states"),
  VALUE(ENVIRONMENT_TAG_GENERATION_METHOD, size_t, 0, "How should we generate environment tags?\n0: Randomly\n1: Load from file"),
  VALUE(ENVIRONMENT_TAG_FPATH, std::string, "env_tags.csv", "Path to file where we should load/save environment tags."),
  VALUE(ENVIRONMENT_CHANGE_METHOD, size_t, 0, "How does the environment change? \n0: Randomly\n1: Regular intervals?"),
  VALUE(ENVIRONMENT_CHANGE_PROB, double, 0.125, "Probability of environment change (if changing randomly)"),
  VALUE(ENVIRONMENT_CHANGE_INTERVAL, size_t, 32, "Number of timesteps between environment changes"),
  GROUP(SELECTION_GROUP, "Selection Settings"),
  VALUE(TOURNAMENT_SIZE, size_t, 4, "How big are tournaments when using tournament selection or any selection method that uses tournaments?"),
  VALUE(SELECTION_METHOD, size_t, 0, "Which selection method are we using? \n0: Tournament\n1: Lexicase\n2: Eco-EA (resource)\n3: MAP-Elites\n4: Roulette"),
  VALUE(ELITE_SELECT__ELITE_CNT, size_t, 1, "How many elites get free reproduction passes?"),
  GROUP(SGP_PROGRAM_GROUP, "SignalGP program Settings"),
  VALUE(SGP_PROG_MAX_FUNC_CNT, size_t, 8, "Used for generating SGP programs. How many functions do we generate?"),
  VALUE(SGP_PROG_MIN_FUNC_CNT, size_t, 1, "Used for generating SGP programs. How many functions do we generate?"),
  VALUE(SGP_PROG_MAX_FUNC_LEN, size_t, 8, ".."),
  VALUE(SGP_PROG_MIN_FUNC_LEN, size_t, 1, ".."),
  VALUE(SGP_PROG_MAX_TOTAL_LEN, size_t, 256, "Maximum length of SGP programs."),
  GROUP(SGP_HARDWARE_GROUP, "SignalGP Hardware Settings"),
  VALUE(SGP_ENVIRONMENT_SIGNALS, bool, true, "Can environment signals trigger functions?"),
  VALUE(SGP_ACTIVE_SENSORS, bool, true, "Do agents have function active sensors?"),
  VALUE(SGP_HW_MAX_CORES, size_t, 16, "Max number of hardware cores; i.e., max number of simultaneous threads of execution hardware will support."),
  VALUE(SGP_HW_MAX_CALL_DEPTH, size_t, 128, "Max call depth of hardware unit"),
  VALUE(SGP_HW_MIN_BIND_THRESH, double, 0.0, "Hardware minimum referencing threshold"),
  GROUP(SGP_MUTATION_GROUP, "SignalGP Mutation Settings"),
  VALUE(SGP__PROG_MAX_ARG_VAL, int, 16, "Maximum argument value for instructions."),
  VALUE(SGP__PER_BIT__TAG_BFLIP_RATE, double, 0.005, "Per-bit mutation rate of tag bit flips."),
  VALUE(SGP__PER_INST__SUB_RATE, double, 0.005, "Per-instruction/argument subsitution rate."),
  VALUE(SGP__PER_INST__INS_RATE, double, 0.005, "Per-instruction insertion mutation rate."),
  VALUE(SGP__PER_INST__DEL_RATE, double, 0.005, "Per-instruction deletion mutation rate."),
  VALUE(SGP__PER_FUNC__SLIP_RATE, double, 0.05, "Per-function rate of slip mutations."),
  VALUE(SGP__PER_FUNC__FUNC_DUP_RATE, double, 0.05, "Per-function rate of function duplications."),
  VALUE(SGP__PER_FUNC__FUNC_DEL_RATE, double, 0.05, "Per-function rate of function deletions."),
  GROUP(DATA_GROUP, "Data Collection Settings"),
  VALUE(SYSTEMATICS_INTERVAL, size_t, 100, "Interval to record systematics summary stats."),
  VALUE(FITNESS_INTERVAL, size_t, 100, "Interval to record fitness summary stats."),
  VALUE(POP_SNAPSHOT_INTERVAL, size_t, 10000, "Interval to take a full snapshot of the population."),
  VALUE(DATA_DIRECTORY, std::string, "./", "Location to dump data output."),
  GROUP(ANALYSIS_GROUP, "Analysis Settings"),
  VALUE(ANALYSIS, size_t, 0, "..."),
  VALUE(ANALYZE_AGENT_FPATH, std::string, "ancestor.gp", "Path to single agent program to analzye."),
  VALUE(ANALYSIS_OUTPUT_FNAME, std::string, "analysis.csv", "...")
)

#endif
