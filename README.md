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
...
---

## 🚀 How to Compile

To compile the C++ MaxSAT optimization engine, run:
```bash
make
