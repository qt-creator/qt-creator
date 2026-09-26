// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdkmanager.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/futuresynchronizer.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/qtcsettings.h>
#include <utils/shutdownguard.h>
#include <utils/stringutils.h>

#include <QFutureWatcher>
#include <QRegularExpression>
#include <QSettings>

#include <chrono>
#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

namespace Java {

const char SETTINGS_GROUP_KEY[] = "JavaDevelopmentKits";
const char JDKS_KEY[] = "Jdks";
const char DEFAULT_JDK_KEY[] = "DefaultJdk";

const char ID_KEY[] = "Id";
const char NAME_KEY[] = "Name";
const char JAVA_HOME_KEY[] = "JavaHome";

static QList<Jdk> s_jdks;

// The default the user chose, where they did: invalid for a list nothing has
// been picked out of, which defaultJdkId() answers for the machine.
static Id s_chosenDefaultId;

// Whether the machine has been looked at yet. The list holds what the kits
// named before it has, and what those name says nothing about which JDK the
// machine builds with.
static bool s_machineWasRead = false;

FilePath javaExecutableIn(const FilePath &javaHome)
{
    return javaHome.pathAppended("bin/java").withExecutableSuffix();
}

FilePath javaCompilerIn(const FilePath &javaHome)
{
    return javaHome.pathAppended("bin/javac").withExecutableSuffix();
}

FilePath Jdk::javaExecutable() const
{
    return javaExecutableIn(javaHome);
}

FilePath Jdk::compilerExecutable() const
{
    return javaCompilerIn(javaHome);
}

// Whether a directory holds a java and a javac is asked of every painted row
// of the list of JDKs and of every comparison a sort of it makes, and for a
// JDK that lies on another device each question is a round trip to it. What
// such a directory holds changes rarely enough for the answer to keep until
// the list itself does.
static QHash<FilePath, std::pair<bool, bool>> s_probes;

static std::pair<bool, bool> probe(const FilePath &javaHome)
{
    const auto known = s_probes.constFind(javaHome);
    if (known != s_probes.constEnd())
        return *known;

    const bool valid = !javaHome.isEmpty() && javaExecutableIn(javaHome).isExecutableFile();
    const bool compiler = valid && javaCompilerIn(javaHome).isExecutableFile();
    return *s_probes.insert(javaHome, {valid, compiler});
}

bool Jdk::hasJava() const
{
    return probe(javaHome).first;
}

bool Jdk::hasCompiler() const
{
    return probe(javaHome).second;
}

void Jdk::toMap(Store &store) const
{
    store.insert(ID_KEY, id.toSetting());
    store.insert(NAME_KEY, name);
    store.insert(JAVA_HOME_KEY, javaHome.toSettings());
    detectionSource.toMap(store);
}

void Jdk::fromMap(const Store &store)
{
    id = Id::fromSetting(store.value(ID_KEY));
    name = store.value(NAME_KEY).toString();
    javaHome = FilePath::fromSettings(store.value(JAVA_HOME_KEY));
    detectionSource.fromMap(store);
}

// The properties a JDK describes itself with, which it keeps in a file named
// "release" in its home directory. Empty for a directory that has none.
static QHash<QString, QString> readRelease(const FilePath &javaHome)
{
    QHash<QString, QString> properties;
    const Result<QByteArray> contents = javaHome.pathAppended("release").fileContents();
    if (!contents)
        return properties;

    const QStringList lines = QString::fromUtf8(*contents).split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const int equalSign = line.indexOf('=');
        if (equalSign < 0)
            continue;
        QString value = line.mid(equalSign + 1).trimmed();
        if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"'))
            value = value.mid(1, value.size() - 2);
        properties.insert(line.left(equalSign).trimmed(), value);
    }
    return properties;
}

QString javaVersionIn(const FilePath &javaHome)
{
    return readRelease(javaHome).value("JAVA_VERSION");
}

