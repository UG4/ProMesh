# This is a complete sequence of shell commands AND additional patching
# instructions for building ProMesh4 application on macOS. It has been tested
# on macOS 26 Tahoe on Jan. and Feb. 2026.
# Author: D. Logashenko
# Essentially based on instructions by A. Naegel and S. Reiter.
# In particular, see readme_compilation.txt

# Before executing every line, read the preceeding comments (if any)!
# Do not run this text as a shell script!

# It is recommended to run make with -j <max-num-avail-cores>, otherwise the
# building process could take really long time. In the text below, "-j 12" is
# used but it should be matched for every particular machine.

# ProMesh uses OpenGL. In the last versions of macOS, OpenGL is considered as
# "deprecated". In macOS 26 Tahoe, it is still present, but its framework and
# headers are excluded from the standard settings. They can be installed with
# the XCode command line tools. If this has not been done before, i.e.
# if the OpenGL headers are required, run the following commands:

xcode-select --install

# Create a directory for the entire installation. Execute all the following
# commands in this directory.

# Get and build Qt5

mkdir qt5-build qt5-install
git clone git://code.qt.io/qt/qt5.git
cd qt5
git checkout 5.15.17
git submodule update --init --recursive

# Patch qtbase/mkspecs/common/mac.conf:
# Replace Lines 21-22 by
#     /System/Library/Frameworks/OpenGL.framework/Headers
# (former line 21 without "\" at the end)
# Replace (original) Line 33 by
# QMAKE_LIBS_OPENGL       = -framework OpenGL
# (remove " -framework AGL" at the end)
# Alternatively: Replace the entire file with the version in the dev branch from the same repository (i.e. from qt6)

cd ../qt5-build
../qt5/configure -release -opensource -prefix ../qt5-install -skip qtconnectivity -skip qtwebengine -skip qtwebglplugin -nomake examples -nomake tests -confirm-license
make -j 12
make -j 12 install

cd ..

# Get and build ug4 in the configuration for ProMesh

git clone https://github.com/UG4/ughub
mkdir ug4 ug4-build
cd ug4
../ughub/ughub init
../ughub/ughub install LuaShell ProMesh tetgen
cd externals/EigenForUG4
git checkout feature-eigen34
cd ../../../ug4-build
cmake -DTARGET=libgrid -DDIM=3 -DCPU=1 -DSTATIC_BUILD=ON -DCMAKE_BUILD_TYPE=Release -DLuaShell=ON -DProMesh=ON -DPARALLEL=OFF -Dtetgen=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ../ug4
make -j 12

cd ..

# Get and build the ProMesh4 application

git clone https://github.com/UG4/ProMesh
mkdir ProMesh-build
cd ProMesh-build
cmake -DCMAKE_BUILD_TYPE=Release -DUG_ROOT_PATH=../ug4 -DQT_CMAKE_PATH=../qt5-install/lib/cmake -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ../ProMesh
make -j 12
cd ProMesh4.app/Contents
mkdir Resources
cp ../../../ProMesh/deployment/data/AppIcon.icns Resources/
cp ../../../ProMesh/deployment/data/Info.plist .
cd ../..

# Now, the application is ./ProMesh4.app (in ProMesh-build, the current directory).
# This is a standard macOS application.

# End of File
