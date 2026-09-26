// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javakitaspect.h"

#include "javaconstants.h"
#include "javaparser.h"
#include "javatr.h"
#include "jdkmanager.h"

#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/task.h>

#include <utils/environment.h>
#include <utils/id.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QAbstractListModel>

#include <optional>
#include <utility>

using namespace ProjectExplorer;
using namespace Utils;

namespace Java {
namespace Internal {

// A JDK that a kit names while the detection of the machine does not find it
// is one the machine was told about rather than one it has, so it is not for
// a re-detection to take away. It is marked as named by a kit all the same:
// the kits are restored before the looking comes back, and a JDK the machine
// turns out to have after all is one of the machine's.
static Jdk registerJavaHome(const FilePath &javaHome)
{
    return JdkManager::registerJavaHome(javaHome,
                                        {DetectionSource::Manual, JDK_NAMED_BY_A_KIT});
}

// What the plugins say the JDK of this kit is, \a jdkFor being what turns a
// directory one of them names into an entry of the list. Nothing at all where
// none of them knows the kit, which is the one case the default of the
// machine answers: a plugin that speaks for a kit and names no JDK says that
// the kit is to have none, as one whose kits take theirs from their toolchain
// does.
// \a jdkFor is JdkManager::jdkForJavaHome() where only the JDKs the list
// holds already will do: a kit that is looking for another JDK because one
// was taken out of the list must not put one back in, which is what
// registering the offer of a plugin whose offer is that very JDK would do.
static std::optional<Jdk> providedJdkForKit(const Kit *k, Jdk (*jdkFor)(const FilePath &))
{
    for (const JavaHomeCandidateProvider &provider : std::as_const(javaHomeCandidateProviders())) {
        const std::optional<FilePaths> offered = provider(k);
        if (!offered)
            continue;
        for (const FilePath &candidate : *offered) {
            // An entry outlives the directory it names, since one the user
            // made is theirs to remove. A kit given such an entry would
            // export a JAVA_HOME that names nothing, so what the plugin
            // offers has to be there - on this machine, that is, since one
            // on another device is not asked about.
            const Jdk jdk = jdkFor(candidate);
            if (jdk.id.isValid() && (!jdk.javaHome.isLocal() || jdk.hasJava()))
                return jdk;
        }
        return Jdk();
    }
    return std::nullopt;
}

// The default of the machine, for a kit that is built on the machine: the
// JDKs of the list are the host's, and one of those names nothing on the
// device a Docker or a remote kit builds on. A kit left with one would be
// shown a JDK its build never uses, since addToBuildEnvironment() exports
// only what the device it builds on can reach.
//
// Nothing either until the machine has been looked at: the list then holds
// what the kits named while the looking was going on, and the first of those
// is no answer to which JDK the machine builds with. A kit that has none is
// given one when the looking comes back, which is what jdksChanged() is for.
static Jdk defaultJdkForKit(const Kit *k)
{
    if (!JdkManager::jdksWereDetected())
        return {};
    const Jdk jdk = JdkManager::defaultJdk();
    const IDevice::ConstPtr device = BuildDeviceKitAspect::device(k);
    if (!device || !jdk.javaHome.isSameDevice(device->rootPath()))
        return {};
    return jdk;
}

// What a kit is depends on its device type, which is picked after the Kits
// page has made it and set it up, so a plugin may well come to speak for a
// kit that was answered for by the machine - and then it is that plugin's
// answer that holds. A JDK the user picked themselves is theirs, and carries
// no mark.
using Constants::JAVA_HOME_OF_THE_MACHINE;

static void setTheJdkOfTheMachine(Kit *k, const Jdk &jdk)
{
    // The mark first: writing the JDK is what tells the kit, and what reads
    // the kit then - the Kits page takes a copy of it - is to find the two
    // together rather than a JDK that looks like one the user picked.
    k->setValueSilently(JAVA_HOME_OF_THE_MACHINE, jdk.id.toSetting());
    k->setValue(JavaKitAspect::id(), jdk.id.toSetting());
}

// Whether what the kit names is still what the machine answered with, rather
// than something picked over it since.
static bool namesTheJdkOfTheMachine(const Kit *k)
{
    const QVariant ofTheMachine = k->value(JAVA_HOME_OF_THE_MACHINE);
    return ofTheMachine.isValid() && ofTheMachine == k->value(JavaKitAspect::id());
}

static void theJdkIsTheirsNow(Kit *k)
{
    // Silently, as the mark goes in: what the kit names has not changed, and
    // a kit told of a change builds the environment of every project on it
    // again.
    k->removeKeySilently(JAVA_HOME_OF_THE_MACHINE);
}

class JdkListModel final : public QAbstractListModel
{
public:
    using QAbstractListModel::QAbstractListModel;

