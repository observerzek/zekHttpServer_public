CURRENT_PATH=$(pwd)

MAKE_PATH=$CURRENT_PATH/../../build/2_unitTest/build

cd $MAKE_PATH

cmake ..

make 

cd $CURRENT_PATH

./httpserver_test
