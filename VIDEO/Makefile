TARGET=video

CFLAGS=-g -ljpeg
#all:$(TARGET)
all:
	for file in ../VIDEO/a/*.c; \
    	do\
		echo  $$file; \
	echo	./IPDOME_$$(echo  $$file | sed 's/.*\/\(.*\)\..*/\1 /g').a; \
	echo ./a/ip_$$(echo $$file | awk -F'[./]' '{print $$(NF-1)}').a ; \
    	done

video:
	arm-linux-gcc $(CFLAGS) video.c -o video


clean:
	rm $(TARGET) -f
