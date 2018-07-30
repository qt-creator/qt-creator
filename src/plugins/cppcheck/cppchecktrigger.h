/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
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

#include <QHash>
#include <QPointer>

namespace Utils {
class FileName;
using FileNameList = QList<FileName>;
}

namespace ProjectExplorer {
class Project;
}

namespace Core {
class IDocument;
class IEditor;
}

namespace CppTools {
class ProjectInfo;
}

namespace Cppcheck {
namespace Internal {

class CppcheckTextMarkManager;
class CppcheckTool;

class CppcheckTrigger final : public QObject
{
    Q_OBJECT

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
    void check(const Utils::FileNameList &files);
    void remove(const Utils::FileNameList &files);

    CppcheckTextMarkManager &m_marks;
    CppcheckTool &m_tool;
    QPointer<ProjectExplorer::Project> m_currentProject;
    QHash<Utils::FileName, QDateTime> m_checkedFiles;
};

} // namespace Internal
} // namespace Cppcheck
