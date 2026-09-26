// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdkoptionspage.h"

#include "javaconstants.h"
#include "javatr.h"
#include "jdkmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <projectexplorer/kitaspect.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/detailswidget.h>
#include <utils/groupedmodel.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QFutureWatcher>
#include <QPushButton>

#ifdef WITH_TESTS
#include "javakitaspect_test.h"

#include <projectexplorer/kit.h>

#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace ProjectExplorer;
using namespace Utils;

namespace Java::Internal {

class JdkModel final : public TypedGroupedModel<Jdk>
{
public:
    JdkModel();

    int rowForId(Id id) const;
    void addJdk(const Jdk &jdk);
    void removeJdk(Id id);
    void updateJdk(Id id, const QString &name, const FilePath &javaHome);

    int cloneRow(int row) override;
    void markRemoved(int row) override;
    void setVolatileDefaultRow(int row) override;
    void redetect(const QList<Jdk> &detected);
    void follow();
    void apply() final;

private:
    void showDefault();
    void theListAnsweredAgain();
    void show(const QList<Jdk> &jdks);

    QVariant variantData(int row, int column, int role) const override;

    // The entry the page shows as the default, which is the one the machine
    // answers with where the user has picked none. Showing it is not picking
    // it, so applying a page nobody touched the default of must not hand it
    // back as a choice. Invalid from the moment the user moves the flag
    // themselves, which is what makes it a choice.
    Id m_shownDefaultId;
};

JdkModel::JdkModel()
{
    setShowDefault(true);
    setHeader({Tr::tr("Name"), Tr::tr("Location")});
    setFilters(ProjectExplorer::Constants::msgAutoDetected(),
               {{ProjectExplorer::Constants::msgManual(), [this](int row) {
                     const Jdk jdk = item(row);
                     return !jdk.detectionSource.isAutoDetected();
                 }}});

    setItems(JdkManager::jdks());
    showDefault();

    // The list goes on turning while this page is open: a plugin enters the
    // JDK of a kit of its own, and at the start of a session the looking at
    // the machine comes back. Applying writes the page back whole, so a row
    // left as it was would put a JDK the list has dropped back into it.
    connect(JdkManager::instance(), &JdkManager::jdksChanged, this, &JdkModel::follow);
}

int JdkModel::rowForId(Id id) const
{
    if (!id.isValid())
        return -1;
    for (int row = 0; row < itemCount(); ++row) {
        if (item(row).id == id)
            return row;
    }
    return -1;
}

// What a re-detection found or lost is a change of the page like any the
// user makes by hand, so it holds until they apply it and Cancel undoes it.
void JdkModel::addJdk(const Jdk &jdk)
{
    if (rowForId(jdk.id) < 0)
        appendVolatileItem(jdk);
}

void JdkModel::removeJdk(Id id)
{
    if (const int row = rowForId(id); row >= 0 && !isRemoved(row))
        markRemoved(row);
}

void JdkModel::updateJdk(Id id, const QString &name, const FilePath &javaHome)
{
    // The row may have gone while its name was being typed in: the list
    // answers again, and what it dropped is no row to write to.
    const int row = rowForId(id);
    if (row < 0)
        return;

    Jdk jdk = item(row);
    if (jdk.name == name && jdk.javaHome == javaHome)
        return;
    jdk.name = name;
    jdk.javaHome = javaHome;
    setVolatileItem(row, jdk);
    notifyRowChanged(row);
}

int JdkModel::cloneRow(int row)
{
    Jdk jdk = item(row);
    jdk.id = Id::generate();
    jdk.name = Tr::tr("Clone of %1").arg(jdk.name);
    // Plainly the user's own entry, whatever row it was made from: what the
    // row it came from was entered by is nothing to it, and a mark carried
    // across would have the detection take the clone over.
    jdk.detectionSource = DetectionSource::Manual;
    return appendVolatileItem(jdk);
}

void JdkModel::markRemoved(int row)
{
    // Only a removal that moves the flag is the list answering again: where
    // the user has picked another row since, this one holds no flag to move.
    const bool heldTheDefault = isDefault(row);
    GroupedModel::markRemoved(row);
    if (heldTheDefault)
        theListAnsweredAgain();
}

// Moving the flag is the user picking, whoever asked for it: the button,
// or taking back a removal that moved it off the row they had picked. What
// the page was showing before is no answer of the machine any more.
void JdkModel::setVolatileDefaultRow(int row)
{
    GroupedModel::setVolatileDefaultRow(row);
    m_shownDefaultId = Id();
}

// The flag has gone to another row because the one that held it is gone.
// That is the list answering for itself again rather than the user picking,
// so the page goes on showing an answer rather than a choice.
void JdkModel::theListAnsweredAgain()
{
    const int defaultRow = GroupedModel::defaultRow();
    m_shownDefaultId = defaultRow >= 0 ? item(defaultRow).id : Id();
}

// Brings the rows the user has not touched in line with the list, which
// theirs are no business of: a row they changed is what they will apply.
// Picking a default touches no row - the flag only moves from one to
// another - so the two rows it moves between follow the list like the rest.
void JdkModel::follow()
{
    const QList<Jdk> jdks = JdkManager::jdks();
    for (int row = itemCount() - 1; row >= 0; --row) {
        if (isItemDirty(row))
            continue;
        const Jdk shown = item(row);
        const Jdk ofTheList = Utils::findOrDefault(jdks, [&shown](const Jdk &jdk) {
            return jdk.id == shown.id;
        });
        if (ofTheList.id.isValid()) {
            if (!(ofTheList == shown))
                resetItem(row, ofTheList);
            continue;
        }
        const bool heldTheDefault = isDefault(row);
        removeItem(row);
        if (heldTheDefault)
            theListAnsweredAgain();
    }

    for (const Jdk &jdk : jdks) {
        if (rowForId(jdk.id) < 0)
            appendItem(jdk);
    }

    // What the machine answers with is another JDK now that the list holds
    // others - the looking at it comes back with the ones no kit named - and
    // a page showing an answer is to show the one there is. A page showing a
    // choice of the user goes on showing theirs.
    const int shown = GroupedModel::defaultRow();
    if (shown < 0 || item(shown).id == m_shownDefaultId)
        showDefault();
}

void JdkModel::redetect(const QList<Jdk> &detected)
{
    // The merge the start of a session makes, so that pressing Re-detect and
    // starting over come to the same list. A row the user has taken out is
    // out: the machine having that JDK brings it back as one of its own
    // rather than putting their row back.
    QList<Jdk> current;
    for (int row = 0; row < itemCount(); ++row) {
        if (!isRemoved(row))
            current << item(row);
    }
    show(JdkManager::mergedJdks(current, detected));
}

// Brings the rows in line with \a jdks: what is no longer among them goes,
// what has come to be called something else or to lie elsewhere says so, and
// what is new is added. Volatile throughout, since the page is applied as a
// whole or not at all.
void JdkModel::show(const QList<Jdk> &jdks)
{
    // Backwards, since a row added since the page was filled is taken out of
    // the model rather than struck through, and the rows after it move up.
    for (int row = itemCount() - 1; row >= 0; --row) {
        const Jdk jdk = item(row);
        const Jdk merged = Utils::findOrDefault(jdks, [&jdk](const Jdk &entry) {
            return entry.id == jdk.id;
        });
        if (!merged.id.isValid()) {
            if (!isRemoved(row))
                markRemoved(row);
            continue;
        }
        if (!(merged == jdk)) {
            setVolatileItem(row, merged);
            notifyRowChanged(row);
        }
    }

    for (const Jdk &jdk : jdks)
        addJdk(jdk);
}

void JdkModel::showDefault()
{
    m_shownDefaultId = JdkManager::defaultJdkId();
    // The flag is about the rows the page was filled with. A row the user
    // added is none of them, and no JDK the list answers with is on one.
    const int row = rowForId(m_shownDefaultId);
    if (row < 0 || !isAdded(row))
        setDefaultRow(row);
}

void JdkModel::apply()
{
    QList<Jdk> jdks;
    for (int row = 0; row < itemCount(); ++row) {
        if (isRemoved(row))
            continue;
        const Jdk jdk = item(row);
        if (!jdk.javaHome.isEmpty()) {
            jdks << jdk;
            continue;
        }
        // A row whose location is empty names no JDK. Storing it would last
        // until the next start, which reads no entry without one: a kit
        // pointed at it in the meantime would be moved to another JDK there
        // and then. The row stays on the page to be filled in - and where
        // the list holds the entry already, so does what it holds there:
        // emptying the field is not taking the JDK away from the kits that
        // name it, and taking a row out is what the Remove button is for.
        if (const Jdk stored = JdkManager::jdk(jdk.id); stored.id.isValid())
            jdks << stored;
    }

    const int defaultRow = GroupedModel::defaultRow();
    const Id shown = defaultRow >= 0 ? item(defaultRow).id : Id();
    // What the user picked is the flag on a row that neither held it when the
    // page was filled nor was left with it by a removal. The row the page
    // opened on is the answer of the machine where they have picked nothing,
    // and showing that answer is not picking it - which holds for a row the
    // flag is put back on when a removal is taken back as well.
    const bool ofTheMachine = (defaultRow >= 0 && isOriginalDefault(defaultRow))
                              || shown == m_shownDefaultId;
    // And a pick is of a JDK the list comes to hold. A row the page leaves
    // out of it names none, and its id stored as the choice of the user would
    // sit there unanswered until the row is filled in - and take the default
    // over then, long after the page that showed another one was applied.
    const bool ofTheList = Utils::anyOf(jdks, [shown](const Jdk &jdk) {
        return jdk.id == shown;
    });
    const Id defaultId = ofTheMachine || !ofTheList ? JdkManager::chosenDefaultJdkId() : shown;

    JdkManager::setJdks(jdks, defaultId);
    GroupedModel::apply();

    // The list answers with a default of its own where the row that held it
    // is one of those just removed, and the page is to show the default there
    // is.
    showDefault();
}

QVariant JdkModel::variantData(int row, int column, int role) const
{
    const Jdk jdk = item(row);
    switch (role) {
    case Qt::DisplayRole:
        return column == 0 ? jdk.name : jdk.javaHome.toUserOutput();
    case Qt::DecorationRole:
        if (column == 0 && !jdk.hasCompiler())
            return Icons::CRITICAL.icon();
        return {};
    case Qt::ToolTipRole: {
        if (jdk.javaHome.isEmpty())
            return Tr::tr("No location set.");
        if (!jdk.hasJava()) {
            return Tr::tr("\"%1\" has no java in its bin directory.")
                .arg(jdk.javaHome.toUserOutput());
        }
        if (!jdk.hasCompiler()) {
            return Tr::tr("\"%1\" has no javac in its bin directory, so it is a runtime "
                          "environment rather than a development kit.")
                .arg(jdk.javaHome.toUserOutput());
        }
        const QString version = javaVersionIn(jdk.javaHome);
        return version.isEmpty() ? QString() : Tr::tr("Version: %1").arg(version);
    }
    case KitAspect::IdRole:
        return jdk.id.toSetting();
    case KitAspect::QualityRole:
        return jdk.hasCompiler() ? 2 : (jdk.hasJava() ? 1 : 0);
    default:
        return {};
    }
}

class JdkOptionsPageWidget final : public Core::IOptionsPageWidget
{
public:
    JdkOptionsPageWidget();

private:
    void apply() final { m_model.apply(); }
    void cancel() final { m_model.cancel(); }
    bool isDirty() const final { return m_model.isDirty(); }

