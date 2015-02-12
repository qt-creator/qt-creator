/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef REMOTELINUXCUSTOMRUNCONFIGURATION_H
#define REMOTELINUXCUSTOMRUNCONFIGURATION_H

#include "abstractremotelinuxrunconfiguration.h"

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxCustomRunConfiguration : public AbstractRemoteLinuxRunConfiguration
{
    Q_OBJECT
public:
    RemoteLinuxCustomRunConfiguration(ProjectExplorer::Target *parent);
    RemoteLinuxCustomRunConfiguration(ProjectExplorer::Target *parent,
                                      RemoteLinuxCustomRunConfiguration *source);

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

    bool isEnabled() const { return true; }
    bool isConfigured() const;
    ConfigurationState ensureConfigured(QString *errorMessage);
    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;

    QString localExecutableFilePath() const { return m_localExecutable; }
    QString remoteExecutableFilePath() const { return m_remoteExecutable; }
    QStringList arguments() const { return m_arguments; }
    QString workingDirectory() const { return m_workingDirectory; }
    Utils::Environment environment() const;

    void setLocalExecutableFilePath(const QString &executable) { m_localExecutable = executable; }
    void setRemoteExecutableFilePath(const QString &executable);
    void setArguments(const QStringList &args) { m_arguments = args; }
    void setWorkingDirectory(const QString &wd) { m_workingDirectory = wd; }

    static Core::Id runConfigId();
    static QString runConfigDefaultDisplayName();

private:
    void init();

    QString m_localExecutable;
    QString m_remoteExecutable;
    QStringList m_arguments;
    QString m_workingDirectory;

};

} // namespace Internal
} // namespace RemoteLinux

#endif // Include guard.
