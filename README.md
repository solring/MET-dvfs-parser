# DVFS switch records parser

## Compile

### Python
1. install python
2. python parseDFSswt.py [target dir] [sample rate of MET log]

### C++
1. compile: make
2. ./parseDFSswt [target_dir] [sample rate of MET log]
0. clean: make clean

## Input Files
Exported MET traces(.csv)) of DVFS switch and CPU load.

Please put all the exported trace files in a folder in order to process in batch.

You can use solution.xlsx to export CPU load and DVFS traces from the .xlsx file exported by MET GUI as .csv in batch.

## Usage
parseDFSswt [target folder] [sample rate of MET]

## Output File Format
output .csv file

- InputFileName

- Switch statistic

[ Switch(level1-level2), Count ]

- Switch history and corresponding CPU utilization

[ Time, Freq1, Freq2, Load ]

- CPU load history and corresponding frequency level

[ Time, Load, Freq Level ]


# Stage parser

Supportive tool for TeaPot

## Compile

C++: make

## Input Files
1. Exported MET traces(.csv)) of DVFS switch, CPU load.

Please put all the exported trace files in a folder in order to process in batch.

You can use solution.xlsx to export CPU load and DVFS traces from the .xlsx file exported by MET GUI as .csv in batch.

2. A .sec file with timestamps separating each stages in a format like:

281
282.7
285
289

Note that timestamps must in the rage of timestamps in MET traces, and the file name must be the same with the prefix of MET traces.

## Usage
getStageData [target folder] [sample rate of MET] [# of core]

## Output File Format

[ Start time, End time, Duration, Avg. freq1., Avg. load1, ..., Avg. mem. bandwidth ]

