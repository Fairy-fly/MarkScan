@echo off
setlocal
if not defined OpenCV_DIR (
  echo Set OpenCV_DIR to the OpenCV CMake package directory.
  exit /b 1
)
cmake -S "%~dp0.." -B "%~dp0..\build-windows" -G Ninja -DCMAKE_BUILD_TYPE=Release %*
if errorlevel 1 exit /b %errorlevel%
cmake --build "%~dp0..\build-windows"
exit /b %errorlevel%
