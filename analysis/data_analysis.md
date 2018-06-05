---
title: "Data Analysis for Evolving Event-driven Programs with SignalGP (GECCO 2018)"
output: 
  html_document: 
    keep_md: yes
    toc: true
    toc_float: true
    toc_depth: 4
    collapsed: false
    theme: default
  pdf_document: default
---

## Overview
Here, we analyze the experimental results from our GECCO 2018 paper, "Evolving Event-driven Programs with SignalGP" (Lalejini and Ofria, 2018). 

### (just a little) Background
Our paper introduces SignalGP, a new genetic programming (GP) technique designed to provide evolution access to the [event-driven programming paradigm](https://en.wikipedia.org/wiki/Event-driven_programming). Additionally, we use SignalGP to demonstrate the value of evolving event-driven programs by evolving fully event-driven SignalGP programs and imperative variants of SignalGP programs to solve two problems: the changing environment problem and the distributed leader election problem. This document fully details the data analyses performed to analyze our experimental results for both of these problems. 

For the full context of these analyses, a detailed description of our experimental design, and musings on SignalGP, see our paper (Lalejini and Ofria, 2018). A pre-print can be found [here](https://arxiv.org/pdf/1804.05445.pdf).

## Source Code
The GitHub repository that contains the source code for this document and our experiments can be found here: [https://github.com/amlalejini/GECCO-2018-Evolving-Event-driven-Programs-with-SignalGP](https://github.com/amlalejini/GECCO-2018-Evolving-Event-driven-Programs-with-SignalGP). 

## Dependencies & General Setup
If you haven't noticed already, this is an R markdown file. We used R version 3.3.2 (2016-10-31) for all statistical analyses (R Core Team, 2016). 

All Dunn's tests were performed using the FSA package (Ogle, 2017).

```r
library(FSA)
```

```
## ## FSA v0.8.20. See citation('FSA') if used in publication.
## ## Run fishR() for related website and fishR('IFAR') for related book.
```

In addition to R, we used Python 3 for data manipulation and visualization. To get Python and R to place nice together, we use the [reticulate](https://rstudio.github.io/reticulate/index.html) R package.

```r
library(reticulate)
# We have to tell reticulate which python we want it to use.
use_python("/anaconda3/bin/python")
```

```r
knitr::knit_engines$set(python = reticulate::eng_python)
```

All visualizations use the seaborn Python package (Waskom et al., 2017), which wraps Python's matplotlib library, providing a "high-level interface for drawing attractive statistical graphics". 

```python
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec
```

To handle our data on the Python side of things, we used the [pandas](https://pandas.pydata.org/) Python package, which provides "high-performance, easy-to-use data structures and data analysis tools for the Python programming language".

```python
import pandas as pd
```

---

## Problem: Changing Environment
### Description
The changing environment problem is a toy problem to test GP programs' capacity to respond appropriately to environmental signals. The changing environment problem is explicitly designed to be solved using the event-driven paradigm. Thus, under-performance by imperative SignalGP variants was expected. 

From our paper (Lalejini and Ofria, 2018):

> The changing environment problem requires agents to coordinate to coordinate their behavior with a randomly changing environment. The environment can be in one of _K_ possible states; to maximize fitness, agents must match their internal state to the current state of the environment. The environment is initialized to a random state and has a 12.5% chance of changing to a random state at every subsequent time step. Successful agents must adjust their internal state whenever an environmental change occurs. 

Agents adjust their internal state by executing on of _K_ state-altering instructions. Thus, to be successful, agents must monitor the environment state while adjusting their internal state as appropriate. 

To explore the value of giving evolution access to the event-driven programming paradigm, we compared the performance of programs with three different mechanisms for sensing the environment: 

1. an **event-driven treatment** where environmental changes produce signals that have environment-specific tags and can trigger SignalGP program functions
2. an **imperative control** where programs need to actively poll the environment to determine its state (using _K_ sensory instructions to test if the environment is in a particular state)
3. a **combined treatment** where agents are capable of using environmental signals or actively polling the environment

We evolved SignalGP agents to solve the changing environment problem at _K_ equal to 2, 4, 8, and 16 environmental states. The changing environment problem is designed to scale in difficulty as the number of environments increases.
We ran 100 replicate populations of each treatment at _K_ equal to 2, 4, 8, and 16 environmental states. In all replicates and across all treatments, we evolved populations of 1000 agents for 10,000 generations, starting from a simple ancestor program. 

### Statistical Methods
For every run, we extracted the program with the highest fitness after 10,000 generations of evolution. To account for environmental stochasticity, we tested each program 100 times, using a program's average performance as its fitness in our analyses. For each environmental size, we compared the performances of evolved programs across treatments. For our analyses, we set our significance level, $\alpha$, to:

```r
sig_level <- 0.05
```
To determine if any of the treatments were significant within a set (_p_ < 0.05), we performed a [Kruskall-Wallis rank sum test](https://en.wikipedia.org/wiki/Kruskal%E2%80%93Wallis_one-way_analysis_of_variance). For a set in which the Kruskal-Wallis test was significant, we performed a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons. 

### Results
Finally, enough with all of that mumbo-jumbo explanation stuff! Results! Pretty pictures! P values or whatever! 

But, before all that, we'll load in our data:

```r
chgenv_data <- read.csv("../data/chg_env/mt_final_fitness.csv")
```

A note about how treatments are named within the data: treatment names describe the parameters and their values used when running the experiment. Parameters and their values are adjacent in the name, and parameter-value combinations are separated by underscores. For example, ED1_AS1_ENV2_TSK0 indicates that event-driven (ED) signals were enabled (1), active sensors (AS) were enabled (1), and there were two environments states. In other words, ED1_AS1_ENV2_TSK0 indicates the two-state environment combined treatment. The trailing TSK0 can be ignored. 

#### Two-state Environment
Let's partition out the two-state environment data:

```r
chgenv_s2_data <- chgenv_data[grep("_ENV2_", chgenv_data$treatment),]
chgenv_s2_data <- chgenv_s2_data[chgenv_s2_data$analysis == "fdom",]
chgenv_s2_data$treatment <- relevel(chgenv_s2_data$treatment, ref="ED1_AS1_ENV2_TSK0")
```

**Accio, visualization!**

![](data_analysis_files/figure-html/unnamed-chunk-8-1.png)<!-- -->

From the boxplot, the event-driven and combined treatments alone are able to produce optimally-performing programs. But, we'll be responsible and perform some actual statistical analyses.

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s2_kt <- kruskal.test(fitness ~ treatment, chgenv_s2_data)
chgenv_s2_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by treatment
## Kruskal-Wallis chi-squared = 283.26, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s2_dt <- dunnTest(fitness ~ treatment, data=chgenv_s2_data, method="bonferroni")
chgenv_s2_dt_comps <- chgenv_s2_dt$res$Comparison
chgenv_s2_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                              Comparison         Z      P.unadj
## 1 ED0_AS1_ENV2_TSK0 - ED1_AS0_ENV2_TSK0 -14.57562 4.014904e-48
## 2 ED0_AS1_ENV2_TSK0 - ED1_AS1_ENV2_TSK0 -14.57562 4.014904e-48
## 3 ED1_AS0_ENV2_TSK0 - ED1_AS1_ENV2_TSK0   0.00000 1.000000e+00
##          P.adj
## 1 1.204471e-47
## 2 1.204471e-47
## 3 1.000000e+00
```



We'll use some Python code (most of which is under the hood of this document; check out the source code on Git to stare directly into the sun) to pretty-print our Dunn's test results. 

```
## ===== Significant comparisons: =====
## Imperative - Event-driven
##   adjusted p-value: 1.2044712461463627e-47
##   Z statistic: -14.575615035405951
## Imperative - Combined
##   adjusted p-value: 1.2044712461463627e-47
##   Z statistic: -14.575615035405951
## ===== Not significant comparisons: =====
## Event-driven - Combined
##  adjusted p-value: 1.0
##  Z statistic: 0.0
```

What we saw in the boxplot is confirmed: the imperative treatment produced significantly lower performing than the combined or event-driven treatment. 

##### Teasing Apart the Combined Treatment
In the combined treatment, evolution had access to both the event-driven (signal-based) strategy and the imperative (sensor-polling) strategy. As shown above, performance in the event-driven and combined treatments did not significantly differ. However, this result along does not reveal what strategies were favored in the combined treatment. 

To tease this apart, we re-evaluated programs evolved under the combined treatment in two distinct conditions: one in which we deactivated polling sensors and one in which we prevented environment change signals from triggering functions in SignalGP functions.

First, we'll extract the relevant data. 

```r
chgenv_s2_comb_data <- chgenv_data[grep("ED1_AS1_ENV2_", chgenv_data$treatment),]
```

Visualize!
![](data_analysis_files/figure-html/unnamed-chunk-14-1.png)<!-- -->

When we take away signals, performance decreases; however, when we take away sensors, there's no effect. 

Stats to confirm!

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s2_comb_kt <- kruskal.test(fitness ~ analysis, chgenv_s2_comb_data)
chgenv_s2_comb_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by analysis
## Kruskal-Wallis chi-squared = 283.27, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s2_comb_dt <- dunnTest(fitness ~ analysis, data=chgenv_s2_comb_data, method="bonferroni")
chgenv_s2_comb_dt_comps <- chgenv_s2_comb_dt$res$Comparison
chgenv_s2_comb_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                Comparison        Z      P.unadj        P.adj
## 1       fdom - no_sensors  0.00000 1.000000e+00 1.000000e+00
## 2       fdom - no_signals 14.57563 4.014092e-48 1.204228e-47
## 3 no_sensors - no_signals 14.57563 4.014092e-48 1.204228e-47
```

Yup!

#### Four-state Environment
**Spoiler alert!** The four-, eight-, and sixteen- state environments are all pretty much the same story as the two-state environment. I'm going to cut out most of the commentary for the rest of these results. 

Let's partition out the four-state environment data:

```r
chgenv_s4_fdom_data <- chgenv_data[grep("_ENV4_", chgenv_data$treatment),]
chgenv_s4_fdom_data <- chgenv_s4_fdom_data[chgenv_s4_fdom_data$analysis == "fdom",]
chgenv_s4_fdom_data$treatment <- relevel(chgenv_s4_fdom_data$treatment, ref="ED1_AS1_ENV4_TSK0")
```

**Accio, visualization!**

![](data_analysis_files/figure-html/unnamed-chunk-18-1.png)<!-- -->

From the boxplot, the event-driven and combined treatments alone are able to produce optimally-performing programs. But, we'll be responsible and perform some actual statistical analyses.

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s4_fdom_kt <- kruskal.test(fitness ~ treatment, chgenv_s4_fdom_data)
chgenv_s4_fdom_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by treatment
## Kruskal-Wallis chi-squared = 283.26, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s4_fdom_dt <- dunnTest(fitness ~ treatment, data=chgenv_s4_fdom_data, method="bonferroni")
chgenv_s4_fdom_dt_comps <- chgenv_s4_fdom_dt$res$Comparison
chgenv_s4_fdom_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                              Comparison         Z      P.unadj
## 1 ED0_AS1_ENV4_TSK0 - ED1_AS0_ENV4_TSK0 -14.57561 4.015039e-48
## 2 ED0_AS1_ENV4_TSK0 - ED1_AS1_ENV4_TSK0 -14.57561 4.015039e-48
## 3 ED1_AS0_ENV4_TSK0 - ED1_AS1_ENV4_TSK0   0.00000 1.000000e+00
##          P.adj
## 1 1.204512e-47
## 2 1.204512e-47
## 3 1.000000e+00
```



```
## ===== Significant comparisons: =====
## Imperative - Event-driven
##   adjusted p-value: 1.2045118388700292e-47
##   Z statistic: -14.575612733980755
## Imperative - Combined
##   adjusted p-value: 1.2045118388700292e-47
##   Z statistic: -14.575612733980755
## ===== Not significant comparisons: =====
## Event-driven - Combined
##  adjusted p-value: 1.0
##  Z statistic: 0.0
```

##### Teasing Apart the Combined Treatment
In the combined treatment, evolution had access to both the event-driven (signal-based) strategy and the imperative (sensor-polling) strategy. As shown above, performance in the event-driven and combined treatments did not significantly differ. However, this result along does not reveal what strategies were favored in the combined treatment. 

To tease this apart, we re-evaluated programs evolved under the combined treatment in two distinct conditions: one in which we deactivated polling sensors and one in which we prevented environment change signals from triggering functions in SignalGP functions.

First, we'll extract the relevant data. 

```r
chgenv_s4_comb_data <- chgenv_data[grep("ED1_AS1_ENV4_", chgenv_data$treatment),]
```

Visualize!
![](data_analysis_files/figure-html/unnamed-chunk-23-1.png)<!-- -->

When we take away signals, performance decreases; however, when we take away sensors, there's no effect. 

Stats to confirm!

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s4_comb_kt <- kruskal.test(fitness ~ analysis, chgenv_s4_comb_data)
chgenv_s4_comb_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by analysis
## Kruskal-Wallis chi-squared = 283.27, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s4_comb_dt <- dunnTest(fitness ~ analysis, data=chgenv_s4_comb_data, method="bonferroni")
chgenv_s4_comb_dt_comps <- chgenv_s4_comb_dt$res$Comparison
chgenv_s4_comb_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                Comparison        Z      P.unadj        P.adj
## 1       fdom - no_sensors  0.00000 1.000000e+00 1.000000e+00
## 2       fdom - no_signals 14.57564 4.013687e-48 1.204106e-47
## 3 no_sensors - no_signals 14.57564 4.013687e-48 1.204106e-47
```

#### Eight-state Environment

Let's partition out the eight-state environment data:

```r
chgenv_s8_fdom_data <- chgenv_data[grep("_ENV8_", chgenv_data$treatment),]
chgenv_s8_fdom_data <- chgenv_s8_fdom_data[chgenv_s8_fdom_data$analysis == "fdom",]
chgenv_s8_fdom_data$treatment <- relevel(chgenv_s8_fdom_data$treatment, ref="ED1_AS1_ENV8_TSK0")
```

**Accio, visualization!**

![](data_analysis_files/figure-html/unnamed-chunk-27-1.png)<!-- -->

From the boxplot, the event-driven and combined treatments alone are able to produce optimally-performing programs. But, we'll be responsible and perform some actual statistical analyses.

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s8_fdom_kt <- kruskal.test(fitness ~ treatment, chgenv_s8_fdom_data)
chgenv_s8_fdom_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by treatment
## Kruskal-Wallis chi-squared = 273.26, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s8_fdom_dt <- dunnTest(fitness ~ treatment, data=chgenv_s8_fdom_data, method="bonferroni")
chgenv_s8_fdom_dt_comps <- chgenv_s8_fdom_dt$res$Comparison
chgenv_s8_fdom_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                              Comparison           Z      P.unadj
## 1 ED0_AS1_ENV8_TSK0 - ED1_AS0_ENV8_TSK0 -14.2165237 7.235911e-46
## 2 ED0_AS1_ENV8_TSK0 - ED1_AS1_ENV8_TSK0 -14.4131139 4.279713e-47
## 3 ED1_AS0_ENV8_TSK0 - ED1_AS1_ENV8_TSK0  -0.1965902 8.441483e-01
##          P.adj
## 1 2.170773e-45
## 2 1.283914e-46
## 3 1.000000e+00
```



```
## ===== Significant comparisons: =====
## Imperative - Event-driven
##   adjusted p-value: 2.1707733744361592e-45
##   Z statistic: -14.21652374092487
## Imperative - Combined
##   adjusted p-value: 1.2839138978367486e-46
##   Z statistic: -14.413113919526637
## ===== Not significant comparisons: =====
## Event-driven - Combined
##  adjusted p-value: 1.0
##  Z statistic: -0.19659017860176725
```

##### Teasing Apart the Combined Treatment
In the combined treatment, evolution had access to both the event-driven (signal-based) strategy and the imperative (sensor-polling) strategy. As shown above, performance in the event-driven and combined treatments did not significantly differ. However, this result along does not reveal what strategies were favored in the combined treatment. 

To tease this apart, we re-evaluated programs evolved under the combined treatment in two distinct conditions: one in which we deactivated polling sensors and one in which we prevented environment change signals from triggering functions in SignalGP functions.

First, we'll extract the relevant data. 

```r
chgenv_s8_comb_data <- chgenv_data[grep("ED1_AS1_ENV8_", chgenv_data$treatment),]
```

Visualize!
![](data_analysis_files/figure-html/unnamed-chunk-32-1.png)<!-- -->

When we take away signals, performance crashes hard; however, when we take away sensors, there's no effect. 

Stats to confirm!

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s8_comb_kt <- kruskal.test(fitness ~ analysis, chgenv_s8_comb_data)
chgenv_s8_comb_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by analysis
## Kruskal-Wallis chi-squared = 290.92, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s8_comb_dt <- dunnTest(fitness ~ analysis, data=chgenv_s8_comb_data, method="bonferroni")
chgenv_s8_comb_dt_comps <- chgenv_s8_comb_dt$res$Comparison
chgenv_s8_comb_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                Comparison        Z      P.unadj        P.adj
## 1       fdom - no_sensors  0.00000 1.000000e+00 1.000000e+00
## 2       fdom - no_signals 14.77117 2.247789e-49 6.743368e-49
## 3 no_sensors - no_signals 14.77117 2.247789e-49 6.743368e-49
```


#### Sixteen-state environment

Let's partition out the sixteen-state environment data:

```r
chgenv_s16_fdom_data <- chgenv_data[grep("_ENV16_", chgenv_data$treatment),]
chgenv_s16_fdom_data <- chgenv_s16_fdom_data[chgenv_s16_fdom_data$analysis == "fdom",]
chgenv_s16_fdom_data$treatment <- relevel(chgenv_s16_fdom_data$treatment, ref="ED1_AS1_ENV16_TSK0")
```

**Accio, visualization!**

![](data_analysis_files/figure-html/unnamed-chunk-36-1.png)<!-- -->

From the boxplot, the event-driven and combined treatments produce higher-performing programs than the imperative treatment. 

**NOTE:** Since running this experiment (and submitting the associated paper), I've done a bit more parameter exploration with SignalGP. The performance of the event-driven and combined treatments suffered from an astronomically high tag mutation rate. In subsequent work, I've lowered the tag mutation rate, and SignalGP can easily solve the 16-state environment in 10K generations. 

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s16_fdom_kt <- kruskal.test(fitness ~ treatment, chgenv_s16_fdom_data)
chgenv_s16_fdom_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by treatment
## Kruskal-Wallis chi-squared = 199.38, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s16_fdom_dt <- dunnTest(fitness ~ treatment, data=chgenv_s16_fdom_data, method="bonferroni")
chgenv_s16_fdom_dt_comps <- chgenv_s16_fdom_dt$res$Comparison
chgenv_s16_fdom_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                                Comparison           Z      P.unadj
## 1 ED0_AS1_ENV16_TSK0 - ED1_AS0_ENV16_TSK0 -12.1256145 7.727836e-34
## 2 ED0_AS1_ENV16_TSK0 - ED1_AS1_ENV16_TSK0 -12.3285843 6.355345e-35
## 3 ED1_AS0_ENV16_TSK0 - ED1_AS1_ENV16_TSK0  -0.2029699 8.391586e-01
##          P.adj
## 1 2.318351e-33
## 2 1.906603e-34
## 3 1.000000e+00
```



```
## ===== Significant comparisons: =====
## Imperative - Event-driven
##   adjusted p-value: 2.3183509341297077e-33
##   Z statistic: -12.125614491174186
## Imperative - Combined
##   adjusted p-value: 1.9066033725005922e-34
##   Z statistic: -12.32858434148526
## ===== Not significant comparisons: =====
## Event-driven - Combined
##  adjusted p-value: 1.0
##  Z statistic: -0.20296985031107417
```

##### Teasing Apart the Combined Treatment
In the combined treatment, evolution had access to both the event-driven (signal-based) strategy and the imperative (sensor-polling) strategy. As shown above, performance in the event-driven and combined treatments did not significantly differ. However, this result along does not reveal what strategies were favored in the combined treatment. 

To tease this apart, we re-evaluated programs evolved under the combined treatment in two distinct conditions: one in which we deactivated polling sensors and one in which we prevented environment change signals from triggering functions in SignalGP functions.

First, we'll extract the relevant data. 

```r
chgenv_s16_comb_data <- chgenv_data[grep("ED1_AS1_ENV16_", chgenv_data$treatment),]
```

Visualize!
![](data_analysis_files/figure-html/unnamed-chunk-41-1.png)<!-- -->

When we take away signals, performance crashes hard; however, when we take away sensors, there's no effect. 

Stats to confirm!

First, we'll do a Kruskal-Wallis test to confirm that at least one of the treatments within the set is different from the others.

```r
chgenv_s16_comb_kt <- kruskal.test(fitness ~ analysis, chgenv_s16_comb_data)
chgenv_s16_comb_kt
```

```
## 
## 	Kruskal-Wallis rank sum test
## 
## data:  fitness by analysis
## Kruskal-Wallis chi-squared = 207.01, df = 2, p-value < 2.2e-16
```

According to the Kruskal-Wallis test, at least one treatment is significantly different from the others. Next, we'll do a post-hoc Dunn's test, applying a Bonferroni correction for multiple comparisons.

```r
chgenv_s16_comb_dt <- dunnTest(fitness ~ analysis, data=chgenv_s16_comb_data, method="bonferroni")
chgenv_s16_comb_dt_comps <- chgenv_s16_comb_dt$res$Comparison
chgenv_s16_comb_dt
```

```
## Dunn (1964) Kruskal-Wallis multiple comparison
```

```
##   p-values adjusted with the Bonferroni method.
```

```
##                Comparison        Z      P.unadj        P.adj
## 1       fdom - no_sensors  0.00000 1.000000e+00 1.000000e+00
## 2       fdom - no_signals 12.46013 1.231736e-35 3.695207e-35
## 3 no_sensors - no_signals 12.46013 1.231736e-35 3.695207e-35
```

---

## Problem: Distributed Leader Election
### Description
### Results

---

## References
Lalejini, A., & Ofria, C. (2018). Evolving Event-driven Programs with SignalGP. In Proceedings of the Genetic and Evolutionary Computation Conference. ACM. https://doi.org/10.1145/3205455.3205523

Ogle, D.H. 2017. FSA: Fisheries Stock Analysis. R package version 0.8.17.

R Core Team (2016). R: A language and environment for statistical computing. R Foundation for Statistical Computing, Vienna, Austria. URL https://www.R-project.org/.

Michael Waskom, Olga Botvinnik, Drew O'Kane, Paul Hobson, Saulius Lukauskas, David C Gemperline, … Adel Qalieh. (2017, September 3). mwaskom/seaborn: v0.8.1 (September 2017) (Version v0.8.1). Zenodo. http://doi.org/10.5281/zenodo.883859
