/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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

    static QList<IncludeGroup> filterMixedIncludeGroups(const QList<IncludeGroup> &groups);
    static QList<IncludeGroup> filterIncludeGroups(const QList<IncludeGroup> &groups,
                                                   CPlusPlus::Client::IncludeType includeType);

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
                               const CPlusPlus::Document::Ptr cppDocument,
                               MocIncludeMode mocIncludeMode = IgnoreMocIncludes,
                               IncludeStyle includeStyle = AutoDetect);

    /// Returns the line (1-based) at which the include directive should be inserted.
    /// On error, -1 is returned.
    int operator()(const QString &newIncludeFileName, unsigned *newLinesToPrepend = 0,
                   unsigned *newLinesToAppend = 0);

private:
    int findInsertLineForVeryFirstInclude(unsigned *newLinesToPrepend, unsigned *newLinesToAppend);
    QList<IncludeGroup> getGroupsByIncludeType(const QList<IncludeGroup> &groups,
                                               IncludeType includeType);

    const QTextDocument *m_textDocument;
    const CPlusPlus::Document::Ptr m_cppDocument;

    IncludeStyle m_includeStyle;
    QList<Include> m_includes;
};

} // namespace IncludeUtils
} // namespace CppTools

#endif // INCLUDEUTILS_H
