# Common Makefile for prototype linux build of pmic fw components

# Require a build directory that should ideally be outside the repo
ifndef BUILD_DIR
$(error BUILD_DIR environment variable must be defined)
endif

# Require the microchip 6.3.1 tools install directory, which should look something like
# /path/to/arm-none-eabi
#ifndef ARM_TOOLS_DIR
#$(error ARM_TOOLS_DIR environment variable for the 6.3.1-508 release must be defined)
#endif

# If the build type is not specified, use default
ifndef BUILD_TYPE
BUILD_TYPE = Debug
else
ifneq ($(BUILD_TYPE), Debug)
ifneq ($(BUILD_TYPE), Release)
$(error BUILD_TYPE must be either Debug or Release)
endif
endif
endif

.PHONY: all clean

# settings for Makefiles in APP and bootloader (not needed for top-level Makefile)
ifneq ($(TARGET),)
.PHONY: $(TARGET) $(TARGET_BUILD_DIR) clean-$(TARGET)
TARGET_BUILD_DIR = $(BUILD_DIR)/$(TARGET)

ARM_GCC = arm-none-eabi-gcc
ARM_SIZE = arm-none-eabi-size
ARM_OBJCOPY = arm-none-eabi-objcopy
ARM_OBJDUMP = arm-none-eabi-objdump
#ARM_LIB_DIR = $(ARM_TOOLS_DIR)/lib/
ARM_LIBGCC = $(shell $(ARM_GCC) --print-file-name=libgcc.a)
ARM_LIBC = $(shell $(ARM_GCC) --print-file-name=libc.a)
ARM_LIBNOSYS = $(shell $(ARM_GCC) --print-file-name=libnosys.a)
ARM_LIBS = $(ARM_LIBGCC) $(ARM_LIBC) $(ARM_LIBNOSYS)

ifeq ($(BUILD_TYPE), Debug)
CFLAGS += -Os -g
else
CFLAGS += -Os
endif

CFLAGS += -x c -c -mthumb -D__SAMD20J18__ -DDEBUG -DARM_MATH_CM0PLUS=true -Wall -Wundef \
          -mlong-calls -mcpu=cortex-m0plus -std=gnu99 -fno-strict-aliasing  -Wstrict-prototypes \
          -Wmissing-prototypes -Wpointer-arith -Wfloat-equal -Wshadow -Wbad-function-cast \
          -Wwrite-strings -Wsign-compare -Waggregate-return -Wmissing-declarations \
          -Wmissing-format-attribute -Wno-deprecated-declarations -Wnested-externs -Wno-long-long \
          -Wunreachable-code -Wcast-align -MD -MP -MF "$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)"

OBJS = $(addprefix $(TARGET_BUILD_DIR)/, $(addsuffix .o, $(basename $(notdir $(SRCS)))))
DEPS = $(addsuffix .d, $(basename $(OBJS)))

LDFLAGS += -mthumb -Wl,-Map=$(MAP_FILE) -Wl,--start-group -lm  -Wl,--end-group -Wl,--gc-sections \
           -mcpu=cortex-m0plus --specs=nano.specs -Wl,--gc-sections

ELF_FILE = $(TARGET_BUILD_DIR)/$(TARGET).elf
BIN_FILE = $(TARGET_BUILD_DIR)/$(TARGET).bin
HEX_FILE = $(TARGET_BUILD_DIR)/$(TARGET).hex
EEP_FILE = $(TARGET_BUILD_DIR)/$(TARGET).eep
MAP_FILE = $(TARGET_BUILD_DIR)/$(TARGET).map
LSS_FILE = $(TARGET_BUILD_DIR)/$(TARGET).lss
SREC_FILE = $(TARGET_BUILD_DIR)/$(TARGET).srec
TARGET_FILES = $(ELF_FILE) $(BIN_FILE) $(HEX_FILE) $(LSS_FILE) $(EEP_FILE) \
               $(MAP_FILE) $(LSS_FILE) $(SREC_FILE)

$(TARGET): $(TARGET_FILES)
	$(ARM_SIZE) $(ELF_FILE)

$(ELF_FILE): $(OBJS) $(LINKER_SCRIPT) $(ARM_LIBS) $(LINKER_SCRIPT_DIR)
	@$(ARM_GCC) -o $@ $(OBJS) $(LDFLAGS) -T $(LINKER_SCRIPT) -L $(LINKER_SCRIPT_DIR) $(ARM_LIBS)

$(BIN_FILE): $(ELF_FILE)
	@$(ARM_OBJCOPY) -O binary $< $@

$(HEX_FILE): $(ELF_FILE)
	@$(ARM_OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature $< $@

$(EEP_FILE): $(ELF_FILE)
	@$(ARM_OBJCOPY) -j .eeprom --set-section-flags=.eeprom=alloc,load --change-section-lma .eeprom=0 --no-change-warnings -O binary $< $@

$(LSS_FILE): $(ELF_FILE)
	@$(ARM_OBJDUMP) -h -S $< > $@

$(SREC_FILE): $(ELF_FILE)
	@$(ARM_OBJCOPY) -O srec -R .eeprom -R .fuse -R .lock -R .signature $< $@

$(TARGET_BUILD_DIR)/%.o : %.c
	@$(ARM_GCC) $(CFLAGS) -o $@ $<

$(TARGET_BUILD_DIR):
	mkdir -p $(TARGET_BUILD_DIR)

$(OBJS): | $(TARGET_BUILD_DIR)

ifneq ($(MAKECMDGOALS), clean)
ifneq ($(strip $(DEPS)),)
-include $(DEPS)
endif
endif

clean clean-$(TARGET):
	rm -fr $(OBJS) $(DEPS) $(TARGET_FILES)
endif
