// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codegenerator.h"
#include "externaleditors.h"
#include "gettingstartedwelcomepage.h"
#include "profilereader.h"
#include "qscxmlcgenerator.h"
#include "qtkitaspect.h"
#include "qtoptionspage.h"
#include "qtoutputformatter.h"
#include "qtparser.h"
#include "qtprojectimporter.h"
#include "qtsupporttr.h"
#include "qttestparser.h"
#include "qtversionmanager.h"
#include "qtversions.h"
#include "translationwizardpage.h"
#include "uicgenerator.h"

#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/jsonwizard/jsonwizardfactory.h>
#include <projectexplorer/buildpropertiessettings.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <proparser/qmakeevaluator.h>

#include <utils/filepath.h>
#include <utils/infobar.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>

#include <QInputDialog>

using namespace Core;
using namespace Utils;
using namespace ProjectExplorer;

namespace QtSupport::Internal {

static void processRunnerCallback(ProcessData *data)
{
    FilePath rootPath = FilePath::fromString(data->deviceRoot);

    Process proc;
    // Docker and others do not support different processChannelModes (yet).
    // So we have to ignore what the caller wants here.
    //proc.setProcessChannelMode(data->processChannelMode);
    proc.setCommand({rootPath.withNewPath("/bin/sh"), {QString("-c"), data->command}});
    proc.setWorkingDirectory(FilePath::fromString(data->workingDirectory));
    proc.setEnvironment(Environment(data->environment.toStringList(), OsTypeLinux));

    proc.runBlocking();

    data->exitCode = proc.exitCode();
    data->exitStatus = proc.exitStatus();
    data->stdErr = proc.rawStdErr();
    data->stdOut = proc.rawStdOut();
}

class QtSupportPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QtSupport.json")

    void initialize() final;
    void extensionsInitialized() final;
    ShutdownFlag aboutToShutdown() final;
};

void QtSupportPlugin::initialize()
{
#ifdef WITH_TESTS
    addTestCreator(createQtOutputFormatterTest);
    addTestCreator(createQtBuildStringParserTest);
    addTestCreator(createQtOutputParserTest);
    addTestCreator(createQtTestParserTest);
    addTestCreator(createQtProjectImporterTest);
#endif

    setupQtVersionManager(this);

    setupDesktopQtVersion();
    setupEmbeddedLinuxQtVersion();
    setupGettingStartedWelcomePage();
    setupQtSettingsPage();
    setupQtOutputFormatter();
    setupUicGenerator(this);
    setupQScxmlcGenerator(this);

    setupExternalDesigner(this);
    setupExternalLinguist();

    setupTranslationWizardPage();

    theProcessRunner() = processRunnerCallback;

    thePrompter() = [this](const QString &msg, const QStringList &context) -> std::optional<QString> {
        std::optional<QString> res;
        QEventLoop loop;

        QMetaObject::invokeMethod(this, [msg, context, &res, &loop] {
            QString text;
            if (!context.isEmpty()) {
                text = "Preceding lines:<i><br>&nbsp;&nbsp;&nbsp;..."
                       + context.join("<br>&nbsp;&nbsp;&nbsp;")
                       + "</i><p>";
            }
            text += msg;
            bool ok = false;
            const QString line = QInputDialog::getText(
                ICore::dialogParent(),
                /*title*/ "QMake Prompt",
                /*label*/ text,
                /*echo mode*/ QLineEdit::Normal,
                /*text*/ QString(),
                /*ok*/ &ok,
                /*flags*/ Qt::WindowFlags(),
                /*QInputMethodHints*/ Qt::ImhNone);
            if (ok)
                res = line;
            loop.quit();
        }, Qt::QueuedConnection);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
        return res;
    };

    QMakeParser::initialize();
    ProFileEvaluator::initialize();
    new ProFileCacheManager(this);

    JsExpander::registerGlobalObject<CodeGenerator>("QtSupport");

    BuildPropertiesSettings::showQtSettings();

    QtVersionManager::initialized();
}

ExtensionSystem::IPlugin::ShutdownFlag QtSupportPlugin::aboutToShutdown()
{
    QtVersionManager::shutdown();
    return SynchronousShutdown;
}

const char kLinkWithQtInstallationSetting[] = "LinkWithQtInstallation";

static void askAboutQtInstallation()
{
    // if the install settings exist, the Qt Creator installation is (probably) already linked to
    // a Qt installation, so don't ask
    if (!LinkWithQtSupport::canLinkWithQt() || LinkWithQtSupport::isLinkedWithQt()
        || !ICore::infoBar()->canInfoBeAdded(kLinkWithQtInstallationSetting))
        return;

    Utils::InfoBarEntry info(
        kLinkWithQtInstallationSetting,
        Tr::tr(
            "Link with a Qt installation to automatically register Qt versions and kits? To do "
            "this later, select Edit > Preferences > Kits > Qt Versions > Link with Qt."),
        Utils::InfoBarEntry::GlobalSuppression::Enabled);
    info.setTitle(Tr::tr("Link with Qt"));
    info.addCustomButton(Tr::tr("Link with Qt"), [] {
        ICore::infoBar()->removeInfo(kLinkWithQtInstallationSetting);
        QTimer::singleShot(0, ICore::dialogParent(), &LinkWithQtSupport::linkWithQt);
    });
    ICore::infoBar()->addInfo(info);
}

