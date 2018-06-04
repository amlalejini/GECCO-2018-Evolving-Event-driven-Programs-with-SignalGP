'''
aggregator.py
This script does the following:
    * Aggregate final fitness data for a benchmark into a .csv:
        - benchmark, treatment, run_id, final_update, mean_fitness, max_fitness
'''


import argparse, os, copy, errno

aggregator_dump = "./aggregated_data"

def mkdir_p(path):
    """
    This is functionally equivalent to the mkdir -p [fname] bash command
    """
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

def main():
    parser = argparse.ArgumentParser(description="Data aggregation script.")
    parser.add_argument("data_directory", type=str, help="Target experiment directory.")
    parser.add_argument("benchmark", type=str, help="What benchmark is this?")

    parser.add_argument("-u", "--update", type=int, help="Update to get fitness for. If no update is given, last line of fitness file is used.")
    parser.add_argument("-f", "--fitness_file", type=str, help="File to pull fitness from.")
    parser.add_argument("-mtf", "--mt_fitness_file", type=str, action="append")

    args = parser.parse_args()

    data_directory = args.data_directory
    benchmark = args.benchmark
    dump = os.path.join(aggregator_dump, benchmark)

    # Get a list of all runs.
    runs = [d for d in os.listdir(data_directory) if "TSK0" in d]
    runs.sort()

    fit_update = args.update
    fit_file = args.fitness_file
    mt_fit_files = args.mt_fitness_file

    if (mt_fit_files):
        print("Aggregating multi-trial fitnesses: " + str(mt_fit_files))
        mkdir_p(dump)
        ff_content="benchmark,treatment,run_id,analysis,fitness\n"
        for run in runs:
            run_dir = os.path.join(data_directory, run)
            print("Run: " + run)
            run_info = run.split("_")
            treat = "_".join(run_info[:-1])
            run_id = run_info[-1]
            for mt_file in mt_fit_files:
                analysis = ".".join(mt_file.split(".")[:-1])
                if (not mt_file in os.listdir(run_dir)): continue
                with open(os.path.join(run_dir, mt_file), "r") as fp:
                    fitness_contents = fp.readlines()
                header = fitness_contents[0].split(",")
                header_lu = {header[i].strip():i for i in range(0, len(header))}
                fitness_contents = fitness_contents[1:]
                # Aggregate data.
                trials = 0
                fit_agg = 0
                for line in fitness_contents:
                    line = line.split(",")
                    fitness = float(line[header_lu["fitness"]])
                    trials += 1
                    fit_agg += fitness
                agg_fitness = fit_agg / float(trials)
                ff_content += ",".join([benchmark, treat, run_id, analysis, str(agg_fitness)]) + "\n"
        with open(os.path.join(dump, "mt_final_fitness.csv"), "w") as fp:
            fp.write(ff_content)


if __name__ == "__main__":
    main()
