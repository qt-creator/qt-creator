// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugininstallwizard.h"

#include "coreplugin.h"
#include "coreplugintr.h"
#include "icore.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/async.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>
#include <utils/unarchiver.h>
#include <utils/wizard.h>
#include <utils/wizardpage.h>

#include <QButtonGroup>
#include <QCheckBox>
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
    std::unique_ptr<PluginSpec> pluginSpec = nullptr;
    bool loadImmediately = true;
    bool prepareForUpdate = false;
};

static bool hasLibSuffix(const FilePath &path)
{
    return (HostOsInfo::isWindowsHost() && path.endsWith(".dll"))
           || (HostOsInfo::isLinuxHost() && path.completeSuffix().startsWith("so"))
           || (HostOsInfo::isMacHost() && path.endsWith(".dylib"));
}

namespace Core {

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
        if (!QFileInfo::exists(path.toUrlishString())) {
            m_info->setText(Tr::tr("File does not exist."));
            return false;
        }
        if (hasLibSuffix(path)) {
            if (Utils::anyOf(PluginManager::pluginPaths(), [path](const FilePath &pluginPath) {
                    return path.isChildOf(pluginPath);
                })) {
                m_info->setText(Tr::tr("Plugin is already installed."));
                return false;
            }
            return true;
        }
        return true;
    }

    InfoLabel *m_info = nullptr;
    Data *m_data = nullptr;
};

using CheckResult = expected_str<PluginSpec *>;

static Result<> checkPlugin(PluginSpec *spec, bool update)
{
    const bool pluginAlreadyExists = PluginManager::specExists(spec->id());

    if (!update && pluginAlreadyExists) {
        return ResultError(
            Tr::tr("A plugin with ID \"%1\" is already installed.").arg(spec->id()));
    } else if (update && !pluginAlreadyExists) {
        return ResultError(Tr::tr("No plugin with ID \"%1\" is installed.").arg(spec->id()));
    }

    if (!spec->resolveDependencies(PluginManager::plugins())) {
        return ResultError(
            Tr::tr("Plugin failed to resolve dependencies:") + " " + spec->errorString());
    }
    return ResultOk;
}

static expected_str<std::unique_ptr<PluginSpec>> checkPlugin(
    expected_str<std::unique_ptr<PluginSpec>> spec, bool update)
{
    if (!spec)
        return spec;
    const Result<> ok = checkPlugin(spec->get(), update);
    if (ok)
        return spec;
    return Utils::make_unexpected(ok.error());
}

