TARGETS = TRS/TRS TCS/TCS User/user

all: $(TARGETS)
	 cp $(TARGETS) .

TRS/TRS:
	(cd TRS && make)

TCS/TCS:
	(cd TCS && make)

User/user:
	(cd User && make)

clean:
	(cd TRS && make clean)
	(cd TCS && make clean)
	(cd User && make clean)
