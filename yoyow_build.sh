mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/usr/local/boost_version/boost_1_57_0 ../
make yoyow_node -j4
#make yoyow_client -j4
