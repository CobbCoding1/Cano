
if [ -d ./build ]; then	
	cp build/cano /usr/local/bin/
else
	echo "Please build Cano before installing."
fi

