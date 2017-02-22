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

#include "debuggeritemmanager.h"
#include "debuggeritem.h"
#include "debuggerkitinformation.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <utils/treemodel.h>
#include <utils/winutils.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
#include <QPointer>
#include <QPushButton>
#include <QTreeView>
#include <QWidget>

using namespace Debugger::Internal;
using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
namespace Internal {

const char DEBUGGER_COUNT_KEY[] = "DebuggerItem.Count";
const char DEBUGGER_DATA_KEY[] = "DebuggerItem.";
const char DEBUGGER_LEGACY_FILENAME[] = "/qtcreator/profiles.xml";
const char DEBUGGER_FILE_VERSION_KEY[] = "Version";
const char DEBUGGER_FILENAME[] = "/qtcreator/debuggers.xml";
const char debuggingToolsWikiLinkC[] = "http://wiki.qt.io/Qt_Creator_Windows_Debugging";

class DebuggerItemModel;

class DebuggerItemManagerPrivate
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerItemManager)
public:
    DebuggerItemManagerPrivate();
    ~DebuggerItemManagerPrivate();

    void restoreDebuggers();
    void saveDebuggers();

    void addDebugger(const DebuggerItem &item);
    QVariant registerDebugger(const DebuggerItem &item);
    void readDebuggers(const FileName &fileName, bool isSystem);
    void autoDetectCdbDebuggers();
    void autoDetectGdbOrLldbDebuggers();
    void readLegacyDebuggers(const FileName &file);
    QString uniqueDisplayName(const QString &base);

    PersistentSettingsWriter m_writer;
    DebuggerItemModel *m_model;
    IOptionsPage *m_optionsPage = 0;
};

static DebuggerItemManagerPrivate *d = 0;

// -----------------------------------------------------------------------
// DebuggerItemConfigWidget
// -----------------------------------------------------------------------

class DebuggerItemConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerItemManager)

public:
    explicit DebuggerItemConfigWidget();
    void load(const DebuggerItem *item);
    void store() const;

private:
    void binaryPathHasChanged();
    DebuggerItem item() const;
    void setAbis(const QStringList &abiNames);

    QLineEdit *m_displayNameLineEdit;
    QLineEdit *m_typeLineEdit;
    QLabel *m_cdbLabel;
    QLineEdit *m_versionLabel;
    PathChooser *m_binaryChooser;
    PathChooser *m_workingDirectoryChooser;
    QLineEdit *m_abis;
    bool m_autodetected = false;
    DebuggerEngineType m_engineType = NoEngineType;
    QVariant m_id;
};

// --------------------------------------------------------------------------
// DebuggerTreeItem
// --------------------------------------------------------------------------

class DebuggerTreeItem : public TreeItem
{
public:
    DebuggerTreeItem(const DebuggerItem &item, bool changed)
        : m_item(item), m_orig(item), m_added(changed), m_changed(changed)
    {}

    QVariant data(int column, int role) const
    {
        switch (role) {
            case Qt::DisplayRole:
                switch (column) {
                case 0: return m_item.displayName();
                case 1: return m_item.command().toUserOutput();
                case 2: return m_item.engineTypeName();
                }
                break;

            case Qt::FontRole: {
                QFont font;
                if (m_changed)
                    font.setBold(true);
                if (m_removed)
                    font.setStrikeOut(true);
                return font;
            }

            case Qt::DecorationRole:
                if (column == 0)
                    return m_item.decoration();
                break;

            case Qt::ToolTipRole:
                return m_item.validityMessage();
        }
        return QVariant();
    }

    DebuggerItem m_item; // Displayed, possibly unapplied data.
    DebuggerItem m_orig; // Stored original data.
    bool m_added;
    bool m_changed;
    bool m_removed = false;
};

// --------------------------------------------------------------------------
// DebuggerItemModel
// --------------------------------------------------------------------------

class DebuggerItemModel : public TreeModel<TreeItem, StaticTreeItem, DebuggerTreeItem>
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerOptionsPage)

