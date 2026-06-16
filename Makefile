all:
	cmake -B build -DBUILD_EXAMPLES=ON && cmake --build build -j4