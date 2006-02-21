cutestuff {
	INCLUDEPATH += \
		$$CS_BASE/util \
		$$CS_BASE/network

	HEADERS += \
		$$CS_BASE/util/base64.h \
		$$CS_BASE/util/bytestream.h \
		$$CS_BASE/util/bconsole.h \
		#$$CS_BASE/util/safedelete.h \
		#$$CS_BASE/network/ndns.h \
		#$$CS_BASE/network/srvresolver.h \
		$$CS_BASE/network/bsocket.h \
		#$$CS_BASE/network/httpconnect.h \
		#$$CS_BASE/network/httppoll.h \
		$$CS_BASE/network/servsock.h \
		#$$CS_BASE/network/socks.h

	SOURCES += \
		$$CS_BASE/util/base64.cpp \
		$$CS_BASE/util/bytestream.cpp \
		$$CS_BASE/util/bconsole.cpp \
		#$$CS_BASE/util/safedelete.cpp \
		#$$CS_BASE/network/ndns.cpp \
		#$$CS_BASE/network/srvresolver.cpp \
		$$CS_BASE/network/bsocket.cpp \
		#$$CS_BASE/network/httpconnect.cpp \
		#$$CS_BASE/network/httppoll.cpp \
		$$CS_BASE/network/servsock.cpp \
		#$$CS_BASE/network/socks.cpp
}

