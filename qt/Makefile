RESOURCE_DIR=\"resources/\"       

QT_INCLUDE=-I/usr/lib/qt-2.3.1/include -I/usr/include/qt2  # include files for qt - seem to be two places for it
QT_LIBRARY=-L/usr/lib/qt-2.3.1/lib      # location of the qt library

JXTA_LIB=-L../install/lib               # location of jpr and jxta libary

CPP=g++
MOC=moc
CC=gcc

INCLUDE=${QT_INCLUDE}  -I./include -I../include -I../lib/apr/include -I../src
LIBS=${QT_LIBRARY} -L../lib/apr/lib -L../src/.libs -L../src/jpr/.libs
CFLAGS=-I. -pipe -O2 -march=i386 -mcpu=i686 -fno-exceptions -DRESOURCE_DIR=${RESOURCE_DIR} -pthread 
LDFLAGS=-lqt -ljxta -ljpr -lapr -laprutil -lexpat -lcrypt -lgdbm -ldl -ldb -ldb1

all: MyCJxta 

mc_ConfigDialog.o: include/ConfigDialog.h
	moc include/ConfigDialog.h -o mc_ConfigDialog.cpp
	${CPP} -c ${INCLUDE} ${CFLAGS} -o mc_ConfigDialog.o mc_ConfigDialog.cpp

ConfigDialog.o: src/ConfigDialog.cpp 
	${CPP} -c ${INCLUDE} ${CFLAGS} -o ConfigDialog.o  src/ConfigDialog.cpp

util.o: src/util.cpp
	${CPP} -c ${INCLUDE} ${CFLAGS} -o util.o  src/util.cpp

MyCJxta: ConfigDialog.o util.o src/main.cpp mc_ConfigDialog.o c_wrapper.o
	${CPP}  ${INCLUDE} ${LIBS} ${CFLAGS} ${LDFLAGS}  \
                 src/main.cpp       \
                 ConfigDialog.o     \
                 util.o             \
                 mc_ConfigDialog.o  \
                 c_wrapper.o        \
                 -o MyCJxta 
c_wrapper.o: src/c_wrapper.c
	${CC} ${INCLUDE} ${CFLAGS} -c src/c_wrapper.c -o  c_wrapper.o

clean:
	rm -f *.o
	rm -f mc_*.cpp