public:
    DebuggerItemModel();

    QModelIndex lastIndex() const;
    void setCurrentIndex(const QModelIndex &index);
    void addDebugger(const DebuggerItem &item, bool changed = false);
    void updateDebugger(const DebuggerItem &item);
    void apply();
    void cancel();

    DebuggerTreeItem *m_currentTreeItem = nullptr;
};

template <class Predicate>
void forAllDebuggers(const Predicate &pred)
{
    d->m_model->forItemsAtLevel<2>([pred](DebuggerTreeItem *titem) {
        pred(titem->m_item);
    });
}

template <class Predicate>
const DebuggerItem *findDebugger(const Predicate &pred)
{
    DebuggerTreeItem *titem = d->m_model->findItemAtLevel<2>([pred](DebuggerTreeItem *titem) {
        return pred(titem->m_item);
    });
    return titem ? &titem->m_item : nullptr;
}

DebuggerItemModel::DebuggerItemModel()
{
    setHeader({tr("Name"), tr("Location"), tr("Type")});
    rootItem()->appendChild(new StaticTreeItem(tr("Auto-detected")));
    rootItem()->appendChild(new StaticTreeItem(tr("Manual")));
}

void DebuggerItemModel::addDebugger(const DebuggerItem &item, bool changed)
{
    QTC_ASSERT(item.id().isValid(), return);
    int group = item.isAutoDetected() ? 0 : 1;
    rootItem()->childAt(group)->appendChild(new DebuggerTreeItem(item, changed));
}

void DebuggerItemModel::updateDebugger(const DebuggerItem &item)
{
    auto matcher = [item](DebuggerTreeItem *n) { return n->m_item.m_id == item.id(); };
    DebuggerTreeItem *treeItem = findItemAtLevel<2>(matcher);
    QTC_ASSERT(treeItem, return);

    TreeItem *parent = treeItem->parent();
    QTC_ASSERT(parent, return);

    treeItem->m_changed = treeItem->m_orig != item;
    treeItem->m_item = item;
    treeItem->update(); // Notify views.
}

QModelIndex DebuggerItemModel::lastIndex() const
{
    TreeItem *manualGroup = rootItem()->lastChild();
    TreeItem *lastItem = manualGroup->lastChild();
    return lastItem ? indexForItem(lastItem) : QModelIndex();
}

void DebuggerItemModel::apply()
{
    QList<DebuggerTreeItem *> toRemove;
    forItemsAtLevel<2>([&toRemove](DebuggerTreeItem *titem) {
        titem->m_added = false;
        if (titem->m_changed) {
            titem->m_changed = false;
            titem->m_orig = titem->m_item;
        }
        if (titem->m_removed)
            toRemove.append(titem);
    });
    for (DebuggerTreeItem *titem : toRemove)
        destroyItem(titem);
}

void DebuggerItemModel::cancel()
{
    QList<DebuggerTreeItem *> toRemove;
    forItemsAtLevel<2>([&toRemove](DebuggerTreeItem *titem) {
        titem->m_removed = false;
        if (titem->m_changed) {
            titem->m_changed = false;
            titem->m_item = titem->m_orig;
        }
        if (titem->m_added)
            toRemove.append(titem);
    });
    for (DebuggerTreeItem *titem : toRemove)
        destroyItem(titem);
}

void DebuggerItemModel::setCurrentIndex(const QModelIndex &index)
{
    TreeItem *treeItem = itemForIndex(index);
    m_currentTreeItem = treeItem && treeItem->level() == 2 ? static_cast<DebuggerTreeItem *>(treeItem) : 0;
}

