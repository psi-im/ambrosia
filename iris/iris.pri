# libidn
LIBIDN_BASE = $$IRIS_BASE/libidn
CONFIG += libidn
include($$IRIS_BASE/libidn.pri)

DEFINES += XMPP_TEST

INCLUDEPATH += $$IRIS_BASE/include $$IRIS_BASE/xmpp-core #$$IRIS_BASE/xmpp-im $$IRIS_BASE/jabber

HEADERS += \
	$$IRIS_BASE/xmpp-core/hash.h \
	$$IRIS_BASE/xmpp-core/simplesasl.h \
	$$IRIS_BASE/xmpp-core/securestream.h \
	$$IRIS_BASE/xmpp-core/parser.h \
	$$IRIS_BASE/xmpp-core/xmlprotocol.h \
	$$IRIS_BASE/xmpp-core/protocol.h \
	$$IRIS_BASE/xmpp-core/td.h \
	#$$IRIS_BASE/xmpp-im/xmpp_tasks.h \
	#$$IRIS_BASE/xmpp-im/xmpp_xmlcommon.h \
	#$$IRIS_BASE/xmpp-im/xmpp_vcard.h \
	#$$IRIS_BASE/jabber/s5b.h \
	#$$IRIS_BASE/jabber/xmpp_ibb.h \
	#$$IRIS_BASE/jabber/xmpp_jidlink.h \
	#$$IRIS_BASE/jabber/filetransfer.h \
	$$IRIS_BASE/include/xmpp.h \
	#$$IRIS_BASE/include/im.h

SOURCES += \
	$$IRIS_BASE/xmpp-core/connector.cpp \
	$$IRIS_BASE/xmpp-core/tlshandler.cpp \
	$$IRIS_BASE/xmpp-core/jid.cpp \
	$$IRIS_BASE/xmpp-core/hash.cpp \
	$$IRIS_BASE/xmpp-core/simplesasl.cpp \
	$$IRIS_BASE/xmpp-core/securestream.cpp \
	$$IRIS_BASE/xmpp-core/parser.cpp \
	$$IRIS_BASE/xmpp-core/xmlprotocol.cpp \
	$$IRIS_BASE/xmpp-core/protocol.cpp \
	$$IRIS_BASE/xmpp-core/stream.cpp \
	#$$IRIS_BASE/xmpp-im/types.cpp \
	#$$IRIS_BASE/xmpp-im/client.cpp \
	#$$IRIS_BASE/xmpp-im/xmpp_tasks.cpp \
	#$$IRIS_BASE/xmpp-im/xmpp_xmlcommon.cpp \
	#$$IRIS_BASE/xmpp-im/xmpp_vcard.cpp \
	#$$IRIS_BASE/jabber/s5b.cpp \
	#$$IRIS_BASE/jabber/xmpp_ibb.cpp \
	#$$IRIS_BASE/jabber/xmpp_jidlink.cpp \
	#$$IRIS_BASE/jabber/filetransfer.cpp