    void addJdk();
    void redetect();
    void currentJdkChanged(int row);
    void load(const Jdk &jdk);
    void store();

    JdkModel m_model;
    QFutureWatcher<QList<Jdk>> m_detection;
    GroupedView m_groupedView{m_model};
    QPushButton m_addButton;
    QPushButton m_detectButton;
    DetailsWidget m_container;

    QWidget m_itemConfigWidget;
    AspectContainer m_data;
    StringAspect m_name{&m_data};
    FilePathAspect m_javaHome{&m_data};
    StringAspect m_version{&m_data};
    Id m_id;
    bool m_loadingItem = false;
};

JdkOptionsPageWidget::JdkOptionsPageWidget()
{
    m_addButton.setText(Tr::tr("Add"));
    m_detectButton.setText(Tr::tr("Re-detect"));

    m_name.setDisplayStyle(StringAspect::LineEditDisplay);
    m_name.setLabelText(Tr::tr("Name:"));

    m_javaHome.setExpectedKind(PathChooserKind::ExistingDirectory);
    m_javaHome.setHistoryCompleter("PE.JavaHome.History");
    m_javaHome.setLabelText(Tr::tr("Location:"));
    m_javaHome.setToolTip(Tr::tr("The directory that JAVA_HOME is to point at, which holds "
                                 "bin/java and bin/javac."));

    m_version.setDisplayStyle(StringAspect::LabelDisplay);
    m_version.setLabelText(Tr::tr("Version:"));

    m_container.setState(DetailsWidget::NoSummary);
    m_container.setVisible(false);
    m_container.setWidget(&m_itemConfigWidget);

    using namespace Layouting;
    Form {
        m_name, br,
        m_javaHome, br,
        m_version, br,
        noMargin,
    }.attachTo(&m_itemConfigWidget);

    Row {
        Column {
            m_groupedView.view(),
            m_container,
        },
        Column {
            m_addButton,
            m_groupedView.cloneButton(),
            m_groupedView.removeButton(),
            m_groupedView.makeDefaultButton(),
            m_detectButton,
            st,
        },
    }.attachTo(this);

    m_groupedView.makeDefaultButton().setToolTip(
        Tr::tr("Set as the JDK a kit that has yet to be given one is set up with."));
    m_groupedView.setCanRemoveRow([this](int row) {
        return !m_model.item(row).detectionSource.isAutoDetected();
    });
    // A row that names no directory yet is no answer either, and a kit on
    // another device puts the JDK of that device into the list - which is no
    // answer for a kit that builds here: picking one would leave every kit of
    // this machine without a JDK at all.
    m_groupedView.setCanMakeRowDefault([this](int row) {
        const FilePath javaHome = m_model.item(row).javaHome;
        return !javaHome.isEmpty() && javaHome.isLocal();
    });

    connect(&m_groupedView, &GroupedView::currentRowChanged,
            this, [this](int, int row) { currentJdkChanged(row); }, Qt::QueuedConnection);
    connect(&m_addButton, &QAbstractButton::clicked, this, &JdkOptionsPageWidget::addJdk);
    connect(&m_detectButton, &QAbstractButton::clicked, this, &JdkOptionsPageWidget::redetect);
    connect(&m_detection, &QFutureWatcherBase::finished, this, [this] {
        m_detectButton.setEnabled(true);
        if (!m_detection.isCanceled())
            m_model.redetect(m_detection.result());
    });

    m_name.addOnVolatileValueChanged(this, [this] { store(); });
    m_javaHome.addOnVolatileValueChanged(this, [this] {
        store();
        m_version.setValue(javaVersionIn(m_javaHome.expandedVolatileValue()));
    });

    connect(&m_data, &AspectContainer::changed, &checkSettingsDirty);
}

void JdkOptionsPageWidget::store()
{
    if (!m_loadingItem && m_id.isValid())
        m_model.updateJdk(m_id, m_name.volatileValue(), m_javaHome.expandedVolatileValue());
}

void JdkOptionsPageWidget::load(const Jdk &jdk)
{
    m_loadingItem = true; // Avoid intermediate signal handling.
    m_id = Id();

    const bool autoDetected = jdk.detectionSource.isAutoDetected();
    m_name.setEnabled(!autoDetected);
    m_name.setValue(jdk.name);
    m_javaHome.setReadOnly(autoDetected);
    m_javaHome.setValue(jdk.javaHome);
    m_version.setValue(javaVersionIn(jdk.javaHome));

    m_id = jdk.id;
    m_loadingItem = false;
}

void JdkOptionsPageWidget::currentJdkChanged(int row)
{
    const bool hasItem = row >= 0 && row < m_model.itemCount();
    load(hasItem ? m_model.item(row) : Jdk());
    m_container.setVisible(hasItem);
}

void JdkOptionsPageWidget::addJdk()
{
    Jdk jdk;
    jdk.id = Id::generate();
    jdk.name = Utils::makeUniquelyNumbered(Tr::tr("New JDK"),
                                           Utils::transform(m_model.volatileItems(), &Jdk::name));
    jdk.detectionSource = DetectionSource::Manual;
    m_groupedView.selectRow(m_model.appendVolatileItem(jdk));
}

void JdkOptionsPageWidget::redetect()
{
    // Looking where every system keeps its JDKs reads directories, and the
    // tool of macOS takes longer still to answer that the machine has none
    // at all. Neither belongs on the thread the GUI is on, so the button
    // stands pressed until the answer is in.
    m_detectButton.setEnabled(false);
    m_detection.setFuture(JdkManager::detectJdksAsync());
}

class JdkOptionsPage final : public Core::IOptionsPage
{
public:
    JdkOptionsPage()
    {
        setId(Constants::JDK_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("JDKs"));
        setCategory(ProjectExplorer::Constants::KITS_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new JdkOptionsPageWidget; });
    }
};

