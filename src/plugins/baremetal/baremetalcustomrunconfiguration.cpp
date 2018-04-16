/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
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

#include "baremetalcustomrunconfiguration.h"

#include "baremetalconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <qtsupport/qtoutputformatter.h>
#include <utils/pathchooser.h>

#include <QFormLayout>

using namespace Utils;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

class BareMetalCustomRunConfigWidget : public RunConfigWidget
{
public:
    BareMetalCustomRunConfigWidget(BareMetalCustomRunConfiguration *runConfig)
        : m_runConfig(runConfig)
    {
        auto executableChooser = new PathChooser;
        executableChooser->setExpectedKind(PathChooser::File);
        executableChooser->setPath(m_runConfig->localExecutableFilePath());

        auto clayout = new QFormLayout(this);
        clayout->addRow(BareMetalCustomRunConfiguration::tr("Executable:"), executableChooser);

        runConfig->extraAspect<ArgumentsAspect>()->addToConfigurationLayout(clayout);
        runConfig->extraAspect<WorkingDirectoryAspect>()->addToConfigurationLayout(clayout);

        connect(executableChooser, &PathChooser::pathChanged,
                this, &BareMetalCustomRunConfigWidget::handleLocalExecutableChanged);
    }

private:
    void handleLocalExecutableChanged(const QString &path)
    {
        m_runConfig->setLocalExecutableFilePath(path.trimmed());
    }

    QString displayName() const { return m_runConfig->displayName(); }

    BareMetalCustomRunConfiguration * const m_runConfig;
};

BareMetalCustomRunConfiguration::BareMetalCustomRunConfiguration(Target *parent)
    : BareMetalRunConfiguration(parent)
{
}

bool BareMetalCustomRunConfiguration::isConfigured() const
{
    return !m_localExecutable.isEmpty();
}

RunConfiguration::ConfigurationState
BareMetalCustomRunConfiguration::ensureConfigured(QString *errorMessage)
{
    if (!isConfigured()) {
        if (errorMessage) {
            *errorMessage = tr("The remote executable must be set "
                               "in order to run a custom remote run configuration.");
        }
        return UnConfigured;
    }
    return Configured;
}

QWidget *BareMetalCustomRunConfiguration::createConfigurationWidget()
{
    return wrapWidget(new BareMetalCustomRunConfigWidget(this));
}

static QString exeKey()
{
    return QLatin1String("BareMetal.CustomRunConfig.Executable");
}

bool BareMetalCustomRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!BareMetalRunConfiguration::fromMap(map))
            return false;
    m_localExecutable = map.value(exeKey()).toString();
    return true;
}

QVariantMap BareMetalCustomRunConfiguration::toMap() const
{
    QVariantMap map = BareMetalRunConfiguration::toMap();
    map.insert(exeKey(), m_localExecutable);
    return map;
}

// BareMetalCustomRunConfigurationFactory

BareMetalCustomRunConfigurationFactory::BareMetalCustomRunConfigurationFactory() :
    FixedRunConfigurationFactory(BareMetalCustomRunConfiguration::tr("Custom Executable"), true)
{
    registerRunConfiguration<BareMetalCustomRunConfiguration>("BareMetal.CustomRunConfig");
    setDecorateDisplayNames(true);
    addSupportedTargetDeviceType(BareMetal::Constants::BareMetalOsType);
}

} // namespace Internal
} // namespace BareMetal
