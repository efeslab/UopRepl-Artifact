# Below is an example of the source.sh file that you can use to set up the environment variables for the simulator.
# Please modify the paths to match your own setup.

export DRIO_ROOT=/code/3rdparty/dynamorio
export PIN_ROOT=/code/3rdparty/pinplay-drdebug-3.5-pin-3.5-97503-gac534ca30-gcc-linux
export LD_LIBRARY_PATH=$PIN_ROOT/extras/xed-intel64/lib/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=$PIN_ROOT/intel64/runtime/pincrt/:$LD_LIBRARY_PATH

export UOP_SIMULATOR_ROOT=/code/simulators
export UOP_DATACENTER_TRACES=/code/datacenterTrace
export UOP_SCRIPTS_ROOT=/code/scripts