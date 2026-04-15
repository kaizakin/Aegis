g++ -O2 -o memtest tests/memory_test.cpp

sudo mkdir -p /tmp/aegis/lower/bin
sudo cp ./memtest /tmp/aegis/lower/bin/

ldd ./memtest | grep -oE '/[^ ]+' | xargs -I '{}' sudo cp --parents '{}' /tmp/aegis/lower/
sudo cp /usr/lib64/ld-linux-x86-64.so.2 /tmp/aegis/lower/lib64/

g++ src/*.cpp -Iinclude -o aegis  

sudo ./aegis /bin/memtest
