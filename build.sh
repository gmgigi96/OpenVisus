#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}
CACHE_DIR=${CACHE_DIR:-${BUILD_DIR}/.cache}

# cmake flags
PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

# in case you want to try manylinux-like compilation (for debugging only)
USE_LINUX_PACKAGES=${USE_LINUX_PACKAGES:-1}

# //////////////////////////////////////////////////////
function DownloadFile {

	set +x
	url=$1
	filename=$(basename $url)
	if [ -f "${filename}" ] ; then
		echo "file $filename already downloaded"
		set -x
		return 0	
	fi
	curl -fsSL --insecure "$url" -O
	set -x
}


# //////////////////////////////////////////////////////
# return the next word after the pattern and parse the version in the format MAJOR.MINOR.whatever
function GetVersionFromCommand {	
	set +x
	__version__=""
	__major__=""
	__minor__=""
	__patch__=""
	cmd=$1
	pattern=$2

	set +e
	__content__=$(${cmd} 2>/dev/null)
	retcode=$?
	set -e

	if [ $? == 0 ] ; then 
		__version__=$(echo ${__content__} | awk -F "${pattern}" '{print $2}' | cut -d' ' -f1)
		__major__=$(echo ${__version__} | cut -d'.' -f1)
		__minor__=$(echo ${__version__} | cut -d'.' -f2)
		__patch__=$(echo ${__version__} | cut -d'.' -f3)
		echo "Found version ${__version__}"
	else
		echo "Cannot find any version"
	fi
	set -x
}


