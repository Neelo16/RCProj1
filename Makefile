TARGETS = TRS_src/TRS TCS_src/TCS user-src/user

all: $(TARGETS)
	 cp $(TARGETS) .

TRS_src/TRS:
	(cd TRS_src && make)

TCS_src/TCS:
	(cd TCS_src && make)

user-src/user:
	(cd user-src && make)

clean:
	(cd TRS_src && make clean)
	(cd TCS_src && make clean)
	(cd user-src && make clean)
	rm -f TRS user TCS
