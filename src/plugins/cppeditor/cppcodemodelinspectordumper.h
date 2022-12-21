// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectpart.h"
#include "projectinfo.h"

#include <cplusplus/CppDocument.h>

#include <QFile>
#include <QTextStream>

namespace CppEditor {
class WorkingCopy;

namespace CppCodeModelInspector {

struct Utils
{
    static QString toString(bool value);
    static QString toString(int value);
    static QString toString(unsigned value);
    static QString toString(const QDateTime &dateTime);
    static QString toString(CPlusPlus::Document::CheckMode checkMode);
    static QString toString(CPlusPlus::Document::DiagnosticMessage::Level level);
    static QString toString(ProjectExplorer::HeaderPathType type);
    static QString toString(::Utils::LanguageVersion languageVersion);
    static QString toString(::Utils::LanguageExtensions languageExtension);
    static QString toString(::Utils::QtMajorVersion qtVersion);
    static QString toString(ProjectExplorer::BuildTargetType buildTargetType);
    static QString toString(const QVector<ProjectFile> &projectFiles);
    static QString toString(ProjectFile::Kind kind);
    static QString toString(CPlusPlus::Kind kind);
    static QString toString(const ProjectExplorer::Abi &abi);
    static QString partsForFile(const ::Utils::FilePath &filePath);
    static QString unresolvedFileNameWithDelimiters(const CPlusPlus::Document::Include &include);
    static QString pathListToString(const QStringList &pathList);
    static QString pathListToString(const ProjectExplorer::HeaderPaths &pathList);
    static QList<CPlusPlus::Document::Ptr> snapshotToList(const CPlusPlus::Snapshot &snapshot);
};

class Dumper
{
public:
    explicit Dumper(const CPlusPlus::Snapshot &globalSnapshot,
                    const QString &logFileId = QString());
    ~Dumper();

    void dumpProjectInfos(const QList<ProjectInfo::ConstPtr> &projectInfos);
    void dumpSnapshot(const CPlusPlus::Snapshot &snapshot,
                      const QString &title,
                      bool isGlobalSnapshot = false);
    void dumpWorkingCopy(const WorkingCopy &workingCopy);
    void dumpMergedEntities(const ProjectExplorer::HeaderPaths &mergedHeaderPaths,
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
} // namespace CppEditor
