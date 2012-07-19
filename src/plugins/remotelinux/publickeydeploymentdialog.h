/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/
#ifndef PUBLICKEYDEPLOYMENTDIALOG_H
#define PUBLICKEYDEPLOYMENTDIALOG_H

#include "remotelinux_export.h"

#include <QSharedPointer>
#include <QProgressDialog>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace RemoteLinux {
class LinuxDeviceConfiguration;

namespace Internal {
class PublicKeyDeploymentDialogPrivate;
} // namespace Internal

class REMOTELINUX_EXPORT PublicKeyDeploymentDialog : public QProgressDialog
{
    Q_OBJECT
public:
    // Asks for public key and returns null if the file dialog is canceled.
    static PublicKeyDeploymentDialog *createDialog(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfig,
        QWidget *parent = 0);

    ~PublicKeyDeploymentDialog();

private slots:
    void handleDeploymentError(const QString &errorMsg);
    void handleDeploymentSuccess();
    void handleCanceled();

private:
    explicit PublicKeyDeploymentDialog(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfig,
        const QString &publicKeyFileName, QWidget *parent = 0);
    void handleDeploymentFinished(const QString &errorMsg);

    Internal::PublicKeyDeploymentDialogPrivate * const d;
};

} // namespace RemoteLinux

#endif // PUBLICKEYDEPLOYMENTDIALOG_H
