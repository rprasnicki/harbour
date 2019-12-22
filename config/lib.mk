include $(TOP)$(ROOT)config/global.mk


HB_DYN_LIBS := \
   hbcommon \
   hbpp \
   hbrtl \
   hbmacro \
   hblang \
   hbcpage \
   hbextern \
   hbrdd \
   rddntx \
   rddnsx \
   rddcdx \
   rddfpt \
   hbsix \
   hbhsx \
   hbusrrdd \
   gtstd \
   gtwvt \
   gtwin \
   gtcgi \
   gttrm \
   gtxwc \
   hbvm \
   hbvmmt \
   hbmaindllh

ifneq ($(HB_HAS_PCRE2_LOCAL),)
   HB_DYN_LIBS += hbpcre2
endif
#ifneq ($(HB_HAS_ZLIB),)
#   HB_DYN_LIBS += zlib
#endif
ifneq ($(HB_HAS_XLSWRITER_LOCAL),)
   HB_DYN_LIBS += xlswriter
endif


# Added only for hbpp
-include $(TOP)$(ROOT)config/$(HB_PLATFORM)/libs.mk

ifneq ($(__HB_BUILD_NOSYSLIB),)
   SYSLIBS := $(filter-out $(__HB_BUILD_NOSYSLIB),$(SYSLIBS))
endif

include $(TOP)$(ROOT)config/$(HB_PLATFORM)/$(HB_COMPILER).mk
include $(TOP)$(ROOT)config/c.mk
include $(TOP)$(ROOT)config/prg.mk

LIB_NAME := $(LIB_PREF)$(LIBNAME)$(LIB_EXT)

LIB_FILE := $(LIB_DIR)/$(LIB_NAME)

ALL_OBJS := $(ALL_C_OBJS) $(ALL_PRG_OBJS)

first:: dirbase descend

descend:: dirbase
	+@$(MK) $(MKFLAGS) -C $(OBJ_DIR) -f $(GRANDP)Makefile TOP=$(GRANDP) $(LIB_NAME)

vpath $(LIB_NAME) $(LIB_DIR)
$(LIB_NAME) : $(ALL_OBJS)
	$(AR_RULE)

INSTALL_FILES := $(LIB_FILE)
INSTALL_DIR := $(HB_INSTALL_LIB)

include $(TOP)$(ROOT)config/instsh.mk
INSTALL_RULE_LIBS := $(INSTALL_RULE)

ifneq ($(INSTALL_RULE_LIBS),)

install:: first
	$(INSTALL_RULE_LIBS)

endif