DebuggerItemConfigWidget::DebuggerItemConfigWidget()
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_typeLineEdit = new QLineEdit(this);
    m_typeLineEdit->setEnabled(false);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);
    m_binaryChooser->setHistoryCompleter("DebuggerPaths");

    m_workingDirectoryChooser = new PathChooser(this);
    m_workingDirectoryChooser->setExpectedKind(PathChooser::Directory);
    m_workingDirectoryChooser->setMinimumWidth(400);
    m_workingDirectoryChooser->setHistoryCompleter("DebuggerPaths");

    m_cdbLabel = new QLabel(this);
    m_cdbLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_cdbLabel->setOpenExternalLinks(true);

    m_versionLabel = new QLineEdit(this);
    m_versionLabel->setPlaceholderText(tr("Unknown"));
    m_versionLabel->setEnabled(false);

    m_abis = new QLineEdit(this);
    m_abis->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
    formLayout->addRow(m_cdbLabel);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("Type:")), m_typeLineEdit);
    formLayout->addRow(new QLabel(tr("ABIs:")), m_abis);
    formLayout->addRow(new QLabel(tr("Version:")), m_versionLabel);
    formLayout->addRow(new QLabel(tr("Working directory:")), m_workingDirectoryChooser);

    connect(m_binaryChooser, &PathChooser::pathChanged,
            this, &DebuggerItemConfigWidget::binaryPathHasChanged);
    connect(m_workingDirectoryChooser, &PathChooser::pathChanged,
            this, &DebuggerItemConfigWidget::store);
    connect(m_displayNameLineEdit, &QLineEdit::textChanged,
            this, &DebuggerItemConfigWidget::store);
}

DebuggerItem DebuggerItemConfigWidget::item() const
{
    DebuggerItem item(m_id);
    item.setUnexpandedDisplayName(m_displayNameLineEdit->text());
    item.setCommand(m_binaryChooser->fileName());
    item.setWorkingDirectory(m_workingDirectoryChooser->fileName());
    item.setAutoDetected(m_autodetected);
    QList<ProjectExplorer::Abi> abiList;
    foreach (const QString &a, m_abis->text().split(QRegExp(QLatin1String("[^A-Za-z0-9-_]+")))) {
        if (a.isNull())
            continue;
        abiList << a;
    }
    item.setAbis(abiList);
    item.setVersion(m_versionLabel->text());
    item.setEngineType(m_engineType);
    return item;
}

void DebuggerItemConfigWidget::store() const
{
    if (!m_id.isNull())
        d->m_model->updateDebugger(item());
}

void DebuggerItemConfigWidget::setAbis(const QStringList &abiNames)
{
    m_abis->setText(abiNames.join(QLatin1String(", ")));
}

void DebuggerItemConfigWidget::load(const DebuggerItem *item)
{
    m_id = QVariant(); // reset Id to avoid intermediate signal handling
    if (!item)
        return;

    // Set values:
    m_autodetected = item->isAutoDetected();

    m_displayNameLineEdit->setEnabled(!item->isAutoDetected());
    m_displayNameLineEdit->setText(item->unexpandedDisplayName());

    m_typeLineEdit->setText(item->engineTypeName());

    m_binaryChooser->setReadOnly(item->isAutoDetected());
    m_binaryChooser->setFileName(item->command());

    m_workingDirectoryChooser->setReadOnly(item->isAutoDetected());
    m_workingDirectoryChooser->setFileName(item->workingDirectory());

    QString text;
    QString versionCommand;
    if (item->engineType() == CdbEngineType) {
        const bool is64bit = is64BitWindowsSystem();
        const QString versionString = is64bit ? tr("64-bit version") : tr("32-bit version");
        //: Label text for path configuration. %2 is "x-bit version".
        text = tr("<html><body><p>Specify the path to the "
                  "<a href=\"%1\">Windows Console Debugger executable</a>"
                  " (%2) here.</p>""</body></html>").
                arg(QLatin1String(debuggingToolsWikiLinkC), versionString);
        versionCommand = QLatin1String("-version");
    } else {
        versionCommand = QLatin1String("--version");
    }

    m_cdbLabel->setText(text);
    m_cdbLabel->setVisible(!text.isEmpty());
    m_binaryChooser->setCommandVersionArguments(QStringList(versionCommand));
    m_versionLabel->setText(item->version());
    setAbis(item->abiNames());
    m_engineType = item->engineType();
    m_id = item->id();
}

