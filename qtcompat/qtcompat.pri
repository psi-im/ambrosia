QTCOMPAT_BASE = $$PWD
INCLUDEPATH += $$QTCOMPAT_BASE

HEADERS += \
	$$QTCOMPAT_BASE/q3ptrcollection.h \
	$$QTCOMPAT_BASE/q3gvector.h \
	$$QTCOMPAT_BASE/q3glist.h \
	$$QTCOMPAT_BASE/q3ptrlist.h \
	$$QTCOMPAT_BASE/q3gdict.h \
	$$QTCOMPAT_BASE/q3dict.h \
	$$QTCOMPAT_BASE/q3valuelist.h \
	$$QTCOMPAT_BASE/q3ptrvector.h \
	$$QTCOMPAT_BASE/q3strlist.h \
	$$QTCOMPAT_BASE/q3ptrdict.h \
	$$QTCOMPAT_BASE/q3socketdevice.h \
	$$QTCOMPAT_BASE/q3serversocket.h \
	$$QTCOMPAT_BASE/q3dns.h

SOURCES += \
	$$QTCOMPAT_BASE/q3ptrcollection.cpp \
	$$QTCOMPAT_BASE/q3gvector.cpp \
	$$QTCOMPAT_BASE/q3glist.cpp \
	$$QTCOMPAT_BASE/q3gdict.cpp \
	$$QTCOMPAT_BASE/q3socketdevice.cpp \
	$$QTCOMPAT_BASE/q3socketdevice_unix.cpp \
	$$QTCOMPAT_BASE/q3serversocket.cpp \
	$$QTCOMPAT_BASE/q3dns.cpp

