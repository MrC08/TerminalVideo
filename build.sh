cmake . -DCMAKE_BUILD_TYPE=Debug

if make
then
	./TerminalVideo $*
fi

# Make sure the 'temp' is gone. It should be auto-removed but if the program encounters a segmentation fault it will persist.
rm ./temp