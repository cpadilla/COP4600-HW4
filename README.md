Module:

to "Build": 'make'

to "Install": 'sudo insmod FIFOWrite.ko'
			  'sudo insmod FIFORead.ko'

Test at your leasure. To use our example test case:
	
	gcc test -o testChar.c
	sudo ./tets

You will see the test program write characters to the write device, and then read them back from the read device. 

to "Remove":  'sudo rmmod FIFOWrite'
			  'sudo rmmod FIFORead'

to "View":  'dmesg'

to "View Log": 'tail -f /var/log/kern.og'
