all:
	gcc -o lcd lcdST7565.c -lpigpio -lpthread -lrt

clean:
	rm -rf lcd
	
.PHONY: all clean
