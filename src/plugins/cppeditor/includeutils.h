// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>
#include <cplusplus/PreprocessorClient.h>

#include <QList>
#include <QObject>
#include <QString>

QT_FORWARD_DECLARE_CLASS(QTextDocument)

namespace CppEditor {
namespace IncludeUtils {

using Include = CPlusPlus::Document::Include;
using IncludeType = CPlusPlus::Client::IncludeType;

class IncludeGroup
{
public:
    static QList<IncludeGroup> detectIncludeGroupsByNewLines(QList<Include> &includes);
    static QList<IncludeGroup> detectIncludeGroupsByIncludeDir(const QList<Include> &includes);
    static QList<IncludeGroup> detectIncludeGroupsByIncludeType(const QList<Include> &includes);

    static QList<IncludeGroup> filterMixedIncludeGroups(const QList<IncludeGroup> &groups);
    static QList<IncludeGroup> filterIncludeGroups(const QList<IncludeGroup> &groups,
                                                   CPlusPlus::Client::IncludeType includeType);

public:
    explicit IncludeGroup(const QList<Include> &includes) : m_includes(includes) {}

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

class LineForNewIncludeDirective
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
    int operator()(const QString &newIncludeFileName, unsigned *newLinesToPrepend = nullptr,
                   unsigned *newLinesToAppend = nullptr);

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

#ifdef WITH_TESTS
namespace Internal {
class IncludeGroupsTest : public QObject
{
    Q_OBJECT

private slots:
    void testDetectIncludeGroupsByNewLines();
    void testDetectIncludeGroupsByIncludeDir();
    void testDetectIncludeGroupsByIncludeType();
};
} // namespace Internal
#endif // WITH_TESTS

} // namespace CppEditor
