TARGETS = TRS_src/TRS TCS_src/TCS user_src/user

test:
	make clean
	make all

all: $(TARGETS)
	 cp $(TARGETS) .

TRS_src/TRS:
	@(cd TRS_src && make)

TCS_src/TCS:
	@(cd TCS_src && make)

user_src/user:
	@(cd user_src && make)

clean:
	@(cd TRS_src && make clean)
	@(cd TCS_src && make clean)
	@(cd user_src && make clean)
	rm -f TRS user TCS
