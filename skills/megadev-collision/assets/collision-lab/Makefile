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
CC_FLAGS+=-DCOLLISION_AUTOTEST
endif
MEGADEV_PATH?=/opt/megadev
include $(MEGADEV_PATH)/megadev.make
.DEFAULT_GOAL:=all
.PHONY: all clean assets
all: $(DISC_PATH)/ipx.mmd $(DISC_PATH)/spx.smd $(DISC_PATH)/room.mmd $(PROJECT_ID).iso
$(DISC_PATH)/ipx.mmd: ipx_layout.s resident_init.s ipx.c sound.c collision.c demo.c probe.c gfx.c font.s art.s bridge.s assets io.macros.s
$(DISC_PATH)/room.mmd: room_layout.s std_init.s room.c $(DISC_PATH)/ipx.mmd
$(DISC_PATH)/spx.smd: spx_layout.s sub_init.s spx.c $(BUILD_PATH)/sp.bin
clean:
	@rm -f $(PROJECT_ID).iso $(PROJECT_ID).cue $(DISC_PATH)/*.mmd $(DISC_PATH)/*.smd $(BUILD_PATH)/*

assets:
	@python3 tools/make_font.py
	@python3 tools/make_art.py

# Local objects must track headers and assembly-incbin resources. The SDK's
# generic rules only track the source; compile $<, never all prerequisites ($^).
LOCAL_C := $(notdir $(wildcard $(SRC_PATH)/*.c))
LOCAL_S := $(notdir $(wildcard $(SRC_PATH)/*.s))
$(addprefix $(BUILD_PATH)/,$(addsuffix .o,$(LOCAL_C))): $(BUILD_PATH)/%.c.o: $(SRC_PATH)/%.c $(wildcard $(SRC_PATH)/*.h) $(wildcard $(BUILD_PATH)/*assets.h)
	@$(CC) $(CC_FLAGS) $(INC) -c $< -o $@
$(addprefix $(BUILD_PATH)/,$(addsuffix .o,$(LOCAL_S))): $(BUILD_PATH)/%.s.o: $(SRC_PATH)/%.s $(wildcard $(SRC_PATH)/*.h) $(wildcard $(RES_PATH)/*) $(wildcard $(BUILD_PATH)/*assets.h)
	@$(CC) $(CC_FLAGS) $(AS_FLAGS) $(INC) $(AS_INC) -x assembler-with-cpp -c $< -o $@
