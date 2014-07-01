/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxrunconfiguration.h"
#include "qnxconstants.h"

#include <remotelinux/remotelinuxrunconfigurationwidget.h>
#include <utils/environment.h>

#include <QLabel>
#include <QLineEdit>

using namespace Qnx;
using namespace Qnx::Internal;

namespace {
const char QtLibPathKey[] = "Qt4ProjectManager.QnxRunConfiguration.QtLibPath";
}

QnxRunConfiguration::QnxRunConfiguration(ProjectExplorer::Target *parent, Core::Id id, const QString &proFilePath)
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

Utils::Environment QnxRunConfiguration::environment() const
{
    Utils::Environment env = RemoteLinuxRunConfiguration::environment();
    if (!m_qtLibPath.isEmpty()) {
        env.appendOrSet(QLatin1String("LD_LIBRARY_PATH"),
                        m_qtLibPath + QLatin1String("/lib:$LD_LIBRARY_PATH"));
        env.appendOrSet(QLatin1String("QML_IMPORT_PATH"),
                        m_qtLibPath + QLatin1String("/imports:$QML_IMPORT_PATH"));
        env.appendOrSet(QLatin1String("QML2_IMPORT_PATH"),
                        m_qtLibPath + QLatin1String("/qml:$QML2_IMPORT_PATH"));
        env.appendOrSet(QLatin1String("QT_PLUGIN_PATH"),
                        m_qtLibPath + QLatin1String("/plugins:$QT_PLUGIN_PATH"));
        env.set(QLatin1String("QT_QPA_FONTDIR"),
                        m_qtLibPath + QLatin1String("/lib/fonts"));
    }

    return env;
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