    // The JDKs of the machine, and "None" after them.
    void reset()
    {
        beginResetModel();
        m_jdks = JdkManager::jdks();
        endResetModel();
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        if (parent.isValid())
            return 0;
        return m_jdks.size() + 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount())
            return {};
        if (index.row() < m_jdks.size())
            return jdkData(m_jdks.at(index.row()), role);
        return noneData(role);
    }

private:
    static QVariant jdkData(const Jdk &jdk, int role)
    {
        const bool isDefault = jdk.id == JdkManager::defaultJdkId();
        switch (role) {
        case Qt::DisplayRole:
            return isDefault ? Tr::tr("%1 (Default)").arg(jdk.name) : jdk.name;
        case Qt::FontRole: {
            QFont font;
            font.setItalic(isDefault);
            return font;
        }
        case Qt::DecorationRole:
            return jdk.hasCompiler() ? QVariant() : Icons::CRITICAL.icon();
        case Qt::ToolTipRole:
            return jdk.javaHome.toUserOutput();
        case KitAspect::IdRole:
            return jdk.id.toSetting();
        case KitAspect::QualityRole:
            return jdk.hasCompiler() ? 2 : (jdk.hasJava() ? 1 : 0);
        default:
            return {};
        }
    }

    static QVariant noneData(int role)
    {
        if (role == Qt::DisplayRole)
            return Tr::tr("None", "No JDK");
        if (role == KitAspect::IsNoneRole)
            return true;
        return {};
    }

    QList<Jdk> m_jdks;
};

static void fixJavaHome(Kit *k);
static void removeJavaHome(Kit *k);

class JavaKitAspectImpl final : public KitAspect
{
public:
    JavaKitAspectImpl(Kit *k, const KitAspectFactory *factory) : KitAspect(k, factory)
    {
        setManagingPage(Constants::JDK_SETTINGS_PAGE_ID);

        const auto model = new JdkListModel(this);
        auto getter = [](const Kit &k) { return k.value(JavaKitAspect::id()); };
        auto setter = [](Kit &k, const QVariant &id) {
            k.setValue(JavaKitAspect::id(), id);
            theJdkIsTheirsNow(&k);
        };
        auto resetModel = [model] { model->reset(); };
        addListAspectSpec({model, std::move(getter), std::move(setter), std::move(resetModel)});

        connect(JdkManager::instance(), &JdkManager::jdksChanged,
                this, &JavaKitAspectImpl::refresh);
    }

private:
    // The kit page works on a copy of the kit, which the factory does not
    // reach: it goes through the kits of the manager, and the copy is none of
    // them. A copy naming a JDK that has gone out of the list would be left
    // naming it, with no row in the field to show it - and the field would
    // then fall back on "None", which is a choice of the user's and lasts.
    void refresh() override
    {
        if (Kit *k = kit())
            fixJavaHome(k);
        KitAspect::refresh();
    }
};

class JavaKitAspectFactory final : public KitAspectFactory
{
public:
    JavaKitAspectFactory();

