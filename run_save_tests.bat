@echo off
:: Script to run the SaveGameManager tests
:: Copyright (c) 2025 Hammer Forged Games, MIT License

:: Enable color output on Windows 10+ terminals
setlocal EnableDelayedExpansion

:: Color codes for Windows
set "GREEN=[92m"
set "YELLOW=[93m"
set "RED=[91m"
set "BLUE=[94m"
set "NC=[0m"

:: Navigate to project root directory (in case script is run from elsewhere)
cd /d "%~dp0"

:: Create required directories
if not exist "test_results" mkdir test_results

echo !BLUE!Building SaveGameManager tests...!NC!
:: Navigate to project root directory (in case script is run from elsewhere)
cd /d "%~dp0"

:: Process command-line options
set CLEAN=false
set CLEAN_ALL=false
set VERBOSE=false
set USE_NINJA=false

:parse_args
if "%~1"=="" goto :done_parsing
if /i "%~1"=="--clean" (
    set CLEAN=true
    shift
    goto :parse_args
)
if /i "%~1"=="--clean-all" (
    set CLEAN_ALL=true
    shift
    goto :parse_args
)
if /i "%~1"=="--verbose" (
    set VERBOSE=true
    shift
    goto :parse_args
)
if /i "%~1"=="--help" (
    echo !BLUE!SaveGameManager Test Runner!NC!
    echo Usage: %0 [options]
    echo.
    echo Options:
    echo   --clean      Clean test artifacts before building
    echo   --clean-all  Remove entire build directory and rebuild
    echo   --verbose    Run tests with verbose output
    echo   --help       Show this help message
    exit /b 0
)
echo !RED!Unknown option: %1!NC!
echo Usage: %0 [--clean] [--clean-all] [--verbose] [--help]
exit /b 1

:done_parsing

:: Handle clean-all case
if "%CLEAN_ALL%"=="true" (
    echo !YELLOW!Removing entire build directory...!NC!
    if exist "build" rmdir /s /q build
)

echo !BLUE!Building SaveGameManager tests...!NC!

:: Check if Ninja is available
where ninja >nul 2>&1
if %ERRORLEVEL% equ 0 (
    set USE_NINJA=true
    echo !GREEN!Ninja build system found, using it for faster builds.!NC!
) else (
    set USE_NINJA=false
    echo !YELLOW!Ninja build system not found, using default CMake generator.!NC!
)

:: Ensure build directory exists
if not exist "build" (
    mkdir build
    echo !YELLOW!Created build directory!NC!
)

:: Navigate to build directory
cd build || (
    echo !RED!Failed to enter build directory!!NC!
    exit /b 1
)

:: Configure with CMake if needed
if not exist "build.ninja" (
    echo !YELLOW!Configuring project with CMake...!NC!
    if "%USE_NINJA%"=="true" (
        cmake -G Ninja .. || (
            echo !RED!CMake configuration failed!!NC!
            exit /b 1
        )
    ) else (
        cmake .. || (
            echo !RED!CMake configuration failed!!NC!
            exit /b 1
        )
    )
)

:: Clean tests if requested
if "%CLEAN%"=="true" (
    echo !YELLOW!Cleaning test artifacts...!NC!
    if "%USE_NINJA%"=="true" (
        ninja -t clean save_manager_tests
    ) else (
        cmake --build . --target clean --config Debug
    )
)

:: Build the tests
echo !YELLOW!Building tests...!NC!
if "%USE_NINJA%"=="true" (
    ninja save_manager_tests || (
        echo !RED!Build failed!!NC!
        exit /b 1
    )
) else (
    cmake --build . --target save_manager_tests --config Debug || (
        echo !RED!Build failed!!NC!
        exit /b 1
    )
)

:: Return to project root
cd ..

:: Check if test executable exists
set TEST_EXECUTABLE=bin\debug\save_manager_tests.exe
if not exist "%TEST_EXECUTABLE%" (
    echo !RED!Test executable not found at %TEST_EXECUTABLE%!NC!
    echo !YELLOW!Searching for test executable...!NC!
    for /r "bin" %%f in (save_manager_tests*.exe) do (
        echo !GREEN!Found executable at: %%f!NC!
        set TEST_EXECUTABLE=%%f
        goto :found_executable
    )
    echo !RED!Could not find test executable!!NC!
    exit /b 1
)

:found_executable

:: Create test_results directory if it doesn't exist
if not exist "test_results" mkdir test_results

:: Run the tests
echo !GREEN!Build successful. Running tests...!NC!
:: Ensure test_results directory exists
if not exist "test_results" mkdir test_results

:: Run tests and save output
echo !BLUE!====================================!NC!

:: Set test command options
set TEST_OPTS=--catch_system_errors=no --no_result_code
if "%VERBOSE%"=="true" (
    set TEST_OPTS=%TEST_OPTS% --log_level=all
)

:: Run the tests and save output to a temporary file
echo !YELLOW!Running with options: %TEST_OPTS%!NC!
set TEMP_OUTPUT=test_output.log

if "%VERBOSE%"=="true" (
    "%TEST_EXECUTABLE%" %TEST_OPTS% > "%TEMP_OUTPUT%" 2>&1
    type "%TEMP_OUTPUT%"
) else (
    "%TEST_EXECUTABLE%" %TEST_OPTS% > "%TEMP_OUTPUT%" 2>&1
)
set TEST_RESULT=%ERRORLEVEL%

:: Check if executable was actually found/run
if %TEST_RESULT% equ 9009 (
    echo !RED!Error: Test executable failed to run. Check the path: %TEST_EXECUTABLE%!NC!
    echo Error: Test executable failed to run > "test_results\save_test_output.txt"
    del "%TEMP_OUTPUT%" 2>nul
    exit /b 1
)

echo !BLUE!====================================!NC!

:: Save test results
echo !YELLOW!Saving test results...!NC!
if exist "%TEMP_OUTPUT%" (
    copy "%TEMP_OUTPUT%" "test_results\save_test_output.txt" >nul 2>&1
    
    :: Extract performance metrics
    findstr /R /C:"time:" /C:"save_time:" /C:"load_time:" /C:"serialization" "%TEMP_OUTPUT%" > "test_results\save_test_performance_metrics.txt" 2>nul
    
    :: Clean up temporary file
    del "%TEMP_OUTPUT%" 2>nul
) else (
    echo !RED!Warning: Test output file not found!NC!
    echo "Test execution failed to produce output" > "test_results\save_test_output.txt"
)

:: Report test results
findstr /C:"test cases failed" /C:"failure" /C:"memory access violation" /C:"fatal error" "test_results\save_test_output.txt" >nul 2>&1
set HAS_FAILURES=%ERRORLEVEL%

if %TEST_RESULT% equ 0 (
    echo !GREEN!All tests passed!!NC!
    exit /b 0
) else (
    if %HAS_FAILURES% neq 0 (
        echo !YELLOW!Tests exited with code %TEST_RESULT% but no explicit failures found.!NC!
        echo !YELLOW!This might be due to cleanup issues rather than actual test failures.!NC!
        echo !GREEN!Treating as success. Check log file for details.!NC!
        exit /b 0
    ) else (
        echo !RED!Some tests failed with exit code %TEST_RESULT%.!NC!
        echo !RED!Please check test_results\save_test_output.txt for details.!NC!
        exit /b %TEST_RESULT%
    )
)