void DebuggerItemConfigWidget::binaryPathHasChanged()
{
    // Ignore change if this is no valid DebuggerItem
    if (!m_id.isValid())
        return;

    DebuggerItem tmp;
    QFileInfo fi = QFileInfo(m_binaryChooser->path());
    if (fi.isExecutable()) {
        tmp = item();
        tmp.reinitializeFromFile();
    }

    setAbis(tmp.abiNames());
    m_versionLabel->setText(tmp.version());
    m_engineType = tmp.engineType();
    m_typeLineEdit->setText(tmp.engineTypeName());

    store();
}

// --------------------------------------------------------------------------
// DebuggerConfigWidget
// --------------------------------------------------------------------------

class DebuggerConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerOptionsPage)
public:
    DebuggerConfigWidget()
    {
        m_addButton = new QPushButton(tr("Add"), this);

        m_cloneButton = new QPushButton(tr("Clone"), this);
        m_cloneButton->setEnabled(false);

        m_delButton = new QPushButton(this);
        m_delButton->setEnabled(false);

        m_container = new DetailsWidget(this);
        m_container->setState(DetailsWidget::NoSummary);
        m_container->setVisible(false);

        m_debuggerView = new QTreeView(this);
        m_debuggerView->setModel(d->m_model);
        m_debuggerView->setUniformRowHeights(true);
        m_debuggerView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_debuggerView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_debuggerView->expandAll();

        auto header = m_debuggerView->header();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(2, QHeaderView::Stretch);

        auto buttonLayout = new QVBoxLayout();
        buttonLayout->setSpacing(6);
        buttonLayout->setContentsMargins(0, 0, 0, 0);
        buttonLayout->addWidget(m_addButton);
        buttonLayout->addWidget(m_cloneButton);
        buttonLayout->addWidget(m_delButton);
        buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

        auto verticalLayout = new QVBoxLayout();
        verticalLayout->addWidget(m_debuggerView);
        verticalLayout->addWidget(m_container);

        auto horizontalLayout = new QHBoxLayout(this);
        horizontalLayout->addLayout(verticalLayout);
        horizontalLayout->addLayout(buttonLayout);

        connect(m_debuggerView->selectionModel(), &QItemSelectionModel::currentChanged,
                this, &DebuggerConfigWidget::currentDebuggerChanged, Qt::QueuedConnection);

        connect(m_addButton, &QAbstractButton::clicked,
                this, &DebuggerConfigWidget::addDebugger, Qt::QueuedConnection);
        connect(m_cloneButton, &QAbstractButton::clicked,
                this, &DebuggerConfigWidget::cloneDebugger, Qt::QueuedConnection);
        connect(m_delButton, &QAbstractButton::clicked,
                this, &DebuggerConfigWidget::removeDebugger, Qt::QueuedConnection);

        m_itemConfigWidget = new DebuggerItemConfigWidget;
        m_container->setWidget(m_itemConfigWidget);
        updateButtons();
    }

    void cloneDebugger();
    void addDebugger();
    void removeDebugger();
    void currentDebuggerChanged(const QModelIndex &newCurrent);
    void updateButtons();

    QTreeView *m_debuggerView;
    QPushButton *m_addButton;
    QPushButton *m_cloneButton;
    QPushButton *m_delButton;
    DetailsWidget *m_container;
    DebuggerItemConfigWidget *m_itemConfigWidget;
};

void DebuggerConfigWidget::cloneDebugger()
{
    if (!d->m_model->m_currentTreeItem)
        return;

    DebuggerItem *item = &d->m_model->m_currentTreeItem->m_item;
    DebuggerItem newItem;
    newItem.createId();
    newItem.setCommand(item->command());
    newItem.setUnexpandedDisplayName(d->uniqueDisplayName(tr("Clone of %1").arg(item->displayName())));
    newItem.reinitializeFromFile();
    newItem.setAutoDetected(false);
    d->m_model->addDebugger(newItem, true);
    m_debuggerView->setCurrentIndex(d->m_model->lastIndex());
}