// Async. Result is set if any issue was found.
void checkContents(QPromise<CheckResult> &promise, const FilePath &tempDir, bool update)
{
    QList<PluginSpec *> plugins = pluginSpecsFromArchive(tempDir);
    if (plugins.isEmpty()) {
        promise.addResult(Utils::make_unexpected(Tr::tr("No plugins found.")));
        return;
    }
    if (plugins.size() > 1) {
        promise.addResult(Utils::make_unexpected(Tr::tr("More than one plugin found.")));
        qDeleteAll(plugins);
        return;
    }

    PluginSpec *plugin = plugins.front();
    const Result<> ok = checkPlugin(plugin, update);
    if (!ok) {
        promise.addResult(Utils::make_unexpected(ok.error()));
        delete plugin;
        return;
    }

    promise.addResult(plugin);
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
        m_label->setAlignment(Qt::AlignTop);
        m_cancelButton = new QPushButton(Tr::tr("Cancel"));
        m_cancelButton->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
        connect(m_cancelButton, &QPushButton::clicked, this, [this] {
            m_taskTreeRunner.reset();
            m_cancelButton->setVisible(false);
            m_label->setType(InfoLabel::Information);
            m_label->setText(Tr::tr("Canceled."));
        });
        m_output = new QTextEdit;
        m_output->setReadOnly(true);

        using namespace Layouting;
        Column {
            Row { m_label, m_cancelButton },
            m_output,
        }.attachTo(this);
    }

    void initializePage() final
    {
        m_isComplete = false;
        emit completeChanged();
        if (hasLibSuffix(m_data->sourcePath)) {
            m_cancelButton->setVisible(false);
            expected_str<std::unique_ptr<PluginSpec>> spec
                = checkPlugin(readCppPluginSpec(m_data->sourcePath), m_data->prepareForUpdate);
            if (!spec) {
                m_label->setType(InfoLabel::Error);
                m_label->setText(spec.error());
                return;
            }
            m_label->setType(InfoLabel::Ok);
            m_label->setText(Tr::tr("Archive is OK."));
            m_data->pluginSpec.swap(*spec);
            m_isComplete = true;
            return;
        }

        FilePath tmpDirBase;
        if (m_data->prepareForUpdate)
            tmpDirBase = appInfo().userResources / "install-prep-area";
        else
            tmpDirBase = TemporaryDirectory::masterDirectoryFilePath();

        tmpDirBase.ensureWritableDir();

        m_tempDir = std::make_unique<QTemporaryDir>((tmpDirBase / "plugininstall").path());
        m_tempDir->setAutoRemove(false);

        m_data->extractedPath = FilePath::fromString(m_tempDir->path());
        m_label->setText(Tr::tr("Checking archive..."));
        m_label->setType(InfoLabel::None);

        m_output->clear();

        const auto onUnarchiverSetup = [this](Unarchiver &unarchiver) {
            connect(&unarchiver, &Unarchiver::progress, this, [this](const FilePath &filePath) {
                m_output->append(filePath.toUserOutput());
            });

            unarchiver.setArchive(m_data->sourcePath);
            unarchiver.setDestination(FilePath::fromString(m_tempDir->path()));
        };

        const auto onUnarchiverDone = [this](const Unarchiver &unarchiver) {
            if (unarchiver.result()) {
                m_label->setType(InfoLabel::Ok);
                m_label->setText(Tr::tr("Archive extracted successfully."));
                return DoneResult::Success;
            }

            m_label->setType(InfoLabel::Error);
            m_label->setText(
                Tr::tr("There was an error while unarchiving: %1").arg(unarchiver.result().error()));
            return DoneResult::Error;
        };

        const auto onCheckerSetup = [this](Async<CheckResult> &async) {
            if (!m_data->extractedPath.exists())
                return SetupResult::StopWithError;

            async.setConcurrentCallData(
                checkContents, m_data->extractedPath, m_data->prepareForUpdate);
            return SetupResult::Continue;
        };
        const auto onCheckerDone = [this](const Async<CheckResult> &async) {
            expected_str<PluginSpec *> result = async.result();
            if (!result) {
                m_label->setType(InfoLabel::Error);
                m_label->setText(result.error());
            } else {
                m_label->setType(InfoLabel::Ok);
                m_label->setText(Tr::tr("Archive is OK."));
                m_data->pluginSpec.reset(*result);
                m_isComplete = true;
            }
            emit completeChanged();
        };

        // clang-format off
        const Group root{
            UnarchiverTask(onUnarchiverSetup, onUnarchiverDone),
            AsyncTask<CheckResult>(onCheckerSetup, onCheckerDone, CallDoneIf::Success)
        };
        // clang-format on
        m_cancelButton->setVisible(true);
        m_taskTreeRunner.start(root, {}, [this](DoneWith) { m_cancelButton->setVisible(false); });
    }

    void cleanupPage() final
    {
        // back button pressed
        m_taskTreeRunner.reset();
        if (m_tempDir) {
            m_tempDir->remove();
            m_tempDir.reset();
        }
    }

    bool isComplete() const final { return m_isComplete; }

    int nextId() const override
    {
        if (!m_data->pluginSpec || !m_data->pluginSpec->termsAndConditions())
            return WizardPage::nextId() + 1;
        return WizardPage::nextId();
    }

    std::unique_ptr<QTemporaryDir> m_tempDir;
    TaskTreeRunner m_taskTreeRunner;
    InfoLabel *m_label = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QTextEdit *m_output = nullptr;
    Data *m_data = nullptr;
    bool m_isComplete = false;
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
        m_summaryLabel->setTextFormat(Qt::MarkdownText);
        m_summaryLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        m_summaryLabel->setOpenExternalLinks(true);

        m_loadImmediately = new QCheckBox(Tr::tr("Load plugin immediately"));
        m_loadImmediately->setChecked(m_data->loadImmediately);
        connect(m_loadImmediately, &QCheckBox::toggled, this, [this](bool checked) {
            m_data->loadImmediately = checked;
        });

        // clang-format off
        using namespace Layouting;
        Column {
            m_summaryLabel,
            m_loadImmediately
        }.attachTo(this);
        // clang-format on
    }

    void initializePage() final
    {
        QTC_ASSERT(m_data && m_data->pluginSpec, return);
        const FilePath installLocation = m_data->pluginSpec->installLocation(true);
        installLocation.ensureWritableDir();

        m_summaryLabel->setText(
            Tr::tr("%1 will be installed into %2.")
                .arg(QString("[%1](%2)")
                         .arg(m_data->sourcePath.fileName())
                         .arg(m_data->sourcePath.parentDir().toUrl().toString(QUrl::FullyEncoded)))
                .arg(QString("[%1](%2)")
                         .arg(installLocation.fileName())
                         .arg(installLocation.toUrl().toString(QUrl::FullyEncoded))));

        m_loadImmediately->setVisible(
            m_data->pluginSpec && m_data->pluginSpec->isSoftLoadable() && !m_data->prepareForUpdate);
    }

