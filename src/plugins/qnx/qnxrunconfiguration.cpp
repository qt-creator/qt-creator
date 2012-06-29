/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxrunconfiguration.h"
#include "qnxconstants.h"

#include <remotelinux/remotelinuxrunconfigurationwidget.h>

#include <QLabel>
#include <QLineEdit>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char QtLibPathKey[] = "Qt4ProjectManager.QnxRunConfiguration.QtLibPath";
}

QnxRunConfiguration::QnxRunConfiguration(ProjectExplorer::Target *parent, const Core::Id id, const QString &proFilePath)
    : RemoteLinux::RemoteLinuxRunConfiguration(parent, id, proFilePath)
{
}

QnxRunConfiguration::QnxRunConfiguration(ProjectExplorer::Target *parent, QnxRunConfiguration *source)
    : RemoteLinux::RemoteLinuxRunConfiguration(parent, source)
    , m_qtLibPath(source->m_qtLibPath)
{
}

void QnxRunConfiguration::setQtLibPath(const QString &path)
{
    m_qtLibPath = path;
}

QString QnxRunConfiguration::environmentPreparationCommand() const
{
    QString command;
    const QStringList filesToSource = QStringList() << QLatin1String("/etc/profile")
        << QLatin1String("$HOME/.profile");
    foreach (const QString &filePath, filesToSource)
        command += QString::fromLatin1("test -f %1 && . %1;").arg(filePath);
    if (!workingDirectory().isEmpty())
        command += QLatin1String("cd ") + workingDirectory() + QLatin1String(";");

    if (!m_qtLibPath.isEmpty())
        command += QLatin1String("LD_LIBRARY_PATH=") + m_qtLibPath + QLatin1String(":$LD_LIBRARY_PATH");
    else
        command.chop(1); // Trailing semicolon.

    return command;
}

QWidget *QnxRunConfiguration::createConfigurationWidget()
{
    RemoteLinux::RemoteLinuxRunConfigurationWidget *rcWidget =
            qobject_cast<RemoteLinux::RemoteLinuxRunConfigurationWidget *>(RemoteLinux::RemoteLinuxRunConfiguration::createConfigurationWidget());

    QLabel *label = new QLabel(tr("Path to Qt libraries on device:"));
    QLineEdit *lineEdit = new QLineEdit(m_qtLibPath);

    connect(lineEdit, SIGNAL(textChanged(QString)), this, SLOT(setQtLibPath(QString)));

    rcWidget->addFormLayoutRow(label, lineEdit);

    return rcWidget;
}

QVariantMap QnxRunConfiguration::toMap() const
{
    QVariantMap map(RemoteLinux::RemoteLinuxRunConfiguration::toMap());
    map.insert(QLatin1String(QtLibPathKey), m_qtLibPath);
    return map;
}

bool QnxRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RemoteLinux::RemoteLinuxRunConfiguration::fromMap(map))
        return false;

    setQtLibPath(map.value(QLatin1String(QtLibPathKey)).toString());
    return true;
}
