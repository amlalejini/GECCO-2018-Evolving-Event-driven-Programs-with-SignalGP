'''
ChgEnv-specific script to extract final dominant program from pop files.

'''

import argparse, os, copy, errno, subprocess

default_final_update = 10000

def main():
    parser = argparse.ArgumentParser(description="Experiment cleanup script.")
    parser.add_argument("directory", type=str, help="Target directory to clean up.")
    parser.add_argument("-u", "--update", type=int, help="Final update of experiment.")
    parser.add_argument("-r", "--run_fdom_analysis", action="store_true", help="Run analysis on fdom.")
    args = parser.parse_args()

    final_update = args.update if args.update else default_final_update
    exp_dir = args.directory

    print("Experiment data directory: " + exp_dir)
    print("  Final update: " + str(final_update))

    runs = [d for d in os.listdir(exp_dir) if "TSK0" in d]
    for run in runs:
        run_dir = os.path.join(exp_dir, run)
        output_dir = os.path.join(run_dir, "output")
        try:
            with open(os.path.join(output_dir, "pop_%d/pop_%d.pop" % (final_update, final_update)), "r") as fp:
                final_pop = fp.read()
        except:
            print("Could not open pop file for: " + run_dir)
            continue
        final_pop = final_pop.split("===")
        fdom = final_pop[0]
        fdom_fpath = os.path.join(run_dir, "fdom.gp")
        with open(fdom_fpath, "w") as fp:
            fp.write(fdom)

    if (args.run_fdom_analysis):
        for run in runs:
            run_dir = os.path.join(exp_dir, run)
            run_info = run.split("_")
            ED = "0" if run_info[0] == "ED0" else "1"
            AS = "0" if run_info[1] == "AS0" else "1"
            ENV = run_info[2][3:]
            RND = run_info[-1]
            args = {"TASKS_ON": "0",
                    "TRIAL_CNT": "100",
                    "ENVIRONMENT_STATES": ENV,
                    "SGP_ENVIRONMENT_SIGNALS": ED,
                    "SGP_ACTIVE_SENSORS": AS,
                    "ENVIRONMENT_TAG_GENERATION_METHOD": "1",
                    "ENVIRONMENT_TAG_FPATH": "env_tags.csv",
                    "ENVIRONMENT_CHANGE_METHOD": "0",
                    "RANDOM_SEED": RND,
                    "ENVIRONMENT_CHANGE_PROB":"0.125",
                    "RUN_MODE": "1",
                    "ANALYZE_AGENT_FPATH":"fdom.gp",
                    "ANALYSIS": "0",
                    "ANALYSIS_OUTPUT_FNAME": "fdom.csv"
                    }

            cp_cmd = "cp /mnt/home/lalejini/devo_ws/ALIFE2018-SGP-EA/logic9_changing_environment/l9_chg_env %s" % run_dir
            return_code = subprocess.call(cp_cmd, shell=True)

            arg_str = ["-%s %s" % (key, args[key]) for key in args]
            cmd = "./l9_chg_env " + " ".join(arg_str)
            print("Running: " + cmd)
            return_code = subprocess.call(cmd, shell = True, cwd = run_dir)

            # If combined treatment, we want to toggle sensors off and toggle events off (individually).
            if (ED == "1" and AS == "1"):
                print("Combined treatment! Do some extra stuff!")
                # 1) No sensors
                args["ANALYSIS_OUTPUT_FNAME"] = "no_sensors.csv"
                args["SGP_ENVIRONMENT_SIGNALS"] = "1"
                args["SGP_ACTIVE_SENSORS"] = "0"
                arg_str = ["-%s %s" % (key, args[key]) for key in args]
                cmd = "./l9_chg_env " + " ".join(arg_str)
                print("Running: " + cmd)
                return_code = subprocess.call(cmd, shell = True, cwd = run_dir)
                # 2) No signals
                args["ANALYSIS_OUTPUT_FNAME"] = "no_signals.csv"
                args["SGP_ENVIRONMENT_SIGNALS"] = "0"
                args["SGP_ACTIVE_SENSORS"] = "1"
                arg_str = ["-%s %s" % (key, args[key]) for key in args]
                cmd = "./l9_chg_env " + " ".join(arg_str)
                print("Running: " + cmd)
                return_code = subprocess.call(cmd, shell = True, cwd = run_dir)

if __name__ == "__main__":
    main()
