CURRENT_PATH=$(pwd)

MAKE_PATH=$CURRENT_PATH/../../build/3_unitTest/build

cd $MAKE_PATH

cmake ..

make 

cd $CURRENT_PATH

./ssl_test
