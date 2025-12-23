CURRENT_PATH=$(pwd)

MAKE_PATH=$CURRENT_PATH/../../build/1_unitTest/build

cd $MAKE_PATH

cmake ..

make 

cd $CURRENT_PATH

./httpcontent_test