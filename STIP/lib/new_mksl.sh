# it's worthy noting that opencv3 is compatible with opencv2
# after installing opencv 2.4.x
sudo ln -s  usr/local/lib/libopencv_core.so /usr/lib/libcxcore.so.2
sudo ln -s  usr/local/lib/libopencv_imgproc.so /usr/lib/libcv.so.2
# sudo ln -s  usr/local/lib/libopencv_legacy.so /usr/lib/libcv.so.2
sudo ln -s  usr/local/lib/libopencv_highgui.so /usr/lib/libhighgui.so.2
sudo ln -s  usr/local/lib/libopencv_ml.so /usr/lib/libml.so.2
sudo ln -s  usr/local/lib/libopencv_video.so /usr/lib/libcvaux.so.2

export LD_LIBRARY_PATH=/usr/lib
sudo ldconfig
