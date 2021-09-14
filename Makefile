all:
	cd server; $(MAKE)
	cd client; $(MAKE)

clean:
	cd server; $(MAKE) clean
	cd client; $(MAKE) clean