# ///////////////////////////////////////////////////////////////////////////////////////////////
function BeginSection {
	set +x	
	echo "//////////////////////////////////////////////////////////////////////// $1"
	set -x
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function DetectOS {	

	BeginSection "DetectOS"

	# if is docker or not
	DOCKER=0
	grep 'docker\|lxc' /proc/1/cgroup && :
	if [ $? == 0 ] ; then 
		export DOCKER=1
	fi

	# If is travis or not 
	if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
		export TRAVIS=1 
	fi

	# sudo allowed or not (in general I assume I cannot use sudo)
	IsRoot=0
	SudoCmd="sudo"
	if (( EUID== 0 || DOCKER == 1 || TRAVIS == 1 )); then 
		IsRoot=1
		SudoCmd=""
	fi

	# osx
	if [ $(uname) = "Darwin" ]; then
		OSX=1
		CheckPackageCommand="brew list"
		PackageCommand="brew"
		echo "Detected OSX"

	# ubuntu
	elif [ -x "$(command -v apt-get)" ]; then
		UBUNTU=1

		if [ -f /etc/os-release ]; then
			source /etc/os-release
			export UBUNTU_VERSION=$VERSION_ID

		elif type lsb_release >/dev/null 2>&1; then
			export UBUNTU_VERSION=$(lsb_release -sr)

		elif [ -f /etc/lsb-release ]; then
			source /etc/lsb-release
			export UBUNTU_VERSION=$DISTRIB_RELEASE
		fi

		CheckPackageCommand="dpkg -s"
		PackageCommand="${SudoCmd} apt-get --quiet --yes --allow-unauthenticated"
		echo "Detected ubuntu ${UBUNTU_VERSION}"

	# opensuse
	elif [ -x "$(command -v zypper)" ]; then
		OPENSUSE=1
		CheckPackageCommand="rpm -q"
		PackageCommand="${SudoCmd} zypper --quiet --non-interactive"
		echo "Detected opensuse"

	# centos
	elif [ -x "$(command -v yum)" ]; then
		CENTOS=1
		GetVersionFromCommand "cat /etc/redhat-release" "CentOS release "
		CENTOS_VERSION=${__version__}
		CENTOS_MAJOR=${__major__}
		CheckPackageCommand="yum --quiet list installed"
		PackageCommand="${SudoCmd} yum --quiet -y"
		echo "Detected centos ${CENTOS_VERSION}"

	else
		echo "Failed to detect OS version, I will keep going but it could be that I won't find some dependency"
	fi
}




# //////////////////////////////////////////////////////
function InstallPackages {

	set +x

	packages=$@
	echo "Installing packages ${packages}..."

	AlreadyInstalled=1
	for package in ${packages} ; do
		${CheckPackageCommand} ${package} 1>/dev/null 2>/dev/null && : 
		retcode=$?

		if [ ${retcode} == 0 ] ; then 
			echo "Package  ${package} already installed"
		else

			if [[ "${SudoCmd}" != "" && ${PackageCommand} == *"${SudoCmd}"* && "${IsRoot}" == "0" ]]; then
				echo "Failed to install ${package} because I need ${SudoCmd}: ${packages}"
				set -x
				return 1
			fi

			${PackageCommand} install ${package}  1>/dev/null && : 
			retcode=$?
			if ((  retcode != 0 )) ; then 
				echo "Failed to install: ${package}"
				set -x
				return 1
			fi

			echo "Installed package ${packages}"
		fi
	done

	set -x
	return 0
}


# //////////////////////////////////////////////////////
function InstallCMakeForLinux {

	BeginSection "InstallCMakeForLinux"

	if (( USE_LINUX_PACKAGES == 1 )); then
		InstallPackages cmake && :

		# already installed
		if [[ -x "$(command -v cmake)" ]]; then
			GetVersionFromCommand "cmake --version" "cmake version "
			if (( __major__== 3 && __minor__ >= 9 )); then
				echo "Good version: cmake==${__version__}"
				return 0
			else
				echo "Wrong version: cmake==${__version__} "
			fi
		fi 
	fi

	if [ ! -f "${CACHE_DIR}/bin/cmake" ]; then

		echo "installing cached cmake"
	
		__version__=3.10.1
		if (( CENTOS == 1 && CENTOS_MAJOR <= 5 )) ; then  
			__version__=3.4.3  # Error with other  versions: `GLIBC_2.6' not found (required by cmake)
		fi 

		url="https://github.com/Kitware/CMake/releases/download/v${__version__}/cmake-${__version__}-Linux-x86_64.tar.gz"
		filename=$(basename ${url})
		DownloadFile "${url}"
		tar xzf ${filename} -C ${CACHE_DIR} --strip-components=1 
		rm -f ${filename}
	fi

	return 0
}

# //////////////////////////////////////////////////////
function InstallSwigForLinux {

	BeginSection "InstallSwigForLinux"
	if (( USE_LINUX_PACKAGES == 1 )); then

		InstallPackages swig3.0 swig && :

		# already installed
		if [[ -x "$(command -v swig3.0)" ]]; then
			SWIG_EXECUTABLE=$(which swig3.0)
			return 0
		fi

		# already installed and good version
		if [[ -x "$(command -v swig)" ]]; then
			GetVersionFromCommand "swig -version" "SWIG Version "
			if (( __major__>= 3)); then
				echo "Good version: swig==${__version__}"
				SWIG_EXECUTABLE=$(which swig)	
				return 0
			else
				echo "Wrong version: swig==${__version__}"
			fi
		fi 
	fi

	SWIG_EXECUTABLE=${CACHE_DIR}/bin/swig
	if [ ! -f "${SWIG_EXECUTABLE}" ]; then
		echo "installing cached swig"
		url="https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd swig-3.0.12
		DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
		./Tools/pcre-build.sh 1>/dev/null
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s -j 4 1>/dev/null 
		make install 1>/dev/null 
		popd
		rm -Rf swig-3.0.12
	fi

	
	return 0
}

# //////////////////////////////////////////////////////
function InstallPatchElfForLinux {

	BeginSection "InstallPatchElfForLinux"

	InstallPackages patchelf && :
	if [ -x "$(command -v patchelf)" ]; then
		echo "Already installed: patchelf"
		return 0
	fi

	if [ ! -f "${CACHE_DIR}/bin/patchelf" ]; then
		url="https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd patchelf-0.9
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null 
		autoreconf -f -i
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null 
		popd
		rm -Rf patchelf-0.9
	fi

	return 0
}

# //////////////////////////////////////////////////////
function CheckOpenSSLVersion {

	GetVersionFromCommand "$1 version" "OpenSSL "

	if [[ "${__major__}" == "" || "${__minor__}" == "" || "${__patch__}" == "" ]]; then
		echo "OpenSSL not found"
		return -1

	# Python requires an OpenSSL 1.0.2 or 1.1 compatible libssl with X509_VERIFY_PARAM_set1_host().
	elif [[ "${__major__}" == "1" && "${__minor__}" == "0" && "${__patch__:0:1}" -lt "2" ]]; then
		echo "OpenSSL version(${__version__}) too old"
		return -1

	else
		echo "Openssl version(${__version__}) ok"
		return 0

	fi
}


# //////////////////////////////////////////////////////
function InstallOpenSSLForLinux {

	if (( USE_LINUX_PACKAGES == 1 )); then

		if (( UBUNTU == 1 )) ; then
			InstallPackages libssl-dev && : 

		elif (( OPENSUSE == 1 )) ; then
			InstallPackages libopenssl-devel && :

		elif (( CENTOS == 1 )) ; then
			InstallPackages openssl-devel
		fi

		if [ -x "/usr/bin/openssl" ] ; then
			CheckOpenSSLVersion /usr/bin/openssl && : 
			if [ $? == 0 ] ; then return 0 ; fi
		fi

	fi

	export OPENSSL_DIR="${CACHE_DIR}" 
	if [ ! -f "${OPENSSL_DIR}/bin/openssl" ]; then
		echo "installing cached openssl"
		url="https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd openssl-1.0.2a
		./config --prefix=${CACHE_DIR} -fPIC shared 1>/dev/null 
		make -s 1>/dev/null  
		make install 1>/dev/null 
		popd
		rm -Rf openssl-1.0.2a
	fi	
	export LD_LIBRARY_PATH="${CACHE_DIR}/lib:${LD_LIBRARY_PATH}"
	return 0
}


# //////////////////////////////////////////////////////
function InstallApacheForLinux {

	BeginSection "InstallApacheForLinux"

	if (( USE_LINUX_PACKAGES == 1 )); then

		if (( UBUNTU == 1 )); then
			InstallPackages apache2 apache2-dev   && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( OPENSUSE == 1 )); then
			InstallPackages apache2 apache2-devel   && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( CENTOS == 1 )) ; then
			# for centos I prefer to build from scratch
			echo "centos, prefers source apache"
		fi

	fi

	APR_DIR={CACHE_DIR}
	APACHE_DIR=${CACHE_DIR}
	if [ ! -f "${APACHE_DIR}/include/httpd.h" ] ; then

		echo "installing cached apache"

		# expat
		url="https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-2.2.6.tar.bz2"
		filename=$(basename ${url})
		DownloadFile  ${url}
		tar xjf ${filename}
		pushd expat-2.2.6
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make
		make install
		popd
		rm -Rf expat-2.2.6

		# install apr 
		url="http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd apr-1.6.5
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null 
		popd
		rm -Rf apr-1.6.5

		# install apr utils 
		url="http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd apr-util-1.6.1
		./configure --prefix=${CACHE_DIR} --with-apr=${CACHE_DIR} --with-expat=${CACHE_DIR} 1>/dev/null  
		make -s 1>/dev/null 
		make install 1>/dev/null 
		popd
		rm -Rf apr-util-1.6.1

		# install pcre 
		url="https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd pcre-8.42
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null 
		popd
		rm -Rf pcre-8.42

		# install httpd
		url="http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd httpd-2.4.38
		./configure --prefix=${CACHE_DIR} --with-apr=${CACHE_DIR} --with-pcre=${CACHE_DIR} --with-ssl=${CACHE_DIR} --with-expat=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null 
		popd
		rm -Rf httpd-2.4.38
	fi	

	return 0
}



