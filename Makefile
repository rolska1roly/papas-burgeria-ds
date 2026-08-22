include $(DEVKITARM)/ds_rules

TARGET   := papas-burgeria
SOURCES  := source
INCLUDES := include

ARCH     := -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -O2 -march=armv4t -mtune=arm946e-s -fomit-frame-pointer -ffast-math $(ARCH)
LIBS     := -lnds9

INCLUDE  := -I$(CURDIR)/$(SOURCES) -I$(LIBNDS)/include
CFILES   := $(wildcard $(SOURCES)/*.c)
OFILES   := $(CFILES:.c=.o)

.PHONY: all clean

all: $(TARGET).nds

$(TARGET).nds: $(TARGET).elf
	ndstool -c $@ -9 $<

$(TARGET).elf: $(OFILES)
	$(CC) $(ARCH) -specs=ds_arm9.specs $(OFILES) -L$(LIBNDS)/lib $(LIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(SOURCES)/*.o $(TARGET).elf $(TARGET).nds
