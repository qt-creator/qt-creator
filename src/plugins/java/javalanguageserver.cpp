// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javalanguageserver.h"

#include "javaclasspath.h"
#include "javaconstants.h"
#include "javatr.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <languageclient/client.h>
#include <languageclient/languageclientinterface.h>
#include <languageclient/languageclientmanager.h>
#include <languageclient/languageclientsettings.h>
#include <languageclient/languageclientutils.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/futuresynchronizer.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QJsonArray>
#include <QJsonObject>

using namespace LanguageClient;
using namespace ProjectExplorer;
using namespace Utils;

namespace Java::Internal {

class JLSSettings final : public StdIOSettings
{
public:
    JLSSettings();

    bool applyFromSettingsWidget(QWidget *widget) final;
    QWidget *createSettingsWidget(QWidget *parent) final;
    bool isValid() const final;
    Client *createClient(BaseClientInterface *interface) const final;
    BaseClientInterface *createInterface(BuildConfiguration *) const final;

    FilePathAspect languageServer{this};
};

class JLSSettingsWidget : public QWidget
{
public:
    JLSSettingsWidget(const JLSSettings *settings, QWidget *parent)
        : QWidget(parent)
    {
        using namespace Layouting;
        Form {
            settings->name, br,
            settings->executable, br,
            settings->languageServer
        }.attachTo(this);
    }
};

JLSSettings::JLSSettings()
{
    settingsTypeId.setValue(Constants::JLS_SETTINGS_ID);
    name.setValue("Java Language Server");

    executable.setLabelText(Tr::tr("Java:"));

    languageServer.setSettingsKey("languageServer");
    languageServer.setExpectedKind(PathChooserKind::File);
    languageServer.setLabelText(Tr::tr("Java Language Server:"));
    languageServer.setPlaceHolderText(Tr::tr("Path to equinox launcher jar"));
    languageServer.setPromptDialogFilter("org.eclipse.equinox.launcher_*.jar");

    startBehavior.setValue(RequiresProject);
    mimeTypes.setValue({Utils::Constants::JAVA_MIMETYPE});

    // The server is a Java program of its own, and what runs it is the java
    // of the machine rather than anything the kits build with. Whether that
    // one runs is not asked here: this is a default the user is shown and
    // can change, and a settings page is no place to start a process from.
    const FilePath java = javaIn(Environment::systemEnvironment());
    if (java.isExecutableFile())
        executable.setValue(java);
}

FilePath javaIn(const Environment &environment)
{
    // A JAVA_HOME is set by whoever installed a JDK, and names it outright.
    if (const QString home = environment.value("JAVA_HOME"); !home.isEmpty()) {
        const FilePath java = FilePath::fromUserInput(home) / "bin"
                              / HostOsInfo::withExecutableSuffix("java");
        if (java.isExecutableFile())
            return java;
    }
    return environment.searchInPath("java");
}

bool javaRuns(const FilePath &java)
{
    if (!java.isExecutableFile())
        return false;

    Process process;
    process.setCommand({java, {"-version"}});
    process.runBlocking();
    return process.result() == ProcessResult::FinishedWithSuccess;
}

// The configuration of the server is per system and per architecture, and an
// installation that has both ships config_mac and config_mac_arm. An older one
// has the plain directory alone, which is the x86 one.
static FilePath configurationDirectory(const FilePath &installation)
{
    QString name;
    if constexpr (HostOsInfo::hostOs() == OsTypeWindows)
        name = "config_win";
    else if constexpr (HostOsInfo::hostOs() == OsTypeLinux)
        name = "config_linux";
    else if constexpr (HostOsInfo::hostOs() == OsTypeMac)
        name = "config_mac";

    // A system none of them is named for is one no installation ships a
    // configuration for, and an empty name would make the installation
    // directory itself look like one.
    if (name.isEmpty())
        return {};

    if (HostOsInfo::hostArchitecture() == OsArchArm64) {
        if (const FilePath arm = installation / (name + "_arm"); arm.isReadableDir())
            return arm;
    }
    const FilePath configuration = installation / name;
    return configuration.isReadableDir() ? configuration : FilePath();
}

// What a java is to be told to run the server that this launcher jar belongs
// to. Empty where the installation it lies in has no configuration for this
// machine, since a server started without one comes up and does nothing.
static QString argumentsForLauncherJar(const FilePath &launcherJar)
{
    if (launcherJar.isEmpty())
        return {};

    // <installation>/plugins/org.eclipse.equinox.launcher_*.jar
    const FilePath configuration = configurationDirectory(launcherJar.parentDir().parentDir());
    if (configuration.isEmpty())
        return {};

    return QString("-Declipse.application=org.eclipse.jdt.ls.core.id1 "
                   "-Dosgi.bundles.defaultStartLevel=4 "
                   "-Declipse.product=org.eclipse.jdt.ls.core.product "
                   "-Dlog.level=WARNING "
                   "-noverify "
                   "-Xmx1G "
                   "-jar \"%1\" "
                   "-configuration \"%2\"")
        .arg(launcherJar.path(), configuration.path());
}

// A "java" told to run a jar needs to be told which one, while a launcher of
// the server works the jar, the configuration and the JDK out by itself.
static bool isJavaExecutable(const FilePath &executable)
{
    const QString name = executable.baseName();
    return name == "java" || name == "javaw";
}

// The arguments belong to the launcher jar, and only a java is told to run
// one: a launcher of the server works the jar out by itself and knows nothing
// of the ones the jar of a setting used to name. None either for a java that
// is given a jar this knows no command line for, which leaves a setting that
// cannot start a server rather than one that starts the wrong one.
QString jdtlsArguments(const FilePath &executable, const FilePath &launcherJar)
{
    return isJavaExecutable(executable) ? argumentsForLauncherJar(launcherJar) : QString();
}

bool JLSSettings::applyFromSettingsWidget(QWidget *widget)
{
    bool changed = StdIOSettings::applyFromSettingsWidget(widget);

    // The settings widget asks for the java and the jar and works the rest
    // out, so what is there is replaced rather than added to.
    const QString args = jdtlsArguments(executable(), languageServer());
    changed |= arguments() != args;
    arguments.setValue(args);

    return changed;
}

QWidget *JLSSettings::createSettingsWidget(QWidget *parent)
{
    return new JLSSettingsWidget(this, parent);
}

bool JLSSettings::isValid() const
{
    // A java with no command line would come up and do nothing, so it counts
    // as a setting that is not set up rather than as a server that is there.
    // What the command line is was worked out when the setting was last
    // written: this runs for every document that is opened, and asking the
    // file system again from here would be asking it over and over.
    return StdIOSettings::isValid()
           && (!isJavaExecutable(executable()) || !arguments().isEmpty());
}

class JLSInterface : public StdIOClientInterface
{
public:
    FilePath workspaceDir() const { return m_workspaceDir.path(); }

private:
    TemporaryDirectory m_workspaceDir = TemporaryDirectory("QtCreator-jls-XXXXXX");
};

BaseClientInterface *JLSSettings::createInterface(BuildConfiguration *) const
{
    auto interface = new JLSInterface();
    CommandLine cmd{executable(), arguments(), CommandLine::Raw};
    cmd.addArgs({"-data", interface->workspaceDir().path()});
    interface->setCommandLine(cmd);
    return interface;
}

class JLSClient : public Client
{
public:
    using Client::Client;

