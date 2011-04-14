/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "profilekeywords.h"

using namespace Qt4ProjectManager::Internal;

static const char *const variableKeywords[] = {
    "CCFLAG",
    "CONFIG",
    "DEFINES",
    "DEF_FILE",
    "DEPENDPATH",
    "DEPLOYMENT",
    "DESTDIR",
    "DESTDIR_TARGET",
    "DISTFILES",
    "DLLDESTDIR",
    "FORMS",
    "HEADERS",
    "ICON",
    "INCLUDEPATH",
    "INSTALLS",
    "LEXSOURCES",
    "LIBS",
    "MAKEFILE",
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
    "PRECOMPILED_HEADER",
    "PRE_TARGETDEPS",
    "QMAKE",
    "QMAKESPEC",
    "QT",
    "RCC_DIR",
    "RC_FILE",
    "REQUIRES",
    "RESOURCES",
    "RES_FILE",
    "SOURCES",
    "SRCMOC",
    "STATECHARTS",
    "SUBDIRS",
    "TARGET",
    "TARGET.CAPABILITY",
    "TARGET.EPOCHEAPSIZE",
    "TARGET.UID3",
    "TARGET_EXT",
    "TARGET_x",
    "TARGET_x.y.z",
    "TEMPLATE",
    "TRANSLATIONS",
    "UI_DIR",
    "UI_HEADERS_DIR",
    "UI_SOURCES_DIR",
    "VER_MAJ",
    "VER_MIN",
    "VER_PAT",
    "VERSION",
    "VPATH",
    "YACCSOURCES",
    0
};

static const char *const functionKeywords[] = {
    "basename",
    "contains",
    "count",
    "dirname",
    "error",
    "exists",
    "find",
    "for",
    "include",
    "infile",
    "isEmpty",
    "join",
    "member",
    "message",
    "prompt",
    "quote",
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
