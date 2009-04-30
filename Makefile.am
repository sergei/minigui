SUBDIRS = src ext vcongui include etc m4

EXTRA_DIST = \
            Version README NEWS \
            LICENSE.POLICY LICENSE.POLICY.zh   

# The directory where the include files will be installed
libminiguiincludedir = $(includedir)/minigui

# Which header files to install
libminiguiinclude_HEADERS = mgconfig.h

docs:
	doxygen Doxyfile