FilePath javaHomeIdentity(const FilePath &javaHome)
{
    // Following the links of another device is a round trip to it for every
    // component of the path, and this is asked while the kits are restored,
    // where a device that cannot be reached would hold the restore up. A JDK
    // that is not on this machine is taken by the name it is given.
    if (javaHome.isEmpty() || !javaHome.isLocal())
        return javaHome;
    return javaHome.resolveSymlinks();
}

// The properties are read from a file, so a caller that wants both the name
// and the version of a JDK passes the ones it has rather than asking twice.
static QString nameForJavaHome(const FilePath &javaHome, const QHash<QString, QString> &release)
{
    const QString version = release.value("JAVA_VERSION");
    if (!version.isEmpty()) {
        const QString implementor = release.value("IMPLEMENTOR");
        return implementor.isEmpty() ? QString("JDK %1").arg(version)
                                     : QString("%1 JDK %2").arg(implementor, version);
    }

    // A JDK of macOS lives in a bundle, whose last two path components say
    // nothing about which JDK it is.
    FilePath directory = javaHome;
    while (directory.fileName() == "Home" || directory.fileName() == "Contents") {
        const FilePath parent = directory.parentDir();
        if (parent.isEmpty() || parent == directory)
            break;
        directory = parent;
    }
    return directory.fileName();
}

// The directory a JDK describes itself in, which is the one JAVA_HOME is
// meant to name. A package manager may put java into a directory that holds
// little else and link it into the real one, and macOS puts a stub there that
// reports no JDK at all - so a directory that holds a java is not yet a JDK.
static FilePath javaHomeOf(const FilePath &candidate)
{
    if (candidate.isEmpty() || !javaExecutableIn(candidate).isExecutableFile())
        return {};
    if (candidate.pathAppended("release").isFile())
        return candidate;

    const FilePath resolved = javaExecutableIn(candidate).resolveSymlinks().parentDir().parentDir();
    if (resolved != candidate && resolved.pathAppended("release").isFile())
        return resolved;
    return {};
}

// /usr/libexec/java_home answers with the JDK the system would pick, and with
// every one it knows of when asked for a property list, which is what -X
// prints.
static FilePaths javaHomesFromMacOsTool()
{
    if (!HostOsInfo::isMacHost())
        return {};

    static const FilePath tool = "/usr/libexec/java_home";
    if (!tool.isExecutableFile())
        return {};

    Process process;
    process.setCommand({tool, {"-X"}});
    process.runBlocking(std::chrono::seconds(2));
    if (process.result() != ProcessResult::FinishedWithSuccess)
        return {};

    static const QRegularExpression homeEntry("<key>JVMHomePath</key>\\s*<string>([^<]*)</string>");
    FilePaths homes;
    QRegularExpressionMatchIterator entries = homeEntry.globalMatch(process.cleanedStdOut());
    while (entries.hasNext())
        homes << FilePath::fromUserInput(entries.next().captured(1));
    return homes;
}

static FilePaths javaHomesIn(const FilePath &directory,
                             const QString &nameFilter,
                             const QString &suffix)
{
    if (!directory.isReadableDir())
        return {};

    const FilePaths entries = directory.dirEntries(
        {{nameFilter}, DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot});
    return Utils::transform(entries, [&suffix](const FilePath &entry) {
        return suffix.isEmpty() ? entry : entry.pathAppended(suffix);
    });
}

// An installer of Windows that put a JDK somewhere else than under Program
// Files says where in the registry, which is the only place an Oracle JDK in
// a directory of the user's choosing can be found.
static FilePaths javaHomesFromWindowsRegistry()
{
#ifdef Q_OS_WIN
    // Both views of the registry: a JDK built for the other architecture than
    // the one Qt Creator was is registered in the other one.
    const QList<QSettings::Format> formats{QSettings::Registry64Format,
                                           QSettings::Registry32Format};
    // "Java Development Kit" is what the installers wrote before JDK 9.
    const QStringList keys{"HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\JDK",
                           "HKEY_LOCAL_MACHINE\\SOFTWARE\\JavaSoft\\Java Development Kit"};

    FilePaths homes;
    for (const QString &key : keys) {
        for (const QSettings::Format format : formats) {
            QSettings registry(key, format);
            const QStringList versions = registry.childGroups();
            for (const QString &version : versions) {
                const QString home = registry.value(version + "/JavaHome").toString();
                if (!home.isEmpty())
                    homes << FilePath::fromUserInput(home);
            }
        }
    }
    return homes;
#else
    return {};
#endif
}

