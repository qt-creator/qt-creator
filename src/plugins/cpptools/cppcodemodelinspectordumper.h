/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPCODEMODELINSPECTORDUMPER_H
#define CPPCODEMODELINSPECTORDUMPER_H

#include "cpptools_global.h"

#include <cpptools/cppmodelmanagerinterface.h>
#include <cplusplus/CppDocument.h>

#include <QFile>
#include <QTextStream>

namespace CppTools {
namespace CppCodeModelInspector {

struct CPPTOOLS_EXPORT Utils
{
    static QString toString(bool value);
    static QString toString(unsigned value);
    static QString toString(const QDateTime &dateTime);
    static QString toString(CPlusPlus::Document::CheckMode checkMode);
    static QString toString(CPlusPlus::Document::DiagnosticMessage::Level level);
    static QString toString(CppTools::ProjectPart::CVersion cVersion);
    static QString toString(CppTools::ProjectPart::CXXVersion cxxVersion);
    static QString toString(CppTools::ProjectPart::CXXExtensions cxxExtension);
    static QString toString(CppTools::ProjectPart::QtVersion qtVersion);
    static QString toString(const QList<CppTools::ProjectFile> &projectFiles);
    static QString toString(CppTools::ProjectFile::Kind kind);
    static QString toString(CPlusPlus::Kind kind);
    static QString partsForFile(const QString &fileName);
    static QString unresolvedFileNameWithDelimiters(const CPlusPlus::Document::Include &include);
    static QString pathListToString(const QStringList &pathList);
    static QList<CPlusPlus::Document::Ptr> snapshotToList(const CPlusPlus::Snapshot &snapshot);
};

class CPPTOOLS_EXPORT Dumper
{
public:
    explicit Dumper(const CPlusPlus::Snapshot &globalSnapshot,
                    const QString &logFileId = QString());
    ~Dumper();

    void dumpProjectInfos(const QList<CppTools::CppModelManagerInterface::ProjectInfo> &projectInfos);
    void dumpSnapshot(const CPlusPlus::Snapshot &snapshot,
                      const QString &title,
                      bool isGlobalSnapshot = false);
    void dumpWorkingCopy(const CppTools::CppModelManagerInterface::WorkingCopy &workingCopy);
    void dumpMergedEntities(const QStringList &mergedIncludePaths,
                            const QStringList &mergedFrameworkPaths,
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

#endif // CPPCODEMODELINSPECTORDUMPER_H
