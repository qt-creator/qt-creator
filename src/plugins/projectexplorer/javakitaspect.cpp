// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javakitaspect.h"

#include "devicesupport/devicekitaspects.h"
#include "devicesupport/idevice.h"
#include "kit.h"
#include "kitaspect.h"
#include "kitmanager.h"
#include "projectexplorertr.h"
#include "task.h"

#include <utils/environment.h>
#include <utils/guard.h>
#include <utils/hostosinfo.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDeadlineTimer>
#include <QMutex>

#include <chrono>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

static FilePath javaExecutable(const FilePath &javaHome)
{
    return javaHome.pathAppended("bin/java").withExecutableSuffix();
}

static FilePath compilerExecutable(const FilePath &javaHome)
{
    return javaHome.pathAppended("bin/javac").withExecutableSuffix();
}

static bool isJavaHome(const FilePath &path)
{
    return !path.isEmpty() && javaExecutable(path).isExecutableFile();
}

class JavaKitAspectFactory;

// How long an answer that the machine has no JDK stands for: one may well be
// installed while Creator is open, and a process at every turn is not the way
// to notice.
static constexpr std::chrono::minutes theTimeBetweenAsks(1);

// What /usr/libexec/java_home last answered, when it is worth asking it
// again, who is to tell the kits of an answer that arrives, and whether an
// environment was built without one in the meantime.
struct MacOsJdk
{
    QMutex mutex;
    FilePath javaHome;
    QDeadlineTimer askAgain;
    JavaKitAspectFactory *factory = nullptr;
    bool wentWithout = false;
};

static MacOsJdk &theMacOsJdk()
{
    static MacOsJdk jdk;
    return jdk;
}


// What the plugins offer for a kit that names no JDK of its own. Only asked
// where there is nothing to overrule: the value of the kit is what the kit
// page shows and lets the user change, so anything reading another one would
// make that field say one thing and the build do another.
static FilePath offeredJavaHome(const Kit *k)
{
    for (const JavaHomeCandidateProvider &provider : std::as_const(javaHomeCandidateProviders())) {
        const FilePaths offered = provider(k);
        for (const FilePath &candidate : offered) {
            if (isJavaHome(candidate))
                return candidate;
        }
    }
    return {};
}

class JavaKitAspectImpl final : public KitAspect
{
public:
    JavaKitAspectImpl(Kit *k, const KitAspectFactory *factory) : KitAspect(k, factory)
    {
        m_chooser = createSubWidget<PathChooser>();
        m_chooser->setExpectedKind(PathChooserKind::ExistingDirectory);
        m_chooser->setHistoryCompleter("PE.JavaHome.History");
        m_chooser->setFilePath(JavaKitAspect::javaHome(k));
        connect(m_chooser, &PathChooser::textChanged, this, &JavaKitAspectImpl::pathWasChanged);
    }

    ~JavaKitAspectImpl() override { delete m_chooser; }

private:
    void makeReadOnly(bool readOnly) override { m_chooser->setReadOnly(readOnly); }

    void addToInnerLayout(Layouting::Layout &layout) override
    {
        addMutableAction(m_chooser);
        layout.addItem(Layouting::Span(2, m_chooser));
    }

    void refresh() override
    {
        if (!m_ignoreChanges.isLocked())
            m_chooser->setFilePath(JavaKitAspect::javaHome(kit()));
    }

    void pathWasChanged()
    {
        const GuardLocker locker(m_ignoreChanges);
        JavaKitAspect::setJavaHome(kit(), m_chooser->filePath());
    }

    PathChooser *m_chooser;
    Guard m_ignoreChanges;
};

class JavaKitAspectFactory final : public KitAspectFactory
{
public:
    JavaKitAspectFactory();

    void setup(Kit *k) override;
    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;
    void addToBuildEnvironment(const Kit *k, Environment &env) const override;
    void addToRunEnvironment(const Kit *k, Environment &env) const override;
    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;

    void askTheMacOsToolForItsJdk();
};

