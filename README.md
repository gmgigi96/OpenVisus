# OpenViSUS Visualization project  

![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)


The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license.

Table of content:

- [Binary Distribution](#binary-distribution)

- [Minimal compilation](#minimal-compilation)

- [Windows compilation Visual Studio](#windows-compilation-visual-studio)

- [Windows compilation mingw](#windows-compilation-mingw)

- [MacOSX compilation clang](#macosx-compilation-clang)

- [MacOSX compilation gcc](#macosx-compilation-gcc)

- [Linux compilation gcc](#linux-compilation-gcc)


<!--//////////////////////////////////////////////////////////////////////// -->
## Binary distribution

If you are using `pip`

```
# For Linux sometimes you have to install some python libraries 
# sudo apt-get install python3.6 libpython3/6

python -m pip install --user --upgrade OpenVisus
python -m OpenVisus configure --user
python -m OpenVisus viewer
```

If you are using `conda`:

```
conda install --yes --channel visus openvisus
python -m OpenVisus configure 
python -m OpenVisus viewer
```

Give a look to directory `Samples/python` and Jupyter examples:

[Samples/jupyter/quick_tour.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/quick_tour.ipynb)

[Samples/jupyter/Agricolture.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Agricolture.ipynb)

[Samples/jupyter/Climate.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Climate.ipynb)

[Samples/jupyter/ReadAndView.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/ReadAndView.ipynb)

<!--//////////////////////////////////////////////////////////////////////// -->
## Minimal compilation

Minimal compilation disable 

- Image support
- Network support
- Python supports

it enables only minimal IDX read/write operations.

Add `-DVISUS_MINIMAL=1`. And stop at the CMake `install` step below

<!--//////////////////////////////////////////////////////////////////////// -->
## Windows compilation Visual Studio

Install git, cmake and swig.  
The fastest way is to use `chocolatey`:

```
choco install -y git cmake swig
```

Install Python3.x.

Install Qt5 (http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe)

To compile OpenVisus (change the paths as needed):

```
set Python_EXECUTABLE=C:\Python37\python.exe
set Qt5_DIR=D:\Qt\5.12.8\5.12.8\msvc2017_64\lib\cmake\Qt5

python -m pip install numpy

git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR=%Qt5_DIR% -DPython_EXECUTABLE=%Python_EXECUTABLE% ../ 
cmake --build . --target ALL_BUILD --config Release
cmake --build . --target INSTALL   --config Release

set PYTHON_PATH=.\Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```


<!--//////////////////////////////////////////////////////////////////////// -->
## Windows compilation mingw

NOTE: only VISUS_MINIMAL is supported.

Install prerequisites. The fastest way is to use `chocolatey`:

```
choco install -y git cmake mingw
```

To compile OpenVisus (change the paths as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

mkdir build_gcc
cd build_gcc

set PATH=%PATH%;C:\ProgramData\chocolatey\lib\mingw\tools\install\mingw64\bin


cmake -G "MinGW Makefiles" -DVISUS_MINIMAL=1 ../ 
cmake --build . --target all       --config Release
cmake --build . --target INSTALL   --config Release

set PYTHON_PATH=.\Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```


<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation clang

Make sure you have command line tools:

```
sudo xcode-select --install || sudo xcode-select --reset
```

Build the repository (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# change as needed if you have python in another place
Python_ROOT_DIR=/Library/Frameworks/Python.framework/Versions/3.6

# install prerequisites
brew install swig cmake

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build 
cd build

cmake -GXcode -DPython_ROOT_DIR=${Python_ROOT_DIR} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4
cmake --build ./ --target install   --config Release

export PYTHONPATH=$(pwd)/Release

# this command will install PyQt5 and link OpenVisus to PyQt5 in user space (given that you don't want to run as root)
python3 -m OpenVisus configure --user
python3 -m OpenVisus test
python3 -m OpenVisus viewer

# OPTIONAL
python3 -m pip install --upgrade opencv-python opencv-contrib-python 
python3 -m OpenVisus viewer1
python3 -m OpenVisus viewer2

# OPTIONAL
python3 -m pip install --upgrade jupyter
python3 -m jupyter notebook ../Samples/jupyter/Agricolture.ipynb
```


<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation gcc

Build the repository (change as needed):

```

# change the path for your gcc
export CC=/usr/local/Cellar/gcc/9.1.0/bin/gcc-9
export CXX=/usr/local/Cellar/gcc/9.1.0/bin/g++-9


git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# change as needed if you have python in another place
Python_ROOT_DIR=/Library/Frameworks/Python.framework/Versions/3.6

# install prerequisites
brew install swig cmake

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build_gcc
cd build_gcc
cmake -G"Unix Makefiles" -DPython_ROOT_DIR=${Python_ROOT_DIR} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all       --config Release --parallel 4
cmake --build ./ --target install   --config Release

export PYTHONPATH=$(pwd)/Release

# this command will install PyQt5 and link OpenVisus to PyQt5 in user space (given that you don't want to run as root)
python3 -m OpenVisus configure --user
python3 -m OpenVisus test
python3 -m OpenVisus viewer

# OPTIONAL
python3 -m pip install --upgrade opencv-python opencv-contrib-python 
python3 -m OpenVisus viewer1
python3 -m OpenVisus viewer2

# OPTIONAL
python3 -m pip install --upgrade jupyter
python3 -m jupyter notebook ../Samples/jupyter/Agricolture.ipynb
```

<!--//////////////////////////////////////////////////////////////////////// -->
## Linux compilation gcc

We are showing as an example how to build OpenVisus on Ubuntu 16.

Install prerequisites:

```
sudo apt install -y patchelf swig
```

Install a recent cmake. For example:

```
wget https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-Linux-x86_64.sh
sudo mkdir /opt/cmake
sudo sh cmake-3.17.2-Linux-x86_64.sh --skip-license --prefix=/opt/cmake
sudo ln -s /opt/cmake/bin/cmake /usr/bin/cmake
```

Install python (choose the version you prefer, here we are assuming 3.7):

```
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install -y python3.7 python3.7-dev python3-pip
python3.7 -m pip install numpy
```

Install apache developer files (OPTIONAL for mod_visus):

```
sudo apt-get install -y libapr1 libapr1-dev libaprutil1 libaprutil1-dev apache2-dev
```

Install qt5 (5.12 or another version):

```
#sudo add-apt-repository -y ppa:beineri/opt-qt592-xenial
#sudo apt update
#sudo apt-get install -y qt59base qt59imageformats

sudo add-apt-repository -y ppa:beineri/opt-qt-5.12.8-xenial
sudo apt update
sudo apt-get install -y qt512base qt512imageformats
```


Compile OpenVisus (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

export Python_EXECUTABLE=python3.7
export Qt5_DIR=/opt/qt512/lib/cmake/Qt5 
alias python=${Python_EXECUTABLE}

mkdir build 
cd build

cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4 
cmake --build ./ --target install --config Release

python -m pip install --upgrade pip

export PYTHONPATH=./Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```




<!--//////////////////////////////////////////////////////////////////////// -->
## Commit CI

For OpenVisus developers only:

```
TAG=$(python Libs/swig/setup.py new-tag) && echo ${TAG}
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


