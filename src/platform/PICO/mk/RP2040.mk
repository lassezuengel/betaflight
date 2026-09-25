#
# Raspberry Pi RP2040 platform support.
#
# The RP2040 and RP2350 ports share the RP2 peripheral implementation.  The
# included makefile selects the CPU-, math- and linker-specific pieces from
# TARGET_MCU.
#

include $(TARGET_PLATFORM_DIR)/mk/RP2350.mk