JavaKitAspectFactory::JavaKitAspectFactory()
{
    setId(JavaKitAspect::id());
    setDisplayName(Tr::tr("JDK"));
    setDescription(Tr::tr("The Java development kit to build and run Java code with.<br>"
                          "Its location is exported as JAVA_HOME and its bin directory goes "
                          "on the PATH, which is where CMake and other build tools look for "
                          "a JDK. Leave empty to take the one the machine reports: the "
                          "JAVA_HOME of the environment, or the JDK /usr/libexec/java_home "
                          "names on macOS."));
    // Kit aspects are applied in order of descending priority. Between the
    // toolchain, whose JAVA_HOME this one is to have the last word over, and
    // the environment of the kit, which is the user's and so has the last word
    // over this one - without this having to work out what they meant.
    setPriority(29500);

    if (!HostOsInfo::isMacHost())
        return;
    // Asked as soon as there is an event loop to ask in, which is before the
    // projects of the session build their first environment: one built without
    // the JDK of the machine is built again when the answer arrives, and a
    // CMake project is configured again with it.
    const QMutexLocker locker(&theMacOsJdk().mutex);
    theMacOsJdk().factory = this;
    theMacOsJdk().askAgain = QDeadlineTimer(theTimeBetweenAsks);
    QMetaObject::invokeMethod(this, [this] { askTheMacOsToolForItsJdk(); },
                              Qt::QueuedConnection);
}

void JavaKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(k, return);
    if (k->hasValue(JavaKitAspect::id()))
        return;

    // Only what a plugin named for this kit is worth keeping. What the machine
    // happens to have is read where the environment is built instead, so that
    // a kit follows a JDK the machine changes rather than the one it had when
    // the kit was set up.
    //
    // Kit::setValue() enters the key whatever the value, and this runs only
    // while there is none. An empty one would settle the question for good on
    // a machine that may well grow a JDK later.
    if (const FilePath home = offeredJavaHome(k); !home.isEmpty())
        JavaKitAspect::setJavaHome(k, home);
}

Tasks JavaKitAspectFactory::validate(const Kit *k) const
{
    const FilePath home = JavaKitAspect::javaHome(k);
    if (home.isEmpty() || !home.isLocal())
        return {};

    if (!home.isDir()) {
        return {BuildSystemTask(Task::Warning,
                                Tr::tr("The JDK \"%1\" is not a directory.")
                                    .arg(home.toUserOutput()))};
    }
    if (!javaExecutable(home).isExecutableFile()) {
        return {BuildSystemTask(Task::Warning,
                                Tr::tr("The JDK \"%1\" has no java in its bin directory.")
                                    .arg(home.toUserOutput()))};
    }
    if (!compilerExecutable(home).isExecutableFile()) {
        return {BuildSystemTask(Task::Warning,
                                Tr::tr("\"%1\" has no javac in its bin directory, so it is a "
                                       "runtime environment rather than a development kit. "
                                       "Java code cannot be built with it.")
                                    .arg(home.toUserOutput()))};
    }
    return {};
}

KitAspect *JavaKitAspectFactory::createKitAspect(Kit *k) const
{
    QTC_ASSERT(k, return nullptr);
    return new JavaKitAspectImpl(k, this);
}

KitAspectFactory::ItemList JavaKitAspectFactory::toUserOutput(const Kit *k) const
{
    const FilePath home = JavaKitAspect::javaHome(k);
    if (home.isEmpty())
        return {};
    return {{displayName(), home.toUserOutput()}};
}

// macOS has no JDK of its own: /usr/bin/java is a stub that reports where an
// installed one is, and fails when there is none. Asking /usr/libexec/java_home
// beats finding the stub on the PATH and taking it for a runtime.
//
// The ask is a process, and it takes its time to answer that the machine has
// no JDK at all - which is what a Mac started from Finder usually answers. So
// it is made here, where there is an event loop to wait in, rather than on
// the thread that builds an environment, which is the one of the GUI.
void JavaKitAspectFactory::askTheMacOsToolForItsJdk()
{
    static const FilePath tool = "/usr/libexec/java_home";
    if (!tool.isExecutableFile())
        return;

    auto process = new Process(this);
    process->setCommand({tool, {}});
    connect(process, &Process::done, process, [this, process] {
        process->deleteLater();
        if (process->result() != ProcessResult::FinishedWithSuccess)
            return;
        const FilePath javaHome = FilePath::fromUserInput(process->cleanedStdOut().trimmed());
        if (!isJavaHome(javaHome))
            return;
        bool tellTheKits = false;
        {
            const QMutexLocker locker(&theMacOsJdk().mutex);
            if (theMacOsJdk().javaHome == javaHome)
                return;
            theMacOsJdk().javaHome = javaHome;
            tellTheKits = theMacOsJdk().wentWithout;
            theMacOsJdk().wentWithout = false;
        }
        if (!tellTheKits)
            return;
        // An environment was built without this JDK, so the kits that take
        // theirs from the machine are told of it and theirs is built again.
        // Where none was, there is nothing to build again, and no project to
        // configure again either.
        for (Kit *k : KitManager::kits()) {
            if (JavaKitAspect::javaHome(k).isEmpty())
                notifyAboutUpdate(k);
        }
    });
    process->start();
}

