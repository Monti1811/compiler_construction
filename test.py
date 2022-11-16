import difflib
import glob
import re
import os
import subprocess
import sys

from typing import Tuple

COMPILER_PATH = "./build/debug/c4"
verbose = False
fullDiff = False
ci = False

tests: "list[str]" = list()

for file in glob.iglob(f"tests/**/*.c4", recursive=True):
    tests.append(file[:-3])

argIdx = 1
while len(sys.argv) > argIdx and sys.argv[argIdx][:2] == "--":
    arg = sys.argv[argIdx][2:]
    if arg == "verbose":
        verbose = True
    elif arg == "full-diff":
        fullDiff = True
    elif arg == "ci":
        ci = True
    elif arg == "help":
        print(f"Usage: {sys.argv[0]} [--verbose|--full-diff] [filters...]")
        print("    --verbose    Always show explanation for failed tests")
        print("    --full-diff  Show a more detailed diff for failed tests")
        print("    filters      Only run tests whose name matches one of these filters")
        exit(0)
    argIdx += 1

filters = sys.argv[argIdx:]

def shouldRunTest(file: str):
    return any(map(lambda filter: filter in file, filters))

if len(filters) > 0:
    tests = list(filter(shouldRunTest, tests))

if len(tests) == 1:
    verbose = True

def make():
    try:
        result = subprocess.run(["make"], capture_output=True, text=True)
        print(result.stdout, result.stderr)
    except subprocess.CalledProcessError as e:
        print(e.stdout, e.stderr)

def runCompiler(arg: str, file: str) -> "Tuple[list[str], list[str], int]":
    try:
        result = subprocess.run([f"{COMPILER_PATH}", arg, f"{file}.c4"], capture_output=True, text=True, encoding="ISO-8859-1")
        return result.stdout.splitlines(), result.stderr.splitlines(), result.returncode
    except subprocess.CalledProcessError as e:
        return e.stdout.splitlines(), e.stderr.splitlines(), e.returncode

def compareResults(expected: "Tuple[list[str], list[str], int]", actual: "Tuple[list[str], list[str], int]") -> str:
    expectedStdout, expectedStderr, expectedSuccess = expected
    stdout, stderr, success = actual

    result: list[str] = list()

    if success != expectedSuccess:
        result.append(f"Incorrect exit code: Should be {int(not expectedSuccess)}, is {int(not success)}")

    stdoutDelta = difflib.unified_diff(expectedStdout, stdout, lineterm="", fromfile="expected stdout", tofile="actual stdout")
    if fullDiff and len(list(stdoutDelta)) > 0:
        differ = difflib.Differ(None, None)
        result.extend(differ.compare(list(expectedStdout), list(stdout)))
    else:
        result.extend(stdoutDelta)

    stderrDelta = difflib.unified_diff(expectedStderr, stderr, lineterm="", fromfile="expected stderr", tofile="actual stderr")
    if fullDiff and len(list(stderrDelta)) > 0:
        differ = difflib.Differ(None, None)
        result.extend(differ.compare(list(stderrDelta), list(stderr)))
    else:
        result.extend(stderrDelta)

    return "\n".join(result)

def formatExpected(file: str, expectedFile: str) -> "Tuple[list[str], list[str], int]":
    expectedLines = open(expectedFile, encoding="ISO-8859-1").read()
    expectedLines = expectedLines.splitlines()
    expectedLines = map(lambda line: line.strip(), expectedLines)
    expectedLines = filter(lambda line: len(line) > 0, expectedLines)
    expectedLines = map(lambda line: f"{file}.c4:{line}", expectedLines)

    def isErrorLine(line: str) -> bool:
        return not (re.match(r"^([^:]+:\d+:\d+: error).*?$", line) == None)

    expectedStdout = list()
    expectedStderr = list()

    for line in expectedLines:
        (expectedStderr if isErrorLine(line) else expectedStdout).append(line)

    return expectedStdout, expectedStderr, 0 if len(expectedStderr) == 0 else 1

def runTokenizeTest(file: str) -> "None | str":
    expectedFile: str = f"{file}.tokenized"
    if not os.path.exists(expectedFile):
        return None

    expected = formatExpected(file, expectedFile)
    actual = runCompiler("--tokenize", file)

    return compareResults(expected, actual)

def runParseTest(file: str) -> "None | str":
    expectedFile: str = f"{file}.parsed"
    formattedFile: str = f"{file}.formatted"

    expected = ["", "", 0]
    if os.path.exists(expectedFile):
        expected = formatExpected(file, expectedFile)
    elif not os.path.exists(formattedFile):
        return None
    
    actual = runCompiler("--parse", file)

    return compareResults(expected, actual)

def runFormatTest(file: str) -> "None | str":
    expectedFile: str = f"{file}.formatted"
    if not os.path.exists(expectedFile):
        return None
    
    expectedStdout = open(expectedFile, encoding="ISO-8859-1").read().splitlines()
    expected = [expectedStdout, "", 0]
    actual = runCompiler("--print-ast", file)

    return compareResults(expected, actual)

successCount, failedCount, skippedCount = 0, 0, 0

def runTest(file: str):
    global successCount, failedCount, skippedCount

    print(f"\033[94m{file[6:]}\033[90m ... \033[0m", end = "")

    tokenizeResult: "None | str" = runTokenizeTest(file)
    parseResult: "None | str" = runParseTest(file)
    formatResult: "None | str" = runFormatTest(file)

    if tokenizeResult == None and parseResult == None and formatResult == None:
        print("\033[93mSKIPPED\033[0m")
        skippedCount += 1
    elif tokenizeResult != None and tokenizeResult != "":
        print("\033[91mFAILED (tokenize)\033[0m")
        if verbose or fullDiff:
            print(tokenizeResult)
        failedCount += 1
    elif parseResult != None and parseResult != "":
        print("\033[91mFAILED (parse)\033[0m")
        if verbose or fullDiff:
            print(parseResult)
        failedCount += 1
    elif formatResult != None and formatResult != "":
        print("\033[91mFAILED (format)\033[0m")
        if verbose or fullDiff:
            print(formatResult)
        failedCount += 1
    else:
        print("\033[92mOK\033[0m")
        successCount += 1

if not ci:
    make()

for test in tests:
    result = runTest(test)

print(f"""\033[97m{len(tests)} tests executed: \
\033[92m{successCount} ok\033[97m, \
\033[91m{failedCount} failed\033[97m, \
\033[93m{skippedCount} skipped\033[97m.\033[0m""")

if failedCount > 0:
    exit(1)
