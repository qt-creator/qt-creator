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

#include "plugindialog.h"

#include "icore.h"

#include "dialogs/restartdialog.h"

#include <app/app_version.h>

#include <extensionsystem/plugindetailsview.h>
#include <extensionsystem/pluginerrorview.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/pluginview.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/environment.h>
#include <utils/fancylineedit.h>
#include <utils/infolabel.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

using namespace Utils;

namespace Core {
namespace Internal {

static bool s_isRestartRequired = false;

const char kPath[] = "Path";
const char kApplicationInstall[] = "ApplicationInstall";

static bool hasLibSuffix(const QString &path)
{
    return (HostOsInfo().isWindowsHost() && path.endsWith(".dll", Qt::CaseInsensitive))
           || (HostOsInfo().isLinuxHost() && QFileInfo(path).completeSuffix().startsWith(".so"))
           || (HostOsInfo().isMacHost() && path.endsWith(".dylib"));
}

static bool isZipFile(const QString &path)
{
    const QList<MimeType> mimeType = mimeTypesForFileName(path);
    return anyOf(mimeType, [](const MimeType &mt) { return mt.inherits("application/zip"); });
}

struct Tool
{
    FilePath executable;
    QStringList arguments;
};

static Utils::optional<Tool> unzipTool(const FilePath &src, const FilePath &dest)
{
    const FilePath unzip = Utils::Environment::systemEnvironment().searchInPath(
        Utils::HostOsInfo::withExecutableSuffix("unzip"));
    if (!unzip.isEmpty())
        return Tool{unzip, {"-o", src.toString(), "-d", dest.toString()}};

    const FilePath sevenzip = Utils::Environment::systemEnvironment().searchInPath(
        Utils::HostOsInfo::withExecutableSuffix("7z"));
    if (!sevenzip.isEmpty())
        return Tool{sevenzip, {"x", QString("-o") + dest.toString(), "-y", src.toString()}};

    const FilePath cmake = Utils::Environment::systemEnvironment().searchInPath(
        Utils::HostOsInfo::withExecutableSuffix("cmake"));
    if (!cmake.isEmpty())
        return Tool{cmake, {"-E", "tar", "xvf", src.toString()}};

    return {};
}

class SourcePage : public WizardPage
{
public:
    SourcePage(QWidget *parent)
        : WizardPage(parent)
    {
        setTitle(PluginDialog::tr("Source"));
        auto vlayout = new QVBoxLayout;
        setLayout(vlayout);

        auto label = new QLabel(
            "<p>"
            + PluginDialog::tr(
                "Choose source location. This can be a plugin library file or a zip file.")
            + "</p>");
        label->setWordWrap(true);
        vlayout->addWidget(label);

        auto path = new PathChooser;
        path->setExpectedKind(PathChooser::Any);
        vlayout->addWidget(path);
        registerFieldWithName(kPath, path, "path", SIGNAL(pathChanged(QString)));
        connect(path, &PathChooser::pathChanged, this, &SourcePage::updateWarnings);

        m_info = new InfoLabel;
        m_info->setType(InfoLabel::Error);
        m_info->setVisible(false);
        vlayout->addWidget(m_info);
    }

    void updateWarnings()
    {
        m_info->setVisible(!isComplete());
        emit completeChanged();
    }

    bool isComplete() const
    {
        const QString path = field(kPath).toString();
        if (!QFile::exists(path)) {
            m_info->setText(PluginDialog::tr("File does not exist."));
            return false;
        }
        if (hasLibSuffix(path))
            return true;

        if (!isZipFile(path)) {
            m_info->setText(PluginDialog::tr("File format not supported."));
            return false;
        }
        if (!unzipTool({}, {})) {
            m_info->setText(
                PluginDialog::tr("Could not find unzip, 7z, or cmake executable in PATH."));
            return false;
        }
        return true;
    }

