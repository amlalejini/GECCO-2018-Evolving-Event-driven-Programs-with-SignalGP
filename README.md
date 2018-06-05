## Overview
This repository is associated with our 2018 GECCO submission, Evolving Event-Driven Programs with SignalGP (Lalejini and Ofria, 2018).
A pre-print of this work can be found here: [https://arxiv.org/pdf/1804.05445.pdf](https://arxiv.org/pdf/1804.05445.pdf)

### Authors
- [Alexander Lalejini](http://lalejini.com)
- [Charles Ofria](http://ofria.com) (PhD advisor extraordinaire)

### Abstract
> We present SignalGP, a new genetic programming (GP) technique designed to incorporate the event-driven programming paradigm into computational evolution’s toolbox. Event-driven programming is a software design philosophy that simplifies the development of reactive programs by automatically triggering program modules (event-handlers) in response to external events, such as signals from the environment or messages from other programs. SignalGP incorporates these concepts by extending existing tag-based referencing techniques into an event-driven context. Both events and functions are labeled with evolvable tags; when an event occurs,the function with the closest matching tag is triggered. In this work, we apply SignalGP in the context of linear GP. We demonstrate the value of the event-driven paradigm using two distinct test problems (an environment coordination problem and a distributed leader election problem) by comparing SignalGP to variants that are otherwise identical, but must actively use sensors to process events or messages. In each of these problems, rapid interaction with the environment or other agents is critical for maximizing fitness. We also discuss ways in which SignalGP can be generalized beyond our linear GP implementation. 

### TL;DR (_i.e._, the abstract is _sooo_ long and technical, what does it say in three sentences?)
- In traditional GP techniques, evolution is typically given access to either the [imperative](https://en.wikipedia.org/wiki/Imperative_programming) or [functional](https://en.wikipedia.org/wiki/Functional_programming) programming paradigm. 
- We introduce SignalGP, a new GP technique designed to let evolution program using the [event-driven programming paradigm](https://en.wikipedia.org/wiki/Event-driven_programming).
- We use SignalGP to experimentally demonstrate the value of evolving event-driven programs in domains where programs must interact heavily with the environment or with other agents/programs. 

## Experiment Source Code
The source code and configs for the changing environment problem can be found here: [experiments/changing_environment](https://github.com/amlalejini/GECCO-2018-Evolving-Event-driven-Programs-with-SignalGP/tree/master/experiments/changing_environment).

The source code and configs for the distributed leader election problem can be found here: [experiments/election](https://github.com/amlalejini/GECCO-2018-Evolving-Event-driven-Programs-with-SignalGP/tree/master/experiments/election).

Both problems are implemented in C++ using the [Empirical library](https://github.com/devosoft/Empirical), which is required to compile and re-run the experiments. 

**WARNING:** the Empirical library is under development, and as a result, it can often change in ways that may break the code used for the experiments used in this work. I make no promises that I will keep these problems up to date with the latest changes to the Empirical library. However, **I am more than happy to update the code upon request**. Just submit an issue/email me (amlalejini@gmail.com). 

## Data and Analyses
The data for all of our experiments can be found in this repository: [data/](https://github.com/amlalejini/GECCO-2018-Evolving-Event-driven-Programs-with-SignalGP/tree/master/data).

The full statistical details along with a step-by-step (source code and all) walk-through can be found here: [analysis/data_analysis.html](analysis/data_analysis.html)

## Updates Since Publication
- **NOTE:** Since running this experiment (and submitting the associated paper), I've done a bit more parameter exploration with SignalGP. The performance of the event-driven and combined treatments suffered from an astronomically high tag mutation rate. In subsequent work, I've lowered the tag mutation rate, and SignalGP can easily solve the 16-state environment in 10K generations. The upside to conference deadlines is that they put deadlines on getting research done; the downside is that sometimes there isn't enough time to do proper parameter explorations on systems. (6/2018)

## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523