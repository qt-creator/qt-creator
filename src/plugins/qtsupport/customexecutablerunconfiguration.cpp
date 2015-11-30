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

#include "customexecutablerunconfiguration.h"
#include "customexecutableconfigurationwidget.h"
#include "qtkitinformation.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/abi.h>

#include <coreplugin/icore.h>

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>

#include <QDir>

using namespace QtSupport;
using namespace QtSupport::Internal;
using namespace ProjectExplorer;

namespace {
const char CUSTOM_EXECUTABLE_ID[] = "ProjectExplorer.CustomExecutableRunConfiguration";

const char EXECUTABLE_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.Executable";
const char WORKING_DIRECTORY_KEY[] = "ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory";
}

void CustomExecutableRunConfiguration::ctor()
{
    setDefaultDisplayName(defaultDisplayName());
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *parent) :
    LocalApplicationRunConfiguration(parent, Core::Id(CUSTOM_EXECUTABLE_ID)),
    m_workingDirectory(QLatin1String(Constants::DEFAULT_WORKING_DIR)),
    m_dialog(0)
{
    addExtraAspect(new LocalEnvironmentAspect(this));
    addExtraAspect(new ArgumentsAspect(this, QStringLiteral("ProjectExplorer.CustomExecutableRunConfiguration.Arguments")));
    addExtraAspect(new TerminalAspect(this, QStringLiteral("ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal")));
    if (!parent->activeBuildConfiguration())
        m_workingDirectory = QLatin1String(Constants::DEFAULT_WORKING_DIR_ALTERNATE);
    ctor();
}

CustomExecutableRunConfiguration::CustomExecutableRunConfiguration(Target *parent,
                                                                   CustomExecutableRunConfiguration *source) :
    LocalApplicationRunConfiguration(parent, source),
    m_executable(source->m_executable),
    m_workingDirectory(source->m_workingDirectory),
    m_dialog(0)
{
    ctor();
}

// Note: Qt4Project deletes all empty customexecrunconfigs for which isConfigured() == false.
CustomExecutableRunConfiguration::~CustomExecutableRunConfiguration()
{
    if (m_dialog) {
        emit configurationFinished();
        disconnect(m_dialog, SIGNAL(finished(int)),
                   this, SLOT(configurationDialogFinished()));
        delete m_dialog;
    }
}

// Dialog embedding the CustomExecutableConfigurationWidget
// prompting the user to complete the configuration.
class CustomExecutableDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CustomExecutableDialog(CustomExecutableRunConfiguration *rc, QWidget *parent = 0);

    void accept();

    bool event(QEvent *event);

private slots:
    void changed()
    {
        setOkButtonEnabled(m_widget->isValid());
    }

private:
    inline void setOkButtonEnabled(bool e)
    {
        m_dialogButtonBox->button(QDialogButtonBox::Ok)->setEnabled(e);
    }

    QDialogButtonBox *m_dialogButtonBox;
    CustomExecutableConfigurationWidget *m_widget;
};

CustomExecutableDialog::CustomExecutableDialog(CustomExecutableRunConfiguration *rc, QWidget *parent)
    : QDialog(parent)
    , m_dialogButtonBox(new  QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel(tr("Could not find the executable, please specify one."));
    label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    layout->addWidget(label);
    m_widget = new CustomExecutableConfigurationWidget(rc, CustomExecutableConfigurationWidget::DelayedApply);
    m_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    connect(m_widget, SIGNAL(validChanged()), this, SLOT(changed()));
    layout->addWidget(m_widget);
    setOkButtonEnabled(false);
    connect(m_dialogButtonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_dialogButtonBox, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(m_dialogButtonBox);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
}

void CustomExecutableDialog::accept()
{
    m_widget->apply();
    QDialog::accept();
}

bool CustomExecutableDialog::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
            ke->accept();
            return true;
        }
    }
    return QDialog::event(event);
}

// CustomExecutableRunConfiguration

RunConfiguration::ConfigurationState CustomExecutableRunConfiguration::ensureConfigured(QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    if (m_dialog) {// uhm already shown
        *errorMessage = QLatin1String(""); // no error dialog
        m_dialog->activateWindow();
        m_dialog->raise();
        return UnConfigured;
    }

    m_dialog = new CustomExecutableDialog(this, Core::ICore::mainWindow());
    connect(m_dialog, SIGNAL(finished(int)),
            this, SLOT(configurationDialogFinished()));
    m_dialog->setWindowTitle(displayName()); // pretty pointless
    m_dialog->show();
    return Waiting;
}

void CustomExecutableRunConfiguration::configurationDialogFinished()
{
    disconnect(m_dialog, SIGNAL(finished(int)),
            this, SLOT(configurationDialogFinished()));
    m_dialog->deleteLater();
    m_dialog = 0;
    emit configurationFinished();
}

// Search the executable in the path.
bool CustomExecutableRunConfiguration::validateExecutable(QString *executable, QString *errorMessage) const
{
    if (executable)
        executable->clear();
    if (m_executable.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("No executable.");
        return false;
    }
    Utils::Environment env;
    EnvironmentAspect *aspect = extraAspect<EnvironmentAspect>();
    if (aspect)
        env = aspect->environment();
    const Utils::FileName exec = env.searchInPath(macroExpander()->expand(m_executable),
                                                  QStringList(workingDirectory()));
    if (exec.isEmpty()) {
        if (errorMessage)
            *errorMessage = tr("The executable\n%1\ncannot be found in the path.").
                            arg(QDir::toNativeSeparators(m_executable));
        return false;
    }
    if (executable)
        *executable = exec.toString();
    return true;
}

