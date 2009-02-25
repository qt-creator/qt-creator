/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "profilehighlighter.h"

#include <QtCore/QRegExp>
#include <QtGui/QColor>
#include <QtGui/QTextDocument>
#include <QtGui/QTextEdit>

using namespace Qt4ProjectManager::Internal;

#define MAX_VARIABLES 48
const char *const variables[MAX_VARIABLES] = {
    "CONFIG",
    "DEFINES",
    "DEF_FILE",
    "DEPENDPATH",
    "DESTDIR",
    "DESTDIR_TARGET",
    "DISTFILES",
    "DLLDESTDIR",
    "FORMS",
    "HEADERS",
    "INCLUDEPATH",
    "LEXSOURCES",
    "LIBS",
    "MAKEFILE",
    "MOC_DIR",
    "OBJECTS",
    "OBJECTS_DIR",
    "OBJMOC",
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
    "SUBDIRS",
    "TARGET",
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

#define MAX_FUNCTIONS 22
const char *const functions[MAX_FUNCTIONS] = {
    "basename",
    "CONFIG",
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

struct KeywordHelper
{
    inline KeywordHelper(const QString &word) : needle(word) {}
    const QString needle;
};

static bool operator<(const KeywordHelper &helper, const char *kw)
{
    return helper.needle < QLatin1String(kw);
}

static bool operator<(const char *kw, const KeywordHelper &helper)
{
    return QLatin1String(kw) < helper.needle;
}

static bool isVariable(const QString &word)
{
    const char *const *start = &variables[0];
    const char *const *end = &variables[MAX_VARIABLES - 1];
    const char *const *kw = qBinaryFind(start, end, KeywordHelper(word));
    return *kw != 0;
}

static bool isFunction(const QString &word)
{
    const char *const *start = &functions[0];
    const char *const *end = &functions[MAX_FUNCTIONS - 1];
    const char *const *kw = qBinaryFind(start, end, KeywordHelper(word));
    return *kw != 0;
}

ProFileHighlighter::ProFileHighlighter(QTextDocument *document) :
    QSyntaxHighlighter(document)
{
}

void ProFileHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty())
        return;

    QString buf;
    bool inCommentMode = false;

    QTextCharFormat emptyFormat;
    int i = 0;
    for (;;) {
        const QChar c = text.at(i);
        if (inCommentMode) {
            setFormat(i, 1, m_formats[ProfileCommentFormat]);
        } else {
            if (c.isLetter() || c == '_' || c == '.') {
                buf += c;
                setFormat(i - buf.length()+1, buf.length(), emptyFormat);
                if (!buf.isEmpty() && isFunction(buf))
                    setFormat(i - buf.length()+1, buf.length(), m_formats[ProfileFunctionFormat]);
                else if (!buf.isEmpty() && isVariable(buf))
                    setFormat(i - buf.length()+1, buf.length(), m_formats[ProfileVariableFormat]);
            } else if (c == '(') {
                if (!buf.isEmpty() && isFunction(buf))
                    setFormat(i - buf.length(), buf.length(), m_formats[ProfileFunctionFormat]);
                buf.clear();
            } else if (c == '#') {
                inCommentMode = true;
                setFormat(i, 1, m_formats[ProfileCommentFormat]);
                buf.clear();
            } else {
                if (!buf.isEmpty() && isVariable(buf))
                    setFormat(i - buf.length(), buf.length(), m_formats[ProfileVariableFormat]);
                buf.clear();
            }
        }
        i++;
        if (i >= text.length())
            break;
    }
}