void setupJdkOptionsPage()
{
    static JdkOptionsPage theJdkOptionsPage;
}

#ifdef WITH_TESTS

// Enough of a JDK for the list: a directory that holds a java and a javac.
static FilePath makeFakeJdk(const QTemporaryDir &dir, const QString &name)
{
    const FilePath bin = FilePath::fromString(dir.path()) / name / "bin";
    if (!bin.ensureWritableDir())
        return {};
    for (const QString &tool : {QString("java"), QString("javac")}) {
        const FilePath path = (bin / tool).withExecutableSuffix();
        if (!path.writeFileContents("#!/bin/sh\n"))
            return {};
        path.setPermissions(path.permissions() | QFile::ExeOwner | QFile::ExeUser);
    }
    return bin.parentDir();
}

class JdkOptionsPageTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        m_detected = makeFakeJdk(m_dir, "jdk-detected");
        m_ofTheUser = makeFakeJdk(m_dir, "jdk-of-the-user");
        QVERIFY(!m_detected.isEmpty());
        QVERIFY(!m_ofTheUser.isEmpty());

        m_savedJdks = JdkManager::jdks();
        m_savedDefaultId = JdkManager::chosenDefaultJdkId();
        // Every change to the list has the JavaKitAspect factory bring every
        // kit of the session in line with it, so what the kits of whoever
        // started this named is put back along with the list.
        m_savedJdksOfTheKits = theJdksOfTheKits();
    }

    void cleanupTestCase() { putTheListBack(); }

    void init() { JdkManager::setJdks({jdk(m_detected, DetectionSource::FromSystem),
                                       jdk(m_ofTheUser, DetectionSource::Manual)}, Id()); }

    void cleanup() { putTheListBack(); }

    // The page shows the JDK the machine answers with as the default, and
    // showing it is not picking it: applying a page nobody touched the
    // default of leaves the question as open as it was.
    void testApplyingAnUntouchedPagePicksNoDefault()
    {
        JdkModel model;
        QVERIFY(JdkManager::defaultJdkId().isValid());

        model.apply();

        QVERIFY2(!JdkManager::chosenDefaultJdkId().isValid(),
                 "the answer of the machine was written into the settings as though the user "
                 "had picked it, so a JDK installed later no longer moves the default");
    }

    // Taking the row that holds the default out of the list moves the flag to
    // another row. That is the list answering again, not the user picking.
    void testRemovingTheDefaultRowPicksNoDefault()
    {
        JdkModel model;
        const Id wasDefault = JdkManager::defaultJdkId();
        QVERIFY(wasDefault.isValid());

        model.removeJdk(wasDefault);
        model.apply();

        QVERIFY2(!JdkManager::chosenDefaultJdkId().isValid(),
                 "the row the flag fell to was stored as the choice of the user, so the "
                 "default stops following the machine");
    }

    void testPickingADefaultKeepsIt()
    {
        JdkModel model;
        const Id theirs = JdkManager::jdkForJavaHome(m_ofTheUser).id;
        QVERIFY(theirs.isValid());

        model.setVolatileDefaultRow(model.rowForId(theirs));
        model.apply();

        QCOMPARE(JdkManager::chosenDefaultJdkId(), theirs);
    }

    // Taking a row out of the list after picking a default is no reason to
    // lose the pick: the row that is going holds no flag to move.
    void testKeepsAPickWhenTheRowThatHeldTheDefaultGoes()
    {
        JdkModel model;
        const Id wasDefault = JdkManager::defaultJdkId();
        const Id theirs = JdkManager::jdkForJavaHome(m_ofTheUser).id;
        QVERIFY(wasDefault.isValid());
        QVERIFY(theirs.isValid());
        QVERIFY(wasDefault != theirs);

        model.setVolatileDefaultRow(model.rowForId(theirs));
        model.removeJdk(wasDefault);
        model.apply();

        QCOMPARE(JdkManager::chosenDefaultJdkId(), theirs);
    }

    // The flag falling to another row is the list answering, but only until
    // the user moves it themselves. What they pick afterwards is a choice of
    // theirs, whichever row the flag was left on in between.
    void testKeepsAPickMadeAfterTheFlagFellToAnotherRow()
    {
        const FilePath third = makeFakeJdk(m_dir, "jdk-a-third");
        QVERIFY(!third.isEmpty());
        JdkManager::setJdks(JdkManager::jdks() + QList<Jdk>{jdk(third, DetectionSource::Manual)},
                            Id());

        JdkModel model;
        const Id wasDefault = JdkManager::defaultJdkId();
        const Id theirs = JdkManager::jdkForJavaHome(m_ofTheUser).id;
        const Id other = JdkManager::jdkForJavaHome(third).id;
        QVERIFY(wasDefault.isValid());
        QVERIFY(theirs.isValid());
        QVERIFY(other.isValid());
        QVERIFY(wasDefault != theirs);

        model.removeJdk(wasDefault);
        model.setVolatileDefaultRow(model.rowForId(other));
        model.setVolatileDefaultRow(model.rowForId(theirs));
        model.apply();

        QVERIFY2(JdkManager::chosenDefaultJdkId() == theirs,
                 "the pick of the user was dropped as though the flag had only fallen where "
                 "it is, so the kits go on being set up with another JDK");
    }

    // The button is to follow the field the user fills in, not only the row
    // they are on: a row added by the Add button names no directory yet, so
    // the button starts out off and is to come on once they name one.
    void testMakeDefaultFollowsTheLocationOfTheRow()
    {
        JdkOptionsPageWidget page;
        QPushButton *add = buttonOf(page, Tr::tr("Add"));
        QPushButton *makeDefault = buttonOf(page, Tr::tr("Make Default"));
        QVERIFY(add);
        QVERIFY(makeDefault);

        add->click();
        // The field is filled in from the row that was selected through a
        // queued connection, so that it is the row the typing then goes to.
        QCoreApplication::processEvents();
        QVERIFY2(!makeDefault->isEnabled(), "a row that names no directory could be made the "
                                            "default, which is what the page leaves out of "
                                            "the list");

        PathChooser *location = page.findChild<PathChooser *>();
        QVERIFY(location);
        location->setFilePath(m_detected);

        QVERIFY2(makeDefault->isEnabled(),
                 "the button went on showing what the row held before the user named a "
                 "directory, so the JDK they added cannot be made the default");
    }

    // A row the page leaves out of the list is no default: the id would sit
    // in the settings unanswered until the row is given a location, and take
    // the default over then.
    void testARowWithoutALocationIsNoDefault()
    {
        JdkModel model;

        Jdk added;
        added.id = Id::generate();
        added.name = "New JDK";
        added.detectionSource = DetectionSource::Manual;
        model.setVolatileDefaultRow(model.appendVolatileItem(added));
        model.apply();

        QVERIFY2(JdkManager::chosenDefaultJdkId() != added.id,
                 "a row that names no directory was stored as the default, which takes the "
                 "default over as soon as the row is given one");
    }

    // Pressing Re-detect and starting over come to the same list: a JDK the
    // machine still has but this run did not name - the JAVA_HOME of the
    // shell Creator was started from last time names one such - is no JDK to
    // take from the kits.
    void testRedetectKeepsAJdkItDidNotNameThisTime()
    {
        JdkModel model;
        const int row = model.rowForId(JdkManager::jdkForJavaHome(m_detected).id);
        QVERIFY(row >= 0);

        model.redetect({});

        QVERIFY2(!model.isRemoved(row),
                 "a JDK the machine still has was taken out of the list, and every kit "
                 "naming it sent to another one");
    }

    // A row the user took out is out, and the machine having that JDK brings
    // it back as one of its own rather than putting their row back.
    void testRedetectBringsBackAJdkOfARowTheUserTookOut()
    {
        JdkModel model;
        const Id theirs = JdkManager::jdkForJavaHome(m_ofTheUser).id;
        QVERIFY(theirs.isValid());
        model.removeJdk(theirs);

        model.redetect({detected(m_ofTheUser)});

        const bool shown = Utils::anyOf(rowsOf(model), [this](const std::pair<Jdk, bool> &row) {
            return !row.second && row.first.javaHome == m_ofTheUser;
        });
        QVERIFY2(shown,
                 "the machine has that JDK and the page does not show it, so applying takes "
                 "it out of the list for the rest of the session");
    }

    // The list goes on turning while the page is open - the looking at the
    // machine comes back at the start of a session - and applying writes the
    // page back whole. A row of a JDK the list has dropped would put it back.
    void testFollowsTheListWhileThePageIsOpen()
    {
        JdkModel model;
        const Id gone = JdkManager::jdkForJavaHome(m_detected).id;
        QVERIFY(gone.isValid());

        JdkManager::setJdks(Utils::filtered(JdkManager::jdks(),
                                            [gone](const Jdk &jdk) { return jdk.id != gone; }),
                            Id());
        model.apply();

        QVERIFY2(!Utils::anyOf(JdkManager::jdks(),
                               [gone](const Jdk &jdk) { return jdk.id == gone; }),
                 "applying the page put a JDK the list had dropped back into it, where no "
                 "row of the page can take it out again");
    }

    // A JDK uninstalled between two re-detections leaves a row to take out,
    // and a row the detection added is taken out of the model rather than
    // struck through, which moves the rows after it up. Two of them next to
    // each other are both to go.
    void testRedetectTakesOutEveryRowWhoseJdkIsGone()
    {
        const FilePath first = makeFakeJdk(m_dir, "jdk-gone-first");
        const FilePath second = makeFakeJdk(m_dir, "jdk-gone-second");
        QVERIFY(!first.isEmpty());
        QVERIFY(!second.isEmpty());

        JdkModel model;
        const Jdk firstJdk = detected(first);
        const Jdk secondJdk = detected(second);
        model.redetect({firstJdk, secondJdk});
        QVERIFY(model.rowForId(firstJdk.id) >= 0);
        QVERIFY(model.rowForId(secondJdk.id) >= 0);

        QVERIFY(first.removeRecursively());
        QVERIFY(second.removeRecursively());
        model.redetect({});

        QVERIFY2(model.rowForId(firstJdk.id) < 0 && model.rowForId(secondJdk.id) < 0,
                 "a row of a JDK the machine no longer has was left on the page, and "
                 "applying enters it into the list again");
    }

    // A pick of the user is about which row holds the flag rather than about
    // what any row holds, so the rows go on following the list - which is
    // what applying writes back.
    void testFollowsTheListWhileThePageHoldsAPick()
    {
        JdkModel model;
        const Id theirs = JdkManager::jdkForJavaHome(m_ofTheUser).id;
        const Id gone = JdkManager::jdkForJavaHome(m_detected).id;
        QVERIFY(theirs.isValid());
        QVERIFY(gone.isValid());

        model.setVolatileDefaultRow(model.rowForId(theirs));
        JdkManager::setJdks(Utils::filtered(JdkManager::jdks(),
                                            [gone](const Jdk &jdk) { return jdk.id != gone; }),
                            Id());
        model.apply();

        QVERIFY2(!Utils::anyOf(JdkManager::jdks(),
                               [gone](const Jdk &jdk) { return jdk.id == gone; }),
                 "a page the user had picked a default on stopped following the list, so "
                 "applying it put a JDK the list had dropped back in");
    }

    // The list dropping the row that holds the default moves the flag to
    // another row. Nobody picked anything, so the page has no change to
    // report for it.
    void testFollowingTheListOverTheDefaultRowIsNoChangeOfThePage()
    {
        JdkModel model;
        const Id wasDefault = JdkManager::defaultJdkId();
        QVERIFY(wasDefault.isValid());
        QVERIFY(!model.isDirty());

        JdkManager::setJdks(Utils::filtered(JdkManager::jdks(),
                                            [wasDefault](const Jdk &jdk) {
                                                return jdk.id != wasDefault;
                                            }),
                            Id());

        QVERIFY2(!model.isDirty(),
                 "the page reports a change nobody made, so the settings dialog offers to "
                 "apply it and asks about unsaved changes on closing");
    }

    // What the settings dialog shows is what the hook told it last, which
    // fires while the flag is being moved as well as after.
    void testFollowingTheListOverTheDefaultRowTellsTheDialogOfNoChange()
    {
        JdkModel model;
        const Id wasDefault = JdkManager::defaultJdkId();
        QVERIFY(wasDefault.isValid());

        std::optional<bool> heardLast;
        Utils::Internal::setCheckSettingsDirtyHook([&model, &heardLast] {
            heardLast = model.isDirty();
        });
        const QScopeGuard dropTheHook([] { Utils::Internal::setCheckSettingsDirtyHook({}); });

        JdkManager::setJdks(Utils::filtered(JdkManager::jdks(),
                                            [wasDefault](const Jdk &jdk) {
                                                return jdk.id != wasDefault;
                                            }),
                            Id());

        QVERIFY(heardLast.has_value());
        QVERIFY2(!*heardLast,
                 "the settings dialog was last told of a change nobody made, and nothing "
                 "tells it otherwise");
    }

    // The default of the machine is another JDK once the looking at it comes
    // back, and a page showing an answer rather than a choice is to show the
    // answer there is: the kits are set up with that one.
    void testFollowsTheDefaultTheListAnswersWith()
    {
        JdkModel model;
        const FilePath found = makeFakeJdk(m_dir, "jdk-found-later");
        QVERIFY(!found.isEmpty());

        Jdk ofTheMachine = jdk(found, DetectionSource::FromSystem);
        JdkManager::setJdks(QList<Jdk>{ofTheMachine} + JdkManager::jdks(), Id());

        QCOMPARE(JdkManager::defaultJdkId(), ofTheMachine.id);
        QCOMPARE(model.item(model.defaultRow()).id, JdkManager::defaultJdkId());
    }

    // A clone is the user's own entry, whatever row it was made from. One
    // made from a row a kit entered would otherwise carry that kit's mark,
    // and the machine would take the clone over on finding that JDK.
    void testTheMachineDoesNotTakeOverACloneOfARowAKitEntered()
    {
        const Jdk ofAKit = jdk(m_ofTheUser, DetectionSource::Manual);
        JdkManager::setJdks({withTheMarkOfAKit(ofAKit)}, Id());

        JdkModel model;
        const int clone = model.cloneRow(model.rowForId(ofAKit.id));
        QVERIFY(clone >= 0);
        const Id theirs = model.item(clone).id;
        model.removeJdk(ofAKit.id);
        model.apply();

        JdkManager::enterDetectedJdks({detected(m_ofTheUser)});

        QVERIFY2(!JdkManager::jdk(theirs).detectionSource.isAutoDetected(),
                 "the machine took over an entry the user made, which the page then neither "
                 "lets them edit nor lets them remove");
    }

    // Emptying the location of an entry the list holds is not taking the JDK
    // away from the kits that name it: those are moved to another JDK where
    // the entry goes, which the page shows nothing of and Cancel undoes
    // nothing of.
    void testKeepsAnEntryWhoseLocationTheUserEmptied()
    {
        JdkModel model;
        const Id theirs = JdkManager::jdkForJavaHome(m_ofTheUser).id;
        QVERIFY(theirs.isValid());

        Jdk emptied = model.item(model.rowForId(theirs));
        emptied.javaHome = {};
        model.setVolatileItem(model.rowForId(theirs), emptied);
        model.apply();

        QCOMPARE(JdkManager::jdk(theirs).javaHome, m_ofTheUser);
    }

    // A row the location of which was never filled in is what the Add button
    // leaves behind. The next start reads no entry without one, so storing it
    // would take a kit that names it to another JDK there and then.
    void testLeavesARowWithoutALocationOutOfTheList()
    {
        JdkModel model;
        const int before = JdkManager::jdks().size();

        Jdk added;
        added.id = Id::generate();
        added.name = "New JDK";
        added.detectionSource = DetectionSource::Manual;
        model.appendVolatileItem(added);
        model.apply();

        QCOMPARE(JdkManager::jdks().size(), before);
        QVERIFY2(!Utils::anyOf(JdkManager::jdks(),
                               [&added](const Jdk &entry) { return entry.id == added.id; }),
                 "a row that names no directory was entered into the list, which the next "
                 "start drops again");
    }