void QtSupportPlugin::extensionsInitialized()
{
    Utils::MacroExpander *expander = Utils::globalMacroExpander();

    static const auto currentQtVersion = []() -> const QtVersion * {
        return QtKitAspect::qtVersion(activeKitForCurrentProject());
    };
    static const char kCurrentHostBins[] = "CurrentDocument:Project:QT_HOST_BINS";
    expander->registerVariable(
        kCurrentHostBins,
        Tr::tr("Full path to the host bin directory of the Qt version in the active kit "
               "of the project containing the current document."),
        []() {
            const QtVersion * const qt = currentQtVersion();
            return qt ? qt->hostBinPath().toUserOutput() : QString();
        });

    expander->registerVariable(
        "CurrentDocument:Project:QT_INSTALL_BINS",
        Tr::tr("Full path to the target bin directory of the Qt version in the active kit "
               "of the project containing the current document.<br>You probably want %1 instead.")
            .arg(QString::fromLatin1(kCurrentHostBins)),
        []() {
            const QtVersion * const qt = currentQtVersion();
            return qt ? qt->binPath().toUserOutput() : QString();
        });

    expander->registerVariable(
        "CurrentDocument:Project:QT_HOST_LIBEXECS",
        Tr::tr("Full path to the host libexec directory of the Qt version in the active kit "
               "of the project containing the current document."),
        []() {
            const QtVersion *const qt = currentQtVersion();
            return qt ? qt->hostLibexecPath().toUserOutput() : QString();
        });

    static const auto activeQtVersion = []() -> const QtVersion * {
        return QtKitAspect::qtVersion(activeKitForActiveProject());
    };
    static const char kActiveHostBins[] = "ActiveProject:QT_HOST_BINS";
    expander->registerVariable(
        kActiveHostBins,
        Tr::tr("Full path to the host bin directory of the Qt version in the active kit "
               "of the active project."),
        []() {
            const QtVersion * const qt = activeQtVersion();
            return qt ? qt->hostBinPath().toUserOutput() : QString();
        });

    expander->registerVariable(
        "ActiveProject:QT_INSTALL_BINS",
        Tr::tr("Full path to the target bin directory of the Qt version in the active kit "
               "of the active project.<br>You probably want %1 instead.")
            .arg(QString::fromLatin1(kActiveHostBins)),
        []() {
            const QtVersion * const qt = activeQtVersion();
            return qt ? qt->binPath().toUserOutput() : QString();
        });

    expander->registerVariable(
        "ActiveProject::QT_HOST_LIBEXECS",
        Tr::tr("Full path to the libexec directory of the Qt version in the active kit "
               "of the active project."),
        []() {
            const QtVersion *const qt = activeQtVersion();
            return qt ? qt->hostLibexecPath().toUserOutput() : QString();
        });

    HelpItem::setLinkNarrower([](const HelpItem &item, const HelpItem::Links &links) {
        const FilePath filePath = item.filePath();
        if (filePath.isEmpty())
            return links;
        QtVersion *qt = QtKitAspect::qtVersion(
            activeKit(ProjectManager::projectForFile(filePath)));
        if (!qt)
            return links;

        // Find best-suited documentation version, so
        // sort into buckets of links with exact, same minor, and same major, and return the first
        // that has entries.
        const QVersionNumber qtVersion = qt->qtVersion();
        HelpItem::Links exactVersion;
        HelpItem::Links sameMinor;
        HelpItem::Links sameMajor;
        bool hasExact = false;
        bool hasSameMinor = false;
        bool hasSameMajor = false;
        for (const HelpItem::Link &link : links) {
            const QUrl url = link.second;
            const QVersionNumber version = HelpItem::extractQtVersionNumber(url).second;
            // version.isNull() means it's not a Qt documentation URL, so include regardless
            if (version.isNull() || version.majorVersion() == qtVersion.majorVersion()) {
                sameMajor.push_back(link);
                hasSameMajor = true;
                if (version.isNull() || version.minorVersion() == qtVersion.minorVersion()) {
                    sameMinor.push_back(link);
                    hasSameMinor = true;
                    if (version.isNull() || version.microVersion() == qtVersion.microVersion()) {
                        exactVersion.push_back(link);
                        hasExact = true;
                    }
                }
            }
        }
        // HelpItem itself finds the highest version within sameMinor/Major/etc itself
        if (hasExact)
            return exactVersion;
        if (hasSameMinor)
            return sameMinor;
        if (hasSameMajor)
            return sameMajor;
        return links;
    });

    askAboutQtInstallation();
}

} // QtSupport::Internal

#include "qtsupportplugin.moc"
