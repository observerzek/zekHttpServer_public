CURRENT_PATH=$(pwd)

MAKE_PATH=$CURRENT_PATH/build

cd $MAKE_PATH

cmake ..

make 

cd $CURRENT_PATH

./simple_proxy_test