static FilePaths wellKnownJavaHomes()
{
    // SDKMAN keeps the JDKs it installed side by side, with a "current" link
    // among them that is one of them again.
    FilePaths homes = javaHomesIn(FileUtils::homePath() / ".sdkman/candidates/java", "*", {});

    if (HostOsInfo::isMacHost()) {
        homes += javaHomesIn("/Library/Java/JavaVirtualMachines", "*", "Contents/Home");
        homes += javaHomesIn("/System/Library/Java/JavaVirtualMachines", "*", "Contents/Home");
        // Homebrew does not register the JDKs it installs with java_home. It
        // says so in a caveat that asks for a symlink, which few ever make.
        for (const QString &prefix : QStringList{"/opt/homebrew/opt", "/usr/local/opt"}) {
            homes += javaHomesIn(FilePath::fromUserInput(prefix),
                                 "openjdk*",
                                 "libexec/openjdk.jdk/Contents/Home");
        }
    } else if (HostOsInfo::isLinuxHost()) {
        homes += javaHomesIn("/usr/lib/jvm", "*", {});
        homes += javaHomesIn("/usr/java", "*", {});
    } else if (HostOsInfo::isWindowsHost()) {
        const QStringList variables
            = {"ProgramFiles", "ProgramW6432", "ProgramFiles(x86)", "LOCALAPPDATA"};
        for (const QString &variable : variables) {
            const QString programs = qtcEnvironmentVariable(variable);
            if (programs.isEmpty())
                continue;
            const FilePath directory = FilePath::fromUserInput(programs);
            homes += javaHomesIn(directory / "Java", "*", {});
            homes += javaHomesIn(directory / "Eclipse Adoptium", "*", {});
            homes += javaHomesIn(directory / "Amazon Corretto", "*", {});
            homes += javaHomesIn(directory / "Microsoft", "jdk*", {});
            homes += javaHomesIn(directory / "Programs/Eclipse Adoptium", "*", {});
        }
        homes += javaHomesFromWindowsRegistry();
    }
    return homes;
}

// A version like "26.0.2.1" against "17.0.20.1": the components are numbers,
// and where they are equal as far as they go, more of them is the newer one.
static bool isNewerVersion(const QString &first, const QString &second)
{
    static const QRegularExpression separator("[^0-9]+");
    const QStringList firstParts = first.split(separator, Qt::SkipEmptyParts);
    const QStringList secondParts = second.split(separator, Qt::SkipEmptyParts);
    for (int i = 0; i < qMin(firstParts.size(), secondParts.size()); ++i) {
        const int difference = firstParts.at(i).toInt() - secondParts.at(i).toInt();
        if (difference != 0)
            return difference > 0;
    }
    return firstParts.size() > secondParts.size();
}

QList<JdkProvider> &jdkProviders()
{
    static QList<JdkProvider> theProviders;
    return theProviders;
}

// What the plugins know of, which they are asked for on the thread they live
// on rather than on the one the looking is done on.
static FilePaths javaHomesFromProviders()
{
    FilePaths homes;
    for (const JdkProvider &provider : std::as_const(jdkProviders()))
        homes += provider();
    return homes;
}

