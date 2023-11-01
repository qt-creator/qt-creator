// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugininstallwizard.h"

#include "coreplugin.h"
#include "coreplugintr.h"
#include "icore.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/temporarydirectory.h>
#include <utils/unarchiver.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>

#include <QButtonGroup>
#include <QDir>
#include <QDirIterator>
#include <QGuiApplication>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>

#include <memory>

using namespace ExtensionSystem;
using namespace Tasking;
using namespace Utils;

struct Data
{
    FilePath sourcePath;
    FilePath extractedPath;
    bool installIntoApplication = false;
};

static QStringList libraryNameFilter()
{
    if (HostOsInfo::isWindowsHost())
        return {"*.dll"};
    if (HostOsInfo::isLinuxHost())
        return {"*.so"};
    return {"*.dylib"};
}

static bool hasLibSuffix(const FilePath &path)
{
    return (HostOsInfo::isWindowsHost() && path.endsWith(".dll"))
           || (HostOsInfo::isLinuxHost() && path.completeSuffix().startsWith("so"))
           || (HostOsInfo::isMacHost() && path.endsWith(".dylib"));
}

static FilePath pluginInstallPath(bool installIntoApplication)
{
    return FilePath::fromString(installIntoApplication ? Core::ICore::pluginPath()
                                                       : Core::ICore::userPluginPath());
}

namespace Core {
namespace Internal {

class SourcePage : public WizardPage
{
public:
    SourcePage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Source"));

        auto label = new QLabel(
            "<p>"
            + Tr::tr("Choose source location. This can be a plugin library file or a zip file.")
            + "</p>");
        label->setWordWrap(true);

        auto chooser = new PathChooser;
        chooser->setExpectedKind(PathChooser::Any);
        connect(chooser, &PathChooser::textChanged, this, [this, chooser] {
            m_data->sourcePath = chooser->filePath();
            updateWarnings();
        });

        m_info = new InfoLabel;
        m_info->setType(InfoLabel::Error);
        m_info->setVisible(false);

        Layouting::Column { label, chooser, m_info }.attachTo(this);
    }

    void updateWarnings()
    {
        m_info->setVisible(!isComplete());
        emit completeChanged();
    }

    bool isComplete() const final
    {
        const FilePath path = m_data->sourcePath;
        if (!QFileInfo::exists(path.toString())) {
            m_info->setText(Tr::tr("File does not exist."));
            return false;
        }
        if (hasLibSuffix(path))
            return true;

        const auto sourceAndCommand = Unarchiver::sourceAndCommand(path);
        if (!sourceAndCommand)
            m_info->setText(sourceAndCommand.error());

        return bool(sourceAndCommand);
    }

    int nextId() const final
    {
        if (hasLibSuffix(m_data->sourcePath))
            return WizardPage::nextId() + 1; // jump over check archive
        return WizardPage::nextId();
    }

    InfoLabel *m_info = nullptr;
    Data *m_data = nullptr;
};

struct ArchiveIssue
{
    QString message;
    InfoLabel::InfoType type;
};

// Async. Result is set if any issue was found.
void checkContents(QPromise<ArchiveIssue> &promise, const FilePath &tempDir)
{
    PluginSpec *coreplugin = PluginManager::specForPlugin(CorePlugin::instance());

    // look for plugin
    QDirIterator it(tempDir.path(), libraryNameFilter(), QDir::Files | QDir::NoSymLinks,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (promise.isCanceled())
            return;
        it.next();
        PluginSpec *spec = PluginSpec::read(it.filePath());
        if (spec) {
            // Is a Qt Creator plugin. Let's see if we find a Core dependency and check the
            // version
            const QVector<PluginDependency> dependencies = spec->dependencies();
            const auto found = std::find_if(dependencies.constBegin(), dependencies.constEnd(),
                [coreplugin](const PluginDependency &d) { return d.name == coreplugin->name(); });
            if (found == dependencies.constEnd())
                return;
            if (coreplugin->provides(found->name, found->version))
                return;
            promise.addResult(
                ArchiveIssue{Tr::tr("Plugin requires an incompatible version of %1 (%2).")
                                 .arg(QGuiApplication::applicationDisplayName(), found->version),
                             InfoLabel::Error});
            return; // successful / no error
        }
    }
    promise.addResult(
        ArchiveIssue{Tr::tr("Did not find %1 plugin.").arg(QGuiApplication::applicationDisplayName()),
                     InfoLabel::Error});
}

class CheckArchivePage : public WizardPage
{
public:
    CheckArchivePage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Check Archive"));