# //////////////////////////////////////////////////////
function InstallQt5ForLinux {
	
	BeginSection "InstallQt5ForLinux"

	# already set by user
	if [[ "${Qt5_DIR}" != "" ]] ; then
		return 0
	fi
	
	# I need opengl
	if (( UBUNTU == 1 )); then
		InstallPackages mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev  && :
	elif (( OPENSUSE == 1 )); then
		InstallPackages glu-devel  && :
	elif (( CENTOS ==  1 )); then
		InstallPackages mesa-libGL-devel mesa-libGLU-devel && : 
	fi

	# try to use OS Qt5
	if (( USE_LINUX_PACKAGES == 1 )); then

		# ubuntu
		if (( UBUNTU == 1 )) ; then

			# https://launchpad.net/~beineri
			# PyQt5 versions 5.6, 5.7, 5.7.1, 5.8, 5.8.1.1, 5.8.2, 5.9, 5.9.1, 5.9.2, 5.10, 5.10.1, 5.11.2, 5.11.3
			if (( ${UBUNTU_VERSION:0:2} <=14 )); then
				QT5_PACKAGE=qt510base
				QT5_REPOSITORY=ppa:beineri/opt-qt-5.10.1-trusty
				OPT_QT5_DIR=/opt/qt510/lib/cmake/Qt5

			elif (( ${UBUNTU_VERSION:0:2} <=16 )); then
				QT5_PACKAGE=qt511base
				QT5_REPOSITORY=ppa:beineri/opt-qt-5.11.2-xenial
				OPT_QT5_DIR=/opt/qt511/lib/cmake/Qt5

			elif (( ${UBUNTU_VERSION:0:2} <=18)); then
				QT5_PACKAGE=qt511base
				QT5_REPOSITORY=ppa:beineri/opt-qt-5.11.2-bionic
				OPT_QT5_DIR=/opt/qt511/lib/cmake/Qt5

			else
				InternalError
			fi

			if (( IsRoot == 1 )) ; then
				${SudoCmd} add-apt-repository ${QT5_REPOSITORY} -y && :
				${PackageCommand} update && :
			fi

			InstallPackages ${QT5_PACKAGE}  && : 
			if [ $? == 0 ] ; then
				echo "Using Qt5 from unbuntu repository"
				Qt5_DIR=${OPT_QT5_DIR}
				return 0
			fi
		fi

		# opensuse 
		if (( OPENSUSE == 1 )) ; then	
			InstallPackages libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel && : 
			if [ $? == 0 ] ; then return 0 ; fi
		fi

	fi

	# backup plan , use a minimal Qt5 which does not need SUDO
	# if you want to create a "new" minimal Qt5 see CMake/Dockerfile.BuildQt5
	# note this is only to allow compilation
	# in order to execute it you need to use PyUseQt 
	echo "Using minimal Qt5"
	QT_VERSION=5.11.2
	Qt5_DIR=${CACHE_DIR}/qt${QT_VERSION}/lib/cmake/Qt5
	if [ ! -d "${Qt5_DIR}" ] ; then
		url="http://atlantis.sci.utah.edu/qt/qt${QT_VERSION}.tar.gz"
		filename=$(basename ${url})
		DownloadFile "${url}"
		tar xzf ${filename} -C ${CACHE_DIR} 
	fi

	return 0
}