void DebuggerConfigWidget::addDebugger()
{
    DebuggerItem item;
    item.createId();
    item.setAutoDetected(false);
    item.setEngineType(NoEngineType);
    item.setUnexpandedDisplayName(d->uniqueDisplayName(tr("New Debugger")));
    item.setAutoDetected(false);
    d->m_model->addDebugger(item, true);
    m_debuggerView->setCurrentIndex(d->m_model->lastIndex());
}

void DebuggerConfigWidget::removeDebugger()
{
    QTC_ASSERT(d->m_model->m_currentTreeItem, return);
    d->m_model->m_currentTreeItem->m_removed = !d->m_model->m_currentTreeItem->m_removed;
    d->m_model->m_currentTreeItem->update();
    updateButtons();
}

void DebuggerConfigWidget::currentDebuggerChanged(const QModelIndex &newCurrent)
{
    d->m_model->setCurrentIndex(newCurrent);
    updateButtons();
}

void DebuggerConfigWidget::updateButtons()
{
    DebuggerTreeItem *titem = d->m_model->m_currentTreeItem;
    DebuggerItem *item = titem ? &titem->m_item : nullptr;

    m_itemConfigWidget->load(item);
    m_container->setVisible(item != nullptr);
    m_cloneButton->setEnabled(item && item->isValid() && item->canClone());
    m_delButton->setEnabled(item && !item->isAutoDetected());
    m_delButton->setText(item && titem->m_removed ? tr("Restore") : tr("Remove"));
}

// --------------------------------------------------------------------------
// DebuggerOptionsPage
// --------------------------------------------------------------------------

class DebuggerOptionsPage : public Core::IOptionsPage
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::DebuggerOptionsPage)

public:
    DebuggerOptionsPage();

    QWidget *widget() final;
    void apply() final;
    void finish() final;

private:
    QPointer<DebuggerConfigWidget> m_configWidget;
};

