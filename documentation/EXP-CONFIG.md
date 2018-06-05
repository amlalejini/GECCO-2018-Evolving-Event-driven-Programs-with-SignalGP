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
- `WM`: Indicates working memory. 
  - `WM[i]`: indicates accessing working memory at location `i`. 
- `IM`: Indicates input memory. 
  - `IM[i]`: indicates accessing output memory at location `i`.
- `OM`: Indicates output memory. 
  - `OM[i]`: indicates accessing output memory at location `i`. 
- `SM`: Indicates shared memory. 
  - `SM[i]`: indicates accessing shared memory at location `i`.
- `EOF`: Indicates the end of a function. 
- `NOP`: Indicates no operation. (do nothing)

### Table of default instructions

| Instruction | # Arguments | Uses Tag? | Description |
| :---        | :---:       | :---:     | :---        |
| `Inc`       | 1           | No        | `WM[ARG1] = WM[ARG1] + 1` | 
| `Dec`       | 1           | No        | `WM[ARG1] = WM[ARG1] - 1` | 
| `Not`       | 1           | No        | `WM[ARG1] = !WM[ARG1]` Logically toggle `WM[ARG1]` | 
| `Add`       | 3           | No        | `WM[ARG3] = WM[ARG1] + WM[ARG2]` | 
| `Sub`       | 3           | No        | `WM[ARG3] = WM[ARG1] - WM[ARG2]` | 
| `Mult`      | 3           | No        | `WM[ARG3] = WM[ARG1] * WM[ARG2]` | 
| `Div`       | 3           | No        | `WM[ARG3] = WM[ARG1] / WM[ARG2]` Safe; division by 0 is NOP. | 
| `Mod`       | 3           | No        | `WM[ARG3] = WM[ARG1] % WM[ARG2]` Safe| 
| `TestEqu`   | 3           | No        | `WM[ARG3] = (WM[ARG1] == WM[ARG2])` | 
| `TestNEqu`  | 3           | No        | `WM[ARG3] = (WM[ARG1] != WM[ARG2])` | 
| `TestLess`  | 3           | No        | `WM[ARG3] = (WM[ARG1] < WM[ARG2])` | 
| `If`        | 1           | No        | `If WM[ARG1] != 0`, proceed; `else`, skip until next `Close` or `EOF` | 
| `While`     | 1           | No        | `If WM[ARG1] != 0`, loop; `else`, skip until next `Close` or `EOF` | 
| `Countdown` | 1           | No        | Same as `While`, but decrements `WM[ARG1]` | 
| `Close`     | 0           | No        | Indicates end of looping or conditional instruction block | 
| `Break`     | 0           | No        | Break out of current loop | 
| `Call`      | 0           | Yes       | Call function referenced by tag | 
| `Return`    | 0           | No        | Return from current function | 
| `Fork`      | 0           | Yes       | Calls function referenced by tag on a new thread | 
| `SetMem`    | 2           | No        | `WM[ARG1] = ARG2` | 
| `CopyMem`   | 2           | No        | `WM[ARG1] = WM[ARG2]` | 
| `SwapMem`   | 2           | No        | `Swap(WM[ARG1], WM[ARG2])` | 
| `Input`     | 2           | No        | `WM[ARG2] = IM[ARG1]` | 
| `Output`    | 2           | No        | `OM[ARG2] = WM[ARG1]` | 
| `Commit`    | 2           | No        | `SM[ARG2] = WM[ARG1]` | 
| `Pull`      | 2           | No        | `WM[ARG2] = SM[ARG1]` | 
| `Nop`       | 0           | No        | `No-operation` | 

## Changing Environment Problem



## Distributed Leader Election Problem


## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523