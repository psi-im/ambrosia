TEMPLATE = app
CONFIG  += debug
QT      -= gui
QT      += network xml
TARGET   = ambrosia

MOC_DIR     = .moc
OBJECTS_DIR = .obj

include(qca/qca.pri)
include(qtcompat/qtcompat.pri)

CONFIG += cutestuff
CS_BASE = cutestuff
include(cs.pri)

CONFIG += iris
IRIS_BASE = iris
include(iris/iris.pri)

HEADERS += \
	src/router.h

SOURCES += \
	src/router.cpp \
	src/main.cpp

include(conf.pri)

