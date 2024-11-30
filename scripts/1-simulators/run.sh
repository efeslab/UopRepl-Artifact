# first clear all previous make files
# second run the make file

(
    cd $UOP_SIMULATOR_ROOT/scarab_foo/src
    make clean
    make -j
)&

(
    cd $UOP_SIMULATOR_ROOT/scarab_getRawTrace/src
    make clean
    make -j
)&

(
    cd $UOP_SIMULATOR_ROOT/scarab_getRipeTrace/src
    make clean
    make -j
)&

(
    cd $UOP_SIMULATOR_ROOT/scarab_ipc_debug/src
    make clean
    make -j
)&

wait