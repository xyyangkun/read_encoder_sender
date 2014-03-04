CROSS=
#arm-hisiv200-linux-
#arm-linux-
CC=$(CROSS)gcc
CXX=$(CROSS)g++
LINK=$(CROSS)g++ -o
LIBRARY_LINK=$(CROSS)ar cr

OUT=./OUT
SRCDIR=./src
LIBAVDIR=/home/xy/mywork/av/libav-2014-03-02
LIBX264DIR=/home/xy/mywork/av/x264-snapshot-20140218-2245

#头文件
INCLUDE= -I ./include -I$(LIBAVDIR)


#库文件
LIBDIR= -L./lib 
LIBDIR= -L$(LIBX264DIR)
LIBDIR+=-L$(LIBAVDIR)/libavdevice -L$(LIBAVDIR)/libavcodec \
		-L$(LIBAVDIR)/libavformat -L$(LIBAVDIR)/libswscale \
		-L$(LIBAVDIR)/libavfilter -L$(LIBAVDIR)/libavutil
LIBS = 
LIBS+=  -lavdevice  -lavformat  -lavcodec -lswscale  -lavfilter -lavutil -lX11 -lasound -lx264 -lm -lz -pthread -ldl
LIBS+= 
LDLIBS=$(LIBDIR) $(LIBS)

#编译选项
CXXFLAGS=-g
CXXFLAGS+= $(INCLUDE) $(LDLIBS) 
CFLAGS=-g
CFLAGS+=   $(INCLUDE) $(LDLIBS)

#目标
all: test

$(OUT)/%.o:$(SRCDIR)/%.cpp
	$(CXX) -c $< -o $@ $(CXXFLAGS)
$(OUT)/%.o:$(SRCDIR)/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

CPPOBJS = 
CPPOBJS=$(OUT)/v4l2uvc.o
$(SRCDIR)/v4l2uvc.c:$(SRCDIR)/v4l2uvc.h



test:$(OUT)/main.o $(CPPOBJS)
	$(LINK) $@ $< $(CPPOBJS) $(CXXFLAGS) 	
$(OUT)/main.o: $(SRCDIR)/main.cpp
	$(CXX) -c $< -o $@ $(CFLAGS)



clean:
	rm $(OUT)/*
	

