/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <projectexplorer/buildstep.h>

#include <QCoreApplication>

namespace IncrediBuild {
namespace Internal {

class CommandBuilder
{
    Q_DECLARE_TR_FUNCTIONS(IncrediBuild::Internal::CommandBuilder)

public:
    CommandBuilder(ProjectExplorer::BuildStep *buildStep) : m_buildStep(buildStep) {}
    virtual ~CommandBuilder() = default;

    virtual QList<Utils::Id> migratableSteps() const { return {}; }

    ProjectExplorer::BuildStep *buildStep() const { return m_buildStep; }

    virtual QString id() const { return "CustomCommandBuilder"; }
    virtual QString displayName() const { return tr("Custom Command"); }

    virtual void fromMap(const QVariantMap &map);
    virtual void toMap(QVariantMap *map) const;

    virtual Utils::FilePath defaultCommand() const { return {}; }
    virtual QString defaultArguments() const { return QString(); }
    virtual QString setMultiProcessArg(QString args) { return args; }

    Utils::FilePath command() const { return m_command; }
    void setCommand(const Utils::FilePath &command);
    Utils::FilePath effectiveCommand() const { return m_command.isEmpty() ? defaultCommand() : m_command; }

    QString arguments() { return m_args.isEmpty() ? defaultArguments() : m_args; }
    void setArguments(const QString &arguments);

private:
    ProjectExplorer::BuildStep *m_buildStep{};
    Utils::FilePath m_command;
    QString m_args;
};

} // namespace Internal
} // namespace IncrediBuild