private:
    static Jdk withTheMarkOfAKit(Jdk jdk)
    {
        jdk.detectionSource = {DetectionSource::Manual, JDK_NAMED_BY_A_KIT};
        return jdk;
    }

    static Jdk detected(const FilePath &javaHome)
    {
        return jdk(javaHome, DetectionSource::FromSystem);
    }

    static QList<std::pair<Jdk, bool>> rowsOf(const JdkModel &model)
    {
        QList<std::pair<Jdk, bool>> rows;
        for (int row = 0; row < model.itemCount(); ++row)
            rows << std::make_pair(model.item(row), model.isRemoved(row));
        return rows;
    }

    static Jdk jdk(const FilePath &javaHome, DetectionSource::DetectionType type)
    {
        Jdk jdk;
        jdk.id = Id::generate();
        jdk.name = javaHome.fileName();
        jdk.javaHome = javaHome;
        jdk.detectionSource = {type, "JDK"};
        return jdk;
    }

    static QPushButton *buttonOf(const QWidget &page, const QString &text)
    {
        const QList<QPushButton *> buttons = page.findChildren<QPushButton *>();
        return Utils::findOrDefault(buttons, [&text](const QPushButton *button) {
            return button->text() == text;
        });
    }

    void putTheListBack()
    {
        JdkManager::setJdks(m_savedJdks, m_savedDefaultId);
        putTheJdksOfTheKitsBack(m_savedJdksOfTheKits);
    }

    QTemporaryDir m_dir;
    FilePath m_detected;
    FilePath m_ofTheUser;
    QList<Jdk> m_savedJdks;
    Id m_savedDefaultId;
    QHash<Kit *, JdkOfAKit> m_savedJdksOfTheKits;
};

QObject *createJdkOptionsPageTest()
{
    return new JdkOptionsPageTest;
}

#endif // WITH_TESTS

} // namespace Java::Internal

#ifdef WITH_TESTS
#include "jdkoptionspage.moc"
#endif
