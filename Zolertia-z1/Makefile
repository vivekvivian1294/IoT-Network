ifndef TARGET
TARGET=z1
endif

CONTIKI_PROJECT = client basestation login
CONTIKI_SOURCEFILES += cc2420-arch.c
PROJECT_SOURCEFILES = i2cmaster.c tmp102.c adxl345.c

all: $(CONTIKI_PROJECT)

CONTIKI = /home/user/contiki-2.7
include $(CONTIKI)/Makefile.include