        m_label = new InfoLabel;
        m_label->setElideMode(Qt::ElideNone);
        m_label->setWordWrap(true);
        m_cancelButton = new QPushButton(Tr::tr("Cancel"));
        connect(m_cancelButton, &QPushButton::clicked, this, [this] {
            m_taskTree.reset();
            m_cancelButton->setVisible(false);
            m_label->setType(InfoLabel::Information);
            m_label->setText(Tr::tr("Canceled."));
        });
        m_output = new QTextEdit;
        m_output->setReadOnly(true);

        using namespace Layouting;
        Column {
            Row { m_label, st, m_cancelButton },
            m_output,
        }.attachTo(this);
    }

    void initializePage() final
    {
        m_isComplete = false;
        emit completeChanged();

        m_tempDir = std::make_unique<TemporaryDirectory>("plugininstall");
        m_data->extractedPath = m_tempDir->path();
        m_label->setText(Tr::tr("Checking archive..."));
        m_label->setType(InfoLabel::None);

        m_output->clear();

        const auto sourceAndCommand = Unarchiver::sourceAndCommand(m_data->sourcePath);
        if (!sourceAndCommand) {
            m_label->setType(InfoLabel::Error);
            m_label->setText(sourceAndCommand.error());
            return;
        }

        const auto onUnarchiverSetup = [this, sourceAndCommand](Unarchiver &unarchiver) {
            unarchiver.setSourceAndCommand(*sourceAndCommand);
            unarchiver.setDestDir(m_tempDir->path());
            connect(&unarchiver, &Unarchiver::outputReceived, this, [this](const QString &output) {
                m_output->append(output);
            });
        };
        const auto onUnarchiverError = [this](const Unarchiver &) {
            m_label->setType(InfoLabel::Error);
            m_label->setText(Tr::tr("There was an error while unarchiving."));
        };

        const auto onCheckerSetup = [this](Async<ArchiveIssue> &async) {
            if (!m_tempDir)
                return SetupResult::StopWithError;

            async.setConcurrentCallData(checkContents, m_tempDir->path());
            async.setFutureSynchronizer(PluginManager::futureSynchronizer());
            return SetupResult::Continue;
        };
        const auto onCheckerDone = [this](const Async<ArchiveIssue> &async) {
            m_isComplete = !async.isResultAvailable();
            if (m_isComplete) {
                m_label->setType(InfoLabel::Ok);
                m_label->setText(Tr::tr("Archive is OK."));
            } else {
                const ArchiveIssue issue = async.result();
                m_label->setType(issue.type);
                m_label->setText(issue.message);
            }
            emit completeChanged();
        };

        const Group root {
            UnarchiverTask(onUnarchiverSetup, {}, onUnarchiverError),
            AsyncTask<ArchiveIssue>(onCheckerSetup, onCheckerDone)
        };
        m_taskTree.reset(new TaskTree(root));

        const auto onEnd = [this] {
            m_cancelButton->setVisible(false);
            m_taskTree.release()->deleteLater();
        };
        connect(m_taskTree.get(), &TaskTree::done, this, onEnd);
        connect(m_taskTree.get(), &TaskTree::errorOccurred, this, onEnd);

        m_cancelButton->setVisible(true);
        m_taskTree->start();
    }

    void cleanupPage() final
    {
        // back button pressed
        m_taskTree.reset();
        m_tempDir.reset();
    }

    bool isComplete() const final { return m_isComplete; }

    std::unique_ptr<TemporaryDirectory> m_tempDir;
    std::unique_ptr<TaskTree> m_taskTree;
    InfoLabel *m_label = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTextEdit *m_output = nullptr;
    Data *m_data = nullptr;
    bool m_isComplete = false;
};

class InstallLocationPage : public WizardPage
{
public:
    InstallLocationPage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Install Location"));

        auto label = new QLabel("<p>" + Tr::tr("Choose install location.") + "</p>");
        label->setWordWrap(true);

        auto localInstall = new QRadioButton(Tr::tr("User plugins"));
        localInstall->setChecked(!m_data->installIntoApplication);
        auto localLabel = new QLabel(Tr::tr("The plugin will be available to all compatible %1 "
                                            "installations, but only for the current user.")
                                         .arg(QGuiApplication::applicationDisplayName()));
        localLabel->setWordWrap(true);
        localLabel->setAttribute(Qt::WA_MacSmallSize, true);

