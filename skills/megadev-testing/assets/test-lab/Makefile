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
TOUR?=0
ifeq ($(TOUR),1)
CC_FLAGS+=-DAUDIO_AUTOTEST
endif
MEGADEV_PATH?=/opt/megadev
include $(MEGADEV_PATH)/megadev.make
.DEFAULT_GOAL:=all
.PHONY: all clean assets
all: $(DISC_PATH)/ipx.mmd $(DISC_PATH)/spx.smd $(DISC_PATH)/room.mmd $(PROJECT_ID).iso $(PROJECT_ID).cue
$(DISC_PATH)/ipx.mmd: ipx_layout.s resident_init.s ipx.c replay.c stream.c stream_main.c audio.c audio_main.c font.s bridge.s assets io.macros.s
$(DISC_PATH)/room.mmd: room_layout.s std_init.s room.c $(DISC_PATH)/ipx.mmd
$(DISC_PATH)/spx.smd: spx_layout.s sub_init.s spx.c stream.c stream_sub.c audio.c audio_sub.c audio_bios.s audio_res.s file_info.s assets $(BUILD_PATH)/sp.bin
clean:
	@rm -f $(PROJECT_ID).iso $(PROJECT_ID).cue $(DISC_PATH)/*.mmd $(DISC_PATH)/*.smd $(BUILD_PATH)/*

assets:
	@python3 tools/make_font.py
	@python3 tools/verify_art.py
	@python3 tools/make_audio.py

$(PROJECT_ID).iso: $(DISC_PATH)/garden.mda $(DISC_PATH)/cavern.mda
$(DISC_PATH)/garden.mda: res/garden.mda
	@cp $< $@
$(DISC_PATH)/cavern.mda: res/cavern.mda
	@cp $< $@

$(PROJECT_ID).cue: $(PROJECT_ID).iso assets
	@python3 tools/make_cue.py $(PROJECT_ID).iso
