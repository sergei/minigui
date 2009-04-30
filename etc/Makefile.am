# The directory where the data files will be installed

install-data-local:
	if [ ! -d $(prefix)/etc ]; then $(INSTALL) -d $(prefix)/etc; fi

if FLAT_WINDOW_STYLE
	$(INSTALL_DATA) MiniGUI-flat.cfg $(prefix)/etc/MiniGUI.cfg
else
if PHONE_WINDOW_STYLE
	$(INSTALL_DATA) MiniGUI-fashion.cfg $(prefix)/etc/MiniGUI.cfg
else
	$(INSTALL_DATA) MiniGUI-classic.cfg $(prefix)/etc/MiniGUI.cfg
endif
endif

EXTRA_DIST = MiniGUI-flat.cfg MiniGUI-classic.cfg MiniGUI-fashion.cfg MiniGUI-min.cfg