        auto appInstall = new QRadioButton(
            Tr::tr("%1 installation").arg(QGuiApplication::applicationDisplayName()));
        appInstall->setChecked(m_data->installIntoApplication);
        auto appLabel = new QLabel(Tr::tr("The plugin will be available only to this %1 "
                                          "installation, but for all users that can access it.")
                                       .arg(QGuiApplication::applicationDisplayName()));
        appLabel->setWordWrap(true);
        appLabel->setAttribute(Qt::WA_MacSmallSize, true);

        using namespace Layouting;
        Column {
            label, Space(10), localInstall, localLabel, Space(10), appInstall, appLabel,
        }.attachTo(this);

        auto group = new QButtonGroup(this);
        group->addButton(localInstall);
        group->addButton(appInstall);

        connect(appInstall, &QRadioButton::toggled, this, [this](bool toggled) {
            m_data->installIntoApplication = toggled;
        });
    }

    Data *m_data = nullptr;
};

class SummaryPage : public WizardPage
{
public:
    SummaryPage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Summary"));

        m_summaryLabel = new QLabel(this);
        m_summaryLabel->setWordWrap(true);
        Layouting::Column { m_summaryLabel }.attachTo(this);
    }

    void initializePage() final
    {
        m_summaryLabel->setText(
            Tr::tr("\"%1\" will be installed into \"%2\".")
                .arg(m_data->sourcePath.toUserOutput(),
                     pluginInstallPath(m_data->installIntoApplication).toUserOutput()));
    }

private:
    QLabel *m_summaryLabel;
    Data *m_data = nullptr;
};

static std::function<void(FilePath)> postCopyOperation()
{
    return [](const FilePath &filePath) {
        if (!HostOsInfo::isMacHost())
            return;
        // On macOS, downloaded files get a quarantine flag, remove it, otherwise it is a hassle
        // to get it loaded as a plugin in Qt Creator.
        Process xattr;
        xattr.setTimeoutS(1);
        xattr.setCommand({"/usr/bin/xattr", {"-d", "com.apple.quarantine", filePath.absoluteFilePath().toString()}});
        xattr.runBlocking();
    };
}

static bool copyPluginFile(const FilePath &src, const FilePath &dest)
{
    const FilePath destFile = dest.pathAppended(src.fileName());
    if (destFile.exists()) {
        QMessageBox box(QMessageBox::Question,
                        Tr::tr("Overwrite File"),
                        Tr::tr("The file \"%1\" exists. Overwrite?").arg(destFile.toUserOutput()),
                        QMessageBox::Cancel,
                        ICore::dialogParent());
        QPushButton *acceptButton = box.addButton(Tr::tr("Overwrite"), QMessageBox::AcceptRole);
        box.setDefaultButton(acceptButton);
        box.exec();
        if (box.clickedButton() != acceptButton)
            return false;
        destFile.removeFile();
    }
    dest.parentDir().ensureWritableDir();
    if (!src.copyFile(destFile)) {
        QMessageBox::warning(ICore::dialogParent(),
                             Tr::tr("Failed to Write File"),
                             Tr::tr("Failed to write file \"%1\".").arg(destFile.toUserOutput()));
        return false;
    }
    postCopyOperation()(destFile);
    return true;
}

bool PluginInstallWizard::exec()
{
    Wizard wizard(ICore::dialogParent());
    wizard.setWindowTitle(Tr::tr("Install Plugin"));

    Data data;

    auto filePage = new SourcePage(&data, &wizard);
    wizard.addPage(filePage);

    auto checkArchivePage = new CheckArchivePage(&data, &wizard);
    wizard.addPage(checkArchivePage);

    auto installLocationPage = new InstallLocationPage(&data, &wizard);
    wizard.addPage(installLocationPage);

    auto summaryPage = new SummaryPage(&data, &wizard);
    wizard.addPage(summaryPage);

    if (wizard.exec()) {
        const FilePath installPath = pluginInstallPath(data.installIntoApplication);
        if (hasLibSuffix(data.sourcePath)) {
            return copyPluginFile(data.sourcePath, installPath);
        } else {
            QString error;
            if (!FileUtils::copyRecursively(data.extractedPath,
                                            installPath,
                                            &error,
                                            FileUtils::CopyAskingForOverwrite(ICore::dialogParent(),
                                                                              postCopyOperation()))) {
                QMessageBox::warning(ICore::dialogParent(),
                                     Tr::tr("Failed to Copy Plugin Files"),
                                     error);
                return false;
            }
            return true;
        }
    }
    return false;
}

} // namespace Internal
} // namespace Core
