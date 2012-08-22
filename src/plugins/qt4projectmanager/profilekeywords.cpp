/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "profilekeywords.h"

using namespace Qt4ProjectManager::Internal;

static const char *const variableKeywords[] = {
    "BACKUP_REGISTRATION_FILE_MAEMO",
    "CCFLAG",
    "CONFIG",
    "DEFINES",
    "DEF_FILE",
    "DEPENDPATH",
    "DEPLOYMENT",
    "DEPLOYMENT_PLUGIN",
    "DESTDIR",
    "DESTDIR_TARGET",
    "DISTFILES",
    "DLLDESTDIR",
    "DISTFILES",
    "DSP_TEMPLATE",
    "FORMS",
    "FORMS3",
    "GUID",
    "HEADERS",
    "ICON",
    "INCLUDEPATH",
    "INSTALLS",
    "LEXIMPLS",
    "LEXOBJECTS",
    "LEXSOURCES",
    "LIBS",
    "LITERAL_HASH",
    "MAKEFILE",
    "MAKEFILE_GENERATOR",
    "MOBILITY",
    "MOC_DIR",
    "OBJECTIVE_HEADERS",
    "OBJECTIVE_SOURCES",
    "OBJECTS",
    "OBJECTS_DIR",
    "OBJMOC",
    "OTHER_FILES",
    "PKGCONFIG",
    "POST_TARGETDEPS",
    "PRE_TARGETDEPS",
    "PRECOMPILED_HEADER",
    "PWD",
    "OUT_PWD",
    "QMAKE",
    "QMAKESPEC",
    "QMAKE_APP_FLAG",
    "QMAKE_APP_OR_DLL",
    "QMAKE_AR_CMD",
    "QMAKE_BUNDLE_DATA",
    "QMAKE_BUNDLE_EXTENSION",
    "QMAKE_CC",
    "QMAKE_CFLAGS_DEBUG",
    "QMAKE_CFLAGS_MT",
    "QMAKE_CFLAGS_MT_DBG",
    "QMAKE_CFLAGS_MT_DLL",
    "QMAKE_CFLAGS_MT_DLLDBG",
    "QMAKE_CFLAGS_RELEASE",
    "QMAKE_CFLAGS_SHLIB",
    "QMAKE_CFLAGS_THREAD",
    "QMAKE_CFLAGS_WARN_OFF",
    "QMAKE_CFLAGS_WARN_ON",
    "QMAKE_CLEAN",
    "QMAKE_CXX",
    "QMAKE_CXXFLAGS",
    "QMAKE_CXXFLAGS_DEBUG",
    "QMAKE_CXXFLAGS_MT",
    "QMAKE_CXXFLAGS_MT_DBG",
    "QMAKE_CXXFLAGS_MT_DLL",
    "QMAKE_CXXFLAGS_MT_DLLDBG",
    "QMAKE_CXXFLAGS_RELEASE",
    "QMAKE_CXXFLAGS_SHLIB",
    "QMAKE_CXXFLAGS_THREAD",
    "QMAKE_CXXFLAGS_WARN_OFF",
    "QMAKE_CXXFLAGS_WARN_ON",
    "QMAKE_DISTCLEAN",
    "QMAKE_EXTENSION_SHLIB",
    "QMAKE_EXT_MOC",
    "QMAKE_EXT_UI",
    "QMAKE_EXT_PRL",
    "QMAKE_EXT_LEX",
    "QMAKE_EXT_YACC",
    "QMAKE_EXT_OBJ",
    "QMAKE_EXT_CPP",
    "QMAKE_EXT_H",
    "QMAKE_EXTRA_COMPILERS",
    "QMAKE_EXTRA_TARGETS",
    "QMAKE_FAILED_REQUIREMENTS",
    "QMAKE_FILETAGS",
    "QMAKE_FRAMEWORK_BUNDLE_NAME",
    "QMAKE_FRAMEWORK_VERSION",
    "QMAKE_INCDIR",
    "QMAKE_INCDIR_EGL",
    "QMAKE_INCDIR_OPENGL",
    "QMAKE_INCDIR_OPENGL_ES1",
    "QMAKE_INCDIR_OPENGL_ES2",
    "QMAKE_INCDIR_OPENVG",
    "QMAKE_INCDIR_QT",
    "QMAKE_INCDIR_THREAD",
    "QMAKE_INCDIR_X11",
    "QMAKE_INFO_PLIST",
    "QMAKE_LFLAGS",
    "QMAKE_LFLAGS_CONSOLE",
    "QMAKE_LFLAGS_CONSOLE_DLL",
    "QMAKE_LFLAGS_DEBUG",
    "QMAKE_LFLAGS_PLUGIN",
    "QMAKE_LFLAGS_RPATH",
    "QMAKE_LFLAGS_QT_DLL",
    "QMAKE_LFLAGS_RELEASE",
    "QMAKE_LFLAGS_SHAPP",
    "QMAKE_LFLAGS_SHLIB",
    "QMAKE_LFLAGS_SONAME",
    "QMAKE_LFLAGS_THREAD",
    "QMAKE_LFLAGS_WINDOWS",
    "QMAKE_LFLAGS_WINDOWS_DLL",
    "QMAKE_LIBDIR",
    "QMAKE_LIBDIR_FLAGS",
    "QMAKE_LIBDIR_EGL",
    "QMAKE_LIBDIR_OPENGL",
    "QMAKE_LIBDIR_OPENVG",
    "QMAKE_LIBDIR_QT",
    "QMAKE_LIBDIR_X11",
    "QMAKE_LIBS",
    "QMAKE_LIBS_CONSOLE",
    "QMAKE_LIBS_EGL",
    "QMAKE_LIBS_OPENGL",
    "QMAKE_LIBS_OPENGL_QT",
    "QMAKE_LIBS_OPENGL_ES1",
    "QMAKE_LIBS_OPENGL_ES2",
    "QMAKE_LIBS_OPENVG",
    "QMAKE_LIBS_QT",
    "QMAKE_LIBS_QT_DLL",
    "QMAKE_LIBS_QT_OPENGL",
    "QMAKE_LIBS_QT_THREAD",
    "QMAKE_LIBS_RT",
    "QMAKE_LIBS_RTMT",
    "QMAKE_LIBS_THREAD",
    "QMAKE_LIBS_WINDOWS",
    "QMAKE_LIBS_X11",
    "QMAKE_LIBS_X11SM",
    "QMAKE_LIB_FLAG",
    "QMAKE_LINK_SHLIB_CMD",
    "QMAKE_LN_SHLIB",
    "QMAKE_POST_LINK",
    "QMAKE_PRE_LINK",
    "QMAKE_PROJECT_NAME",
    "QMAKE_MAC_SDK",
    "QMAKE_MACOSX_DEPLOYMENT_TARGET",
    "QMAKE_MAKEFILE",
    "QMAKE_MOC_SRC",
    "QMAKE_QMAKE",
    "QMAKE_QT_DLL",
    "QMAKE_RESOURCE_FLAGS",
    "QMAKE_RPATH",
    "QMAKE_RPATHDIR",
    "QMAKE_RUN_CC",
    "QMAKE_RUN_CC_IMP",
    "QMAKE_RUN_CXX",
    "QMAKE_RUN_CXX_IMP",
    "QMAKE_TARGET",
    "QMAKE_UIC",
    "QT",
    "QTPLUGIN",
    "QT_VERSION",
    "QT_MAJOR_VERSION",
    "QT_MINOR_VERSION",
    "QT_PATCH_VERSION",
    "RCC_DIR",
    "RC_FILE",
    "REQUIRES",
    "RESOURCES",
    "RES_FILE",
    "RSS_RULES",
    "SIGNATURE_FILE",
    "SOURCES",
    "SRCMOC",
    "STATECHARTS",
    "SUBDIRS",
    "TEMPLATE",
    "TRANSLATIONS",
    "UICIMPLS",
    "UICOBJECTS",
    "UI_DIR",
    "UI_HEADERS_DIR",
    "UI_SOURCES_DIR",
    "VER_MAJ",
    "VER_MIN",
    "VER_PAT",
    "VERSION",
    "VPATH",
    "YACCIMPLS",
    "YACCOBJECTS",
    "YACCSOURCES",
    "_PRO_FILE_",
    "_PRO_FILE_PWD_",
    0
};

