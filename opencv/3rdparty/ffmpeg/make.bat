rem gcc -Wall -shared -o opencv_ffmpeg.dll -O3 -x c++ -I../include -I../include/ffmpeg_ -I../../modules/highgui/src ffopencv.c -L../lib -lavformat -lavcodec -lavdevice -lswscale -lavutil -lwsock32 -lpthreadG�E2
gcc -Wall -shared -o opencv_ffmpeg.dll -O2 -x c++ -I../include -I../include/ffmpeg_ -I../../modules/highgui/src ffopencv.c -L../lib -lavformat -lavcodec -lavdevice -lswscale -lavutil -lwsock32
set path=c:\apps\mingw64\bin;%path% & gcc -Wall -shared -o opencv_ffmpeg_64.dll -O2 -x c++ -I../include -I../include/ffmpeg_ -I../../modules/highgui/src ffopencv.c -L../lib -lavformat64 -lavcodec64 -lavdevice64 -lswscale64 -lavutil64 -lavcore64 -lwsock32