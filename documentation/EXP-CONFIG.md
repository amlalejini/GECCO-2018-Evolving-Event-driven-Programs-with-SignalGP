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

### Problem-specific Instructions
Instructions specific to the changing environment problem. 

| Instruction   | # Arguments | Uses Tag? | Description |
| :---          | :---:       | :---:     | :---        |
| Terminate     | 0           | No | Kill thread `Terminate` is executed on |
| SenseEnvState0 | 1           | No | `WM[ARG1] = 1` if environment in state 0; else, `WM[ARG1] = 0` |
| SenseEnvState1 | 1           | No | `WM[ARG1] = 1` if environment in state 1; else, `WM[ARG1] = 0` |
| SenseEnvState2 | 1           | No | `WM[ARG1] = 1` if environment in state 2; else, `WM[ARG1] = 0` |
| SenseEnvState3 | 1           | No | `WM[ARG1] = 1` if environment in state 3; else, `WM[ARG1] = 0` |
| SenseEnvState4 | 1           | No | `WM[ARG1] = 1` if environment in state 4; else, `WM[ARG1] = 0` |
| SenseEnvState5 | 1           | No | `WM[ARG1] = 1` if environment in state 5; else, `WM[ARG1] = 0` |
| SenseEnvState6 | 1           | No | `WM[ARG1] = 1` if environment in state 6; else, `WM[ARG1] = 0` |
| SenseEnvState7 | 1           | No | `WM[ARG1] = 1` if environment in state 7; else, `WM[ARG1] = 0` |
| SenseEnvState8 | 1           | No | `WM[ARG1] = 1` if environment in state 8; else, `WM[ARG1] = 0` |
| SenseEnvState9 | 1           | No | `WM[ARG1] = 1` if environment in state 9; else, `WM[ARG1] = 0` |
| SenseEnvState10 | 1           | No | `WM[ARG1] = 1` if environment in state 10; else, `WM[ARG1] = 0` |
| SenseEnvState11 | 1           | No | `WM[ARG1] = 1` if environment in state 11; else, `WM[ARG1] = 0` |
| SenseEnvState12 | 1           | No | `WM[ARG1] = 1` if environment in state 12; else, `WM[ARG1] = 0` |
| SenseEnvState13 | 1           | No | `WM[ARG1] = 1` if environment in state 13; else, `WM[ARG1] = 0` |
| SenseEnvState14 | 1           | No | `WM[ARG1] = 1` if environment in state 14; else, `WM[ARG1] = 0` |
| SenseEnvState15 | 1           | No | `WM[ARG1] = 1` if environment in state 15; else, `WM[ARG1] = 0` |
| SetState0 | 0 | No | Set internal state to 0 |
| SetState1 | 0 | No | Set internal state to 1 |
| SetState2 | 0 | No | Set internal state to 2 |
| SetState3 | 0 | No | Set internal state to 3 |
| SetState4 | 0 | No | Set internal state to 4 |
| SetState5 | 0 | No | Set internal state to 5 |
| SetState6 | 0 | No | Set internal state to 6 |
| SetState7 | 0 | No | Set internal state to 7 |
| SetState8 | 0 | No | Set internal state to 8 |
| SetState9 | 0 | No | Set internal state to 9 |
| SetState10 | 0 | No | Set internal state to 10 |
| SetState11 | 0 | No | Set internal state to 11 |
| SetState12 | 0 | No | Set internal state to 12 |
| SetState13 | 0 | No | Set internal state to 13 |
| SetState14 | 0 | No | Set internal state to 14 |
| SetState15 | 0 | No | Set internal state to 15 |

## Distributed Leader Election Problem

### Problem-specific Instructions
Instructions specific to the distributed leader election problem. 

In the distributed leader election problem, each SignalGP agent in a distribute system has a facing attribute that describes the direction the agent is currently facing: Up, Down, Left, or Right. 

| Instruction   | # Arguments | Uses Tag? | Description |
| :---          | :---:       | :---:     | :---        |
| RotCW | 0 | No | Rotate clockwise (90 degrees) |
| RotCCW | 0 | No | Rotate counter-clockwise (90 degrees) |
| RotDir | 1 | No | Rotate to face direction specified by (`WM[ARG1] % 4`) |
| RandomDir | 1 | No | `WM[ARG1]` is set to a random direction (values 0 through 3) |
| GetDir | 1 | No | `WM[ARG1]` is set to the agent's current orientation |
| SendMsgFacing | 0 | Yes | Send output memory as message to faced neighbor |
| BroadcastMsg | 0 | Yes | Broadcast output memory as message to all neighbors |
| RetrieveMsg | 0 | Yes | Retrieve message from message inbox |
| GetUID | 1 | No | `WM[ARG1]` = agent ID |
| GetOpinion | 1 | No | `WM[ARG1]` = current vote |
| SetOpinion | 1 | No | Set current vote to `WM[ARG1]` |


## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523