# ///////////////////////////////////////////////////////
function InstallPythonForOsx {

	BeginSection InstallPythonForOsx

	# pyenv does not support 3.7.x  maxosx 10.(12|13)
	if (( PYTHON_MAJOR_VERSION > 2 )); then

		PYTHON_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
		package_name=python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}
		InstallPackages sashkab/python/${package_name} 
		package_dir=$(brew --prefix ${package_name})
		PYTHON_EXECUTABLE=${package_dir}/bin/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
		PYTHON_INCLUDE_DIR=${package_dir}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/include/python${PYTHON_M_VERSION}
		PYTHON_LIBRARY=${package_dir}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/lib/libpython${PYTHON_M_VERSION}.dylib
		
	else

		InstallPackages readline zlib openssl openssl@1.1 pyenv libffi

		eval "$(pyenv init -)"

		CONFIGURE_OPTS="--enable-shared" \
		CFLAGS="   -I$(brew --prefix readline)/include -I$(brew --prefix zlib)/include" \
		CPPFLAGS=" -I$(brew --prefix readline)/include -I$(brew --prefix zlib)/include" \
		LDFLAGS="  -L$(brew --prefix readline)/lib     -L$(brew --prefix zlib)/lib" \
		pyenv install --skip-existing ${PYTHON_VERSION} && :

		if [ $? != 0 ] ; then 
			echo "pyenv failed to install"
			pyenv install --list
			exit -1
		fi

		pyenv global ${PYTHON_VERSION}
		pyenv rehash
	
		PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
		PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION}
		PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.dylib 	
		
	fi	

	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine auditwheel

	return 0
}