DebuggerOptionsPage::DebuggerOptionsPage()
{
    setId(ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
    setDisplayName(tr("Debuggers"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *DebuggerOptionsPage::widget()
{
    if (!m_configWidget)
        m_configWidget = new DebuggerConfigWidget;
    return m_configWidget;
}

void DebuggerOptionsPage::apply()
{
    QTC_ASSERT(m_configWidget, return);
    m_configWidget->m_itemConfigWidget->store();
    d->m_model->apply();
}

void DebuggerOptionsPage::finish()
{
    delete m_configWidget;
    m_configWidget = 0;
    d->m_model->cancel();
}

} // namespace Internal

// --------------------------------------------------------------------------
// DebuggerItemManager
// --------------------------------------------------------------------------

DebuggerItemManager::DebuggerItemManager()
{
    new DebuggerItemManagerPrivate;
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, [] { d->saveDebuggers(); });
}

DebuggerItemManager::~DebuggerItemManager()
{
    delete d;
}

QList<DebuggerItem> DebuggerItemManager::debuggers()
{
    QList<DebuggerItem> result;
    forAllDebuggers([&result](const DebuggerItem &item) { result.append(item); });
    return result;
}

void DebuggerItemManagerPrivate::autoDetectCdbDebuggers()
{
    FileNameList cdbs;

    QStringList programDirs;
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramFiles")));
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramFiles(x86)")));
    programDirs.append(QString::fromLocal8Bit(qgetenv("ProgramW6432")));

    foreach (const QString &dirName, programDirs) {
        if (dirName.isEmpty())
            continue;
        QDir dir(dirName);
        // Windows SDK's starting from version 8 live in
        // "ProgramDir\Windows Kits\<version>"
        const QString windowsKitsFolderName = QLatin1String("Windows Kits");
        if (dir.exists(windowsKitsFolderName)) {
            QDir windowKitsFolder = dir;
            if (windowKitsFolder.cd(windowsKitsFolderName)) {
                // Check in reverse order (latest first)
                const QFileInfoList kitFolders =
                    windowKitsFolder.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                   QDir::Time | QDir::Reversed);
                foreach (const QFileInfo &kitFolderFi, kitFolders) {
                    const QString path = kitFolderFi.absoluteFilePath();
                    const QFileInfo cdb32(path + QLatin1String("/Debuggers/x86/cdb.exe"));
                    if (cdb32.isExecutable())
                        cdbs.append(FileName::fromString(cdb32.absoluteFilePath()));
                    const QFileInfo cdb64(path + QLatin1String("/Debuggers/x64/cdb.exe"));
                    if (cdb64.isExecutable())
                        cdbs.append(FileName::fromString(cdb64.absoluteFilePath()));
                }
            }
        }

        // Pre Windows SDK 8: Check 'Debugging Tools for Windows'
        foreach (const QFileInfo &fi, dir.entryInfoList(QStringList(QLatin1String("Debugging Tools for Windows*")),
                                                        QDir::Dirs | QDir::NoDotAndDotDot)) {
            FileName filePath(fi);
            filePath.appendPath(QLatin1String("cdb.exe"));
            if (!cdbs.contains(filePath))
                cdbs.append(filePath);
        }
    }

    foreach (const FileName &cdb, cdbs) {
        if (DebuggerItemManager::findByCommand(cdb))
            continue;
        DebuggerItem item;
        item.createId();
        item.setAutoDetected(true);
        item.setAbis(Abi::abisOfBinary(cdb));
        item.setCommand(cdb);
        item.setEngineType(CdbEngineType);
        item.setUnexpandedDisplayName(uniqueDisplayName(tr("Auto-detected CDB at %1").arg(cdb.toUserOutput())));
        m_model->addDebugger(item);
    }
}

void DebuggerItemManagerPrivate::autoDetectGdbOrLldbDebuggers()
{
    const QStringList filters = {"gdb-i686-pc-mingw32", "gdb-i686-pc-mingw32.exe", "gdb",
                                 "gdb.exe", "lldb", "lldb.exe", "lldb-*"};

//    DebuggerItem result;
//    result.setAutoDetected(true);
//    result.setDisplayName(tr("Auto-detected for Tool Chain %1").arg(tc->displayName()));
    /*
    // Check suggestions from the SDK.
    Environment env = Environment::systemEnvironment();
    if (tc) {
        tc->addToEnvironment(env); // Find MinGW gdb in toolchain environment.
        QString path = tc->suggestedDebugger().toString();
        if (!path.isEmpty()) {
            const QFileInfo fi(path);
            if (!fi.isAbsolute())
                path = env.searchInPath(path);
            result.command = FileName::fromString(path);
            result.engineType = engineTypeFromBinary(path);
            return maybeAddDebugger(result, false);
        }
    }
    */

    FileNameList suspects;

    if (HostOsInfo::isMacHost()) {
        SynchronousProcess lldbInfo;
        lldbInfo.setTimeoutS(2);
        SynchronousProcessResponse response
                = lldbInfo.runBlocking(QLatin1String("xcrun"), {"--find", "lldb"});
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            QString lPath = response.allOutput().trimmed();
            if (!lPath.isEmpty()) {
                const QFileInfo fi(lPath);
                if (fi.exists() && fi.isExecutable() && !fi.isDir())
                    suspects.append(FileName::fromString(fi.absoluteFilePath()));
            }
        }
    }

    QStringList path = Environment::systemEnvironment().path();
    path.removeDuplicates();
    QDir dir;
    dir.setNameFilters(filters);
    dir.setFilter(QDir::Files | QDir::Executable);
    foreach (const QString &base, path) {
        dir.setPath(base);
        foreach (const QString &entry, dir.entryList()) {
            if (entry.startsWith(QLatin1String("lldb-platform-"))
                    || entry.startsWith(QLatin1String("lldb-gdbserver-"))) {
                continue;
            }
            suspects.append(FileName::fromString(dir.absoluteFilePath(entry)));
        }
    }

    foreach (const FileName &command, suspects) {
        const auto commandMatches = [command](const DebuggerTreeItem *titem) {
            return titem->m_item.command() == command;
        };
        if (DebuggerTreeItem *existingItem = m_model->findItemAtLevel<2>(commandMatches)) {
            if (command.toFileInfo().lastModified() != existingItem->m_item.lastModified())
                existingItem->m_item.reinitializeFromFile();
            continue;
        }
        DebuggerItem item;
        item.createId();
        item.setCommand(command);
        item.reinitializeFromFile();
        if (item.engineType() == NoEngineType)
            continue;
        //: %1: Debugger engine type (GDB, LLDB, CDB...), %2: Path
        item.setUnexpandedDisplayName(tr("System %1 at %2")
            .arg(item.engineTypeName()).arg(command.toUserOutput()));
        item.setAutoDetected(true);
        m_model->addDebugger(item);
    }
}

