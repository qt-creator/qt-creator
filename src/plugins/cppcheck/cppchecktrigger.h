// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QHash>
#include <QPointer>

namespace Utils {
class FilePath;
using FilePaths = QList<FilePath>;
}

namespace ProjectExplorer {
class Project;
}

namespace Core {
class IDocument;
class IEditor;
}

namespace Cppcheck::Internal {

class CppcheckTextMarkManager;
class CppcheckTool;

class CppcheckTrigger final : public QObject
{
public:
    explicit CppcheckTrigger(CppcheckTextMarkManager &marks, CppcheckTool &tool);
    ~CppcheckTrigger() override;

    void recheck();

private:
    void checkEditors(const QList<Core::IEditor *> &editors = {});
    void removeEditors(const QList<Core::IEditor *> &editors = {});
    void checkChangedDocument(Core::IDocument *document);
    void changeCurrentProject(ProjectExplorer::Project *project);
    void updateProjectFiles(ProjectExplorer::Project *project);
    void check(const Utils::FilePaths &files);
    void remove(const Utils::FilePaths &files);

    CppcheckTextMarkManager &m_marks;
    CppcheckTool &m_tool;
    QPointer<ProjectExplorer::Project> m_currentProject;
    QHash<Utils::FilePath, QDateTime> m_checkedFiles;
};

} // Cppcheck::Internal
