# -DBUILD_EXAMPLES=ON -DBUILD_SHARED_LIBS=ON 编译动态链接库目前还有一些异常 最好是
cmake -B build -DBUILD_EXAMPLES=ON -DBUILD_TESTS=ON && cmake --build build -j6