// What the last ask brought, and the next one on its way where that was
// nothing: a machine may well grow a JDK while Creator is open, and asking
// after it again at every turn is not worth a process.
static FilePath javaHomeFromMacOsTool()
{
    if (!HostOsInfo::isMacHost())
        return {};

    QMutexLocker locker(&theMacOsJdk().mutex);
    if (isJavaHome(theMacOsJdk().javaHome))
        return theMacOsJdk().javaHome;

    // This environment goes without one, so it is worth telling the kits once
    // an answer is there.
    theMacOsJdk().wentWithout = true;
    JavaKitAspectFactory *factory = theMacOsJdk().factory;
    if (!factory || !theMacOsJdk().askAgain.hasExpired())
        return {};
    theMacOsJdk().askAgain = QDeadlineTimer(theTimeBetweenAsks);
    locker.unlock();

    QMetaObject::invokeMethod(factory, [factory] { factory->askTheMacOsToolForItsJdk(); },
                              Qt::QueuedConnection);
    return {};
}

// Where the kit names none, the machine is asked - but only where the
// environment carries no JDK of its own, since one it already has is both
// newer than anything stored and the user's to change.
static FilePath javaHomeForEnvironment(const Kit *k, const Environment &env,
                                       const IDevice::ConstPtr &device)
{
    if (const FilePath named = JavaKitAspect::javaHome(k); !named.isEmpty())
        return named;
    if (env.hasKey("JAVA_HOME"))
        return {};
    // What the machine has is the host's, which is nothing to an environment
    // of another device - and asking about it costs a process, so it is asked
    // where the answer can be used at all.
    if (!device || !device->rootPath().isLocal())
        return {};
    if (const FilePath fromTool = javaHomeFromMacOsTool(); isJavaHome(fromTool))
        return fromTool;
    return {};
}

static void addJavaHome(const Kit *k, Environment &env, const IDevice::ConstPtr &device)
{
    const FilePath home = javaHomeForEnvironment(k, env, device);
    if (home.isEmpty())
        return;

    // JAVA_HOME and PATH are read on the machine this environment belongs to,
    // and a JDK that is not on it names nothing there. A path of the host's
    // own OS type would also go into a list written in another one's syntax,
    // which is what Environment asserts about.
    if (!device || !home.isSameDevice(device->rootPath()))
        return;

    // The tools reading JAVA_HOME are not Creator, so they want the separators
    // of the system they run on.
    env.set("JAVA_HOME", home.nativePath());
    env.prependOrSetPath(home.pathAppended("bin"));
}

void JavaKitAspectFactory::addToBuildEnvironment(const Kit *k, Environment &env) const
{
    addJavaHome(k, env, BuildDeviceKitAspect::device(k));
}

void JavaKitAspectFactory::addToRunEnvironment(const Kit *k, Environment &env) const
{
    addJavaHome(k, env, RunDeviceKitAspect::device(k));
}

void JavaKitAspectFactory::addToMacroExpander(Kit *kit, MacroExpander *expander) const
{
    QTC_ASSERT(kit, return);
    expander->registerFileVariables("JavaHome", Tr::tr("JDK Location"), [kit] {
        return JavaKitAspect::javaHome(kit);
    });
}

const JavaKitAspectFactory theJavaKitAspectFactory;

} // namespace Internal

Id JavaKitAspect::id()
{
    return "PE.Profile.JavaHome";
}

FilePath JavaKitAspect::javaHome(const Kit *k)
{
    if (!k)
        return {};
    return FilePath::fromSettings(k->value(JavaKitAspect::id()));
}

void JavaKitAspect::setJavaHome(Kit *k, const FilePath &javaHome)
{
    if (!k)
        return;
    k->setValue(JavaKitAspect::id(), javaHome.toSettings());
}

QList<JavaHomeCandidateProvider> &javaHomeCandidateProviders()
{
    static QList<JavaHomeCandidateProvider> theProviders;
    return theProviders;
}

} // namespace ProjectExplorer