// Everything that is read off the machine: directories, the environment, and
// a process on macOS. Nothing of Creator's own is touched, so this is what
// another thread can be left with.
static QList<Jdk> javaHomesOfTheMachine(const FilePaths &fromProviders)
{
    const Environment environment = Environment::systemEnvironment();
    const FilePath fromEnvironment = javaHomeOf(
        FilePath::fromUserInput(environment.value("JAVA_HOME")));

    FilePaths candidates{fromEnvironment};
    candidates += javaHomesFromMacOsTool();
    candidates += wellKnownJavaHomes();
    candidates += fromProviders;
    candidates << environment.searchInPath("java").parentDir().parentDir();

    QList<Jdk> jdks;
    QHash<Id, QString> versions;
    QSet<FilePath> known;
    QStringList names;
    for (const FilePath &candidate : std::as_const(candidates)) {
        const FilePath javaHome = javaHomeOf(candidate);
        if (javaHome.isEmpty())
            continue;
        // A distribution and a version manager name the same JDK from
        // directories of their own, and one JDK found twice is one JDK.
        const FilePath identity = javaHomeIdentity(javaHome);
        if (known.contains(identity))
            continue;
        known.insert(identity);

        const QHash<QString, QString> release = readRelease(javaHome);

        Jdk jdk;
        jdk.id = Id::generate();
        jdk.name = Utils::makeUniquelyNumbered(nameForJavaHome(javaHome, release), names);
        jdk.javaHome = javaHome;
        jdk.detectionSource = {DetectionSource::FromSystem, "JDK"};
        names << jdk.name;
        versions.insert(jdk.id, release.value("JAVA_VERSION"));
        jdks << jdk;
    }

    // The order decides which one becomes the default of the machine, and a
    // machine that has several JDKs is to build with the newest of them -
    // unless its environment names one, which says what it was set up for.
    Utils::sort(jdks, [&versions](const Jdk &first, const Jdk &second) {
        return isNewerVersion(versions.value(first.id), versions.value(second.id));
    });
    const int fromEnvironmentIndex = Utils::indexOf(jdks, [&fromEnvironment](const Jdk &jdk) {
        return jdk.javaHome == fromEnvironment;
    });
    if (fromEnvironmentIndex > 0)
        jdks.move(fromEnvironmentIndex, 0);

    return jdks;
}

QList<Jdk> JdkManager::detectJdks()
{
    return javaHomesOfTheMachine(javaHomesFromProviders());
}

QFuture<QList<Jdk>> JdkManager::detectJdksAsync()
{
    instance();
    const QFuture<QList<Jdk>> future = Utils::asyncRun(javaHomesOfTheMachine,
                                                       javaHomesFromProviders());
    // The thread outlives a shutdown that comes while it is looking, and what
    // it is looking with lives in this plugin.
    Utils::futureSynchronizer()->addFuture(future);
    return future;
}

// The one of them the machine would build with, which is the first it was
// found to have - except that a JDK which cannot build is a poor one to set a
// kit up with, so one that can comes before it.
static Id machineDefaultId(const QList<Jdk> &jdks)
{
    // Of the ones this machine has. A kit on another device puts the JDK of
    // that device into the list, and such a JDK is no answer for a kit that
    // builds here - nor one to ask after, since asking is a round trip to a
    // device that may not be reachable at all.
    const QList<Jdk> ofTheMachine = Utils::filtered(jdks, [](const Jdk &jdk) {
        return jdk.javaHome.isLocal();
    });
    const Jdk withCompiler = Utils::findOrDefault(ofTheMachine, &Jdk::hasCompiler);
    if (withCompiler.id.isValid())
        return withCompiler.id;
    return ofTheMachine.isEmpty() ? Id() : ofTheMachine.first().id;
}

static void readSettings()
{
    s_jdks.clear();
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SETTINGS_GROUP_KEY);
    const QVariantList entries = settings->value(JDKS_KEY).toList();
    for (const QVariant &entry : entries) {
        if (!isStore(entry))
            continue;
        Jdk jdk;
        jdk.fromMap(storeFromVariant(entry));
        if (jdk.id.isValid() && !jdk.javaHome.isEmpty())
            s_jdks << jdk;
    }
    s_chosenDefaultId = Id::fromSetting(settings->value(DEFAULT_JDK_KEY));
    settings->endGroup();
}

