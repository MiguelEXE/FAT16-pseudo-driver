all: compile floppy
floppy:
	dd if=/dev/zero of=disk.bin bs=512 count=65524
	mkfs.fat -F16 -n "TEST" disk.bin
	mcopy -i disk.bin "disk test/a.txt" "::/"
	mcopy -i disk.bin "disk test/test.txt" "::/"
	mcopy -i disk.bin "disk test/fold" "::/"
	mcopy -i disk.bin "disk test/empty.txt" "::/"
	mcopy -i disk.bin "disk test/test_ex" "::/"
compile:
	gcc test.c -o main2
	gcc test2.c -o main
	gcc test3.c -o main3
	gcc test4.c -o main4