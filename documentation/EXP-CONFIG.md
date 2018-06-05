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

### Environment-state Tag Associations
In the event-driven and combined treatments for the changing environment problem, environmental changes produce signals that have environment-specific tags. These tags determine which of a program's functions (if any at all) will be run in response to the signal. 

| Environment-state   | Tag (16-bit) |
| :---: | :---: |
| 0 | 0000000000000000 |
| 1 | 1111111111111111 |
| 2 | 1111000000001111 |
| 3 | 0000111111110000 |
| 4 | 1111000011110000 |
| 5 | 0000111100001111 |
| 6 | 0000000011111111 |
| 7 | 1111111100000000 |
| 8 | 0110011001100110 |
| 9 | 1001100110011001 |
| 10 | 1001011001101001 |
| 11 | 0110100110010110 |
| 12 | 0110011010011001 |
| 13 | 1001100101100110 |
| 14 | 1001011010010110 |
| 15 | 0110100101101001 |

For example, if the environment changes to state 2, an event with the 1111000000001111 tag will be generated. As a result, the function of the program being evaluated with the closest matching tag to 1111000000001111 will be triggered. 

### Hand-coded Solutions
Here, we give hand-coded SignalGP programs that solve the eight-state changing environment problem. 

#### Event-driven Program
The following program follows the event-driven paradigm and can perfectly solve the eight-state changing environment problem. 

`Fn-` denotes the beginning of a function; the function's tag follows `Fn-`. All instructions below a function declaration up until the next function declaration belong to that function. Instructions may be followed by tags given in square brackets, and/or followed by arguments given in parentheses.

```
Fn−0000000000000000: 
  SetState0

Fn−1111111111111111: 
  SetState1

Fn−1111000000001111: 
  SetState2

Fn−0000111111110000: 
  SetState3

Fn−1111000011110000: 
  SetState4

Fn−0000111100001111: 
  SetState5

Fn−0000000011111111: 
  SetState6

```

#### Imperative Program
The following program follows the imperative paradigm. Note that this program cannot solve the changing environment problem perfectly because the resolution of environmental sensing is not fast enough to perfectly track environmental changes at the rate they occur.

`Fn-` denotes the beginning of a function; the function's tag follows `Fn-`. All instructions below a function declaration up until the next function declaration belong to that function. Instructions may be followed by tags given in square brackets, and/or followed by arguments given in parentheses.

```
Fn−0000000000000000: 
  SetState0
  Fork[0000000000000001] 
  Fork[0000000000000011] 
  Fork[0000000000000111]  
  Fork[0000000000001111] 
  Fork[0000000000011111] 
  Fork[0000000000111111]   
  Fork[0000000001111111] 
  Fork[0000000011111111] 
  SetMem(0 ,1)
  While(0) 
    SenseEnvState0(1) 
    If(1)
      SetState0

Fn−0000000000000001: 
  SetMem(0 ,1)
  While(0)
    SenseEnvState1(1) 
    If(1)
      SetState1

Fn−0000000000000011: 
  SetMem(0 ,1) 
  While(0)
    SenseEnvState2(1) 
    If(1)
      SetState2

Fn−0000000000000111: 
  SetMem(0 ,1) 
  While(0)
    SenseEnvState3(1) 
    If(1)
      SetState3

Fn−0000000000001111: 
  SetMem(0 ,1) 
  While(0)
    SenseEnvState4(1) 
    If(1)
      SetState4

Fn−0000000000011111: 
  SetMem(0 ,1) 
  While(0)
    SenseEnvState5(1) 
    If(1)
      SetState5

Fn−0000000000111111: 
  SetMem(0 ,1) 
  While(0)
    SenseEnvState6(1) 
    If(1)
      SetState6

Fn−0000000001111111: 
  SetMem(0 ,1) 
  While(0)
    SenseEnvState7(1) 
    If(1)
      SetState7
```

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

### Hand-coded Solutions
Here, we provide hand-coded SignalGP programs for the distributed leader election problem. Each of these programs can successfully reach consensus. 

#### Event-driven solution
```
Fn−0000000000000000: 
GetUID(0)
SetOpinion(0) 
SetMem(15 ,1) 
While(15)
  GetOpinion (0)
  Output(0 ,0)
  BroadcastMsg(0 ,0 ,0)[1111111111111111]
Close

Fn−1111111111111111: 
  Input(0 ,1) 
  GetOpinion(0) 
  TestLess(1 ,0 ,2)
  If(2) 
    SetOpinion(1)
```

#### Imperative (fork-on-retrieve) solution
```
Fn−0000000000000000: 
  GetUID(0) 
  SetOpinion(0) 
  SetMem(15 ,1) 
  While(15)
    GetOpinion(0)
    Output(0 ,0)
    BroadcastMsg(0 ,0 ,0)[1111111111111111] 
    RetrieveMsg
  Close

Fn−1111111111111111: 
  Input(0 ,1) 
  GetOpinion(0) 
  TestLess(1 ,0 ,2)
  If(2) 
    SetOpinion(1)
```

#### Imperative (copy-on-retrieve) solution
```
Fn−0000000000000000: 
  GetUID(0) 
  GetUID(1) 
  SetOpinion (0) 
  SetMem(15 ,1) 
  While(15)
    GetOpinion (0)
    Output(0 ,0)
    BroadcastMsg(0 ,0 ,0)[1111111111111111] 
    RetrieveMsg
    Input(0 ,1)
    TestLess(1 ,0 ,2)
    If (2) 
      SetOpinion(1)
    Close 
  Close
```

## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523