/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "remotelinuxenvironmentaspectwidget.h"

#include "remotelinuxenvironmentreader.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("RemoteLinux::RemoteLinuxEnvironmentAspectWidget",
                                  "Fetch Device Environment");
} // anonymous namespace

namespace RemoteLinux {

RemoteLinuxEnvironmentAspectWidget::RemoteLinuxEnvironmentAspectWidget(RemoteLinuxEnvironmentAspect *aspect) :
    ProjectExplorer::EnvironmentAspectWidget(aspect, new QPushButton),
    deviceEnvReader(new Internal::RemoteLinuxEnvironmentReader(aspect->runConfiguration(), this))
{
    QPushButton *button = fetchButton();
    button->setText(FetchEnvButtonText);
    connect(button, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(deviceEnvReader, SIGNAL(finished()), this, SLOT(fetchEnvironmentFinished()));
    connect(deviceEnvReader, SIGNAL(error(QString)), this, SLOT(fetchEnvironmentError(QString)));
}

RemoteLinuxEnvironmentAspect *RemoteLinuxEnvironmentAspectWidget::aspect() const
{
    return dynamic_cast<RemoteLinuxEnvironmentAspect *>(EnvironmentAspectWidget::aspect());
}

QPushButton *RemoteLinuxEnvironmentAspectWidget::fetchButton() const
{
    return qobject_cast<QPushButton *>(additionalWidget());
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironment()
{
    QPushButton *button = fetchButton();
    disconnect(button, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(button, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    button->setText(tr("Cancel Fetch Operation"));
    deviceEnvReader->start(aspect()->runConfiguration()->environmentPreparationCommand());
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentFinished()
{
    QPushButton *button = fetchButton();
    disconnect(button, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    connect(button, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    button->setText(FetchEnvButtonText);
    aspect()->setRemoteEnvironment(deviceEnvReader->remoteEnvironment());
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device Error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment()
{
    deviceEnvReader->stop();
    fetchEnvironmentFinished();
}

// --------------------------------------------------------------------
// RemoteLinuxEnvironmentAspectWidget:
// --------------------------------------------------------------------

} // namespace RemoteLinux
