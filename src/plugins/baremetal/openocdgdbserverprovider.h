/****************************************************************************
**
** Copyright (C) 2014 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OPENOCDGDBSERVERPROVIDER_H
#define OPENOCDGDBSERVERPROVIDER_H

#include "gdbserverprovider.h"

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

class OpenOcdGdbServerProviderConfigWidget;
class OpenOcdGdbServerProviderFactory;

class OpenOcdGdbServerProvider : public GdbServerProvider
{
public:
    QString typeDisplayName() const;

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);

    bool operator==(const GdbServerProvider &) const;

    GdbServerProviderConfigWidget *configurationWidget();
    GdbServerProvider *clone() const;

    QString channel() const;
    QString executable() const;
    QStringList arguments() const;

    bool canStartupMode(StartupMode mode) const;
    bool isValid() const;

private:
    explicit OpenOcdGdbServerProvider();
    explicit OpenOcdGdbServerProvider(const OpenOcdGdbServerProvider &);

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    QString m_host;
    quint16 m_port;
    QString m_executableFile;
    QString m_rootScriptsDir;
    QString m_configurationFile;
    QString m_additionalArguments;

    friend class OpenOcdGdbServerProviderConfigWidget;
    friend class OpenOcdGdbServerProviderFactory;
};

class OpenOcdGdbServerProviderFactory : public GdbServerProviderFactory
{
    Q_OBJECT

public:
    explicit OpenOcdGdbServerProviderFactory();

    GdbServerProvider *create();

    bool canRestore(const QVariantMap &data);
    GdbServerProvider *restore(const QVariantMap &data);

    GdbServerProviderConfigWidget *configurationWidget(GdbServerProvider *);
};

class OpenOcdGdbServerProviderConfigWidget : public GdbServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit OpenOcdGdbServerProviderConfigWidget(OpenOcdGdbServerProvider *);

private slots:
    void startupModeChanged();

private:
    void applyImpl();
    void discardImpl();

    void setFromProvider();

    QPointer<HostWidget> m_hostWidget;
    QPointer<Utils::PathChooser> m_executableFileChooser;
    QPointer<Utils::PathChooser> m_rootScriptsDirChooser;
    QPointer<Utils::PathChooser> m_configurationFileChooser;
    QPointer<QLineEdit> m_additionalArgumentsLineEdit;
    QPointer<QPlainTextEdit> m_initCommandsTextEdit;
    QPointer<QPlainTextEdit> m_resetCommandsTextEdit;
};

} // namespace Internal
} // namespace BareMetal

#endif // OPENOCDGDBSERVERPROVIDER_H