QString CustomExecutableRunConfiguration::executable() const
{
    QString result;
    validateExecutable(&result);
    return result;
}

QString CustomExecutableRunConfiguration::rawExecutable() const
{
    return m_executable;
}

bool CustomExecutableRunConfiguration::isConfigured() const
{
    return !m_executable.isEmpty();
}

ApplicationLauncher::Mode CustomExecutableRunConfiguration::runMode() const
{
    return extraAspect<TerminalAspect>()->runMode();
}

QString CustomExecutableRunConfiguration::workingDirectory() const
{
    EnvironmentAspect *aspect = extraAspect<EnvironmentAspect>();
    QTC_ASSERT(aspect, return baseWorkingDirectory());
    return QDir::cleanPath(aspect->environment().expandVariables(
                macroExpander()->expand(baseWorkingDirectory())));
}

QString CustomExecutableRunConfiguration::baseWorkingDirectory() const
{
    return m_workingDirectory;
}


QString CustomExecutableRunConfiguration::commandLineArguments() const
{
    return extraAspect<ArgumentsAspect>()->arguments();
}

QString CustomExecutableRunConfiguration::defaultDisplayName() const
{
    if (m_executable.isEmpty())
        return tr("Custom Executable");
    else
        return tr("Run %1").arg(QDir::toNativeSeparators(m_executable));
}

QVariantMap CustomExecutableRunConfiguration::toMap() const
{
    QVariantMap map(LocalApplicationRunConfiguration::toMap());
    map.insert(QLatin1String(EXECUTABLE_KEY), m_executable);
    map.insert(QLatin1String(WORKING_DIRECTORY_KEY), m_workingDirectory);
    return map;
}

bool CustomExecutableRunConfiguration::fromMap(const QVariantMap &map)
{
    m_executable = map.value(QLatin1String(EXECUTABLE_KEY)).toString();
    m_workingDirectory = map.value(QLatin1String(WORKING_DIRECTORY_KEY)).toString();

    setDefaultDisplayName(defaultDisplayName());
    return LocalApplicationRunConfiguration::fromMap(map);
}

void CustomExecutableRunConfiguration::setExecutable(const QString &executable)
{
    if (executable == m_executable)
        return;
    m_executable = executable;
    setDefaultDisplayName(defaultDisplayName());
    emit changed();
}

void CustomExecutableRunConfiguration::setCommandLineArguments(const QString &commandLineArguments)
{
    extraAspect<ArgumentsAspect>()->setArguments(commandLineArguments);
    emit changed();
}

void CustomExecutableRunConfiguration::setBaseWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
    emit changed();
}

void CustomExecutableRunConfiguration::setRunMode(ApplicationLauncher::Mode runMode)
{
    extraAspect<TerminalAspect>()->setRunMode(runMode);
    emit changed();
}

QWidget *CustomExecutableRunConfiguration::createConfigurationWidget()
{
    return new CustomExecutableConfigurationWidget(this, CustomExecutableConfigurationWidget::InstantApply);
}

Abi CustomExecutableRunConfiguration::abi() const
{
    return Abi(); // return an invalid ABI: We do not know what we will end up running!
}

// Factory

CustomExecutableRunConfigurationFactory::CustomExecutableRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{ setObjectName(QLatin1String("CustomExecutableRunConfigurationFactory")); }

CustomExecutableRunConfigurationFactory::~CustomExecutableRunConfigurationFactory()
{ }

bool CustomExecutableRunConfigurationFactory::canCreate(Target *parent, Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    return id == CUSTOM_EXECUTABLE_ID;
}

RunConfiguration *
CustomExecutableRunConfigurationFactory::doCreate(Target *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new CustomExecutableRunConfiguration(parent);
}

bool CustomExecutableRunConfigurationFactory::canRestore(Target *parent,
                                                         const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    Core::Id id(idFromMap(map));
    return canCreate(parent, id);
}

RunConfiguration *
CustomExecutableRunConfigurationFactory::doRestore(Target *parent, const QVariantMap &map)
{
    Q_UNUSED(map);
    return new CustomExecutableRunConfiguration(parent);
}

bool CustomExecutableRunConfigurationFactory::canClone(Target *parent,
                                                       RunConfiguration *source) const
{
    return canCreate(parent, source->id());
}

RunConfiguration *
CustomExecutableRunConfigurationFactory::clone(Target *parent, RunConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new CustomExecutableRunConfiguration(parent, static_cast<CustomExecutableRunConfiguration*>(source));
}

bool CustomExecutableRunConfigurationFactory::canHandle(Target *parent) const
{
    return parent->project()->supportsKit(parent->kit());
}

QList<Core::Id> CustomExecutableRunConfigurationFactory::availableCreationIds(Target *parent, CreationMode mode) const
{
    Q_UNUSED(mode)
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(CUSTOM_EXECUTABLE_ID);
}

QString CustomExecutableRunConfigurationFactory::displayNameForId(Core::Id id) const
{
    if (id == CUSTOM_EXECUTABLE_ID)
        return tr("Custom Executable");
    return QString();
}

#include "customexecutablerunconfiguration.moc"
