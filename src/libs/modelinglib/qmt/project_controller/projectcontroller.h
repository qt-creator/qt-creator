/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#include "qmt/infrastructure/exceptions.h"
#include "qmt/infrastructure/qmt_global.h"

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
    void fileNameChanged(const QString &fileName);

public:
    Project *project() const { return m_project.data(); }
    bool isModified() const { return m_isModified; }

    void newProject(const QString &fileName);
    void setFileName(const QString &fileName);
    void setModified();

    void load();
    void save();
    void saveAs(const QString &fileName);

private:
    QScopedPointer<Project> m_project;
    bool m_isModified = false;
};

} // namespace qmt
