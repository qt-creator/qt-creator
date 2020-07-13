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

#include <cppcheck/cppcheckoptions.h>

#include <QFuture>
#include <QPointer>
#include <QRegularExpression>

#include <memory>

namespace Utils {
class FilePath;
using FilePaths = QList<FilePath>;
}

namespace CppTools {
class ProjectPart;
}

namespace ProjectExplorer {
class Project;
}

namespace Cppcheck {
namespace Internal {

class CppcheckRunner;
class CppcheckDiagnosticManager;
class CppcheckOptions;

class CppcheckTool final : public QObject
{
    Q_OBJECT

public:
    CppcheckTool(CppcheckDiagnosticManager &manager, const Utils::Id &progressId);
    ~CppcheckTool() override;

    void updateOptions(const CppcheckOptions &options);
    void setProject(ProjectExplorer::Project *project);
    void check(const Utils::FilePaths &files);
    void stop(const Utils::FilePaths &files);

    void startParsing();
    void parseOutputLine(const QString &line);
    void parseErrorLine(const QString &line);
    void finishParsing();

    const CppcheckOptions &options() const;

private:
    void updateArguments();
    void addToQueue(const Utils::FilePaths &files, CppTools::ProjectPart &part);
    QStringList additionalArguments(const CppTools::ProjectPart &part) const;

    CppcheckDiagnosticManager &m_manager;
    CppcheckOptions m_options;
    QPointer<ProjectExplorer::Project> m_project;
    std::unique_ptr<CppcheckRunner> m_runner;
    std::unique_ptr<QFutureInterface<void>> m_progress;
    QHash<QString, QString> m_cachedAdditionalArguments;
    QVector<QRegularExpression> m_filters;
    QRegularExpression m_progressRegexp;
    QRegularExpression m_messageRegexp;
    Utils::Id m_progressId;
};

} // namespace Internal
} // namespace Cppcheck
