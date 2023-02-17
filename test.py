import difflib
import glob
import re
import os
import subprocess
import sys

from typing import Tuple

COMPILER_PATH = "./build/debug/c4"
LLVM_BIN_PATH = "./llvm/install/bin/"

verbose = False
fullDiff = False
ci = False
update = False

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
    elif arg == "update":
        update = True
    elif arg == "help":
        print(f"Usage: {sys.argv[0]} [--verbose|--full-diff|--ci|--update] [filters...]")
        print("    --verbose    Always show explanation for failed tests")
        print("    --full-diff  Show a more detailed diff for failed tests")
        print("    --ci         Skip building the compiler; assume it's already built")
        print("    --update     Update compiler test files with the actual output")
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
        result = subprocess.run([COMPILER_PATH, arg, f"{file}.c4"], capture_output=True, text=True, encoding="ISO-8859-1")
        return result.stdout.splitlines(), result.stderr.splitlines(), result.returncode
    except subprocess.CalledProcessError as e:
        return e.stdout.splitlines(), e.stderr.splitlines(), e.returncode

def executeLlvmFile(file: str) -> "Tuple[list[str], None | int]":
    try:
        result = subprocess.run([LLVM_BIN_PATH + "lli", file], capture_output=True, text=True)
        return result.stderr.splitlines(), result.returncode
    except subprocess.CalledProcessError as e:
        return list(), None

def readFile(file: str) -> "list[str]":
    return open(file, encoding="ISO-8859-1").read().splitlines()

def makeDiff(expected: "list[str]", actual: "list[str]", name: str) -> "list[str]":
    delta = difflib.unified_diff(expected, actual, fromfile="expected " + name, tofile="actual " + name)
    if fullDiff and len(list(delta)) > 0:
        differ = difflib.Differ(None, None)
        return list(differ.compare(list(expected), list(actual)))
    else:
        return list(delta)

def compareResults(expected: "Tuple[list[str], list[str], int]", actual: "Tuple[list[str], list[str], int]") -> str:
    expectedStdout, expectedStderr, expectedSuccess = expected
    stdout, stderr, success = actual

    result: list[str] = list()

    if success != expectedSuccess:
        result.append(f"Incorrect exit code: Should be {expectedSuccess}, is {success}")

    result.extend(makeDiff(expectedStdout, stdout, "stdout"))
    result.extend(makeDiff(expectedStderr, stderr, "stderr"))

    return "\n".join(result)

def formatExpected(file: str, expectedFile: str) -> "Tuple[list[str], list[str], int]":
    expectedLines = readFile(expectedFile)
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
    compiledFile: str = f"{file}.compiled"
    executedFile: str = f"{file}.executed"

    expected = [list(), list(), 0]
    if os.path.exists(expectedFile):
        expected = formatExpected(file, expectedFile)
    elif not os.path.exists(formattedFile) and not os.path.exists(compiledFile) and not os.path.exists(executedFile):
        return None

    actual = runCompiler("--parse", file)

    return compareResults(expected, actual)

def runFormatTest(file: str) -> "None | str":
    expectedFile: str = f"{file}.formatted"
    if not os.path.exists(expectedFile):
        return None
    
    expectedStdout = readFile(expectedFile)
    expected = [expectedStdout, "", 0]
    actual = runCompiler("--print-ast", file)

    return compareResults(expected, actual)

def runCompileTest(file: str) -> "None | str":
    expectedFile: str = f"{file}.compiled"
    if not os.path.exists(expectedFile):
        return None

    expectedLlvmCode = readFile(expectedFile)
    compilerResult = runCompiler("--compile", file)

    result: list[str] = list()
    def cleanupAndGetResult() -> str:
        try:
            os.remove(compilerOutputFile)
        finally:
            return "\n".join(result)

    if compilerResult[2] != 0:
        result.append(f"Incorrect compiler exit code: Should be 0, is {compilerResult[2]}")
        result.extend(compilerResult[1])

    def getLlvmOutputFile(file: str) -> str:
        slash = file.rfind("/")
        if slash >= 0:
            file = file[slash + 1:]
        return file + ".ll"

    compilerOutputFile = getLlvmOutputFile(file)
    if not os.path.exists(compilerOutputFile):
        result.append(f"Could not find file `{compilerOutputFile}`")
        return cleanupAndGetResult()

    actualLlvmCode = readFile(compilerOutputFile)

    result.extend(makeDiff(expectedLlvmCode, actualLlvmCode, "LLVM code output"))

    if update:
        with open(expectedFile, "w") as f:
            f.write("\n".join(actualLlvmCode) + "\n")

    exitCodeFile: str = f"{file}.executed"
    if not os.path.exists(exitCodeFile):
        return cleanupAndGetResult()

    expectedExitCode = readFile(exitCodeFile)
    try:
        expectedExitCode = int("\n".join(expectedExitCode))
    except ValueError:
        result.append("Warning: Cannot get exit code from file " + exitCodeFile)
        cleanupAndGetResult()

    lliStderr, exitCode = executeLlvmFile(compilerOutputFile)

    if exitCode == None:
        result.append("An error occurred while executing LLI")
    elif exitCode != expectedExitCode:
        result.append(f"Incorrect output of generated LLVM file: Should be {expectedExitCode}, is {exitCode}")
    
    if lliStderr:
        result.append("An error occurred while interpreting the generated LLVM file")
        result.extend(lliStderr)

    return cleanupAndGetResult()

successCount, failedCount, skippedCount = 0, 0, 0

def runTest(file: str):
    global successCount, failedCount, skippedCount
    os.system('')
    print(f"\033[94m{file[6:]}\033[90m ... \033[0m", end = "")

    tokenizeResult: "None | str" = runTokenizeTest(file)
    parseResult: "None | str" = runParseTest(file)
    formatResult: "None | str" = runFormatTest(file)
    compileResult: "None | str" = runCompileTest(file)

    if tokenizeResult == None and parseResult == None and formatResult == None and compileResult == None:
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
    elif compileResult != None and compileResult != "":
        print("\033[91mFAILED (compile)\033[0m")
        if verbose or fullDiff:
            print(compileResult)
        failedCount += 1
    else:
        print("\033[92mOK\033[0m")
        successCount += 1

if not ci:
    make()

for test in tests:
    result = runTest(test)

os.system('')
print(f"""\033[97m{len(tests)} tests executed: \
\033[92m{successCount} ok\033[97m, \
\033[91m{failedCount} failed\033[97m, \
\033[93m{skippedCount} skipped\033[97m.\033[0m""")

if failedCount > 0:
    exit(1)
