/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "plugininstallwizard.h"

#include "icore.h"

#include <utils/archive.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>

#include <app/app_version.h>

#include <QButtonGroup>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

using namespace Utils;

const char kPath[] = "Path";
const char kApplicationInstall[] = "ApplicationInstall";

static bool hasLibSuffix(const FilePath &path)
{
    return (HostOsInfo().isWindowsHost() && path.endsWith(".dll"))
           || (HostOsInfo().isLinuxHost() && path.toFileInfo().completeSuffix().startsWith(".so"))
           || (HostOsInfo().isMacHost() && path.endsWith(".dylib"));
}

namespace Core {
namespace Internal {

class SourcePage : public WizardPage
{
public:
    SourcePage(QWidget *parent)
        : WizardPage(parent)
    {
        setTitle(PluginInstallWizard::tr("Source"));
        auto vlayout = new QVBoxLayout;
        setLayout(vlayout);

        auto label = new QLabel(
            "<p>"
            + PluginInstallWizard::tr(
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
        const auto path = FilePath::fromVariant(field(kPath));
        if (!QFile::exists(path.toString())) {
            m_info->setText(PluginInstallWizard::tr("File does not exist."));
            return false;
        }
        if (hasLibSuffix(path))
            return true;

        QString error;
        if (!Archive::supportsFile(path, &error)) {
            m_info->setText(error);
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
        setTitle(PluginInstallWizard::tr("Install Location"));
        auto vlayout = new QVBoxLayout;
        setLayout(vlayout);

        auto label = new QLabel("<p>" + PluginInstallWizard::tr("Choose install location.")
                                + "</p>");
        label->setWordWrap(true);
        vlayout->addWidget(label);
        vlayout->addSpacing(10);

        auto localInstall = new QRadioButton(PluginInstallWizard::tr("User plugins"));
        localInstall->setChecked(true);
        auto localLabel = new QLabel(
            PluginInstallWizard::tr("The plugin will be available to all compatible %1 "
                                    "installations, but only for the current user.")
                .arg(Constants::IDE_DISPLAY_NAME));
        localLabel->setWordWrap(true);
        localLabel->setAttribute(Qt::WA_MacSmallSize, true);

        vlayout->addWidget(localInstall);
        vlayout->addWidget(localLabel);
        vlayout->addSpacing(10);

        auto appInstall = new QRadioButton(
            PluginInstallWizard::tr("%1 installation").arg(Constants::IDE_DISPLAY_NAME));
        auto appLabel = new QLabel(
            PluginInstallWizard::tr("The plugin will be available only to this %1 "
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
        setTitle(PluginInstallWizard::tr("Summary"));

        auto vlayout = new QVBoxLayout;
        setLayout(vlayout);

        m_summaryLabel = new QLabel(this);
        m_summaryLabel->setWordWrap(true);
        vlayout->addWidget(m_summaryLabel);
    }

    void initializePage()
    {
        m_summaryLabel->setText(PluginInstallWizard::tr("\"%1\" will be installed into \"%2\".")
                                    .arg(pluginFilePath(wizard()).toUserOutput(),
                                         pluginInstallPath(wizard()).toUserOutput()));
    }

private:
    QLabel *m_summaryLabel;
};

static bool copyPluginFile(const FilePath &src, const FilePath &dest)
{
    const FilePath destFile = dest.pathAppended(src.fileName());
    if (QFile::exists(destFile.toString())) {
        QMessageBox box(QMessageBox::Question,
                        PluginInstallWizard::tr("Overwrite File"),
                        PluginInstallWizard::tr("The file \"%1\" exists. Overwrite?")
                            .arg(destFile.toUserOutput()),
                        QMessageBox::Cancel,
                        ICore::dialogParent());
        QPushButton *acceptButton = box.addButton(PluginInstallWizard::tr("Overwrite"),
                                                  QMessageBox::AcceptRole);
        box.setDefaultButton(acceptButton);
        box.exec();
        if (box.clickedButton() != acceptButton)
            return false;
        QFile::remove(destFile.toString());
    }
    QDir(dest.toString()).mkpath(".");
    if (!QFile::copy(src.toString(), destFile.toString())) {
        QMessageBox::warning(ICore::dialogParent(),
                             PluginInstallWizard::tr("Failed to Write File"),
                             PluginInstallWizard::tr("Failed to write file \"%1\".")
                                 .arg(destFile.toUserOutput()));
        return false;
    }
    return true;
}

bool PluginInstallWizard::exec()
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
        if (hasLibSuffix(path)) {
            if (copyPluginFile(path, installPath))
                return true;
        } else if (Archive::supportsFile(path)) {
            if (Archive::unarchive(path, installPath, ICore::dialogParent()))
                return true;
        }
    }
    return false;
}

} // namespace Internal
} // namespace Core