# ///////////////////////////////////////////////////////
function InstallPythonForLinux {

	BeginSection InstallPythonForLinux

	# install pyenv
	if [ ! -f "$HOME/.pyenv/bin/pyenv" ]; then
		pushd $HOME
		DownloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
		chmod a+x pyenv-installer
		./pyenv-installer
		rm -f pyenv-installer
		popd
	fi
	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"

	# install python
	export CONFIGURE_OPTS="--enable-shared"

	if [[ "$OPENSSL_DIR" != "" ]] ; then
		export CONFIGURE_OPTS="${CONFIGURE_OPTS} --with-openssl=${OPENSSL_DIR}"
		export CFLAGS="  -I${OPENSSL_DIR}/include -I${OPENSSL_DIR}/include/openssl"
		export CPPFLAGS="-I${OPENSSL_DIR}/include -I${OPENSSL_DIR}/include/openssl"
		export LDFLAGS=" -L${OPENSSL_DIR}/lib"
	fi
	CXX=g++ pyenv install --skip-existing ${PYTHON_VERSION}  && :
	if [ $? != 0 ] ; then 
		echo "pyenv failed to install"
		pyenv install --list
		exit -1
	fi

	unset CONFIGURE_OPTS
	unset CFLAGS
	unset CPPFLAGS
	unset LDFLAGS
	
	# activate pyenv
	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"
	pyenv global ${PYTHON_VERSION}
	pyenv rehash

	PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
	PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION}
	PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so

	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine auditwheel		
	
	return 0

}

# /////////////////////////////////////////////////////////////////////
function InstallCondaPython {

	# here I need sudo! 
	if (( OSX ==  1)) ; then
		if [ ! -d /opt/MacOSX10.9.sdk ] ; then
		  if (( IsRoot == 1 )) ; then
				git clone  https://github.com/phracker/MacOSX-SDKs.git 
				sudo mv MacOSX-SDKs/MacOSX10.9.sdk /opt/
				rm -Rf MacOSX-SDKs
			else
				echo "Missing /opt/MacOSX10.9.sdk, but to install it I need sudo"
				exit -1
			fi
		fi
	fi

	# install Miniconda
	MINICONDA_ROOT=$HOME/miniconda${PYTHON_MAJOR_VERSION}
	if [ ! -d  ${MINICONDA_ROOT} ]; then
		pushd $HOME
		if (( OSX == 1 )) ; then
			DownloadFile https://repo.continuum.io/miniconda/Miniconda${PYTHON_MAJOR_VERSION}-latest-MacOSX-x86_64.sh
			bash Miniconda${PYTHON_MAJOR_VERSION}-latest-MacOSX-x86_64.sh -b
		else
			DownloadFile https://repo.continuum.io/miniconda/Miniconda${PYTHON_MAJOR_VERSION}-latest-Linux-x86_64.sh
			bash Miniconda${PYTHON_MAJOR_VERSION}-latest-Linux-x86_64.sh -b
		fi
		popd
	fi

	# config Miniconda
	export PATH="${MINICONDA_ROOT}/bin:$PATH"

	hash -r	
	conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
	conda install -q conda-build anaconda-client          && :
	conda update  -q conda conda-build                    && :

	conda create -q  -n mypython python=${PYTHON_VERSION} && :
	conda init bash                                       && :
	conda activate mypython                               && :
	PYTHON_EXECUTABLE=python

}

# /////////////////////////////////////////////////////////////////////
function DeployToPyPi {
	WHEEL_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.whl")
	echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
	echo [distutils]                                  > ~/.pypirc
	echo index-servers =  pypi                       >> ~/.pypirc
	echo [pypi]                                      >> ~/.pypirc
	echo username=${PYPI_USERNAME}                   >> ~/.pypirc
	echo password=${PYPI_PASSWORD}                   >> ~/.pypirc

	${PYTHON_EXECUTABLE} -m twine upload --skip-existing "${WHEEL_FILENAME}"
}