void DebuggerItemManagerPrivate::readLegacyDebuggers(const FileName &file)
{
    PersistentSettingsReader reader;
    if (!reader.load(file))
        return;

    foreach (const QVariant &v, reader.restoreValues()) {
        QVariantMap data1 = v.toMap();
        QString kitName = data1.value(QLatin1String("PE.Profile.Name")).toString();
        QVariantMap data2 = data1.value(QLatin1String("PE.Profile.Data")).toMap();
        QVariant v3 = data2.value(DebuggerKitInformation::id().toString());
        QString fn;
        if (v3.type() == QVariant::String)
            fn = v3.toString();
        else
            fn = v3.toMap().value(QLatin1String("Binary")).toString();
        if (fn.isEmpty())
            continue;
        if (fn.startsWith(QLatin1Char('{')))
            continue;
        if (fn == QLatin1String("auto"))
            continue;
        FileName command = FileName::fromUserInput(fn);
        if (!command.exists())
            continue;
        if (DebuggerItemManager::findByCommand(command))
            continue;
        DebuggerItem item;
        item.createId();
        item.setCommand(command);
        item.setAutoDetected(true);
        item.reinitializeFromFile();
        item.setUnexpandedDisplayName(tr("Extracted from Kit %1").arg(kitName));
        m_model->addDebugger(item);
    }
}

const DebuggerItem *DebuggerItemManager::findByCommand(const FileName &command)
{
    return findDebugger([command](const DebuggerItem &item) {
        return item.command() == command;
    });
}

const DebuggerItem *DebuggerItemManager::findById(const QVariant &id)
{
    return findDebugger([id](const DebuggerItem &item) {
        return item.id() == id;
    });
}

const DebuggerItem *DebuggerItemManager::findByEngineType(DebuggerEngineType engineType)
{
    return findDebugger([engineType](const DebuggerItem &item) {
        return item.engineType() == engineType;
    });
}

QVariant DebuggerItemManager::registerDebugger(const DebuggerItem &item)
{
    return d->registerDebugger(item);
}

void DebuggerItemManager::deregisterDebugger(const QVariant &id)
{
    d->m_model->forItemsAtLevel<2>([id](DebuggerTreeItem *titem) {
        if (titem->m_item.id() == id)
            d->m_model->destroyItem(titem);
    });
}

