# Makefile for prototype linux build of pmic fw runtime and bootloader

# Default build directory
BUILD_DIR ?= build
IMAGE_DIR ?= $(BUILD_DIR)/pmic-fw

# common settings for all pmic firmware linux Makefiles
include Makefile.inc

ifneq ("$(filter /%, $(BUILD_DIR))", "")
ABSOLUTE_BUILD_DIR = $(BUILD_DIR)
else
ABSOLUTE_BUILD_DIR = $(CURDIR)/$(BUILD_DIR)
endif

# Check for FW image slot
# Slot 0 and 1 are used for flash image or update image. If standalone image (without bootloader) is required, use Slot = 2.
ifndef SLOT
SLOT = 2
else
ifneq ($(SLOT), 0)
ifneq ($(SLOT), 1)
ifneq ($(SLOT), 2)
$(error SLOT must be either 0 or 1, or 2 for standalone runtime)
endif
endif
endif
endif

# Flash image size limit
# Runtime images size
BOOTLOADER_IMG_SIZE = 8192  # 8K for bootloader
RT_IMG_SIZE = 124928  # 122K for Runtime Image (excluding bootloader)
IMAGE_METADATA_ADDR_OFFSET = 256  # Image metadata header is placed at relative address offset 0x100
# Calculate the flash image size and addresses by including slot image type
ifeq ($(SLOT), 0)
	FLASH_IMG_SIZE = $(shell expr $(RT_IMG_SIZE))
else ifeq  ($(SLOT), 1)
	FLASH_IMG_SIZE = $(shell expr $(RT_IMG_SIZE) + $(RT_IMG_SIZE))
else ifeq  ($(SLOT), 2)
	FLASH_IMG_SIZE = $(shell expr $(RT_IMG_SIZE))
endif


# Function to perform check on image size
CHECK_SIZE = @(FILESIZE=`stat -c'%s' $1` ; \
	if [ $2 -gt 0 ]; then \
		if [ $$FILESIZE -gt $2 ] ; then \
			echo "ERROR: File $1 exceeds flash size limit ($$FILESIZE > $2)" ; \
			exit 1 ; \
		fi ; \
	fi )

TO_LOWER = $(shell echo $1 | tr A-Z a-z)

all: runtime bootloader full-pmic-flash-image

clean: clean-runtime clean-bootloader

clean-build:
	rm -rf $(ABSOLUTE_BUILD_DIR)
	@echo "$(BUILD_DIR) has been cleaned!"

.PHONY: runtime bootloader clean-runtime clean-bootloader clean-build full-pmic-flash-image pmic-slot-image

runtime clean-runtime:
	$(MAKE) -C runtime BUILD_DIR=$(ABSOLUTE_BUILD_DIR) SLOT_IMAGE=$(SLOT) $@
	$(call CHECK_SIZE, $(BUILD_DIR)/runtime/runtime.bin, $(FLASH_IMG_SIZE))

bootloader clean-bootloader:
	$(MAKE) -C bootloader BUILD_DIR=$(ABSOLUTE_BUILD_DIR) $@
	$(call CHECK_SIZE, $(BUILD_DIR)/bootloader/bootloader.bin, $(BOOTLOADER_IMG_SIZE))

full-pmic-flash-image: runtime bootloader
	@mkdir -p $(IMAGE_DIR)
	python3 scripts/genFlashImage.py -b $(BUILD_DIR)/bootloader/bootloader.bin -s$(SLOT) $(BUILD_DIR)/runtime/runtime.bin -m $(IMAGE_METADATA_ADDR_OFFSET) -o $(IMAGE_DIR)/$(if $(SLOT:2=),pmic_flash_image_,pmic_standalone_image_)$(call TO_LOWER, $(BUILD_TYPE)).bin

pmic-slot-image: runtime
	@mkdir -p $(IMAGE_DIR)
	python3 scripts/genUpdateImage.py -s$(SLOT) $(BUILD_DIR)/runtime/runtime.bin -m $(IMAGE_METADATA_ADDR_OFFSET) -o $(IMAGE_DIR)/pmic_slot$(SLOT)_image_$(call TO_LOWER, $(BUILD_TYPE)).bin