    void executeCommand(const LanguageServerProtocol::Command &command) override;
    void setCurrentBuildConfiguration(BuildConfiguration *bc) override;
    void updateConfiguration(const QJsonValue &configuration) override;
    void updateProjectConfiguration();

private:
    void sendConfiguration(const QJsonObject &project);

    // What the build system says the Java of the project is, which is no
    // part of the configuration the user sets and so is kept apart from it.
    QJsonObject m_project;
    // The configuration as it was handed in, before what this client has to
    // say went into it. What Client keeps is the one it was given, so the
    // user's own would not be there to build the next one on.
    QJsonValue m_userConfiguration;
    // The one that went out last, which is how a configuration handed back
    // is told from one the user has just set.
    QJsonValue m_sentConfiguration;
    QList<QMetaObject::Connection> m_buildConfigurationConnections;
};

void JLSClient::executeCommand(const LanguageServerProtocol::Command &command)
{
    if (command.command() == "java.apply.workspaceEdit") {
        const QList<QJsonValue> arguments = command.arguments().value_or(QList<QJsonValue>());
        for (const QJsonValue &argument : arguments) {
            const Utils::Result<LanguageServerProtocol::WorkspaceEdit> edit
                = LanguageServerProtocol::fromJson<LanguageServerProtocol::WorkspaceEdit>(argument);
            if (edit)
                LanguageClient::applyWorkspaceEdit(this, *edit);
            else
                log(QtWarningMsg, edit.error());
        }
    } else {
        Client::executeCommand(command);
    }
}

// The Android support of Qt Creator used to write an Eclipse project into the
// directory of the project - a ".project" and a ".classpath" naming the
// source and the class path of the day. The server imports any directory
// that has one as a project of Eclipse's own, where the settings this client
// sends do not apply at all, so it would go on resolving against a class path
// nothing keeps up to date. What Qt Creator wrote, Qt Creator takes back.
void removeLegacyEclipseProjectFiles(const FilePath &projectDirectory)
{
    static const QString autogenerated = "Autogenerated by Qt Creator.";
    for (const QString &name : {QString(".project"), QString(".classpath")}) {
        const FilePath file = projectDirectory / name;
        const Result<QByteArray> contents = file.fileContents();
        if (contents && QString::fromUtf8(*contents).contains(autogenerated))
            file.removeFile();
    }
}

void JLSClient::setCurrentBuildConfiguration(BuildConfiguration *bc)
{
    Client::setCurrentBuildConfiguration(bc);
    QTC_ASSERT(bc, return);

    removeLegacyEclipseProjectFiles(bc->project()->rootProjectDirectory());

    // What the build configuration before this one has to say is no longer
    // about this client, and connecting to it again would have every parse
    // tell the server the same thing once more.
    for (const QMetaObject::Connection &connection : std::as_const(m_buildConfigurationConnections))
        disconnect(connection);
    m_buildConfigurationConnections.clear();

    updateProjectConfiguration();

    // The source and the class path of the project are what the parse works
    // out, so they are there once it is through rather than when it starts.
    // A parse that failed leaves what it found before it, which is a better
    // thing to go on resolving against than nothing.
    m_buildConfigurationConnections
        << connect(bc->buildSystem(), &BuildSystem::parsingFinished, this, [this](bool success) {
               if (success)
                   updateProjectConfiguration();
           });
    m_buildConfigurationConnections
        << connect(bc->project(), &Project::activeBuildConfigurationChanged, this,
                   [this](BuildConfiguration *active) {
                       if (active && active == buildConfiguration())
                           updateProjectConfiguration();
                   });
}

static QJsonObject javaProjectOf(const QJsonValue &configuration)
{
    return configuration.toObject().value("java").toObject().value("project").toObject();
}

static QJsonValue withJavaProject(const QJsonValue &configuration, const QJsonObject &project)
{
    QJsonObject root = configuration.toObject();
    QJsonObject java = root.value("java").toObject();
    if (project.isEmpty())
        java.remove("project");
    else
        java.insert("project", project);

    if (java.isEmpty())
        root.remove("java");
    else
        root.insert("java", java);
    return root;
}

// "referencedLibraries" is a list of its own or an object of an include list
// beside an exclude one and the sources of the jars, both of which the server
// reads. What a client has to add belongs in the list either way, and the
// rest of such an object is the user's and stays where it is.
static QJsonArray includeList(const QJsonValue &value)
{
    return value.isObject() ? value.toObject().value("include").toArray() : value.toArray();
}

static QJsonValue withIncludeList(const QJsonValue &value, const QJsonArray &list)
{
    if (!value.isObject())
        return list;

    QJsonObject object = value.toObject();
    if (list.isEmpty())
        object.remove("include");
    else
        object.insert("include", list);
    return object;
}

// The configuration of the user as it stands once another one has come in.
// What Client keeps is what it was last handed, and it hands that back when
// the server comes up, so a configuration equal to the one that went out
// last says nothing about what the user set and leaves it alone. Taking it
// for the user's would make what the build system named look like theirs,
// and let go of what is theirs as the class path changes.
QJsonValue userJavaConfiguration(const QJsonValue &incoming,
                                 const QJsonValue &sent,
                                 const QJsonValue &previousUser)
{
    return incoming == sent ? previousUser : incoming;
}

QJsonValue javaConfigurationWithProject(const QJsonValue &user, const QJsonObject &project)
{
    QJsonObject merged = javaProjectOf(user);
    for (auto it = project.begin(); it != project.end(); ++it) {
        const QJsonValue theirs = merged.value(it.key());
        QJsonArray list = includeList(theirs);
        for (const QJsonValue &value : it.value().toArray()) {
            if (!list.contains(value))
                list.append(value);
        }
        merged.insert(it.key(), withIncludeList(theirs, list));
    }
    return withJavaProject(user, merged);
}

// What is to be told about a class path that has just changed. One that has
// gone away is told as empty lists rather than as nothing at all: leaving the
// keys out leaves the server resolving against what it had, and a
// configuration with nothing in it is not sent to it in the first place.
QJsonObject javaProjectToSend(const QJsonObject &project, const QJsonObject &was)
{
    if (!project.isEmpty() || was.isEmpty())
        return project;
    return {{"sourcePaths", QJsonArray()}, {"referencedLibraries", QJsonArray()}};
}

// What the user sets in Projects > Language Server, or in the settings of the
// server, replaces the configuration of the client rather than adding to it,
// so what the build system says about the Java of the project goes back in
// on the way out.
void JLSClient::updateConfiguration(const QJsonValue &configuration)
{
    m_userConfiguration
        = userJavaConfiguration(configuration, m_sentConfiguration, m_userConfiguration);
    sendConfiguration(m_project);
}

void JLSClient::sendConfiguration(const QJsonObject &project)
{
    m_sentConfiguration = javaConfigurationWithProject(m_userConfiguration, project);
    Client::updateConfiguration(m_sentConfiguration);
}

void JLSClient::updateProjectConfiguration()
{
    QTC_ASSERT(buildConfiguration(), return);

    const QJsonObject was = m_project;
    const std::optional<JavaClassPath> classPath = javaClassPath(buildConfiguration());
    if (!classPath || classPath->isEmpty()) {
        // A project that nothing knows the Java layout of is one the server
        // reads by itself: it finds the Gradle or the Maven build file and
        // keeps a project of its own from it. One whose layout has gone away -
        // its Java target was taken out of the build - is such a project now,
        // and must not go on being resolved against what it no longer has.
        m_project = {};
    } else {
        // For everything else the server keeps what it calls an invisible
        // project, whose source and class path are these settings. It resolves
        // them against the workspace directory, which leaves an absolute path
        // alone - and the Java of a project may well lie outside it, as the
        // Java that Qt ships for Android does.
        const auto nativePaths = [](const FilePaths &paths) {
            return QJsonArray::fromStringList(
                Utils::transform<QStringList>(paths, &FilePath::nativePath));
        };
        m_project = QJsonObject{{"sourcePaths", nativePaths(classPath->sourceDirectories)},
                                {"referencedLibraries", nativePaths(classPath->libraries)}};
    }

    // What the user set is already apart from what the build system says, so
    // it goes out as it is: running it through updateConfiguration() again
    // would take out of it whatever the project has come to name as well.
    if (m_project != was)
        sendConfiguration(javaProjectToSend(m_project, was));
}

Client *JLSSettings::createClient(BaseClientInterface *interface) const
{
    return new JLSClient(interface);
}

// The directories an Eclipse JDT language server is installed in on this
// machine. A Qt Creator started from the desktop does not have the PATH of a
// shell, so where a package manager puts one is worth looking at as well.
static FilePaths jdtlsInstallations(const Environment &environment)
{
    FilePaths installations;
    if (const QString home = environment.value("JDTLS_HOME"); !home.isEmpty())
        installations << FilePath::fromUserInput(home);

    // What PATH names is usually a link into the installation rather than
    // the launcher itself, and two directories up from the link is the
    // directory the link lies in, which is no installation at all.
    const FilePath onPath = environment.searchInPath("jdtls");
    if (onPath.isExecutableFile())
        installations << onPath.canonicalPath().parentDir().parentDir();

    const QStringList prefixes{"/opt/homebrew/opt/jdtls/libexec",
                               "/usr/local/opt/jdtls/libexec",
                               "/opt/jdtls",
                               "/usr/share/java/jdtls",
                               FileUtils::homePath().path() + "/.local/share/jdtls"};
    for (const QString &prefix : prefixes)
        installations << FilePath::fromUserInput(prefix);

    return Utils::filtered(installations, &FilePath::isReadableDir);
}

JdtlsCommand jdtlsCommandIn(const FilePath &installation, const FilePath &java)
{
    // The launcher jar before the launcher: the archive of the Eclipse
    // project ships both, and the launcher of it is a Python program, which
    // the machine of the user need not have at all - least of all a Windows
    // one, where the launcher is a batch file that runs that program. A java
    // told to run the jar needs nothing but the JDK, which is there.
    if (!java.isEmpty()) {
        const FilePaths jars = (installation / "plugins")
                                   .dirEntries({{"org.eclipse.equinox.launcher_*.jar"},
                                                DirFilterFlag::Files});
        if (!jars.isEmpty()) {
            if (const QString arguments = argumentsForLauncherJar(jars.first());
                !arguments.isEmpty()) {
                return {java, jars.first(), arguments};
            }
        }
    }

    // An installation a package manager put together need not have the
    // plugins of the archive, or a configuration for this machine, and its
    // launcher is then the only way in.
    const QStringList launcherNames = HostOsInfo::isWindowsHost()
                                          ? QStringList{"jdtls.bat", "jdtls.cmd"}
                                          : QStringList{"jdtls"};
    for (const QString &launcherName : launcherNames) {
        const FilePath launcher = installation / "bin" / launcherName;
        if (launcher.isExecutableFile())
            return {launcher, {}, {}};
    }
    return {};
}

// How the server of this machine is to be started. Reads the directories of
// every installation there may be and runs a java, so it belongs away from
// whatever asked for it.
static JdtlsCommand jdtlsCommand(const Environment &environment)
{
    // The installations before the java, so that the machines that have no
    // server at all - the ones this runs for most - are not made to start a
    // process to find that out.
    const FilePaths installations = jdtlsInstallations(environment);
    if (installations.isEmpty())
        return {};

    // What runs the jar is the java of the machine, as it is what runs the
    // server itself. One that cannot run is no java here: the launcher of an
    // installation brings a JDK of its own along, so it is a way in where
    // the jar is none, and handing the jar to a java that dies on it would
    // leave the user with a server that never comes up.
    const FilePath java = javaIn(environment);
    const FilePath workingJava = javaRuns(java) ? java : FilePath();

    // Each installation is asked for both, so that the one the user named
    // decides even where it has only one of them.
    for (const FilePath &installation : installations) {
        if (const JdtlsCommand command = jdtlsCommandIn(installation, workingJava);
            !command.isEmpty()) {
            return command;
        }
    }
    return {};
}

static bool isServedAlready(Core::IDocument *document)
{
    const QList<BaseSettings *> settings = LanguageClientManager::currentSettings();
    return Utils::anyOf(settings, [document](const BaseSettings *setting) {
        return setting->isValid() && setting->languageFilter().isSupported(document);
    });
}

static constexpr char setupJlsInfoBarId[] = "Java::SetupJls";

// What the machine has is worked out once and kept: a Java file that is
// opened while no server is set up would otherwise walk the PATH and read
// the directories of every installation again, and most of the users this
// happens to have no server to find at all. Nothing until it is there.
static std::optional<JdtlsCommand> &theJdtlsCommand()
{
    static std::optional<JdtlsCommand> command;
    return command;
}

static void offerJavaLanguageServerToOpenDocuments();

// Starts working out what the server of this machine is, once. It reads
// directories and runs a java, neither of which the opening of an editor is
// to wait for: nothing turns on the answer but what an info bar has to
// offer, so it is worked out elsewhere and the documents that are open are
// asked again once it is there.
static void findJdtlsCommand()
{
    static bool started = false;
    if (started)
        return;
    started = true;

    const QFuture<JdtlsCommand> future
        = Utils::asyncRun(jdtlsCommand, Environment::systemEnvironment());
    Utils::futureSynchronizer()->addFuture(future);
    Utils::onResultReady(future,
                         Core::EditorManager::instance(),
                         [](const JdtlsCommand &command) {
                             theJdtlsCommand() = command;
                             offerJavaLanguageServerToOpenDocuments();
                         });
}

// Setting a language server up is the user's to decide, so it is offered
// rather than done: the server is a program of theirs that Qt Creator would
// otherwise start behind their back.
static void offerJavaLanguageServer(Core::IDocument *document)
{
    // The mime type of a Java document is the one the database of the system
    // names, and "text/x-java" is an alias of it rather than that name.
    if (!Utils::mimeTypeForName(document->mimeType()).inherits(Utils::Constants::JAVA_MIMETYPE))
        return;

    InfoBar *infoBar = document->infoBar();
    if (!infoBar->canInfoBeAdded(setupJlsInfoBarId) || isServedAlready(document))
        return;

    // The setting this offers serves the Java of a project, so a file that
    // belongs to none would be left where it is: offering a server that goes
    // on to do nothing tells the user less than saying nothing at all.
    const bool inAProject = Utils::anyOf(ProjectManager::projects(), [document](Project *project) {
        return LanguageClient::fileBelongsToProject(project, document->filePath());
    });
    if (!inAProject)
        return;

    // Nothing knows yet what the server of this machine is. Looking starts
    // here rather than at setup, so that a Qt Creator that never sees a Java
    // file never looks at all, and this document is asked again when the
    // answer is there.
    if (!theJdtlsCommand()) {
        findJdtlsCommand();
        return;
    }

    const JdtlsCommand command = *theJdtlsCommand();
    if (command.isEmpty())
        return;

    InfoBarEntry info(setupJlsInfoBarId,
                      Tr::tr("Set up the Java language server (%1). It provides completion, "
                             "navigation and diagnostics for Java code.")
                          .arg(command.executable.toUserOutput()),
                      InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(Tr::tr("Set Up"), [command] {
        for (Core::IDocument *openDocument : Core::DocumentModel::openedDocuments())
            openDocument->infoBar()->removeInfo(setupJlsInfoBarId);

        auto settings = new JLSSettings;
        settings->executable.setValue(command.executable);
        settings->languageServer.setValue(command.launcherJar);
        settings->arguments.setValue(command.arguments);
        LanguageClientSettings::addSettings(settings);
        LanguageClientManager::applySettings();
    });
    infoBar->addInfo(info);
}

static void offerJavaLanguageServerToOpenDocuments()
{
    for (Core::IDocument *document : Core::DocumentModel::openedDocuments())
        offerJavaLanguageServer(document);
}

void setupJavaLanguageServer()
{
    LanguageClientSettings::registerClientType(
        {Constants::JLS_SETTINGS_ID, Tr::tr("Java Language Server"),
         [] { return new JLSSettings; }});

    QObject::connect(Core::EditorManager::instance(),
                     &Core::EditorManager::editorOpened,
                     Core::EditorManager::instance(),
                     [](Core::IEditor *editor) {
                         if (editor && editor->document())
                             offerJavaLanguageServer(editor->document());
                     });

    // A Java file is as often opened before the project it belongs to as
    // after it, and what is offered for it turns on that project being
    // there, so the ones that are open are asked again once one arrives.
    QObject::connect(ProjectManager::instance(),
                     &ProjectManager::projectAdded,
                     ProjectManager::instance(),
                     [](Project *) {
                         offerJavaLanguageServerToOpenDocuments();
                     });
}

} // namespace Java::Internal
