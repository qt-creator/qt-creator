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

#include <projectexplorer/buildsteplist.h>

#include <QObject>

namespace IncrediBuild {
namespace Internal {

class CommandBuilder
{
public:
    CommandBuilder(ProjectExplorer::BuildStep *buildStep) : m_buildStep(buildStep) {}
    virtual ~CommandBuilder() = default;

    virtual bool canMigrate(ProjectExplorer::BuildStepList*) { return false; }

    ProjectExplorer::BuildStep* buildStep() { return m_buildStep; }

    virtual QString displayName() const { return QString("Custom Command"); }

    virtual bool fromMap(const QVariantMap &map);
    virtual void toMap(QVariantMap *map) const;

    virtual QString defaultCommand() { return QString(); }
    virtual QStringList defaultArguments() { return QStringList(); }
    virtual QString setMultiProcessArg(QString args) { return args; }

    void reset();

    QString command() { return m_command.isEmpty() ? defaultCommand() : m_command; }
    void command(const QString &command) { m_command = command; }

    QStringList arguments() { return m_argsSet ? m_args : defaultArguments(); }
    void arguments(const QStringList &arguments) { m_args = arguments; m_argsSet = !arguments.isEmpty(); }
    void arguments(const QString &arguments);

    bool keepJobNum() const { return m_keepJobNum; }
    void keepJobNum(bool keepJobNum) { m_keepJobNum = keepJobNum; }

    QString fullCommandFlag();

private:
    ProjectExplorer::BuildStep *m_buildStep{};
    QString m_command{};
    QStringList m_args{};
    bool m_argsSet{false}; // Args may be overridden to empty
    bool m_keepJobNum{false};
};

} // namespace Internal
} // namespace IncrediBuild