# /////////////////////////////////////////////////////////////////////
function DeployToGitHub {

	filename=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.tar.gz")

	# rename to avoid collisions
	if (( UBUNTU == 1 )); then
		old_filename=$filename
		filename=${filename/manylinux1_x86_64.tar.gz/ubuntu.${UBUNTU_VERSION}.tar.gz}
		mv $old_filename $filename

	elif (( OPENSUSE == 1 )); then
		old_filename=$filename
		filename=${filename/manylinux1_x86_64.tar.gz/opensuse.tar.gz}
		mv $old_filename $filename

	elif (( CENTOS == 1 )); then
		old_filename=$filename
		filename=${filename/manylinux1_x86_64.tar.gz/centos.${CENTOS_VERSION}.tar.gz}
		mv $old_filename $filename

	fi

	response=$(curl -sH "Authorization: token ${GITHUB_API_TOKEN}" https://api.github.com/repos/sci-visus/OpenVisus/releases/tags/${TRAVIS_TAG})
	eval $(echo "$response" | grep -m 1 "id.:" | grep -w id | tr : = | tr -cd '[[:alnum:]]=')

	curl --data-binary @"${filename}" \
		-H "Authorization: token $GITHUB_API_TOKEN" \
		-H "Content-Type: application/octet-stream" \
		"https://uploads.github.com/repos/sci-visus/OpenVisus/releases/$id/assets?name=$(basename ${filename})"
}

# /////////////////////////////////////////////////////////////////////
function DeployToConda {
	CONDA_BUILD_FILENAME=$(find ${HOME}/miniconda${PYTHON_MAJOR_VERSION}/conda-bld -iname "openvisus*.tar.bz2")
	echo "Doing deploy to anaconda ${CONDA_BUILD_FILENAME}..."
	anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
}

# /////////////////////////////////////////////////////////////////////
function AddCMakeOption {
	key=$1
	value=$2
	if [[ "${value}" != "" ]]; then
		cmake_opts+=(${key}=${value})
	fi
}

# /////////////////////////////////////////////////////////////////////
DetectOS

PYTHON_MAJOR_VERSION=${PYTHON_VERSION:0:1}
PYTHON_MINOR_VERSION=${PYTHON_VERSION:2:1}	

if (( PYTHON_MAJOR_VERSION > 2 )) ; then 
	PYTHON_M_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}m 
else
	PYTHON_M_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
fi

if (( USE_CONDA == 1 )) ; then
	DISABLE_OPENMP=1
	USE_LINUX_PACKAGES=0
	VISUS_GUI=0 # todo: can Qt5 work?
fi

if (( CENTOS == 1 && CENTOS_MAJOR == 5 )) ; then
	DISABLE_OPENMP=1
	USE_LINUX_PACKAGES=0
fi