static void writeSettings()
{
    QVariantList entries;
    for (const Jdk &jdk : std::as_const(s_jdks)) {
        Store store;
        jdk.toMap(store);
        entries << variantFromStore(store);
    }

    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(SETTINGS_GROUP_KEY);
    settings->setValue(JDKS_KEY, entries);
    settings->setValue(DEFAULT_JDK_KEY, s_chosenDefaultId.toSetting());
    settings->endGroup();
}

JdkManager::JdkManager()
{
    readSettings();
}

JdkManager::~JdkManager() = default;

JdkManager *JdkManager::instance()
{
    static GuardedObject<JdkManager> theJdkManager;
    return theJdkManager.get();
}

const QList<Jdk> &JdkManager::jdks()
{
    instance();
    return s_jdks;
}

Jdk JdkManager::jdk(Id id)
{
    if (!id.isValid())
        return {};
    return Utils::findOrDefault(jdks(), [id](const Jdk &jdk) { return jdk.id == id; });
}

Jdk JdkManager::jdkForJavaHome(const FilePath &javaHome)
{
    if (javaHome.isEmpty())
        return {};
    const Jdk named = Utils::findOrDefault(jdks(), [&javaHome](const Jdk &jdk) {
        return jdk.javaHome == javaHome;
    });
    if (named.id.isValid())
        return named;

    // The list may well hold that JDK under another of its directories: which
    // of them the detection entered it by is not the same from one run to the
    // next, and a plugin names it by the one it was configured with. Asking
    // the file system is left to the entries that do not answer by name.
    const FilePath identity = javaHomeIdentity(javaHome);
    return Utils::findOrDefault(jdks(), [&identity](const Jdk &jdk) {
        return javaHomeIdentity(jdk.javaHome) == identity;
    });
}

Jdk JdkManager::defaultJdk()
{
    return jdk(defaultJdkId());
}

Id JdkManager::defaultJdkId()
{
    instance();
    // An entry the user picked out of the list and one that has since been
    // taken out of it are told apart here rather than when it goes: a JDK
    // that is away while the kits are restored comes back. The pick is of
    // this machine for the reason machineDefaultId() gives - a JDK that lies
    // on another device is no answer for a kit that builds here, and taking
    // it for one would leave every such kit without a JDK at all.
    const Jdk chosen = Utils::findOrDefault(s_jdks, [](const Jdk &jdk) {
        return jdk.id == s_chosenDefaultId;
    });
    if (chosen.id.isValid() && chosen.javaHome.isLocal())
        return chosen.id;
    return machineDefaultId(s_jdks);
}

Id JdkManager::chosenDefaultJdkId()
{
    instance();
    return s_chosenDefaultId;
}

bool JdkManager::jdksWereDetected()
{
    instance();
    return s_machineWasRead;
}

void JdkManager::setJdks(const QList<Jdk> &jdks, Id defaultId)
{
    JdkManager *manager = instance();
    QTC_ASSERT(manager, return);
    if (jdks == s_jdks && defaultId == s_chosenDefaultId)
        return;

    s_jdks = jdks;
    s_chosenDefaultId = defaultId;
    s_probes.clear();
    writeSettings();
    emit manager->jdksChanged();
}

Jdk JdkManager::registerJavaHome(const FilePath &javaHome, const DetectionSource &detectionSource)
{
    if (javaHome.isEmpty())
        return {};
    if (const Jdk known = jdkForJavaHome(javaHome); known.id.isValid())
        return known;
    if (javaHome.isLocal() && !javaExecutableIn(javaHome).isExecutableFile())
        return {};

    // A JDK describes itself in a file, and reading one off another device
    // is a round trip to it. This runs while the kits are restored, where a
    // plugin that speaks for a kit hands over the directory it is configured
    // with, so one that is not on this machine is named after its directory
    // alone.
    const QHash<QString, QString> release = javaHome.isLocal() ? readRelease(javaHome)
                                                               : QHash<QString, QString>();

    Jdk jdk;
    jdk.id = Id::generate();
    jdk.name = Utils::makeUniquelyNumbered(nameForJavaHome(javaHome, release),
                                           Utils::transform(s_jdks, &Jdk::name));
    jdk.javaHome = javaHome;
    jdk.detectionSource = detectionSource;

    setJdks(s_jdks + QList<Jdk>{jdk}, s_chosenDefaultId);
    return jdk;
}