    InfoLabel *m_info = nullptr;
};

class InstallLocationPage : public WizardPage
{
public:
    InstallLocationPage(QWidget *parent)
        : WizardPage(parent)
    {
        setTitle(PluginDialog::tr("Install Location"));
        auto vlayout = new QVBoxLayout;
        setLayout(vlayout);

        auto label = new QLabel("<p>" + PluginDialog::tr("Choose install location.") + "</p>");
        label->setWordWrap(true);
        vlayout->addWidget(label);
        vlayout->addSpacing(10);

        auto localInstall = new QRadioButton(PluginDialog::tr("User plugins"));
        localInstall->setChecked(true);
        auto localLabel = new QLabel(
            PluginDialog::tr("The plugin will be available to all compatible %1 "
                             "installations, but only for the current user.")
                .arg(Constants::IDE_DISPLAY_NAME));
        localLabel->setWordWrap(true);
        localLabel->setAttribute(Qt::WA_MacSmallSize, true);

        vlayout->addWidget(localInstall);
        vlayout->addWidget(localLabel);
        vlayout->addSpacing(10);

        auto appInstall = new QRadioButton(
            PluginDialog::tr("%1 installation").arg(Constants::IDE_DISPLAY_NAME));
        auto appLabel = new QLabel(
            PluginDialog::tr("The plugin will be available only to this %1 "
                             "installation, but for all users that can access it.")
                .arg(Constants::IDE_DISPLAY_NAME));
        appLabel->setWordWrap(true);
        appLabel->setAttribute(Qt::WA_MacSmallSize, true);
        vlayout->addWidget(appInstall);
        vlayout->addWidget(appLabel);

        auto group = new QButtonGroup(this);
        group->addButton(localInstall);
        group->addButton(appInstall);

        registerFieldWithName(kApplicationInstall, this);
        setField(kApplicationInstall, false);
        connect(appInstall, &QRadioButton::toggled, this, [this](bool toggled) {
            setField(kApplicationInstall, toggled);
        });
    }
};

static FilePath pluginInstallPath(QWizard *wizard)
{
    return FilePath::fromString(wizard->field(kApplicationInstall).toBool()
                                    ? ICore::pluginPath()
                                    : ICore::userPluginPath());
}

static FilePath pluginFilePath(QWizard *wizard)
{
    return FilePath::fromVariant(wizard->field(kPath));
}

class SummaryPage : public WizardPage
{
public:
    SummaryPage(QWidget *parent)
        : WizardPage(parent)
    {
        setTitle(PluginDialog::tr("Summary"));

        auto vlayout = new QVBoxLayout;
        setLayout(vlayout);

        m_summaryLabel = new QLabel(this);
        m_summaryLabel->setWordWrap(true);
        vlayout->addWidget(m_summaryLabel);
    }