if [[ "$TRAVIS" == "1" && "${TRAVIS_TAG}" != "" ]] ; then

	# deploy to conda?
	if [[ "${USE_CONDA}" == "1" ]] ; then
		DEPLOY_CONDA=1
	fi

	# deploy to pypi | github
	if [[ "${USE_CONDA}" == "0" && "${OSX}" == "1" || "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]] ; then
		DEPLOY_PYPI=1
		DEPLOY_GITHUB=1
	fi
fi

# forward to conda/OpenVisus/build.sh
if (( USE_CONDA == 1 && INSIDE_CONDA == 0 )) ; then

	InstallCondaPython
	
	pushd ${SOURCE_DIR}/conda
	conda-build -q openvisus
	conda install -q --use-local openvisus
	popd

	# test
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	${PYTHON_EXECUTABLE} Samples/python/Array.py
	${PYTHON_EXECUTABLE} Samples/python/Dataflow.py
	${PYTHON_EXECUTABLE} Samples/python/Idx.py
	popd

	# the deploy happens here for the top-level build
	if (( DEPLOY_CONDA == 1 )) ; then
		DeployToConda
	fi

	exit 0
fi

# docker build
if [[ "$DOCKER_IMAGE" != "" ]] ; then

	BeginSection "BuildUsingDocker"

	mkdir -p ${BUILD_DIR}
	mkdir -p ${CACHE_DIR}

	# note: sudo is needed anyway otherwise travis fails
	sudo docker rm -f mydocker 2>/dev/null || true

	declare -a docker_opts

	docker_opts+=(-v ${SOURCE_DIR}:/root/OpenVisus)
	docker_opts+=(-v ${BUILD_DIR}:/root/OpenVisus.build)
	docker_opts+=(-v ${CACHE_DIR}:/root/OpenVisus.cache)

	docker_opts+=(-e SOURCE_DIR=/root/OpenVisus)
	docker_opts+=(-e BUILD_DIR=/root/OpenVisus.build)
	docker_opts+=(-e CACHE_DIR=/root/OpenVisus.cache)

	docker_opts+=(-e PYTHON_VERSION=${PYTHON_VERSION})
	docker_opts+=(-e DISABLE_OPENMP=${DISABLE_OPENMP})
	docker_opts+=(-e VISUS_GUI=${VISUS_GUI})
	docker_opts+=(-e VISUS_MODVISUS=${VISUS_MODVISUS})
	docker_opts+=(-e CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

	# this is needed to forward the deploy to the docker image (where I have a python to do the deploy)
	docker_opts+=(-e DEPLOY_PYPI=${DEPLOY_PYPI})
	docker_opts+=(-e PYPI_USERNAME=${PYPI_USERNAME})
	docker_opts+=(-e PYPI_PASSWORD=${PYPI_PASSWORD})

	# this is needed to foward the deploy to the docker image (where I have a python to do the deploy)
	docker_opts+=(-e DEPLOY_GITHUB=${DEPLOY_GITHUB})
	docker_opts+=(-e GITHUB_API_TOKEN=${GITHUB_API_TOKEN})
	docker_opts+=(-e TRAVIS_TAG=${TRAVIS_TAG})

	sudo docker run -d -ti --name mydocker ${docker_opts[@]} ${DOCKER_IMAGE} /bin/bash
	sudo docker exec mydocker /bin/bash -c "cd /root/OpenVisus && ./build.sh"
	sudo chown -R "$USER":"$USER" ${BUILD_DIR} 1>/dev/null && :
	sudo chmod -R u+rwx           ${BUILD_DIR} 1>/dev/null && :

	exit 0
fi

# all other cases
mkdir -p ${BUILD_DIR}
mkdir -p ${CACHE_DIR}
export PATH=${CACHE_DIR}/bin:$PATH

pushd ${BUILD_DIR}

# install OpenVisus dependencies
if (( USE_CONDA == 0 )) ; then

	if (( OSX == 1 )) ; then

		#  for travis long log
		if (( TRAVIS == 1 )) ; then
			${SudoCmd} gem install xcpretty 
		fi 

		# brew
		if [ !  -x "$(command -v brew)" ]; then
			/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
		fi

		${PackageCommand} update 1>/dev/null
		
		# cmake
		InstallPackages cmake 

		# swig
		InstallPackages swig 
		SWIG_EXECUTABLE=$(which swig)

		# python
		InstallPythonForOsx

		# qt5
		if (( VISUS_GUI == 1 )); then
			if [ ! -d /usr/local/Cellar/qt/5.11.2_1 ] ; then
				echo "installing brew Qt5"
				${PackageCommand} uninstall qt5 1>/dev/null && :
				InstallPackages "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb" 
			fi
			Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5
		fi

	else

		if (( IsRoot == 1 )) ; then

			${PackageCommand} update 

			# install compilers
			if (( UBUNTU == 1  )) ; then
				InstallPackages software-properties-common 
				if (( ${UBUNTU_VERSION:0:2}<=14 )); then
					${SudoCmd} add-apt-repository -y ppa:deadsnakes/ppa
					${PackageCommand} update
				fi
				InstallPackages build-essential make automake 

			elif (( OPENSUSE == 1 )) ; then
				${PackageCommand} install --type pattern devel_basis 
				InstallPackages gcc-c++ make 

			elif (( CENTOS == 1 )) ; then
				InstallPackages gcc-c++ make 

			fi
		fi

		# some libraries I may need later
		if (( UBUNTU == 1 )) ; then
			InstallPackages git curl ca-certificates uuid-dev bzip2 libffi-dev && :

		elif (( OPENSUSE == 1 )) ; then
			InstallPackages git curl lsb-release libuuid-devel libffi-devel  && :

		elif (( CENTOS == 1 )) ; then
			InstallPackages git curl zlib zlib-devel libffi-devel  && :
		fi

		InstallPatchElfForLinux
		InstallCMakeForLinux
		InstallSwigForLinux
		InstallOpenSSLForLinux
		InstallPythonForLinux

		if (( VISUS_MODVISUS == 1 )); then	
			InstallApacheForLinux
		fi

		if (( VISUS_GUI == 1 )); then
			InstallQt5ForLinux
		fi

	fi

fi

BeginSection "Build OpenVisus"

declare -a cmake_opts

CMAKE_TEST_STEP="test"
CMAKE_ALL_STEP="all"
if (( OSX == 1 && USE_CONDA == 0 )) ; then
	cmake_opts+=(-GXcode)
	CMAKE_TEST_STEP="RUN_TESTS"
	CMAKE_ALL_STEP="ALL_BUILD"
fi

AddCMakeOption -DDISABLE_OPENMP       "${DISABLE_OPENMP}"
AddCMakeOption -DVISUS_GUI            "${VISUS_GUI}"
AddCMakeOption -DVISUS_MODVISUS       "${VISUS_MODVISUS}"
AddCMakeOption -DCMAKE_BUILD_TYPE     "${CMAKE_BUILD_TYPE}"
AddCMakeOption -DPYTHON_VERSION       "${PYTHON_VERSION}"
AddCMakeOption -DPYTHON_EXECUTABLE    "${PYTHON_EXECUTABLE}"
AddCMakeOption -DPYTHON_INCLUDE_DIR   "${PYTHON_INCLUDE_DIR}"
AddCMakeOption -DPYTHON_LIBRARY       "${PYTHON_LIBRARY}"
AddCMakeOption -DSWIG_EXECUTABLE      "${SWIG_EXECUTABLE}"
AddCMakeOption -DAPR_DIR              "${APR_DIR}"
AddCMakeOption -DAPACHE_DIR           "${APACHE_DIR}"
AddCMakeOption -DQt5_DIR              "${Qt5_DIR}"
AddCMakeOption -DCMAKE_OSX_SYSROOT    "${CMAKE_OSX_SYSROOT}"
AddCMakeOption -DCMAKE_TOOLCHAIN_FILE "${CMAKE_TOOLCHAIN_FILE}"

# compile and install
cmake ${cmake_opts[@]} ${SOURCE_DIR}
if (( TRAVIS == 1 && OSX == 1 )) ; then
	cmake --build ./ --target ${CMAKE_ALL_STEP} --config ${CMAKE_BUILD_TYPE} | xcpretty -c
else
	cmake --build ./ --target ${CMAKE_ALL_STEP} --config ${CMAKE_BUILD_TYPE}
fi	

cmake --build . --target install --config ${CMAKE_BUILD_TYPE}


# cmake tests 
if (( USE_CONDA == 0 )) ; then
	BeginSection "Test OpenVisus (cmake ${CMAKE_TEST_STEP})"
	pushd ${BUILD_DIR}
	cmake --build  ./ --target  ${CMAKE_TEST_STEP} --config ${CMAKE_BUILD_TYPE}	
	popd
fi

# cmake external app
if (( USE_CONDA == 0 )) ; then
	BeginSection "Test OpenVisus (cmake external app)"
	pushd ${BUILD_DIR}
	cmake --build ./ --target  simple_query --config ${CMAKE_BUILD_TYPE}
	if (( VISUS_GUI == 1 )) ; then
		cmake --build   . --target   simple_viewer2d --config ${CMAKE_BUILD_TYPE}
	fi	
	popd
fi

if (( USE_CONDA == 0 )) ; then
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages
fi

# test extending python
if (( USE_CONDA == 0 )) ; then
	BeginSection "Test OpenVisus (extending python)"
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	${PYTHON_EXECUTABLE} Samples/python/Array.py
	${PYTHON_EXECUTABLE} Samples/python/Dataflow.py
	${PYTHON_EXECUTABLE} Samples/python/Idx.py
	popd
fi

# test embedding python
if (( USE_CONDA == 0 )) ; then
	BeginSection "Test OpenVisus (embedding python)"
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	if (( OSX == 1 )) ; then
		./visus.command
	else
		./visus.sh
	fi
	popd
fi

# deploy
if (( DEPLOY_PYPI == 1 || DEPLOY_GITHUB == 1 )) ; then
	pushd ${BUILD_DIR}
	cmake --build . --target dist --config ${CMAKE_BUILD_TYPE}
	popd

	if (( DEPLOY_PYPI == 1 )) ; then
		DeployToPyPi
	fi

	if (( DEPLOY_GITHUB == 1 )) ; then
		DeployToGitHub
	fi

fi

echo "OpenVisus build finished"
exit 0



