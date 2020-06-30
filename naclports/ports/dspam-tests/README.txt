How to set up the tests:

$ ./configure DSPAM_SRC=(root/dspam_root/src) [DATA_DIR=optional location]
$ make test

To run a specific test:

$ make testX 	# test number X

How to build the stand-alone application:

$ make app
$ ./dspam-app ...

Run
$ ./dspam-app --help
for more information about running the standalone application.