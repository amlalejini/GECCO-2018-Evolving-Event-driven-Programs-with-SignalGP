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

    parser.add_argument("-u", "--update", type=int, help="Update to get information for. If no update is given, last line of fitness file is used.")
    parser.add_argument("-df", "--dominant_file", type=str, help="File to pull dominant information from.")


    args = parser.parse_args()

    data_directory = args.data_directory
    benchmark = args.benchmark
    
    dump = os.path.join(aggregator_dump, benchmark)

    # Get a list of all runs.
    runs = [d for d in os.listdir(data_directory) if "_DELAY" in d]
    runs.sort()

    dom_fname = args.dominant_file
    dom_update = args.update
    if (None in [dom_fname, dom_update]):
        print("Missing dominant information information!")
    
    print("Aggregating dominant fitness information (@ given update and over time)!")
    mkdir_p(dump)
    info_by_run = {}
    for run in runs:
        run_dir = os.path.join(data_directory, run)
        dom_fpath = os.path.join(run_dir, "output", dom_fname)
        print("Run: " + run_dir)
        # Get some basic run information from directory name.
        run_info = run.split("_")
        treat = "_".join(run_info[:-1])
        run_id = run_info[-1]
        # Extract dominant file content.
        dom_content = None
        with open(dom_fpath, "r") as fp:
            dom_content = fp.read().split("\n")
        # Extract and strip off header information.
        header = dom_content[0].split(",")
        header_lu = {header[i].strip():i for i in range(0, len(header))}
        dom_content = dom_content[1:]
        info_by_run[run] = {}
        info_by_run[run]["update_seq"] = []
        info_by_run[run]["update_consensus_evolved"] = "NEVER"
        info_by_run[run]["treatment"] = treat
        info_by_run[run]["run_id"] = run_id
        for line in dom_content:
            # update, score, full_consensus_time, most_recent_consensus_streak, max_consensus_size, valid_vote_cnt, msgs_sent, min_uid, max_uid, leader_uid
            line = [col.strip() for col in line.split(",")]
            if (len(line) != len(header)): continue
            update = line[header_lu["update"]]
            info_by_run[run]["update_seq"].append(update)
            fitness = line[header_lu["score"]]
            time_at_consensus = line[header_lu["full_consensus_time"]]
            achieved_consensus = "0" if time_at_consensus == "0" else "1"
            if (achieved_consensus == "1" and info_by_run[run]["update_consensus_evolved"] == "NEVER"):
                info_by_run[run]["update_consensus_evolved"] = update
            msgs = line[header_lu["msgs_sent"]]
            election_strategy = None
            min_uid = line[header_lu["min_uid"]]
            max_uid = line[header_lu["max_uid"]]
            leader_uid = line[header_lu["leader_uid"]]
            if (achieved_consensus == "0"): election_strategy = "0"
            elif (leader_uid == min_uid): election_strategy = "1"
            elif (leader_uid == max_uid): election_strategy = "2"
            else: election_strategy = "3"
            info_by_run[run][update] = {}
            info_by_run[run][update]["update"] = update
            info_by_run[run][update]["fitness"] = fitness
            info_by_run[run][update]["time_at_consensus"] = time_at_consensus
            info_by_run[run][update]["achieved_consensus"] = achieved_consensus
            info_by_run[run][update]["msgs"] = msgs
            info_by_run[run][update]["election_strategy"] = election_strategy
    
    # Okay, now I have information about every run at every update.
    # - Write fot file.
    fot_content = "benchmark,treatment,run_id,update,fitness,time_at_consensus,msgs\n"
    uf_content = "benchmark,treatment,run_id,fitness,time_at_consensus,update_consensus_evolved,achieved_consensus,msgs,election_strategy\n"
    for run in info_by_run:
        run_info = info_by_run[run]
        update_sequence = run_info["update_seq"]
        # Save out FOT for this run
        for update in update_sequence:
            update_info = run_info[update]
            fot_content += ",".join([benchmark, run_info["treatment"], run_info["run_id"], update, update_info["fitness"], update_info["time_at_consensus"], update_info["msgs"]]) + "\n"
        # Save out update stats
        update_info = run_info[str(dom_update)]
        #                       benchmark,treatment,              run_id,             fitness,                time_at_consensus,                update_consensus_evolved,                achieved_consensus,                msgs,              election_strategy\n"
        uf_content += ",".join([benchmark, run_info["treatment"], run_info["run_id"], update_info["fitness"], update_info["time_at_consensus"], run_info["update_consensus_evolved"], update_info["achieved_consensus"], update_info["msgs"], update_info["election_strategy"]]) + "\n"

    with open(os.path.join(dump, "consensus_fot.csv"), "w") as fp:
        fp.write(fot_content)
    
    with open(os.path.join(dump, "consensus_{}.csv".format(dom_update)), "w") as fp:
        fp.write(uf_content) 

if __name__ == "__main__":
    main()
