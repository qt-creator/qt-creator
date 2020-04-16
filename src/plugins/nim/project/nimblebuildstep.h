/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <projectexplorer/abstractprocessstep.h>

namespace Nim {

class NimbleBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    NimbleBuildStep(ProjectExplorer::BuildStepList *parentList, Core::Id id);

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    QString arguments() const;

    void setArguments(const QString &args);

    void resetArguments();

    bool fromMap(const QVariantMap &map) override;

    QVariantMap toMap() const override;

signals:
    void argumentsChanged(const QString &args);

private:
    QString defaultArguments() const;

    void onArgumentsChanged();

    QString m_arguments;
};

class NimbleBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    NimbleBuildStepFactory();
};

}
