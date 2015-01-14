/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BAREMETALGDBCOMMANDSDEPLOYSTEP_H
#define BAREMETALGDBCOMMANDSDEPLOYSTEP_H

#include <projectexplorer/buildstep.h>

#include <QVariantMap>
#include <QPlainTextEdit>

namespace BareMetal {
namespace Internal {

class BareMetalGdbCommandsDeployStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    BareMetalGdbCommandsDeployStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    BareMetalGdbCommandsDeployStep(ProjectExplorer::BuildStepList *bsl,
                                   BareMetalGdbCommandsDeployStep *other);
    ~BareMetalGdbCommandsDeployStep();

    bool init();
    void run(QFutureInterface<bool> &fi);
    bool runInGuiThread() const { return true;}

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    static Core::Id stepId();
    static QString displayName();

    void updateGdbCommands(const QString &newCommands);
    QString gdbCommands() const;

private:
    void ctor();
    QString m_gdbCommands;
};

const char GdbCommandsKey[] = "BareMetal.GdbCommandsStep.Commands";

class BareMetalGdbCommandsDeployStepWidget: public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit BareMetalGdbCommandsDeployStepWidget(BareMetalGdbCommandsDeployStep &step);

public slots:
    void update();

private:
    QString displayName() const;
    QString summaryText() const;

    BareMetalGdbCommandsDeployStep &m_step;
    QPlainTextEdit *m_commands;
};

} // namespace Internal
} // namespace BareMetal

#endif // BAREMETALGDBCOMMANDSDEPLOYSTEP_H
