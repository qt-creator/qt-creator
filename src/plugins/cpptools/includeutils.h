/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/


#ifndef INCLUDEUTILS_H
#define INCLUDEUTILS_H

#include "cpptools_global.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/PreprocessorClient.h>

#include <QList>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QStringList)
QT_FORWARD_DECLARE_CLASS(QTextDocument)

namespace CppTools {
namespace IncludeUtils {

typedef CPlusPlus::Document::Include Include;
typedef CPlusPlus::Client::IncludeType IncludeType;

class CPPTOOLS_EXPORT IncludeGroup
{
public:
    static QList<IncludeGroup> detectIncludeGroupsByNewLines(QList<Include> &includes);
    static QList<IncludeGroup> detectIncludeGroupsByIncludeDir(const QList<Include> &includes);
    static QList<IncludeGroup> detectIncludeGroupsByIncludeType(const QList<Include> &includes);

    static int lineForAppendedIncludeGroup(const QList<IncludeGroup> &groups,
                                           unsigned *newLinesToPrepend);
    static int lineForPrependedIncludeGroup(const QList<IncludeGroup> &groups,
                                            unsigned *newLinesToAppend);

    static QList<IncludeGroup> filterMixedIncludeGroups(const QList<IncludeGroup> &groups);
    static QList<IncludeGroup> filterIncludeGroups(const QList<IncludeGroup> &groups,
                                                   CPlusPlus::Client::IncludeType includeType);

    static QString includeDir(const QString &include);

public:
    IncludeGroup(const QList<Include> &includes) : m_includes(includes) {}

    QList<Include> includes() const { return m_includes; }
    Include first() const { return m_includes.first(); }
    Include last() const { return m_includes.last(); }
    int size() const { return m_includes.size(); }
    bool isEmpty() const { return m_includes.isEmpty(); }

    QString commonPrefix() const;
    QString commonIncludeDir() const; /// only valid if hasCommonDir() == true
    bool hasCommonIncludeDir() const;
    bool hasOnlyIncludesOfType(CPlusPlus::Client::IncludeType includeType) const;
    bool isSorted() const; /// name-wise

    int lineForNewInclude(const QString &newIncludeFileName,
                          CPlusPlus::Client::IncludeType newIncludeType) const;

private:
    QStringList filesNames() const;

    QList<Include> m_includes;
};

class CPPTOOLS_EXPORT LineForNewIncludeDirective
{
public:
    enum MocIncludeMode { RespectMocIncludes, IgnoreMocIncludes };
    enum IncludeStyle { LocalBeforeGlobal, GlobalBeforeLocal, AutoDetect };

    LineForNewIncludeDirective(const QTextDocument *textDocument,
                               QList<Include> includes,
                               MocIncludeMode mocIncludeMode = IgnoreMocIncludes,
                               IncludeStyle includeStyle = AutoDetect);

    /// Returns the line (1-based) at which the include directive should be inserted.
    /// On error, -1 is returned.
    int operator()(const QString &newIncludeFileName, unsigned *newLinesToPrepend = 0,
                   unsigned *newLinesToAppend = 0);

private:
    QList<IncludeGroup> getGroupsByIncludeType(const QList<IncludeGroup> &groups,
                                               IncludeType includeType);

    const QTextDocument *m_textDocument;
    IncludeStyle m_includeStyle;
    QList<Include> m_includes;
};

} // namespace IncludeUtils
} // namespace CppTools

#endif // INCLUDEUTILS_H
