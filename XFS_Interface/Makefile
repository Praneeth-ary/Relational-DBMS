default: xfs-interface

xfs-interface: *.cpp *.h define/*
	g++ *.cpp -o xfs-interface -Wno-write-strings -Wno-return-type -lreadline

clean:
	$(RM) xfs-interface *.o