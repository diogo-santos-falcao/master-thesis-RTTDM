# Railway Time Table Disruption Management

**Author:** Diogo Falcão

**Advisors:** Prof. Alessandro Gianola, Eng. Carlos Conceição

**Context:** Master's Thesis in Computer Science and Engineering - Instituto Superior Técnico (IST), Universidade de Lisboa.

## Overview

This repository contains a decision support system for real-time train timetable rescheduling under disruption scenarios. It bridges the gap between theoretical optimization models and practical railway operations by integrating a formal MaxSAT solver within a closed-loop execution environment connected to the **Train Director 3 (TD3)** simulator.

The system is designed to automatically generate feasible recovery plans, respecting operational constraints, and dynamically inject them back into the running simulation.

## Acknowledgements & Core Solver Attribution

The core MaxSAT optimization engine (`timetabler`) included in this repository was originally developed by **Alexandre Lemos, Filipe Gouveia, Pedro T. Monteiro, and Inês Lynce**. Their work formally defined the Train Scheduling Optimization Problem under Disruption (TSOPuD) and introduced the iterative core-guided MaxSAT algorithm used here. 

The original repository for their solver can be found at: [ADDALemos/train-schedule-optimisation](https://github.com/ADDALemos/train-schedule-optimisation).

**This thesis builds upon their foundational work by:**
1. Adapting the C++ engine to parse and process TD3 infrastructure and schedule files (`.trk` and `.sch`).
2. Developing a Python-based Traffic Management System (TMS) (`traffic_manager.py`) to orchestrate a real-time Closed-Loop architecture.
3. Automating the detection, disruption injection, and rescheduling application directly via the TD3 Web Server API.

---

## Dependencies & Prerequisites

**1. Railway Simulator:**
* **Train Director 3 (TD3):** The simulation environment is powered by TD3. You must install the program to run the visual simulation and enable the Web Server feature. 
* Download it from the official page: [Train Director 3 Official Website](https://www.backerstreet.com/traindir/en/trdireng.php)

**2. Python TMS (Integration):**
* Python 3.x
* `requests` library (`pip install requests`)

ESCREVER O RESTO

---

## How to Compile

To compile the C++ MaxSAT optimization engine, run:
```bash
make
```
*Note: The Makefile allows choosing the MaxSAT solver. To change the default solver (TT-Open-WBO-INC), modify the `SUPERSOLVERNAME` and `SUPERSOLVERNAMEID` variables in the Makefile.*

## 🚦 How to Run the Project & Data Sets

### 1. Preparing the Simulator (TD3) & Data Sets
Before running any scripts, open **Train Director 3**, load the desired scenario files, and start the simulation. Ensure the TD3 **Web Server** is active on port `8999`.

**Data Sets used in this project:**
* **TD3 Scenarios:** Custom Lier-Herentals models (`.trk` and `.sch`) developed specifically for this thesis. AINDA FALTA
* **Legacy Benchmarks:** [PESP benchmark](http://num.math.uni-goettingen.de/~m.goerigk/pesplib/) | [SBB benchmark](https://github.com/potassco/train-scheduling-with-hybrid-asp)

### 2. Running the Closed-Loop TMS (Thesis Contribution)
With the simulator running, execute the Python Traffic Management System to monitor the network, inject a disruption, and automatically resolve it using the MaxSAT solver:

```bash
python3 traffic_manager.py --trk data/TD3/LierHerentals/LierHerentals.trk --sch data/TD3/LierHerentals/LierHerentals.sch --out data/.out.json --comboio <TRAIN_ID> --atraso <MINUTES>
```

### 3. Running the Standalone Solver (Legacy/Manual Mode)
To run the solver directly on XML files (original implementation) or specific TD3 files without the Python wrapper:
```bash
./timetabler data/PESP/set-01/R1L1.xml -opt-time=2  [solver options]
```

**Solver Options:**
* Entry time variables: `0` - For all section and all time; `1` - For all time; `2` - Smart time.
```bash
-opt-time= <int32>  [   0 ..    2]      (default: 2)
```
