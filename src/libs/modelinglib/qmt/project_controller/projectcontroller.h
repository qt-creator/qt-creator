// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/exceptions.h"
#include "qmt/infrastructure/qmt_global.h"

#include <utils/filepath.h>

#include <QObject>
#include <QString>

namespace qmt {

class Project;

class QMT_EXPORT NoFileNameException : public Exception
{
public:
    NoFileNameException();
};

class QMT_EXPORT ProjectIsModifiedException : public Exception
{
public:
    ProjectIsModifiedException();
};

class QMT_EXPORT ProjectController : public QObject
{
    Q_OBJECT

public:
    explicit ProjectController(QObject *parent = nullptr);
    ~ProjectController() override;

signals:
    void changed();
    void fileNameChanged(const Utils::FilePath &fileName);

public:
    Project *project() const { return m_project.data(); }
    bool isModified() const { return m_isModified; }

    void newProject(const Utils::FilePath &fileName);
    void setFileName(const Utils::FilePath &fileName);
    void setModified();

    void load();
    void save();
    void saveAs(const Utils::FilePath &fileName);

private:
    QScopedPointer<Project> m_project;
    bool m_isModified = false;
};

} // namespace qmt