static const char *const functionKeywords[] = {
    "basename",
    "contains",
    "count",
    "dirname",
    "error",
    "eval",
    "exists",
    "find",
    "for",
    "include",
    "infile",
    "isEmpty",
    "join",
    "member",
    "message",
    "packagesExist",
    "prompt",
    "quote",
    "replace",
    "sprintf",
    "system",
    "unique",
    "warning",
    0
};

class ProFileKeywordsImplementation
{
public:
    static ProFileKeywordsImplementation *instance();
    QStringList variables();
    QStringList functions();
    bool isVariable(const QString &word);
    bool isFunction(const QString &word);
private:
    ProFileKeywordsImplementation();
    static ProFileKeywordsImplementation *m_instance;
    QStringList m_variables;
    QStringList m_functions;
};

ProFileKeywordsImplementation *ProFileKeywordsImplementation::m_instance = 0;

ProFileKeywordsImplementation *ProFileKeywordsImplementation::instance()
{
    if (!m_instance)
        m_instance = new ProFileKeywordsImplementation();
    return m_instance;
}

QStringList ProFileKeywordsImplementation::variables()
{
    return m_variables;
}

QStringList ProFileKeywordsImplementation::functions()
{
    return m_functions;
}

bool ProFileKeywordsImplementation::isVariable(const QString &word)
{
    return m_variables.contains(word);
}

bool ProFileKeywordsImplementation::isFunction(const QString &word)
{
    return m_functions.contains(word);
}

ProFileKeywordsImplementation::ProFileKeywordsImplementation()
{
    for (uint i = 0; i < sizeof variableKeywords / sizeof variableKeywords[0] - 1; i++)
        m_variables.append(QLatin1String(variableKeywords[i]));
    for (uint i = 0; i < sizeof functionKeywords / sizeof functionKeywords[0] - 1; i++)
        m_functions.append(QLatin1String(functionKeywords[i]));
}

ProFileKeywords::ProFileKeywords()
{
}

QStringList ProFileKeywords::variables()
{
    return ProFileKeywordsImplementation::instance()->variables();
}

QStringList ProFileKeywords::functions()
{
    return ProFileKeywordsImplementation::instance()->functions();
}

bool ProFileKeywords::isVariable(const QString &word)
{
    return ProFileKeywordsImplementation::instance()->isVariable(word);
}

bool ProFileKeywords::isFunction(const QString &word)
{
    return ProFileKeywordsImplementation::instance()->isFunction(word);
}