private:
    QLabel *m_summaryLabel;
    QCheckBox *m_loadImmediately;
    Data *m_data = nullptr;
};

class AcceptTermsAndConditionsPage : public WizardPage
{
public:
    AcceptTermsAndConditionsPage(Data *data, QWidget *parent)
        : WizardPage(parent)
        , m_data(data)
    {
        setTitle(Tr::tr("Accept Terms and Conditions"));

        const QLatin1String legal = QLatin1String(
            "I confirm that I have reviewed and accept the terms and conditions\n"
            "of this extension. I confirm that I have the authority and ability to\n"
            "accept the terms and conditions of this extension for the customer.\n"
            "I acknowledge that if the customer and the Qt Company already have a\n"
            "valid agreement in place, that agreement shall apply, but these terms\n"
            "shall govern the use of this extension.");

        using namespace Layouting;
        // clang-format off
        Column {
            Label { bindTo(&m_intro) }, br,
            TextEdit {
                bindTo(&m_terms),
                readOnly(true),
            }, br,
            m_accept = new QCheckBox(legal)
        }.attachTo(this);
        // clang-format on

        connect(m_accept, &QCheckBox::toggled, this, [this]() { emit completeChanged(); });
    }

    void initializePage() final
    {
        QTC_ASSERT(m_data->pluginSpec, return);
        m_intro->setText(Tr::tr("The plugin %1 requires you to accept the following terms and "
                                "conditions:")
                             .arg(m_data->pluginSpec->name()));
        m_terms->setMarkdown(m_data->pluginSpec->termsAndConditions()->text);
    }

    bool isComplete() const final { return m_accept->isChecked(); }

private:
    Data *m_data = nullptr;
    QLabel *m_intro = nullptr;
    QCheckBox *m_accept = nullptr;
    QTextEdit *m_terms = nullptr;
};

static void postCopyOperation(FilePath filePath)
{
    if (!HostOsInfo::isMacHost())
        return;
    // On macOS, downloaded files get a quarantine flag, remove it, otherwise it is a hassle
    // to get it loaded as a plugin in Qt Creator.
    Process xattr;
    xattr.setCommand({"/usr/bin/xattr", {"-d", "com.apple.quarantine", filePath.absoluteFilePath().path()}});
    using namespace std::chrono_literals;
    xattr.runBlocking(1s);
}

static bool copyPluginFile(const FilePath &src, const FilePath &dest)
{
    const FilePath destFile = dest.pathAppended(src.fileName());
    QTC_ASSERT(src != destFile, return true);
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
    if (!destFile.parentDir().ensureWritableDir()) {
        QMessageBox::warning(
            ICore::dialogParent(),
            Tr::tr("Failed to Write File"),
            Tr::tr("Failed to create directory \"%1\".").arg(dest.toUserOutput()));
        return false;
    }
    if (!src.copyFile(destFile)) {
        QMessageBox::warning(ICore::dialogParent(),
                             Tr::tr("Failed to Write File"),
                             Tr::tr("Failed to write file \"%1\".").arg(destFile.toUserOutput()));
        return false;
    }
    postCopyOperation(destFile);
    return true;
}

