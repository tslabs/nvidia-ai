
TARGET = poc

CC = /usr/local/cuda-10.2/bin/nvcc

CFLAGS = -c --std=c++14

INC = -I/usr/local/cuda/include
INC += -I/usr/include/gstreamer-1.0 -I/usr/include/orc-0.4 -I/usr/include/gstreamer-1.0
INC += -I/usr/include/glib-2.0 -I/usr/lib/aarch64-linux-gnu/glib-2.0/include
INC += -I/opt/nvidia/deepstream/deepstream-6.0/sources/includes
# INC += -I/usr/local/include/opencv4

LIBS = -lnvinfer -lnvparsers -lnvinfer_plugin -lnvonnxparser -lcudart -lcublas -lcudnn -lrt -ldl -lpthread
LIBS += -lgstvideo-1.0 -lgstaudio-1.0 -lgstapp-1.0 -lgstbase-1.0 -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0
LIBS += -lnvbufsurface -lnvds_meta -lnvdsgst_meta
# LIBS += -lopencv_dnn -lopencv_gapi -lopencv_highgui -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_video -lopencv_calib3d -lopencv_features2d -lopencv_flann -lopencv_videoio -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -lopencv_cudaimgproc -lopencv_cudawarping

LIBINC = -L"/usr/local/lib" -L"/opt/nvidia/deepstream/deepstream-6.0/lib"

# To get includes and libs, run:
#   pkg-config --cflags --libs gstreamer-1.0 gstreamer-video-1.0 gstreamer-audio-1.0 gstreamer-app-1.0 glib-2.0 gobject-2.0
#   pkg-config opencv4 --cflags --libs

all: clean compile link

compile:
	$(CC) $(CFLAGS) -o getopt.o $(INC) getopt.c
	$(CC) $(CFLAGS) -o main.o $(INC) main.cpp
	$(CC) $(CFLAGS) -o gst.o $(INC) gst.cpp

link:
	$(CC) -o $(TARGET) $(LIBINC) main.o getopt.o gst.o $(LIBS)
	rm -rf *.o

clean:
	rm -rf $(TARGET)
