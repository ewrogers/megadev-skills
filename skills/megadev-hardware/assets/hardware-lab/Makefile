PROJECT_ID:=@PROJECT_ID@
TARGET:=MEGACD
PROJECT_NAME:="@PROJECT_NAME@"
HEADER_SOFTWARE_ID="MEGADEV LAB"
REGION?=US
VIDEO?=NTSC
SRC_PATH:=src
RES_PATH:=res
BUILD_PATH:=build
DISC_PATH:=disc
OPT_FLAGS?=-O2 -fconserve-stack -fomit-frame-pointer
CC_FLAGS?=$(OPT_FLAGS)
USB_LAB?=1
ifeq ($(USB_LAB),1)
CC_FLAGS+=-DHW_USB_LAB
endif
TOUR?=0
ifeq ($(TOUR),1)
CC_FLAGS+=-DHW_AUTOTEST
endif
MEGADEV_PATH?=/opt/megadev
include $(MEGADEV_PATH)/megadev.make
.DEFAULT_GOAL:=all
.PHONY: all clean art
art:
	python3 tools/make_art.py $(REGION) $(VIDEO) $(CC_FLAGS)
$(BUILD_PATH)/art.h: art
$(DISC_PATH)/ipx.mmd: art
$(BUILD_PATH)/lab_video.c.o: $(SRC_PATH)/lab_video.c $(BUILD_PATH)/art.h
	$(CC) $(CC_FLAGS) $(INC) -c $(SRC_PATH)/lab_video.c -o $@
all: $(DISC_PATH)/ipx.mmd $(DISC_PATH)/spx.smd $(DISC_PATH)/room.mmd $(PROJECT_ID).iso
$(DISC_PATH)/ipx.mmd: ipx_layout.s resident_init.s ipx.c model.c probe.c lab_video.c everdrive_link.c usb_input.c bridge.s io.macros.s
$(DISC_PATH)/room.mmd: room_layout.s std_init.s room.c $(DISC_PATH)/ipx.mmd
$(DISC_PATH)/spx.smd: spx_layout.s sub_init.s spx.c probe.c $(BUILD_PATH)/sp.bin
clean:
	@rm -f $(PROJECT_ID).iso $(PROJECT_ID).cue $(DISC_PATH)/*.mmd $(DISC_PATH)/*.smd $(BUILD_PATH)/*
