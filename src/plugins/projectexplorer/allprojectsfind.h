// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basefilefind.h>

#include <QPointer>

namespace ProjectExplorer {

class Project;

namespace Internal {

class AllProjectsFind : public TextEditor::BaseFileFind
{
    Q_OBJECT

public:
    AllProjectsFind();

    QString id() const override;
    QString displayName() const override;

    bool isEnabled() const override;

    QWidget *createConfigWidget() override;
    void writeSettings(Utils::QtcSettings *settings) override;
    void readSettings(Utils::QtcSettings *settings) override;

protected:
    static Utils::FileContainer filesForProjects(const QStringList &nameFilters,
                                                 const QStringList &exclusionFilters,
                                                 const QList<Project *> &projects);
    QString label() const override;
    QString toolTip() const override;

private:
    TextEditor::FileContainerProvider fileContainerProvider() const override;
    void handleFileListChanged();

    QPointer<QWidget> m_configWidget;
};

} // namespace Internal
} // namespace ProjectExplorer
