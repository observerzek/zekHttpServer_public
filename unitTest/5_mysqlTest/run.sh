CURRENT_PATH=$(pwd)

MAKE_PATH=$CURRENT_PATH/../../build/5_unitTest/build

cd $MAKE_PATH

cmake ..

make 

cd $CURRENT_PATH

./mysqlpool_test

