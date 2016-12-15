/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVersionNumber>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Ios {
namespace Internal {

class IosToolChainFactory : public ProjectExplorer::ToolChainFactory
{
    Q_OBJECT

public:
    QSet<Core::Id> supportedLanguages() const override;
    QList<ProjectExplorer::ToolChain *> autoDetect(const QList<ProjectExplorer::ToolChain *> &existingToolChains) override;
};


class IosConfigurations : public QObject
{
    Q_OBJECT

public:
    static QObject *instance();
    static void initialize();
    static bool ignoreAllDevices();
    static void setIgnoreAllDevices(bool ignoreDevices);
    static Utils::FileName developerPath();
    static QVersionNumber xcodeVersion();
    static Utils::FileName lldbPath();
    static void updateAutomaticKitList();

private:
    IosConfigurations(QObject *parent);
    void load();
    void save();
    void updateSimulators();
    static void setDeveloperPath(const Utils::FileName &devPath);

    Utils::FileName m_developerPath;
    QVersionNumber m_xcodeVersion;
    bool m_ignoreAllDevices;
};

} // namespace Internal
} // namespace Ios
