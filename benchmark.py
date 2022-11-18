import os
import subprocess
import sys
import time

COMPILER_PATH = "./build/debug/c4"

benchmarks: "list[str]" = list()

syntaxMsg = f"Syntax: {sys.argv[0]} (--tokenize|--parse|--print-ast) FILENAME"

if len(sys.argv) < 3:
    print(syntaxMsg)
    exit(1)

arg = sys.argv[1]
fileName = sys.argv[2]

if not os.path.exists(fileName):
    print("File does not exist")
    exit(1)

def runBenchmarkOnce(command: "list[str]") -> float:
    timeStarted = time.time()
    subprocess.run(command, capture_output=True)
    return time.time() - timeStarted

def runBenchmark(command: "list[str]"):
    results: list[float] = list()
    sum: float = 0

    for i in range(0, 10):
        results.append(runBenchmarkOnce(command))
        sum += results[i]
    
    results.sort()

    print(f"MIN: {results[0]:2f} secs")
    print(f"AVG: {sum / 10:2f} secs")
    print(f"MAX: {results[9]:2f} secs")

if arg == "--tokenize":
    runBenchmark([COMPILER_PATH, "--tokenize", fileName])
elif arg == "--parse":
    runBenchmark([COMPILER_PATH, "--parse", fileName])
elif arg == "--print-ast":
    runBenchmark([COMPILER_PATH, "--print-ast", fileName])
else:
    print(syntaxMsg)
