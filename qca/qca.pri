QCA_BASE = $$PWD
INCLUDEPATH = $$QCA_BASE

HEADERS += \
	$$QCA_BASE/qca.h \
	$$QCA_BASE/qcaprovider.h \
	$$QCA_BASE/qca-tls.h \
	$$QCA_BASE/qca-sasl.h

SOURCES += \
	$$QCA_BASE/qca.cpp \
	$$QCA_BASE/qca-tls.cpp \
	$$QCA_BASE/qca-sasl.cpp

