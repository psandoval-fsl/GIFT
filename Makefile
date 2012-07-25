
#----------------- development environment here
#important so we know where the libraries are
LTIB_ROOTFS=/home/fsl/12.04/ltib/rootfs

#set CROSS_COMPILE as below
CROSS_COMPILE = /opt/freescale/usr/local/gcc-4.6.2-glibc-2.13-linaro-multilib-2011.12/fsl-linaro-toolchain/bin/arm-fsl-linux-gnueabi-

#extra headers
EXTRA_INC =  $(LTIB_ROOTFS)/usr/include

#system headers
FSL_PLATFORM_INC = $(LTIB_ROOTFS)/usr/include

#extra libraries
EXTRA_LIB = 

#system libraries
FSL_PLATFORM_LIB = $(LTIB_ROOTFS)/usr/lib

#random needed vars
ARCH = arm
CD = cd
DEL_FILE = rm -f
MKDIR = mkdir -p
RMDIR = rmdir
CC = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)g++
CP = cp
MAKE = make

CFLAGS = -DLINUX -DEGL_API_FB -mfloat-abi=softfp -mfpu=vfp -fPIC -O3 -fno-strict-aliasing -fno-optimize-sibling-calls -g -O2 -mcpu=arm1136jf-s -mapcs-frame -fPIC -Wall -W -Wno-unused-parameter

CFLAGS += -I$(FSL_PLATFORM_INC)

LFLAGS = -Wl,--library-path=$(FSL_PLATFORM_LIB),-rpath-link=$(FSL_PLATFORM_LIB) -ldl

EGL_FLAGS = -lEGL

ES11_FLAGS = -lGLESv1_CM

ES20_FLAGS = -lGLESv2 

VG11_FLAGS = -lOpenVG

ASSIMP_FLAGS = -lassimp

DEVIL_FLAGS = -lIL

#-----------------for each app here

APPNAME			= GIFT
DESTDIR			= ./bin
SRCDIR			= .

#LFLAGS                  += $(EGL_FLAGS) $(ES20_FLAGS) -lm
LFLAGS			+= $(EGL_FLAGS) $(ES20_FLAGS) $(ASSIMP_FLAGS) $(DEVIL_FLAGS) -lm

OBJECTS			= GIFT.o obj3d.o fslutil.o TouchScreen.o


first: all

all: $(APPNAME)

.PHONY: all

$(APPNAME): $(OBJECTS) 
	-@$(MKDIR) $(DESTDIR)
	$(LD) $(LFLAGS) -o $(DESTDIR)/$(APPNAME) $(OBJECTS)

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(DEL_FILE) $(OBJECTS) $(UTILOBJS)
	$(DEL_FILE) *~ core *.core

distclean: clean
	$(DEL_FILE) $(DESTDIR)/$(APPNAME)
	-@$(RMDIR) $(DESTDIR)

