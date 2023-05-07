COPS = --specs=rdimon.specs -lgcc -lc -lm -lrdimon
All:
	arm-none-eabi-gcc $(COPS) -c -mcpu=cortex-m4 -std=gnu11 main.c -o build/main.o
	arm-none-eabi-gcc $(COPS) -c -x assembler-with-cpp startup_stm32f411vetx.s -o build/startup.o
	arm-none-eabi-gcc $(COPS)  build/startup.o build/main.o -mcpu=cortex-m4 -T"STM32F411VETX_FLASH.ld" -Wl,-Map="build/mapfile.map" -Wl,--gc-sections -static -o build/stm32.elf
	arm-none-eabi-objcopy -O ihex build/stm32.elf build/stm32.hex
	
Clear:
	rm build/*