QList<Jdk> JdkManager::mergedJdks(const QList<Jdk> &current, const QList<Jdk> &detected)
{
    // What the directories of the list hold is asked of them again: whoever
    // has something to merge in has just been through the machine.
    s_probes.clear();

    // The order the machine has them in, since that is the order it would
    // build with them. A JDK that was found before keeps the identity it was
    // entered with, since that is what the kits using it name - which of its
    // directories this run of the detection names it by is no reason to enter
    // it again, nor to take it from the kits.
    QList<Jdk> jdks;
    QSet<FilePath> identities;
    for (const Jdk &jdk : detected) {
        const FilePath identity = javaHomeIdentity(jdk.javaHome);
        if (identities.contains(identity))
            continue;
        identities << identity;
        const Jdk known = Utils::findOrDefault(current, [&identity](const Jdk &entry) {
            return javaHomeIdentity(entry.javaHome) == identity;
        });
        if (!known.id.isValid()) {
            jdks << jdk;
            continue;
        }

        // What an entry of the machine's own is called and where it lies are
        // the machine's to say again: a package manager that upgrades a JDK
        // points the link it installed it under at another one, and the
        // entry would go on bearing the name of the JDK that was there. Only
        // the id stays, since that is what the kits name it by.
        //
        // One a kit handed over before the looking came back is of the
        // machine's own from here on, so that it goes the way of the rest
        // once the machine no longer has it.
        Jdk entry = known;
        if (known.detectionSource.isAutoDetected()
            || known.detectionSource.id == QLatin1String(JDK_NAMED_BY_A_KIT)) {
            entry.name = jdk.name;
            entry.javaHome = jdk.javaHome;
            entry.detectionSource = jdk.detectionSource;
        }
        jdks << entry;
    }
    // After them the rest of the list. A JDK that was found and has since
    // been uninstalled is not worth keeping, and neither is a second entry
    // for one the machine has - but an entry the user made is theirs to
    // remove, whatever directory they pointed it at.
    for (const Jdk &known : current) {
        const bool taken = Utils::anyOf(jdks, [&known](const Jdk &entry) {
            return entry.id == known.id;
        });
        if (taken)
            continue;
        if (known.detectionSource.isAutoDetected()
            && (identities.contains(javaHomeIdentity(known.javaHome)) || !known.hasJava())) {
            continue;
        }
        jdks << known;
    }
    return jdks;
}

void JdkManager::enterDetectedJdks(const QList<Jdk> &detected)
{
    instance();
    const QList<Jdk> jdks = mergedJdks(s_jdks, detected);

    // The kits are told even where the list comes out as it went in: what a
    // kit that has no JDK has been waiting for is this answer about the
    // machine, and the handler of this signal is what gives it one.
    const bool wasRead = std::exchange(s_machineWasRead, true);
    if (!wasRead && jdks == s_jdks) {
        emit instance()->jdksChanged();
        return;
    }
    setJdks(jdks, s_chosenDefaultId);
}

// What the detection is running in, where it is. Kept so that a second
// restore lets go of the first rather than entering what it finds over it.
static QFutureWatcher<QList<Jdk>> *s_detection = nullptr;

void JdkManager::restoreJdks()
{
    instance();
    // Looking where every system keeps its JDKs reads directories, and the
    // tool of macOS takes its time to answer that the machine has none at all
    // - which is what a Mac started from Finder usually answers. Neither
    // belongs on the thread the GUI is on, and the kits need not wait for it.
    delete s_detection;
    s_machineWasRead = false;
    s_detection = new QFutureWatcher<QList<Jdk>>(instance());
    connect(s_detection, &QFutureWatcherBase::finished, instance(), [] {
        if (!s_detection->isCanceled())
            enterDetectedJdks(s_detection->result());
    });
    s_detection->setFuture(detectJdksAsync());
}

} // namespace Java
