/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cpptools_global.h"

#include <cpptools/projectpart.h>
#include <cpptools/projectinfo.h>
#include <cplusplus/CppDocument.h>

#include <QFile>
#include <QTextStream>

namespace CppTools {
class WorkingCopy;

namespace CppCodeModelInspector {

struct CPPTOOLS_EXPORT Utils
{
    static QString toString(bool value);
    static QString toString(unsigned value);
    static QString toString(const QDateTime &dateTime);
    static QString toString(CPlusPlus::Document::CheckMode checkMode);
    static QString toString(CPlusPlus::Document::DiagnosticMessage::Level level);
    static QString toString(ProjectPartHeaderPath::Type type);
    static QString toString(CppTools::ProjectPart::LanguageVersion languageVersion);
    static QString toString(CppTools::ProjectPart::LanguageExtensions languageExtension);
    static QString toString(CppTools::ProjectPart::QtVersion qtVersion);
    static QString toString(CppTools::ProjectPart::BuildTargetType buildTargetType);
    static QString toString(const QVector<CppTools::ProjectFile> &projectFiles);
    static QString toString(CppTools::ProjectFile::Kind kind);
    static QString toString(CPlusPlus::Kind kind);
    static QString partsForFile(const QString &fileName);
    static QString unresolvedFileNameWithDelimiters(const CPlusPlus::Document::Include &include);
    static QString pathListToString(const QStringList &pathList);
    static QString pathListToString(const ProjectPartHeaderPaths &pathList);
    static QList<CPlusPlus::Document::Ptr> snapshotToList(const CPlusPlus::Snapshot &snapshot);
};

class CPPTOOLS_EXPORT Dumper
{
public:
    explicit Dumper(const CPlusPlus::Snapshot &globalSnapshot,
                    const QString &logFileId = QString());
    ~Dumper();

    void dumpProjectInfos(const QList<CppTools::ProjectInfo> &projectInfos);
    void dumpSnapshot(const CPlusPlus::Snapshot &snapshot,
                      const QString &title,
                      bool isGlobalSnapshot = false);
    void dumpWorkingCopy(const CppTools::WorkingCopy &workingCopy);
    void dumpMergedEntities(const ProjectPartHeaderPaths &mergedHeaderPaths,
                            const QByteArray &mergedMacros);

private:
    void dumpStringList(const QStringList &list, const QByteArray &indent);
    void dumpDocuments(const QList<CPlusPlus::Document::Ptr> &documents,
                       bool skipDetails = false);
    static QByteArray indent(int level);

    CPlusPlus::Snapshot m_globalSnapshot;
    QFile m_logFile;
    QTextStream m_out;
};

} // namespace CppCodeModelInspector
} // namespace CppTools