QString extensionId(PluginSpec *spec)
{
    return spec->vendorId() + "." + spec->id();
}

InstallResult executePluginInstallWizard(const FilePath &archive, bool prepareForUpdate)
{
    Wizard wizard;
    wizard.setWindowTitle(Tr::tr("Install Plugin"));

    Data data;
    data.prepareForUpdate = prepareForUpdate;

    if (archive.isEmpty()) {
        auto filePage = new SourcePage(&data, &wizard);
        wizard.addPage(filePage);
    } else {
        data.sourcePath = archive;
    }

    auto checkArchivePage = new CheckArchivePage(&data, &wizard);
    wizard.addPage(checkArchivePage);

    auto acceptTAndCPage = new AcceptTermsAndConditionsPage(&data, &wizard);
    wizard.addPage(acceptTAndCPage);

    auto summaryPage = new SummaryPage(&data, &wizard);
    wizard.addPage(summaryPage);

    auto install = [&wizard, &data, prepareForUpdate]() {
        if (wizard.exec()) {
            const FilePath installPath = data.pluginSpec->installLocation(true)
                                         / extensionId(data.pluginSpec.get());

            if (prepareForUpdate) {
                const Result<> result = ExtensionSystem::PluginManager::removePluginOnRestart(
                    data.pluginSpec->id());
                if (!result) {
                    qWarning() << "Failed to remove plugin" << data.pluginSpec->id() << ":"
                               << result.error();
                    return false;
                }

                if (hasLibSuffix(data.sourcePath)) {
                    ExtensionSystem::PluginManager::installPluginOnRestart(
                        data.pluginSpec->filePath(), installPath);
                } else {
                    ExtensionSystem::PluginManager::installPluginOnRestart(
                        data.extractedPath, installPath);
                }
                return true;
            }

            if (hasLibSuffix(data.sourcePath)) {
                if (!copyPluginFile(data.sourcePath, installPath))
                    return false;

                auto specs = pluginSpecsFromArchive(installPath.resolvePath(
                    data.pluginSpec->filePath().relativePathFromDir(data.extractedPath)));

                QTC_ASSERT(specs.size() == 1, return false);
                data.pluginSpec.reset(specs.front());
                return true;
            } else {
                QString error;
                FileUtils::CopyAskingForOverwrite copy(&postCopyOperation);
                if (!FileUtils::copyRecursively(data.extractedPath, installPath, &error, copy())) {
                    QMessageBox::warning(
                        ICore::dialogParent(), Tr::tr("Failed to Copy Plugin Files"), error);
                    return false;
                }

                auto specs = pluginSpecsFromArchive(installPath.resolvePath(
                    data.pluginSpec->filePath().relativePathFromDir(data.extractedPath)));

                QTC_ASSERT(specs.size() == 1, return false);
                data.pluginSpec.reset(specs.front());

                return true;
            }
        }
        return false;
    };

    if (!install())
        return InstallResult::Error;

    // install() would have failed if the user did not accept the terms and conditions
    // so we can safely set them as accepted here.
    PluginManager::instance()->setTermsAndConditionsAccepted(data.pluginSpec.get());

    if (prepareForUpdate)
        return InstallResult::NeedsRestart;

    auto spec = data.pluginSpec.release();
    PluginManager::addPlugins({spec});

    if (spec->isEffectivelySoftloadable()) {
        spec->setEnabledBySettings(data.loadImmediately);
        if (data.loadImmediately) {
            PluginManager::loadPluginsAtRuntime({spec});
            ExtensionSystem::PluginManager::writeSettings();
        }
        return InstallResult::Success;
    }

    if (spec->isEffectivelyEnabled())
        return InstallResult::NeedsRestart;

    return InstallResult::Success;
}

} // namespace Core
