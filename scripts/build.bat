@echo on

REM to test
REM set PYTHON_VERSION=37
REM set Python_EXECUTABLE=C:\Python37\python.exe 
REM set Qt5_DIR=D:\Qt\5.9.9\msvc2017_64\lib\cmake\Qt5
REM set GENERATOR=Visual Studio 16 2019
REM build.bat

set Python_EXECUTABLE=C:\Python%PYTHON_VERSION%-x64\python.exe 
set Qt5_DIR=C:\Qt\5.9\msvc2017_64\lib\cmake\Qt5
set GENERATOR=Visual Studio 16 2019

"%Python_EXECUTABLE%" -m pip install numpy setuptools wheel twine --upgrade

mkdir build_appveyor
cd build_appveyor

cmake.exe -G "%GENERATOR%" -A "x64" -DQt5_DIR="%Qt5_DIR%" -DPython_EXECUTABLE=%Python_EXECUTABLE% ../
cmake.exe --build . --target ALL_BUILD --config Release
cmake.exe --build . --target INSTALL   --config Release

cd Release\OpenVisus
 
set PYTHONPATH=..\
"%Python_EXECUTABLE%" Samples/python/Array.py 
"%Python_EXECUTABLE%" Samples/python/Dataflow.py 
"%Python_EXECUTABLE%" Samples/python/Dataflow2.py 
"%Python_EXECUTABLE%" Samples/python/Idx.py 
"%Python_EXECUTABLE%" Samples/python/XIdx.py 
"%Python_EXECUTABLE%" Samples/python/DataConversion1.py 
"%Python_EXECUTABLE%" Samples/python/DataConversion2.py
set PYTHONPATH=

.\visus.bat


if "%APPVEYOR_REPO_TAG%" == "true" (

	rmdir "dist"       /s /q
	rmdir "build"      /s /q
	rmdir "__pycache_" /s /q
	"%Python_EXECUTABLE%" setup.py -q bdist_wheel --python-tag=cp%PYTHON_VERSION% --plat-name=win_amd64

	set HOME=%USERPROFILE% 
	"%Python_EXECUTABLE%" -m twine upload --username %PYPI_USERNAME% --password %PYPI_PASSWORD% --skip-existing  "dist/*.whl"
)


if "BUILD_CONDA" != "0" (

	C:\Miniconda%PYTHON_VERSION%-x64

	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072
	
	conda install conda-build -y
	PYTHONPATH=$(pwd)/.. python -m OpenVisus use-pyqt
	rmdir "dist"       /s /q
	rmdir "build"      /s /q
	rmdir "__pycache_" /s /q
	rm -Rf $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	python setup.py -q bdist_conda 
	CONDA_FILENAME=$(find C:\Miniconda%PYTHON_VERSION%-x64/conda-bld -iname "openvisus*.tar.bz2")

	conda install -y --force-reinstall %CONDA_FILENAME%
	
	python Samples/python/Array.py 
	python Samples/python/Dataflow.py 
	python Samples/python/Dataflow2.py 
	python Samples/python/Idx.py 
	python Samples/python/XIdx.py 
	python Samples/python/DataConversion1.py 
	python Samples/python/DataConversion2.py
	
	if "%APPVEYOR_REPO_TAG%" == "true" (
		conda install anaconda-client -y
		anaconda -t %ANACONDA_TOKEN% upload "%CONDA_BUILD_FILENAME%"
	)

)