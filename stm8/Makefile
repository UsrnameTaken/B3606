SRC=main.c display.c uart.c eeprom.c outputs.c config.c fixedpoint.c parse.c adc.c
CFLAGS= -lstm8 -mstm8 --opt-code-size --std-c99
OBJ=$(SRC:.c=.rel)
DEP=$(SRC:%.c=.%.c.d)
SDCC=sdcc $(CFLAGS)

all: clean b3606.ihx check_size

-include $(DEP)

check_size: b3606.ihx
		@CODESIZE=$$(grep '^CODE' b3606.map |head -n1 | sed -e 's/^.*=\s*\([0-9]\+\).*$$/\1/'); \
			 if [ "$$CODESIZE" -gt 8192 ]; then echo "Code is too large, it is $$CODESIZE bytes"; exit 1; \
			 else echo "Code fits the flash, it is $$CODESIZE bytes"; \
			 fi

b3606.ihx: $(OBJ)
	@echo LINK $@
	@$(SDCC) --out-fmt-ihx --code-size 8192 -o $@ $^

%.rel: %.c
	@echo CC $<
	@$(SDCC) -M $< > .$<.tmp.d
	@mv .$<.tmp.d .$<.d
	@$(SDCC) -c -o $@ $<

test_pwm_accuracy: test_pwm_accuracy.c outputs.c config.c fixedpoint.c
	gcc -g -Wall -o $@ $< -DTEST=1

test_adc_accuracy: test_adc_accuracy.c config.c adc.c fixedpoint.c
	gcc -g -Wall -o $@ $< -DTEST=1

test_parse: test_parse.c parse.c
	gcc -g -Wall -o $@ $< -DTEST=1

deploy: b3606.ihx
	stm8flash -c stlinkv2 -p stm8s003f3 -s flash -w b3606.ihx

clean:
	-rm -f *.rel *.ihx *.lk *.map *.rst *.lst *.asm *.sym .*.d

.PHONY: all clean check_size deploy
