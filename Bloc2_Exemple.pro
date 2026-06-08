

TEMPLATE    = app
QT         += opengl


INCLUDEPATH += ./glm ./Model

FORMS += MyForm.ui

HEADERS += MyForm.h BL2GLWidget.h MyGLWidget.h

SOURCES += main.cpp MyForm.cpp  BL2GLWidget.cpp  MyGLWidget.cpp Model/model.cpp

DISTFILES += \
    shaders/basicShader.frag \
    shaders/basicShader.vert


CONFIG += c++11

# =====================================
# Assimp library and helper classes
#
HEADERS += ./assimp/Mesh.h
SOURCES +=  ./assimp/Mesh.cpp ./assimp/ogldev_texture.cpp ./assimp/ogldev_util.cpp ./assimp/3rdparty/stb_image.cpp

INCLUDEPATH += ./assimp/include

LIBS += ./assimp/lib/libassimp.a
LIBS += ./assimp/lib/libdraco.a
LIBS += ./assimp/lib/libkubazip.a
LIBS += ./assimp/lib/libminizip.a
LIBS += ./assimp/lib/libpoly2tri.a
LIBS += ./assimp/lib/libpolyclipping.a
LIBS += ./assimp/lib/libpugixml.a
LIBS += ./assimp/lib/libz.a


QMAKE_CXXFLAGS += -Wno-ignored-qualifiers -Wno-type-limits -Wno-unused-parameter -fcompare-debug-second


