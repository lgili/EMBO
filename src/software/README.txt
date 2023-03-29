1. open Qt project
2. configure:
    build dirs: 
                EMBO/build-win64
                EMBO/build-win32
                EMBO/build-linux
                EMBO/build-macx
                  
3. add build step: make install

On linux
1 create a build folder 
mkdir EMBO/build-linux && cd EMBO/build-linux

2 configure with qmake
qmake EMBO.pro

3 build
make

