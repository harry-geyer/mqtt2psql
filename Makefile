PACKAGE_NAME := mqtt2psql

CPPFLAGS := -std=c++20 -Wall -Wextra -Iinclude
LIBS := -lpaho-mqttpp3 -lpaho-mqtt3as -lpqxx -lpq

NO_SYSD?=0
ifeq (${NO_SYSD}, 0)
    LIBS += -lsystemd
else
    $(info Building without systemd)
    CPPFLAGS += -DNO_SYSD
endif

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET := $(BIN_DIR)/$(PACKAGE_NAME)

PKG_DIR := pkg
DEB_DIR := $(BUILD_DIR)/pkg
DEBIAN_DIR := $(DEB_DIR)/DEBIAN
BIN_DIR_DEB := $(DEB_DIR)/usr/bin

GIT_TAG := $(shell git describe --tag --abbrev=0)
DEB_VER := $(shell sed -nE 's/Version: *([0-9\.]+)/\1/p' "${PKG_DIR}/DEBIAN/control")
CL_VER := $(shell sed -nE '1s/[^ ].*\(([0-9\.]+)\).*$$/\1/p' "changelog.Debian")
MAN_VER := $(shell sed -nE 's/^\.TH.*"([0-9\.]+)".*$$/\1/p' "mqtt2psql.1")
VERSION := $(GIT_TAG)

CPPFLAGS += -DVERSION_NUMBER='"$(VERSION)"'

define CHECK_VERSION
ifneq ($1,$2)
$$(error GIT_TAG ($1) is not the same as $3 ($2))
endif
endef

$(eval $(call CHECK_VERSION,$(GIT_TAG),$(DEB_VER),DEB_VER))
$(eval $(call CHECK_VERSION,$(GIT_TAG),$(CL_VER),CL_VER))
$(eval $(call CHECK_VERSION,$(GIT_TAG),$(MAN_VER),MAN_VER))

DEB_FILE := $(BUILD_DIR)/$(PACKAGE_NAME)_$(VERSION)_amd64.deb
CHANGELOG:=$(DEB_DIR)/usr/share/doc/$(PACKAGE_NAME)/changelog.gz
MANPAGE:=$(DEB_DIR)/usr/share/man/man1/$(PACKAGE_NAME).1.gz

STATIC_FILES := $(shell find $(PKG_DIR)/ -type f)
BSTATIC_FILES := $(patsubst $(PKG_DIR)/%,$(DEB_DIR)/%,$(STATIC_FILES))

define GZIP_FILE_RULE
$(1): $(2)
	@mkdir -p "$$(@D)"
	gzip --best -n -c $$< > $$@
	chmod 0644 $$@
endef

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(@D)
	$(CXX) $(OBJS) -o $@ $(LIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(INC_DIR)/*.hpp
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) -c $< -o $@

$(BSTATIC_FILES): $(DEB_DIR)/%: $(PKG_DIR)/%
	@mkdir -p $(@D)
	cp $< $@

$(DEB_FILE): $(TARGET) $(BSTATIC_FILES) $(CHANGELOG) $(MANPAGE)
	@mkdir -p $(DEBIAN_DIR) $(BIN_DIR_DEB)
	strip $(TARGET) -o $(BIN_DIR_DEB)/$(PACKAGE_NAME)
	find $(DEB_DIR) -type d -exec chmod 0755 {} \;
	find $(DEB_DIR) -type f -exec chmod -0020 {} \;
	dpkg-deb --root-owner-group --build $(DEB_DIR) $@

$(eval $(call GZIP_FILE_RULE,$(CHANGELOG),changelog.Debian))
$(eval $(call GZIP_FILE_RULE,$(MANPAGE),mqtt2psql.1))

deb: $(DEB_FILE)

deb-lint: $(DEB_FILE)
	lintian $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean deb deb-lint
