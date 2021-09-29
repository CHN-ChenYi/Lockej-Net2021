all:
	cd server; $(MAKE)
	cd client; $(MAKE)

naive:
	cd server; $(MAKE) naive
	cd client; $(MAKE) naive

clean:
	cd server; $(MAKE) clean
	cd client; $(MAKE) clean
