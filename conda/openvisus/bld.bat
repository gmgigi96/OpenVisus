set CMAKE_BUILD_TYPE=RelWithDebugInfo

mkdir build 
cd build 

cmake -G "%CMAKE_GENERATOR%" -DDISABLE_OPENMP=1 -DVISUS_GUI=0 -DVISUS_INTERNAL_DEFAULT=1 ..
if errorlevel 1 exit 1

cmake --build . --target ALL_BUILD --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1

cmake --build . --target INSTALL   --config %CMAKE_BUILD_TYPE%
if errorlevel 1 exit 1

cd install
"%PYTHON%" setup.py install --single-version-externally-managed --record=record.txt
if errorlevel 1 exit 1
