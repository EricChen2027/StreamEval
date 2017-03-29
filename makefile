g++ common.h FlvCheck.* main.cpp pauta.* StreamEval.* Stream2File.* StreamCheck.* ./h264bitstream/* -I /usr/lib64/fcgi/include/ -L ./lib/ -lcurl -lz -lrtmp -lstdc++ -lrt -lfcgi -ljsoncpp -lpcre -o streamEval

cp streamEval /home/chenyu/nginx/cgi-bin

/home/chenyu/nginx/sbin/spawn-fcgi -a 127.0.0.1 -p 8888 -F 8 -f /home/chenyu/nginx/cgi-bin/streamEval
