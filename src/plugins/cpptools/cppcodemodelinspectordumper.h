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

#ifndef CPPCODEMODELINSPECTORDUMPER_H
#define CPPCODEMODELINSPECTORDUMPER_H

#include "cpptools_global.h"

#include <cpptools/cppprojects.h>
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
    static QString toString(CppTools::ProjectPart::LanguageVersion languageVersion);
    static QString toString(CppTools::ProjectPart::LanguageExtensions languageExtension);
    static QString toString(CppTools::ProjectPart::QtVersion qtVersion);
    static QString toString(const QList<CppTools::ProjectFile> &projectFiles);
    static QString toString(CppTools::ProjectFile::Kind kind);
    static QString toString(CPlusPlus::Kind kind);
    static QString partsForFile(const QString &fileName);
    static QString unresolvedFileNameWithDelimiters(const CPlusPlus::Document::Include &include);
    static QString pathListToString(const QStringList &pathList);
    static QString pathListToString(const ProjectPart::HeaderPaths &pathList);
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
    void dumpMergedEntities(const ProjectPart::HeaderPaths &mergedHeaderPaths,
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
