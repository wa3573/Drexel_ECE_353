
all:
	gcc -o producer producer.c -std=c99 -Wall -lm -lpthread -lrt
	gcc -o consumer consumer.c -std=c99 -Wall -lm -lpthread -lrt 
	
clean:
	rm producer
	rm consumer
	
.PHONY: all clean