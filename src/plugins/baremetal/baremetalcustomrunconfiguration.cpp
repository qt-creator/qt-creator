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

#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <qtsupport/qtoutputformatter.h>
#include <utils/detailswidget.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <QLineEdit>

using namespace Utils;
using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

class BareMetalCustomRunConfigWidget : public RunConfigWidget
{
    Q_OBJECT

public:
    BareMetalCustomRunConfigWidget(BareMetalCustomRunConfiguration *runConfig)
        : m_runConfig(runConfig)
    {
        auto const mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);
        auto const detailsContainer = new DetailsWidget(this);
        mainLayout->addWidget(detailsContainer);
        detailsContainer->setState(DetailsWidget::NoSummary);
        auto const detailsWidget = new QWidget(this);
        detailsContainer->setWidget(detailsWidget);

        auto exeLabel = new QLabel(tr("Executable:"));
        auto executableChooser = new PathChooser;
        executableChooser->setExpectedKind(PathChooser::File);
        executableChooser->setPath(m_runConfig->localExecutableFilePath());

        auto wdirLabel = new QLabel(tr("Work directory:"));
        auto workdirChooser = new PathChooser;
        workdirChooser->setExpectedKind(PathChooser::Directory);
        workdirChooser->setPath(m_runConfig->workingDirectory());

        auto clayout = new QFormLayout(this);
        detailsWidget->setLayout(clayout);

        clayout->addRow(exeLabel, executableChooser);
        runConfig->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, clayout);
        clayout->addRow(wdirLabel, workdirChooser);

        connect(executableChooser, &PathChooser::pathChanged,
                this, &BareMetalCustomRunConfigWidget::handleLocalExecutableChanged);
        connect(workdirChooser, &PathChooser::pathChanged,
                this, &BareMetalCustomRunConfigWidget::handleWorkingDirChanged);
        connect(this, &BareMetalCustomRunConfigWidget::setWorkdir,
                workdirChooser, &PathChooser::setPath);
    }

signals:
    void setWorkdir(const QString workdir);

private:
    void handleLocalExecutableChanged(const QString &path)
    {
        m_runConfig->setLocalExecutableFilePath(path.trimmed());
        if (m_runConfig->workingDirectory().isEmpty()) {
            QFileInfo fi(path);
            emit setWorkdir(fi.dir().canonicalPath());
            handleWorkingDirChanged(fi.dir().canonicalPath());
        }
    }

    void handleWorkingDirChanged(const QString &wd)
    {
        m_runConfig->setWorkingDirectory(wd.trimmed());
    }

private:
    QString displayName() const { return m_runConfig->displayName(); }

    BareMetalCustomRunConfiguration * const m_runConfig;
};

BareMetalCustomRunConfiguration::BareMetalCustomRunConfiguration(ProjectExplorer::Target *parent)
    : BareMetalRunConfiguration(parent, runConfigId(), QString())
{
}

BareMetalCustomRunConfiguration::BareMetalCustomRunConfiguration(ProjectExplorer::Target *parent,
        BareMetalCustomRunConfiguration *source)
    : BareMetalRunConfiguration(parent, source)
    , m_localExecutable(source->m_localExecutable)
{
}

bool BareMetalCustomRunConfiguration::isConfigured() const
{
    return !m_localExecutable.isEmpty();
}

ProjectExplorer::RunConfiguration::ConfigurationState
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
    return new BareMetalCustomRunConfigWidget(this);
}

Utils::OutputFormatter *BareMetalCustomRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

Core::Id BareMetalCustomRunConfiguration::runConfigId()
{
    return "BareMetal.CustomRunConfig";
}

QString BareMetalCustomRunConfiguration::runConfigDefaultDisplayName()
{
    return tr("Custom Executable (on GDB server or hardware debugger)");
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

} // namespace Internal
} // namespace BareMetal

#include "baremetalcustomrunconfiguration.moc"
