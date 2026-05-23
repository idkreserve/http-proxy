CFLAGS ?= -O2 -std=gnu17 -Wall -Wextra -Werror
OUT_O_DIR = build

COMMONINC = $(addprefix -I src/, include http/include net/include utils/include)

override CFLAGS += $(COMMONINC)

NODEPS = clean

CSRC_HTTP = $(addprefix http/, \
	http.c)
CSRC_NET = $(addprefix net/, \
	net.c \
	pipeline.c)
CSRC_UTILS = $(addprefix utils/, \
	panic.c \
	utils.c)

CSRC = $(addprefix src/, proxy.c $(CSRC_HTTP) $(CSRC_NET) $(CSRC_UTILS))
COBJ = $(addprefix $(OUT_O_DIR)/, $(CSRC:.c=.o))
DEPS = $(COBJ:.o=.d)

.PHONY: all
all: $(OUT_O_DIR)/proxy

$(OUT_O_DIR)/proxy : $(COBJ)
	$(CC) $^ -o $@ $(LDFLAGS)

$(COBJ) : $(OUT_O_DIR)/%.o : %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(DEPS) : $(OUT_O_DIR)/%.d : %.c
	mkdir -p $(@D)
	$(CC) -E $(CFLAGS) $< -MM -MT $(@:.d=.o) > $@

ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
include $(DEPS)
endif

.PHONY: run
run: all
	@echo server started
	@$(OUT_O_DIR)/proxy

.PHONY: clean
clean:
	rm -rf $(OUT_O_DIR) *.x