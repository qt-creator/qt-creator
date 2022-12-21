// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CMAKEPROJECTCONVERTER_H
#define CMAKEPROJECTCONVERTER_H

#include <utils/fileutils.h>
#include <qmlprojectmanager/qmlproject.h>

namespace QmlProjectManager {

namespace GenerateCmake {

enum ProjectConverterObjectType {
    File,
    Directory
};

struct ProjectConverterObject {
    ProjectConverterObjectType type;
    Utils::FilePath target;
    Utils::FilePath original;
};

class CmakeProjectConverter {

public:
    bool convertProject(const QmlProjectManager::QmlProject *project,
                        const Utils::FilePath &targetDir);
    static void generateMenuEntry(QObject *parent);
    static void onConvertProject();
    static bool isProjectConvertable(const ProjectExplorer::Project *project);
    static bool isProjectCurrentFormat(const ProjectExplorer::Project *project);

private:
    bool prepareAndExecute();
    bool isFileBlacklisted(const Utils::FilePath &file) const;
    bool isDirBlacklisted(const Utils::FilePath &dir) const;
    bool performSanityCheck();
    bool prepareBaseDirectoryStructure();
    bool prepareCopyDirFiles(const Utils::FilePath &dir, const Utils::FilePath &targetDir);
    bool prepareCopyDirTree(const Utils::FilePath &dir, const Utils::FilePath &targetDir);
    bool prepareCopy();
    bool addDirectory(const Utils::FilePath &target);
    bool addFile(const Utils::FilePath &target);
    bool addFile(const Utils::FilePath &original, const Utils::FilePath &target);
    bool addObject(ProjectConverterObjectType type,
                       const Utils::FilePath &original, const Utils::FilePath &target);
    bool createPreparedProject();

    const Utils::FilePath contentDir() const;
    const Utils::FilePath sourceDir() const;
    const Utils::FilePath importDir() const;
    const Utils::FilePath assetDir() const;
    const Utils::FilePath assetImportDir() const;
    const Utils::FilePath newProjectFile() const;

    const QString environmentVariable(const QString &key) const;
    const Utils::FilePath projectMainFile() const;
    const QString projectMainClass() const;
    bool modifyNewFiles();
    bool modifyAppMainQml();
    bool modifyProjectFile();

private:
    QList<ProjectConverterObject> m_converterObjects;
    QStringList m_rootDirFiles;
    Utils::FilePath m_projectDir;
    Utils::FilePath m_newProjectDir;
    const QmlProjectManager::QmlProject *m_project;
    QString m_errorText;
};

} //GenerateCmake
} //QmlProjectManager

#endif // CMAKEPROJECTCONVERTER_H
