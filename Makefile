CFLAGS ?= -O2 -std=gnu17 -Wall -Wextra
OUT_O_DIR = build

COMMONINC = -I include

override CFLAGS += $(COMMONINC)

NODEPS = clean

CSRC = $(addprefix src/, parser.c pollfd_set.c proxy.c utils.c panic.c)
COBJ := $(addprefix $(OUT_O_DIR)/, $(CSRC:.c=.o))
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

.PHONY: debug
debug: clean all
	@gdb -silent $(OUT_O_DIR)/proxy

.PHONY: clean
clean:
	rm -rf $(OUT_O_DIR) *.x