    void initializePage()
    {
        m_summaryLabel->setText(PluginDialog::tr("\"%1\" will be installed into \"%2\".")
                                    .arg(pluginFilePath(wizard()).toUserOutput(),
                                         pluginInstallPath(wizard()).toUserOutput()));
    }

private:
    QLabel *m_summaryLabel;
};

PluginDialog::PluginDialog(QWidget *parent)
    : QDialog(parent),
      m_view(new ExtensionSystem::PluginView(this))
{
    auto vl = new QVBoxLayout(this);

    auto filterLayout = new QHBoxLayout;
    vl->addLayout(filterLayout);
    auto filterEdit = new Utils::FancyLineEdit(this);
    filterEdit->setFiltering(true);
    connect(filterEdit, &Utils::FancyLineEdit::filterChanged,
            m_view, &ExtensionSystem::PluginView::setFilter);
    filterLayout->addWidget(filterEdit);
    m_view->setShowHidden(false);
    auto showHidden = new QCheckBox(tr("Show all"));
    showHidden->setToolTip(tr("Show all installed plugins, including base plugins "
                              "and plugins that are not available on this platform."));
    showHidden->setChecked(m_view->isShowingHidden());
    connect(showHidden, &QCheckBox::stateChanged,
            m_view, &ExtensionSystem::PluginView::setShowHidden);
    filterLayout->addWidget(showHidden);

    vl->addWidget(m_view);

    m_detailsButton = new QPushButton(tr("Details"), this);
    m_errorDetailsButton = new QPushButton(tr("Error Details"), this);
    m_closeButton = new QPushButton(tr("Close"), this);
    m_installButton = new QPushButton(tr("Install Plugin..."), this);
    m_detailsButton->setEnabled(false);
    m_errorDetailsButton->setEnabled(false);
    m_closeButton->setEnabled(true);
    m_closeButton->setDefault(true);

    m_restartRequired = new QLabel(tr("Restart required."), this);
    if (!s_isRestartRequired)
        m_restartRequired->setVisible(false);

    auto hl = new QHBoxLayout;
    hl->addWidget(m_detailsButton);
    hl->addWidget(m_errorDetailsButton);
    hl->addWidget(m_installButton);
    hl->addSpacing(10);
    hl->addWidget(m_restartRequired);
    hl->addStretch(5);
    hl->addWidget(m_closeButton);

    vl->addLayout(hl);

    resize(650, 400);
    setWindowTitle(tr("Installed Plugins"));

    connect(m_view, &ExtensionSystem::PluginView::currentPluginChanged,
            this, &PluginDialog::updateButtons);
    connect(m_view, &ExtensionSystem::PluginView::pluginActivated,
            this, &PluginDialog::openDetails);
    connect(m_view, &ExtensionSystem::PluginView::pluginSettingsChanged,
            this, &PluginDialog::updateRestartRequired);
    connect(m_detailsButton, &QAbstractButton::clicked,
            [this]  { openDetails(m_view->currentPlugin()); });
    connect(m_errorDetailsButton, &QAbstractButton::clicked,
            this, &PluginDialog::openErrorDetails);
    connect(m_installButton, &QAbstractButton::clicked, this, &PluginDialog::showInstallWizard);
    connect(m_closeButton, &QAbstractButton::clicked, this, &PluginDialog::closeDialog);
    updateButtons();
}

void PluginDialog::closeDialog()
{
    ExtensionSystem::PluginManager::writeSettings();
    if (s_isRestartRequired) {
        RestartDialog restartDialog(ICore::dialogParent(),
                                    tr("Plugin changes will take effect after restart."));
        restartDialog.exec();
    }
    accept();
}

static bool copyPluginFile(const FilePath &src, const FilePath &dest)
{
    const FilePath destFile = dest.pathAppended(src.fileName());
    if (QFile::exists(destFile.toString())) {
        QMessageBox box(QMessageBox::Question,
                        PluginDialog::tr("Overwrite File"),
                        PluginDialog::tr("The file \"%1\" exists. Overwrite?")
                            .arg(destFile.toUserOutput()),
                        QMessageBox::Cancel,
                        ICore::dialogParent());
        QPushButton *acceptButton = box.addButton(PluginDialog::tr("Overwrite"), QMessageBox::AcceptRole);
        box.setDefaultButton(acceptButton);
        box.exec();
        if (box.clickedButton() != acceptButton)
            return false;
        QFile::remove(destFile.toString());
    }
    QDir(dest.toString()).mkpath(".");
    if (!QFile::copy(src.toString(), destFile.toString())) {
        QMessageBox::warning(ICore::dialogParent(),
                             PluginDialog::tr("Failed to Write File"),
                             PluginDialog::tr("Failed to write file \"%1\".")
                                 .arg(destFile.toUserOutput()));
        return false;
    }
    return true;
}

static bool unzip(const FilePath &src, const FilePath &dest)
{
    const Utils::optional<Tool> tool = unzipTool(src, dest);
    QTC_ASSERT(tool, return false);
    const QString workingDirectory = dest.toFileInfo().absoluteFilePath();
    QDir(workingDirectory).mkpath(".");
    CheckableMessageBox box(ICore::dialogParent());
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(PluginDialog::tr("Unzipping File"));
    box.setText(PluginDialog::tr("Unzipping \"%1\" to \"%2\".")
                    .arg(src.toUserOutput(), dest.toUserOutput()));
    box.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    box.button(QDialogButtonBox::Ok)->setEnabled(false);
    box.setCheckBoxVisible(false);
    box.setDetailedText(
        PluginDialog::tr("Running %1\nin \"%2\".\n\n", "Running <cmd> in <workingdirectory>")
            .arg(CommandLine(tool->executable, tool->arguments).toUserOutput(), workingDirectory));
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    QObject::connect(&process, &QProcess::readyReadStandardOutput, &box, [&box, &process]() {
        box.setDetailedText(box.detailedText() + QString::fromUtf8(process.readAllStandardOutput()));
    });
    QObject::connect(&process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [&box](int, QProcess::ExitStatus) {
                         box.button(QDialogButtonBox::Ok)->setEnabled(true);
                         box.button(QDialogButtonBox::Cancel)->setEnabled(false);
                     });
    QObject::connect(&box, &QMessageBox::rejected, &process, [&process] {
        SynchronousProcess::stopProcess(process);
    });
    process.setProgram(tool->executable.toString());
    process.setArguments(tool->arguments);
    process.setWorkingDirectory(workingDirectory);
    process.start(QProcess::ReadOnly);
    box.exec();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void PluginDialog::showInstallWizard()
{
    Wizard wizard(ICore::dialogParent());
    wizard.setWindowTitle(tr("Install Plugin"));

    auto filePage = new SourcePage(&wizard);
    wizard.addPage(filePage);

    auto installLocationPage = new InstallLocationPage(&wizard);
    wizard.addPage(installLocationPage);

    auto summaryPage = new SummaryPage(&wizard);
    wizard.addPage(summaryPage);

    if (wizard.exec()) {
        const FilePath path = pluginFilePath(&wizard);
        const FilePath installPath = pluginInstallPath(&wizard);
        if (hasLibSuffix(path.toString())) {
            if (copyPluginFile(path, installPath))
                updateRestartRequired();
        } else if (isZipFile(path.toString())) {
            if (unzip(path, installPath))
                updateRestartRequired();
        }
    }
}

void PluginDialog::updateRestartRequired()
{
    // just display the notice all the time after once changing something
    s_isRestartRequired = true;
    m_restartRequired->setVisible(true);
}

void PluginDialog::updateButtons()
{
    ExtensionSystem::PluginSpec *selectedSpec = m_view->currentPlugin();
    if (selectedSpec) {
        m_detailsButton->setEnabled(true);
        m_errorDetailsButton->setEnabled(selectedSpec->hasError());
    } else {
        m_detailsButton->setEnabled(false);
        m_errorDetailsButton->setEnabled(false);
    }
}

void PluginDialog::openDetails(ExtensionSystem::PluginSpec *spec)
{
    if (!spec)
        return;
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Plugin Details of %1").arg(spec->name()));
    auto layout = new QVBoxLayout;
    dialog.setLayout(layout);
    auto details = new ExtensionSystem::PluginDetailsView(&dialog);
    layout->addWidget(details);
    details->update(spec);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(400, 500);
    dialog.exec();
}

void PluginDialog::openErrorDetails()
{
    ExtensionSystem::PluginSpec *spec = m_view->currentPlugin();
    if (!spec)
        return;
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Plugin Errors of %1").arg(spec->name()));
    auto layout = new QVBoxLayout;
    dialog.setLayout(layout);
    auto errors = new ExtensionSystem::PluginErrorView(&dialog);
    layout->addWidget(errors);
    errors->update(spec);
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close, Qt::Horizontal, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.resize(500, 300);
    dialog.exec();
}

} // namespace Internal
} // namespace Core
