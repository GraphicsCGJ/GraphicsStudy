export PATH=`pwd`:$PATH

build() {
	if [ $# < 1 ]; then
		echo "no input"
	fi

	gcc -o $1 "$1.c"
}

rmbin() {
	rm *.exe
}
