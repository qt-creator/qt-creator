/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
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

#include "baremetalcustomrunconfiguration.h"

#include <projectexplorer/target.h>
#include <qtsupport/qtoutputformatter.h>
#include <utils/detailswidget.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>

#include <QDir>
#include <QFileInfo>
#include <QVBoxLayout>
#include <QLabel>
#include <QString>
#include <QLineEdit>

using namespace Utils;

namespace BareMetal {
namespace Internal {

class BareMetalCustomRunConfigWidget : public ProjectExplorer::RunConfigWidget
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
        auto argumentsLabel = new QLabel(tr("Arguments:"));
        auto arguments = new QLineEdit();
        arguments->setText(m_runConfig->arguments());
        auto wdirLabel = new QLabel(tr("Work directory:"));
        auto workdirChooser = new PathChooser;
        workdirChooser->setExpectedKind(PathChooser::Directory);
        workdirChooser->setPath(m_runConfig->workingDirectory());

        auto clayout = new QGridLayout(this);
        detailsWidget->setLayout(clayout);

        clayout->addWidget(exeLabel, 0, 0);
        clayout->addWidget(executableChooser, 0, 1);
        clayout->addWidget(argumentsLabel, 1, 0);
        clayout->addWidget(arguments, 1, 1);
        clayout->addWidget(wdirLabel, 2, 0);
        clayout->addWidget(workdirChooser, 2, 1);

        connect(executableChooser, &PathChooser::pathChanged,
                this, &BareMetalCustomRunConfigWidget::handleLocalExecutableChanged);
        connect(arguments, &QLineEdit::textChanged,
                this, &BareMetalCustomRunConfigWidget::handleArgumentsChanged);
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

    void handleArgumentsChanged(const QString &arguments)
    {
        m_runConfig->setArguments(arguments.trimmed());
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
