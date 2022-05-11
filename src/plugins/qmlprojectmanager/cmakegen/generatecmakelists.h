/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

namespace QmlProjectManager {
namespace GenerateCmake {
struct GeneratableFile {
    Utils::FilePath filePath;
    QString content;
    bool fileExists;
};

bool operator==(const GeneratableFile &left, const GeneratableFile &right);

void generateMenuEntry();
void onGenerateCmakeLists();
bool isErrorFatal(int error);
int isProjectCorrectlyFormed(const Utils::FilePath &rootDir);
void showProjectDirErrorDialog(int error);
QString readTemplate(const QString &templatePath);
const QString projectEnvironmentVariable(const QString &key);

class FileQueue {
public:
    bool queueFile(const Utils::FilePath &filePath, const QString &fileContent);
    const QVector<GeneratableFile> queuedFiles() const;
    bool writeQueuedFiles();
    void filterFiles(const Utils::FilePaths keepFiles);

private:
    bool writeFile(const GeneratableFile &file);

private:
    QVector<GeneratableFile> m_queuedFiles;
};

class CmakeFileGenerator {
public:
    bool prepare(const Utils::FilePath &rootDir, bool check = true);
    const FileQueue fileQueue() const;
    void filterFileQueue(const Utils::FilePaths &keepFiles);
    bool execute();

private:
    void generateMainCmake(const Utils::FilePath &rootDir);
    void generateImportCmake(const Utils::FilePath &dir, const QString &modulePrefix = QString());
    void generateModuleCmake(const Utils::FilePath &dir, const QString &moduleUri = QString());
    bool generateEntryPointFiles(const Utils::FilePath &dir);
    bool generateMainCpp(const Utils::FilePath &dir);
    bool generateMainQml(const Utils::FilePath &dir);
    QStringList getDirectoryQmls(const Utils::FilePath &dir);
    QStringList getDirectoryResources(const Utils::FilePath &dir);
    QStringList getSingletonsFromQmldirFile(const Utils::FilePath &filePath);
    QStringList getDirectoryTreeQmls(const Utils::FilePath &dir);
    QStringList getDirectoryTreeResources(const Utils::FilePath &dir);
    void queueCmakeFile(const Utils::FilePath &filePath, const QString &content);
    bool isFileResource(const QString &relativeFilePath);
    bool isFileBlacklisted(const QString &fileName);
    bool isDirBlacklisted(const Utils::FilePath &dir);
    bool includeFile(const Utils::FilePath &filePath);

private:
    FileQueue m_fileQueue;
    QStringList m_resourceFileLocations;
    QStringList m_moduleNames;
    bool m_checkFileIsInProject;
};

} //GenerateCmake

} //QmlProjectManager
