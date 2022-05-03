### USAGE ###

# see programs for PROGRAM options
# to compile: make PROGRAM
# to flash latest compile via USB: make flash
# to flash latest compile via cloud: make flash device=DEVICE
# to start serial monitor: make monitor
# to compile & flash: make PROGRAM flash
# to compile, flash & monitor: make PROGRAM flash monitor

### PARAMS ###

# platform and version
PLATFORM?=photon
VERSION?=2.0.0
device?=
# binary file to flash, if none specified uses the latest that was generated
BIN?=

### PROGRAMS ###

MODULES:=
swiss: MODULES=modules/logger modules/relay modules/scheduler modules/valve

### HELPERS ###

# list available devices
list:
	@echo "\nINFO: querying list of available devices..."
	@particle list

# get mac address
mac:
	@echo "\nINFO: checking for MAC address...."
	@particle serial mac

# start serial monitor
monitor:
	@echo "\nINFO: connecting to serial monitor..."
	@trap "exit" INT; while :; do particle serial monitor; done

# start photon repair doctor
doctor:
	@particle usb dfu
	@echo "\nINFO: starting particle doctor..."
	@echo "WARNING: do NOT reset keys if device is not claimed by you - it may become impossible to access"
	@particle device doctor

### COMPILE & FLASH ###

# compile binary
%:
	@echo "\nINFO: compiling $@ in the cloud for $(PLATFORM) $(VERSION)...."
	@cd src && particle compile $(PLATFORM) $(MODULES) $@ $@/project.properties --target $(VERSION) --saveTo ../$(subst /,_,$@)-$(PLATFORM)-$(VERSION).bin

# flash (via cloud if device is set, via usb if none provided)
# by the default the latest bin, unless BIN otherwise specified
flash:
ifeq ($(device),)
ifeq ($(BIN),)
# flash latest bin by usb
	@$(MAKE) usb_flash BIN=$(shell ls -Art *.bin | tail -n 1)
else
# flash specified bin by usb
	@$(MAKE) usb_flash BIN=$(BIN)
endif
else
ifeq ($(BIN),)
# flash latest bin via cloud
	@$(MAKE) cloud_flash BIN=$(shell ls -Art *.bin | tail -n 1)
else
# flash specified bin via cloud
	@$(MAKE) cloud_flash BIN=$(BIN)
endif
endif

# flash via the cloud
cloud_flash:
ifeq ($(device),)
	@echo "ERROR: no device provided, specify the name(s) to flash via make ... device=???."
else
	@for dev in $(device);  \
		do echo "\nINFO: flashing $(BIN) to '$${dev}' via the cloud..."; \
		particle flash $${dev} $(BIN); \
	done;
endif

# usb flash
usb_flash:
	@echo "INFO: putting device into DFU mode"
	@particle usb dfu
	@echo "INFO: flashing $(BIN) over USB (requires device in DFU mode = yellow blinking)..."
	@particle flash --usb  $(BIN)

# cleaning
clean:
	@echo "INFO: removing all .bin files..."
	@rm -f ./*.bin