    void onKitsLoaded() override;
    void setup(Kit *k) override;
    void fix(Kit *k) override;
    Tasks validate(const Kit *k) const override;
    KitAspect *createKitAspect(Kit *k) const override;
    ItemList toUserOutput(const Kit *k) const override;
    void addToBuildEnvironment(const Kit *k, Environment &env) const override;
    void addToRunEnvironment(const Kit *k, Environment &env) const override;
    QList<OutputLineParser *> createOutputParsers(const Kit *k) const override;
    void addToMacroExpander(Kit *kit, MacroExpander *expander) const override;
};

JavaKitAspectFactory::JavaKitAspectFactory()
{
    setId(JavaKitAspect::id());
    setDisplayName(Tr::tr("JDK"));
    setDescription(Tr::tr("The Java development kit to build and run Java code with.<br>"
                          "Its location is exported as JAVA_HOME and its bin directory goes "
                          "on the PATH, which is where CMake and other build tools look for "
                          "a JDK. A kit that names none is set up with the default of "
                          "the machine and keeps it, this field being where it is changed; "
                          "pick \"None\" to keep Java out of the environment of a kit that "
                          "has nothing to do with it."));
    // Kit aspects are applied in order of descending priority. Between the
    // toolchain, whose JAVA_HOME this one is to have the last word over, and
    // the environment of the kit, which is the user's and so has the last word
    // over this one - without this having to work out what they meant.
    setPriority(29500);
    // Above the toolchain (30000) and Qt (26000), whose parsers would take a
    // diagnostic of javac for one of gcc or of moc: they read alike, and this
    // is the parser that knows which of them a .java line is.
    setOutputParserPriority(40000);
}

// The factory is there before anything it could connect to is, and the kits
// it would go through are not loaded either.
void JavaKitAspectFactory::onKitsLoaded()
{
    // A kit whose JDK was removed from the list of them names one that is no
    // longer there, and one that has none may well get the one just added.
    // Not setup() for the latter: it asks the plugins, and entering a JDK into
    // the list while the list is what just changed would put the one the user
    // has taken out of it back in.
    // An entry that stays the entry it was while coming to hold another
    // directory - one the user points elsewhere on the JDKs page, or one
    // whose link a package manager re-points - leaves the kits naming it
    // where they are, so nothing of theirs is written and nothing of theirs
    // is told. What they build with has changed all the same, and an
    // environment already built for them is kept until they are told.
    //
    // The list as it stands is what the first change is held against: the
    // looking at the machine may well have come back before the kits were
    // loaded, and a hash left to fill itself would have nothing to tell the
    // first change the user makes from the list they started with.
    QHash<Id, FilePath> ofTheKits;
    for (const Jdk &jdk : JdkManager::jdks())
        ofTheKits.insert(jdk.id, jdk.javaHome);

    connect(JdkManager::instance(), &JdkManager::jdksChanged, this,
            [this, homes = std::move(ofTheKits)]() mutable {
        QHash<Id, FilePath> ofTheList;
        for (const Jdk &jdk : JdkManager::jdks())
            ofTheList.insert(jdk.id, jdk.javaHome);

        for (Kit *k : KitManager::kits()) {
            if (!k->isAspectRelevant(JavaKitAspect::id()))
                continue;
            if (!k->hasValue(JavaKitAspect::id())) {
                if (const std::optional<Jdk> jdk
                    = providedJdkForKit(k, &JdkManager::jdkForJavaHome)) {
                    if (jdk->id.isValid())
                        k->setValue(JavaKitAspect::id(), jdk->id.toSetting());
                } else if (const Jdk jdk = defaultJdkForKit(k); jdk.id.isValid()) {
                    setTheJdkOfTheMachine(k, jdk);
                }
                continue;
            }
            const Id named = Id::fromSetting(k->value(JavaKitAspect::id()));
            const bool movedHouse = named.isValid() && homes.contains(named)
                                    && homes.value(named) != ofTheList.value(named);
            fix(k);
            if (movedHouse && k->value(JavaKitAspect::id()) == named.toSetting())
                notifyAboutUpdate(k);
        }
        homes = ofTheList;
    });
}

void JavaKitAspectFactory::setup(Kit *k)
{
    QTC_ASSERT(k, return);
    if (k->hasValue(JavaKitAspect::id()))
        return;

    // Kit::setValue() enters the key whatever the value, and this runs only
    // while there is none. An empty one would settle the question for good on
    // a machine that may well grow a JDK later.
    if (const std::optional<Jdk> jdk = providedJdkForKit(k, &registerJavaHome)) {
        if (jdk->id.isValid())
            k->setValue(JavaKitAspect::id(), jdk->id.toSetting());
        return;
    }
    if (const Jdk jdk = defaultJdkForKit(k); jdk.id.isValid())
        setTheJdkOfTheMachine(k, jdk);
}

// Leaves the kit without a JDK, which is what a kit that has yet to be given
// one looks like: the key is not there at all.
//
// Kit::removeKey() takes the flags of the key with it, and whether the field
// is the user's to edit is a question of its own: a kit whose JDK a plugin
// writes goes on being written by it, JDK or none.
static void removeJavaHome(Kit *k)
{
    const Id id = JavaKitAspect::id();
    const bool sticky = k->isSticky(id);
    const bool isMutable = k->isMutable(id);
    k->removeKey(id);
    k->setSticky(id, sticky);
    k->setMutable(id, isMutable);
}

// Brings what the kit names in line with the list of the JDKs of the machine.
static void fixJavaHome(Kit *k)
{
    if (!k->hasValue(JavaKitAspect::id()))
        return;

    // The repair takes the JDK out of the kit before putting another in, and
    // between the two the kit has none. Telling it of each step would have
    // every project on it build an environment without a JAVA_HOME and then
    // build another: it is told once, of where it ends up.
    const KitGuard guard(k);

    // A plugin that has come to speak for this kit answers for its JDK, the
    // machine having answered only for want of it.
    if (namesTheJdkOfTheMachine(k)) {
        const std::optional<Jdk> ofThePlugins = providedJdkForKit(k,
                                                                  &JdkManager::jdkForJavaHome);
        if (ofThePlugins) {
            removeJavaHome(k);
            theJdkIsTheirsNow(k);
            if (ofThePlugins->id.isValid())
                k->setValue(JavaKitAspect::id(), ofThePlugins->id.toSetting());
            return;
        }
    }

    // An empty value is the "None" of the list the user picked, which is a
    // choice of theirs like any other rather than a JDK that has gone away.
    const QVariant value = k->value(JavaKitAspect::id());
    const Id named = Id::fromSetting(value);
    if (!named.isValid())
        return;
    if (JdkManager::jdk(named).id.isValid())
        return;

    // The JDK the kit named is gone from the list. Leaving the key behind
    // would keep the kit from ever picking up another one.
    removeJavaHome(k);
    theJdkIsTheirsNow(k);
    if (const std::optional<Jdk> jdk = providedJdkForKit(k, &JdkManager::jdkForJavaHome)) {
        if (jdk->id.isValid())
            k->setValue(JavaKitAspect::id(), jdk->id.toSetting());
        return;
    }
    if (const Jdk jdk = defaultJdkForKit(k); jdk.id.isValid())
        setTheJdkOfTheMachine(k, jdk);
}

void JavaKitAspectFactory::fix(Kit *k)
{
    QTC_ASSERT(k, return);
    fixJavaHome(k);
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
    if (!javaExecutableIn(home).isExecutableFile()) {
        return {BuildSystemTask(Task::Warning,
                                Tr::tr("The JDK \"%1\" has no java in its bin directory.")
                                    .arg(home.toUserOutput()))};
    }
    if (!javaCompilerIn(home).isExecutableFile()) {
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
    // By the entry the kit names rather than by its directory: two entries
    // may well name one directory - cloning a row of the JDKs page makes such
    // a pair - and the kit named one of them.
    const Jdk jdk = JdkManager::jdk(Id::fromSetting(k->value(JavaKitAspect::id())));
    if (!jdk.id.isValid())
        return {};
    return {{displayName(), QString("%1 (%2)").arg(jdk.name, jdk.javaHome.toUserOutput())}};
}

static void addJavaHome(const Kit *k, Environment &env, const IDevice::ConstPtr &device)
{
    const FilePath home = JavaKitAspect::javaHome(k);
    if (home.isEmpty())
        return;

    // JAVA_HOME and the PATH it goes on are read on the machine whose
    // environment they are, and a JDK that is not on it names nothing there.
    // A path of the host's own OS type would also go into a list written in
    // another one's syntax, which is what Environment asserts about.
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

// Whatever builds the Java of a project, what compiles it is javac, and what
// it says about a file of the user belongs in the issues of the build.
QList<OutputLineParser *> JavaKitAspectFactory::createOutputParsers(const Kit *k) const
{
    // Kit::createOutputParsers() asks every aspect, relevant to the kit or
    // not, and a kit that has nothing to do with Java builds none.
    QTC_ASSERT(k, return {});
    if (!k->isAspectRelevant(JavaKitAspect::id()))
        return {};
    if (JavaKitAspect::javaHome(k).isEmpty())
        return {};
    return {new JavaParser};
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
    return "Java.JavaHome";
}

FilePath JavaKitAspect::javaHome(const Kit *k)
{
    if (!k)
        return {};

    // A kit naming no JDK, or one that has gone out of the list, names no
    // directory either: fix() gives it another one.
    return JdkManager::jdk(Id::fromSetting(k->value(JavaKitAspect::id()))).javaHome;
}

void JavaKitAspect::setJavaHome(Kit *k, const FilePath &javaHome)
{
    if (!k)
        return;

    const KitGuard guard(k);

    // Kit::setValue() enters the key whatever the value, and a key that is
    // there with nothing in it is the "None" the user picked: the kit is never
    // set up again. A directory that holds no JDK leaves the question open
    // instead, on a machine that may well grow one later.
    if (const Jdk jdk = Internal::registerJavaHome(javaHome); jdk.id.isValid())
        k->setValue(JavaKitAspect::id(), jdk.id.toSetting());
    else
        Internal::removeJavaHome(k);
    Internal::theJdkIsTheirsNow(k);
}

QList<JavaHomeCandidateProvider> &javaHomeCandidateProviders()
{
    static QList<JavaHomeCandidateProvider> theProviders;
    return theProviders;
}

} // namespace Java
