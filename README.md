# Usage

## Python
1. install python
2. python parseDFSswt.py [target dir] [sample rate of MET log]

## C++
1. compile: make
2. ./parseDFSswt [target_dir] [sample rate of MET log]
0. clean: make clean

# Output Format
output .csv file

- InputFileName

- Switch statistic

[ Switch(level1-level2), Count ]

- Switch history and corresponding CPU utilization

[ Time, Freq1, Freq2, Load ]

- CPU load history and corresponding frequency level

[ Time, Load, Freq Level ]


