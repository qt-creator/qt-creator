// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QFutureInterface>
#include <QPointer>
#include <QRegularExpression>

#include <memory>

namespace Utils {
class FilePath;
using FilePaths = QList<FilePath>;
} // Utils

namespace CppEditor { class ProjectPart; }

namespace ProjectExplorer { class Project; }

namespace Cppcheck::Internal {

class CppcheckRunner;
class CppcheckDiagnosticManager;

class CppcheckTool final : public QObject
{
    Q_OBJECT

public:
    CppcheckTool(CppcheckDiagnosticManager &manager, const Utils::Id &progressId);
    ~CppcheckTool() override;

    void updateOptions();
    void setProject(ProjectExplorer::Project *project);
    void check(const Utils::FilePaths &files);
    void stop(const Utils::FilePaths &files);

    void startParsing();
    void parseOutputLine(const QString &line);
    void parseErrorLine(const QString &line);
    void finishParsing();

private:
    void updateArguments();
    void addToQueue(const Utils::FilePaths &files, const CppEditor::ProjectPart &part);
    QStringList additionalArguments(const CppEditor::ProjectPart &part) const;

    CppcheckDiagnosticManager &m_manager;
    QPointer<ProjectExplorer::Project> m_project;
    std::unique_ptr<CppcheckRunner> m_runner;
    std::unique_ptr<QFutureInterface<void>> m_progress;
    QHash<QString, QString> m_cachedAdditionalArguments;
    QVector<QRegularExpression> m_filters;
    QRegularExpression m_progressRegexp;
    QRegularExpression m_messageRegexp;
    Utils::Id m_progressId;
};

} // Cppcheck::Internal
