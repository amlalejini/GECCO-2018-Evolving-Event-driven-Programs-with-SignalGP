---
title: Experiment Configuration Details
---

## Experiment Configuration Details
This page documents miscellaneous configuration details for experiments from our GECCO 2018 paper, Evolving Event-Driven Programs with SignalGP (Lalejini and Ofria, 2018). A pre-print of this work can be found here: [https://arxiv.org/pdf/1804.05445.pdf](https://arxiv.org/pdf/1804.05445.pdf)

See the SignalGP section of our paper for context on SignalGP virtual hardware/program evaluation.

## Default SignalGP Instruction Set
Here, we provide details on the default SignalGP instruction set used in this experiment. These instructions are used across both the changing environment and distributed leader election problems. 

Relevant abbreviations:
- `ARG1`, `ARG2`, `ARG3`: First, second, and third argument for an instruction.
- `WM`: Indicates working memory. `WM[i]`: indicates accessing working memory at location `i`. 
- `IM`: Indicates input memory. `IM[i]`: indicates accessing output memory at location `i`.
- `OM`: Indicates output memory. `OM[i]`: indicates accessing output memory at location `i`. 
- `SM`: Indicates shared memory. `SM[i]`: indicates accessing shared memory at location `i`.
- `EOF`: Indicates the end of a function. 

### Table of default instructions

| Instruction | # Arguments | Uses Tag? | Description |
| :---        | :---:       | :---:     | :---        |
| Inc         | 1           | No        | `WM[ARG1] = WM[ARG1] + 1` | 
| Dec         | 1           | No        | | 
| Not         | 1           | No        | | 
| Add         | 3           | No        | | 
| Sub         | 3           | No        | | 
| Mult        | 3           | No        | | 
| Div         | 3           | No        | | 
| Mod         | 3           | No        | | 
| TestEqu     | 3           | No        | | 
| TestNEqu    | 3           | No        | | 
| TestLess    | 3           | No        | | 
| If          | 1           | No        | | 
| While       | 1           | No        | | 
| Countdown   | 1           | No        | | 
| Close       | 0           | No        | | 
| Break       | 0           | No        | | 
| Call        | 0           | Yes       | | 
| Return      | 0           | No        | | 
| Fork        | 0           | Yes       | | 
| SetMem      | 2           | No        | | 
| CopyMem     | 2           | No        | | 
| SwapMem     | 2           | No        | | 
| Input       | 2           | No        | | 
| Output      | 2           | No        | | 
| Commit      | 2           | No        | | 
| Pull        | 2           | No        | | 
| Nop         | 0           | No        | | 


## Changing Environment Problem



## Distributed Leader Election Problem


## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523