namespace Internal {

static FileName userSettingsFileName()
{
    QFileInfo settingsLocation(ICore::settings()->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + QLatin1String(DEBUGGER_FILENAME));
}

DebuggerItemManagerPrivate::DebuggerItemManagerPrivate()
    : m_writer(userSettingsFileName(), "QtCreatorDebuggers")
{
    d = this;
    m_model = new DebuggerItemModel;
    m_optionsPage = new DebuggerOptionsPage;
    ExtensionSystem::PluginManager::addObject(m_optionsPage);
    restoreDebuggers();
}

DebuggerItemManagerPrivate::~DebuggerItemManagerPrivate()
{
    ExtensionSystem::PluginManager::removeObject(m_optionsPage);
    delete m_optionsPage;
    delete m_model;
}

QString DebuggerItemManagerPrivate::uniqueDisplayName(const QString &base)
{
    const DebuggerItem *item = findDebugger([base](const DebuggerItem &item) {
        return item.unexpandedDisplayName() == base;
    });
    return item ? uniqueDisplayName(base + " (1)") : base;
}

QVariant DebuggerItemManagerPrivate::registerDebugger(const DebuggerItem &item)
{
    // Try re-using existing item first.
    DebuggerTreeItem *titem = m_model->findItemAtLevel<2>([item](DebuggerTreeItem *titem) {
        const DebuggerItem &d = titem->m_item;
        return d.command() == item.command()
                && d.isAutoDetected() == item.isAutoDetected()
                && d.engineType() == item.engineType()
                && d.unexpandedDisplayName() == item.unexpandedDisplayName()
                && d.abis() == item.abis();
    });
    if (titem)
        return titem->m_item.id();

    // If item already has an id, use it. Otherwise, create a new id.
    DebuggerItem di = item;
    if (!di.id().isValid())
        di.createId();

    m_model->addDebugger(di);
    return di.id();
}

void DebuggerItemManagerPrivate::readDebuggers(const FileName &fileName, bool isSystem)
{
    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return;
    QVariantMap data = reader.restoreValues();

    // Check version
    int version = data.value(DEBUGGER_FILE_VERSION_KEY, 0).toInt();
    if (version < 1)
        return;

    int count = data.value(DEBUGGER_COUNT_KEY, 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = DEBUGGER_DATA_KEY + QString::number(i);
        if (!data.contains(key))
            continue;
        const QVariantMap dbMap = data.value(key).toMap();
        DebuggerItem item(dbMap);
        if (isSystem) {
            item.setAutoDetected(true);
            // SDK debuggers are always considered to be up-to-date, so no need to recheck them.
        } else {
            // User settings.
            if (item.isAutoDetected()) {
                if (!item.isValid() || item.engineType() == NoEngineType) {
                    qWarning() << QString("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since it is not valid.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
                if (!item.command().toFileInfo().isExecutable()) {
                    qWarning() << QString("DebuggerItem \"%1\" (%2) read from \"%3\" dropped since the command is not executable.")
                                  .arg(item.command().toUserOutput(), item.id().toString(), fileName.toUserOutput());
                    continue;
                }
            }

        }
        registerDebugger(item);
    }
}

void DebuggerItemManagerPrivate::restoreDebuggers()
{
    // Read debuggers from SDK
    QFileInfo systemSettingsFile(ICore::settings(QSettings::SystemScope)->fileName());
    readDebuggers(FileName::fromString(systemSettingsFile.absolutePath() + DEBUGGER_FILENAME), true);

    // Read all debuggers from user file.
    readDebuggers(userSettingsFileName(), false);

    // Auto detect current.
    autoDetectCdbDebuggers();
    autoDetectGdbOrLldbDebuggers();

    // Add debuggers from pre-3.x profiles.xml
    QFileInfo systemLocation(ICore::settings(QSettings::SystemScope)->fileName());
    readLegacyDebuggers(FileName::fromString(systemLocation.absolutePath() + QLatin1String(DEBUGGER_LEGACY_FILENAME)));
    QFileInfo userLocation(ICore::settings()->fileName());
    readLegacyDebuggers(FileName::fromString(userLocation.absolutePath() + QLatin1String(DEBUGGER_LEGACY_FILENAME)));
}

void DebuggerItemManagerPrivate::saveDebuggers()
{
    QVariantMap data;
    data.insert(DEBUGGER_FILE_VERSION_KEY, 1);

    int count = 0;
    forAllDebuggers([&count, &data](DebuggerItem &item) {
        if (item.isValid() && item.engineType() != NoEngineType) {
            QVariantMap tmp = item.toMap();
            if (!tmp.isEmpty()) {
                data.insert(DEBUGGER_DATA_KEY + QString::number(count), tmp);
                ++count;
            }
        }
    });
    data.insert(DEBUGGER_COUNT_KEY, count);
    m_writer.save(data, ICore::mainWindow());

    // Do not save default debuggers as they are set by the SDK.
}

} // namespace Internal
} // namespace Debugger
