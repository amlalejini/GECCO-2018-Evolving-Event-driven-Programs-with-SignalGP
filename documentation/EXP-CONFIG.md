---
title: Experiment Configuration Details
---

## Overview
This page documents miscellaneous configuration details for experiments from our GECCO 2018 paper, Evolving Event-Driven Programs with SignalGP (Lalejini and Ofria, 2018). A pre-print of this work can be found here: [https://arxiv.org/pdf/1804.05445.pdf](https://arxiv.org/pdf/1804.05445.pdf)

See the SignalGP section of our paper for context on SignalGP virtual hardware/program evaluation.

## Default SignalGP Instruction Set
Here, we provide details on the default SignalGP instruction set used in this experiment. These instructions are used across both the changing environment and distributed leader election problems. 

Relevant abbreviations:
- ARG1, ARG2, ARG3: First, second, and third argument for an instruction.
- WM: Indicates working memory. WM[_i_]: indicates accessing working memory at location _i_. 
- IM: Indicates input memory. IM[_i_]: indicates accessing output memory at location _i_.
- OM: Indicates output memory. OM[_i_]: indicates accessing output memory at location _i_. 
- SM: Indicates shared memory. SM[_i_]: indicates accessing shared memory at location _i_.
- EOF: Indicates the end of a function. 

### Table of default instructions
| Instruction | # Arguments | Uses Tag? | Description |
| :---        | :---:       | :---:     | :---        |
| Content Cell  | Content Cell  | Content Cell  | Content Cell  |
| Content Cell  | Content Cell  | Content Cell  | Content Cell  |

## Changing Environment Problem



## Distributed Leader Election Problem


## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523