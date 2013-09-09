/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggerkitconfigwidget.h"

#include <coreplugin/icore.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/elidinglabel.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/persistentsettings.h>
#include <utils/qtcassert.h>

#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <QApplication>
#include <QComboBox>
#include <QDirIterator>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTreeView>
#include <QUuid>

using namespace ProjectExplorer;
using namespace Utils;
using namespace Debugger::Internal;

namespace Debugger {

static const char debuggingToolsWikiLinkC[] = "http://qt-project.org/wiki/Qt_Creator_Windows_Debugging";

static const char DEBUGGER_DATA_KEY[] = "DebuggerItem.";
static const char DEBUGGER_COUNT_KEY[] = "DebuggerItem.Count";
static const char DEBUGGER_FILE_VERSION_KEY[] = "Version";
static const char DEFAULT_DEBUGGER_COUNT_KEY[] = "DefaultDebugger.Count";
static const char DEFAULT_DEBUGGER_ABI_KEY[] = "DefaultDebugger.Abi.";
static const char DEFAULT_DEBUGGER_PATH_KEY[] = "DefaultDebugger.Path.";
static const char DEBUGGER_FILENAME[] = "/qtcreator/debuggers.xml";
static const char DEBUGGER_LEGACY_FILENAME[] = "/qtcreator/profiles.xml";

static const char DEBUGGER_INFORMATION_COMMAND[] = "Binary";
static const char DEBUGGER_INFORMATION_DISPLAYNAME[] = "DisplayName";
static const char DEBUGGER_INFORMATION_ID[] = "Id";
static const char DEBUGGER_INFORMATION_ENGINETYPE[] = "EngineType";
static const char DEBUGGER_INFORMATION_AUTODETECTED[] = "AutoDetected";
static const char DEBUGGER_INFORMATION_ABIS[] = "Abis";

// --------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------

static DebuggerItemManager *theDebuggerItemManager()
{
    static DebuggerItemManager *manager = new DebuggerItemManager(0);
    return manager;
}

// --------------------------------------------------------------------------
// DebuggerItem
// --------------------------------------------------------------------------

namespace Internal {

class DebuggerItem
{
public:
    DebuggerItem();

    bool canClone() const { return true; }
    bool isValid() const { return engineType != NoEngineType; }
    QString engineTypeName() const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &data);
    void reinitializeFromFile();

public:
    QVariant id;
    QString displayName;
    DebuggerEngineType engineType;
    Utils::FileName command;
    bool isAutoDetected;
    QList<ProjectExplorer::Abi> abis;
};

DebuggerItem::DebuggerItem()
{
    engineType = NoEngineType;
    isAutoDetected = false;
}

QString DebuggerItem::engineTypeName() const
{
    switch (engineType) {
    case Debugger::NoEngineType:
        return DebuggerOptionsPage::tr("Not recognized");
    case Debugger::GdbEngineType:
        return QLatin1String("GDB");
    case Debugger::CdbEngineType:
        return QLatin1String("CDB");
    case Debugger::LldbEngineType:
        return QLatin1String("LLDB");
    default:
        return QString();
    }
}

static QStringList toList(const QList<Abi> &abis)
{
    QStringList list;
    foreach (const Abi &abi, abis)
        list.append(abi.toString());
    return list;
}

QVariantMap DebuggerItem::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(DEBUGGER_INFORMATION_DISPLAYNAME), displayName);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ID), id);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_COMMAND), command.toUserOutput());
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ENGINETYPE), int(engineType));
    data.insert(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTED), isAutoDetected);
    data.insert(QLatin1String(DEBUGGER_INFORMATION_ABIS), toList(abis));
    return data;
}

void DebuggerItem::fromMap(const QVariantMap &data)
{
    command = FileName::fromUserInput(data.value(QLatin1String(DEBUGGER_INFORMATION_COMMAND)).toString());
    id = data.value(QLatin1String(DEBUGGER_INFORMATION_ID)).toString();
    displayName = data.value(QLatin1String(DEBUGGER_INFORMATION_DISPLAYNAME)).toString();
    isAutoDetected = data.value(QLatin1String(DEBUGGER_INFORMATION_AUTODETECTED)).toBool();
    engineType = DebuggerEngineType(data.value(QLatin1String(DEBUGGER_INFORMATION_ENGINETYPE)).toInt());

    abis.clear();
    foreach (const QString &a, data.value(QLatin1String(DEBUGGER_INFORMATION_ABIS)).toStringList()) {
        Abi abi(a);
        if (abi.isValid())
            abis.append(abi);
    }
}

void DebuggerItem::reinitializeFromFile()
{
    QProcess proc;
    proc.start(command.toString(), QStringList() << QLatin1String("--version"));
    proc.waitForStarted();
    proc.waitForFinished();
    QByteArray ba = proc.readAll();
    if (ba.contains("gdb")) {
        engineType = GdbEngineType;
//        const char needle[] = "This GDB was configured as \"";
//        int pos1 = ba.indexOf(needle);
//        if (pos1 != -1) {
//            pos1 += sizeof(needle);
//            int pos2 = ba.indexOf('"', pos1 + 1);
//            QByteArray target = ba.mid(pos1, pos2 - pos1);
//            abis.append(Abi::abiFromTargetTriplet(target)); // FIXME: Doesn't exist yet.
//        }
        abis = Abi::abisOfBinary(command); // FIXME: Wrong.
        return;
    }
    if (ba.contains("lldb")) {
        engineType = LldbEngineType;
        abis = Abi::abisOfBinary(command);
        return;
    }
    if (ba.startsWith("Python")) {
        engineType = PdbEngineType;
        return;
    }
    engineType = NoEngineType;
}

} // namespace Internal

// --------------------------------------------------------------------------
// DebuggerKitInformation
// --------------------------------------------------------------------------

DebuggerKitInformation::DebuggerKitInformation()
{
    setObjectName(QLatin1String("DebuggerKitInformation"));
    setId(DebuggerKitInformation::id());
    setPriority(28000);
}

QVariant DebuggerKitInformation::defaultValue(Kit *k) const
{
// This is only called from Kit::Kit()
//    if (isValidDebugger(k)) {
//        DebuggerItem *item = DebuggerItemManager::debuggerFromKit(k);
//        QTC_ASSERT(item, return QVariant());
//        return item->id;
//    }
    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    return theDebuggerItemManager()->defaultDebugger(tc);
}

void DebuggerKitInformation::setup(Kit *k)
{
    k->setValue(DebuggerKitInformation::id(), defaultValue(k));
}

// Check the configuration errors and return a flag mask. Provide a quick check and
// a verbose one with a list of errors.

enum DebuggerConfigurationErrors {
    NoDebugger = 0x1,
    DebuggerNotFound = 0x2,
    DebuggerNotExecutable = 0x4,
    DebuggerNeedsAbsolutePath = 0x8
};

static QVariant debuggerPathOrId(const Kit *k)
{
    QTC_ASSERT(k, return QString());
    QVariant id = k->value(DebuggerKitInformation::id());
    if (!id.isValid())
        return id; // Invalid.

    // With 3.0 we have:
    // <value type="QString" key="Debugger.Information">{75ecf347-f221-44c3-b613-ea1d29929cd4}</value>
    if (id.type() == QVariant::String)
        return id;

    // Before we had:
    // <valuemap type="QVariantMap" key="Debugger.Information">
    //    <value type="QString" key="Binary">/data/dev/debugger/gdb-git/gdb/gdb</value>
    //    <value type="int" key="EngineType">1</value>
    //  </valuemap>
    return id.toMap().value(QLatin1String("Binary"));
}

static unsigned debuggerConfigurationErrors(const Kit *k)
{
    QTC_ASSERT(k, return NoDebugger);

    const DebuggerItem *item = DebuggerItemManager::debuggerFromKit(k);
    if (!item)
        return NoDebugger;

    if (item->command.isEmpty())
        return NoDebugger;

    unsigned result = 0;
    const QFileInfo fi = item->command.toFileInfo();
    if (!fi.exists() || fi.isDir())
        result |= DebuggerNotFound;
    else if (!fi.isExecutable())
        result |= DebuggerNotExecutable;

    if (!fi.exists() || fi.isDir()) {
        if (item->engineType == NoEngineType)
            return NoDebugger;

        // We need an absolute path to be able to locate Python on Windows.
        if (item->engineType == GdbEngineType)
            if (const ToolChain *tc = ToolChainKitInformation::toolChain(k))
                if (tc->targetAbi().os() == Abi::WindowsOS && !fi.isAbsolute())
                    result |= DebuggerNeedsAbsolutePath;
    }
    return result;
}

bool DebuggerKitInformation::isValidDebugger(const Kit *k)
{
    return debuggerConfigurationErrors(k) == 0;
}

QList<Task> DebuggerKitInformation::validateDebugger(const Kit *k)
{
    QList<Task> result;

    const unsigned errors = debuggerConfigurationErrors(k);
    if (!errors)
        return result;

    QString path;
    if (const DebuggerItem *item = DebuggerItemManager::debuggerFromKit(k))
        path = item->command.toUserOutput();

    const Core::Id id = ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM;
    if (errors & NoDebugger)
        result << Task(Task::Warning, tr("No debugger set up."), FileName(), -1, id);

    if (errors & DebuggerNotFound)
        result << Task(Task::Error, tr("Debugger '%1' not found.").arg(path),
                       FileName(), -1, id);
    if (errors & DebuggerNotExecutable)
        result << Task(Task::Error, tr("Debugger '%1' not executable.").arg(path), FileName(), -1, id);

    if (errors & DebuggerNeedsAbsolutePath) {
        const QString message =
                tr("The debugger location must be given as an "
                   "absolute path (%1).").arg(path);
        result << Task(Task::Error, message, FileName(), -1, id);
    }
    return result;
}

KitConfigWidget *DebuggerKitInformation::createConfigWidget(Kit *k) const
{
    return new Internal::DebuggerKitConfigWidget(k, this);
}

KitInformation::ItemList DebuggerKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Debugger"), displayString(k));
}

FileName DebuggerKitInformation::debuggerCommand(const ProjectExplorer::Kit *k)
{
    const DebuggerItem *item = DebuggerItemManager::debuggerFromKit(k);
    QTC_ASSERT(item, return FileName());
    return item->command;
}

DebuggerEngineType DebuggerKitInformation::engineType(const ProjectExplorer::Kit *k)
{
    const DebuggerItem *item = DebuggerItemManager::debuggerFromKit(k);
    QTC_ASSERT(item, return NoEngineType);
    return item->engineType;
}

QString DebuggerKitInformation::displayString(const Kit *k)
{
    const DebuggerItem *item = DebuggerItemManager::debuggerFromKit(k);
    if (!item)
        return tr("No Debugger");
    QString binary = item->command.toUserOutput();
    QString name = tr("%1 Engine").arg(item->engineTypeName());
    return binary.isEmpty() ? tr("%1 <None>").arg(name) : tr("%1 using \"%2\"").arg(name, binary);
}

void DebuggerKitInformation::setDebugger(Kit *k,
    DebuggerEngineType type, const FileName &command)
{
    theDebuggerItemManager()->setDebugger(k, type, command);
}

Core::Id DebuggerKitInformation::id()
{
    return "Debugger.Information";
}

namespace Internal {

static FileName userSettingsFileName()
{
    QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + QLatin1String(DEBUGGER_FILENAME));
}

static QList<QStandardItem *> describeItem(const DebuggerItem &item)
{
    QList<QStandardItem *> row;
    row.append(new QStandardItem(item.displayName));
    row.append(new QStandardItem(item.command.toUserOutput()));
    row.append(new QStandardItem(item.engineTypeName()));
    row.at(0)->setData(item.id);
    row.at(0)->setEditable(false);
    row.at(1)->setEditable(false);
    row.at(2)->setEditable(false);
    row.at(0)->setSelectable(true);
    row.at(1)->setSelectable(true);
    row.at(2)->setSelectable(true);
    return row;
}

static QList<QStandardItem *> createRow(const QString &display)
{
    QList<QStandardItem *> row;
    row.append(new QStandardItem(display));
    row.append(new QStandardItem());
    row.append(new QStandardItem());
    row.at(0)->setEditable(false);
    row.at(1)->setEditable(false);
    row.at(2)->setEditable(false);
    row.at(0)->setSelectable(false);
    row.at(1)->setSelectable(false);
    row.at(2)->setSelectable(false);
    return row;
}

class DebuggerItemConfigWidget;

// --------------------------------------------------------------------------
// DebuggerItemManager
// --------------------------------------------------------------------------

DebuggerItemManager::DebuggerItemManager(QObject *parent)
    : QStandardItemModel(parent)
{
    setColumnCount(3);

    QList<QStandardItem *> row = createRow(tr("Auto-detected"));
    m_autoRoot = row.at(0);
    appendRow(row);

    row = createRow(tr("Manual"));
    m_manualRoot = row.at(0);
    appendRow(row);

    m_writer = new PersistentSettingsWriter(userSettingsFileName(), QLatin1String("QtCreatorDebugger"));

    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveDebuggers()));
}

DebuggerItemManager::~DebuggerItemManager()
{
    disconnect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()),
            this, SLOT(saveDebuggers()));
    delete m_writer;
}

QVariant DebuggerItemManager::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("Name");
        case 1:
            return tr("Path");
        case 2:
            return tr("Type");
        }
    }
    return QVariant();
}

QString DebuggerItemManager::uniqueDisplayName(const QString &base) const
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.displayName == base)
            return uniqueDisplayName(base + QLatin1String(" (1)"));

    return base;
}

void DebuggerItemManager::autoDetectCdbDebugger()
{
    QList<FileName> cdbs;

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
        if (findByCommand(cdb))
            continue;
        DebuggerItem item;
        item.isAutoDetected = true;
        item.abis = Abi::abisOfBinary(cdb);
        item.command = cdb;
        item.engineType = CdbEngineType;
        item.displayName = uniqueDisplayName(tr("Auto-detected CDB at %1").arg(cdb.toUserOutput()));
        doAddDebugger(item);
    }
}

void DebuggerItemManager::autoDetectDebuggers()
{
    autoDetectCdbDebugger();

    QStringList filters;
    filters.append(QLatin1String("gdb-i686-pc-mingw32"));
    filters.append(QLatin1String("gdb"));
    filters.append(QLatin1String("lldb"));
    filters.append(QLatin1String("lldb-*"));

    QFileInfoList suspects;

    QStringList path = Environment::systemEnvironment().path();
    foreach (const QString &base, path) {
        QDir dir(base);
        dir.setNameFilters(filters);
        suspects += dir.entryInfoList();
    }

    foreach (const QFileInfo &fi, suspects) {
        if (fi.exists()) {
            FileName command = FileName::fromString(fi.absoluteFilePath());
            if (findByCommand(command))
                continue;
            DebuggerItem item;
            item.command = command;
            item.id = QUuid::createUuid().toString();
            item.reinitializeFromFile();
            item.displayName = tr("System %1 at %2")
                .arg(item.engineTypeName()).arg(fi.absoluteFilePath());
            item.isAutoDetected = true;
            doAddDebugger(item);
        }
    }
}

void DebuggerItemManager::readLegacyDebuggers()
{
    QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    FileName legacyKits = FileName::fromString(settingsLocation.absolutePath() + QLatin1String(DEBUGGER_LEGACY_FILENAME));

    PersistentSettingsReader reader;
    if (!reader.load(legacyKits))
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
            fn = v3.toMap().value(QLatin1String(DEBUGGER_INFORMATION_COMMAND)).toString();
        if (fn.isEmpty())
            continue;
        if (fn.startsWith(QLatin1Char('{')))
            continue;
        FileName command = FileName::fromUserInput(fn);
        if (findByCommand(command))
            continue;
        DebuggerItem item;
        item.command = command;
        item.isAutoDetected = true;
        item.reinitializeFromFile();
        item.displayName = tr("Extracted from Kit %1").arg(kitName);
        item.id = QUuid::createUuid().toString();
        doAddDebugger(item);
    }
}

QVariant DebuggerItemManager::doAddDebugger(const DebuggerItem &item0)
{
    DebuggerItem item = item0;
    if (item.id.isNull()) {
        QTC_CHECK(false);
        item.id = QUuid::createUuid().toString();
    }
    QList<QStandardItem *> row = describeItem(item);
    (item.isAutoDetected ? m_autoRoot : m_manualRoot)->appendRow(row);
    m_debuggers.append(item);
    emit debuggerAdded(item.id, item.displayName);
    return item.id;
}

const DebuggerItem *DebuggerItemManager::findByCommand(const FileName &command)
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.command == command)
            return &item;

    return 0;
}

const DebuggerItem *DebuggerItemManager::findById(const QVariant &id)
{
    foreach (const DebuggerItem &item, m_debuggers)
        if (item.id == id)
            return &item;

    return 0;
}

QStandardItem *DebuggerItemManager::currentStandardItem() const
{
    for (int i = 0, n = m_autoRoot->rowCount(); i != n; ++i) {
        QStandardItem *sitem = m_autoRoot->child(i);
        if (sitem->data() == m_currentDebugger)
            return sitem;
    }
    for (int i = 0, n = m_manualRoot->rowCount(); i != n; ++i) {
        QStandardItem *sitem = m_manualRoot->child(i);
        if (sitem->data() == m_currentDebugger)
            return sitem;
    }
    return 0;
}

static QList<DebuggerItem> readDebuggers(const FileName &fileName)
{
    QList<DebuggerItem> result;

    PersistentSettingsReader reader;
    if (!reader.load(fileName))
        return result;
    QVariantMap data = reader.restoreValues();

    // Check version
    int version = data.value(QLatin1String(DEBUGGER_FILE_VERSION_KEY), 0).toInt();
    if (version < 1)
        return result;

    // Read default debugger settings (if any)
//    int count = data.value(QLatin1String(DEFAULT_DEBUGGER_COUNT_KEY)).toInt();
//    for (int i = 0; i < count; ++i) {
//        const QString abiKey = QString::fromLatin1(DEFAULT_DEBUGGER_ABI_KEY) + QString::number(i);
//        if (!data.contains(abiKey))
//            continue;
//        const QString pathKey = QString::fromLatin1(DEFAULT_DEBUGGER_PATH_KEY) + QString::number(i);
//        if (!data.contains(pathKey))
//            continue;
//        m_abiToDebugger.insert(data.value(abiKey).toString(),
//                                  FileName::fromString(data.value(pathKey).toString()));
//    }

//    QList<DebuggerFactory *> factories = ExtensionSystem::PluginManager::getObjects<DebuggerFactory>();

    int count = data.value(QLatin1String(DEBUGGER_COUNT_KEY), 0).toInt();
    for (int i = 0; i < count; ++i) {
        const QString key = QString::fromLatin1(DEBUGGER_DATA_KEY) + QString::number(i);
        if (!data.contains(key))
            break;
        const QVariantMap dbMap = data.value(key).toMap();
        DebuggerItem item;
        item.fromMap(dbMap);
        result.append(item);
    }

    return result;
}

void DebuggerItemManager::restoreDebuggers()
{
    QList<DebuggerItem> dbsToCheck;

    // Read debuggers from SDK
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    QList<DebuggerItem> dbsToRegister =
            readDebuggers(FileName::fromString(systemSettingsFile.absolutePath() + QLatin1String(DEBUGGER_FILENAME)));

    // These are autodetected.
    for (int i = 0, n = dbsToRegister.size(); i != n; ++i)
        dbsToRegister[i].isAutoDetected = true;

    // SDK debuggers are always considered to be up-to-date, so no need to recheck them.

    // Read all debuggers from user file.
    foreach (const DebuggerItem &item, readDebuggers(userSettingsFileName())) {
        if (item.isAutoDetected)
            dbsToCheck.append(item);
        else
            dbsToRegister.append(item);
    }

    // Remove debuggers configured by the SDK.
//    foreach (const DebuggerItem &item, dbsToRegister) {
//        for (int i = dbsToCheck.count(); --i >= 0; ) {
//            if (dbsToCheck.at(i).id == item.id)
//                dbsToCheck.removeAt(i);
//        }
//    }

//    QList<DebuggerItem *> detectedDbs;
//    QList<DebuggerFactory *> factories = ExtensionSystem::PluginManager::getObjects<DebuggerFactory>();
//    foreach (DebuggerFactory *f, factories)
//        detectedDbs.append(f->autoDetect());

    // Find/update autodetected debuggers
//    DebuggerItem *toStore = 0;
//    foreach (DebuggerItem *currentDetected, detectedDbs) {
//        toStore = currentDetected;

//        // Check whether we had this debugger stored and prefer the old one with the old id:
//        for (int i = 0; i < dbsToCheck.count(); ++i) {
//            if (*(dbsToCheck.at(i)) == *currentDetected) {
//                toStore = dbsToCheck.at(i);
//                dbsToCheck.removeAt(i);
//                delete currentDetected;
//                break;
//            }
//        }
//        dbsToRegister += toStore;
//    }

    // Keep debuggers that were not rediscovered but are still executable and delete the rest
    foreach (const DebuggerItem &item, dbsToCheck) {
        if (!item.isValid()) {
            qWarning() << QString::fromLatin1("DebuggerItem \"%1\" (%2) dropped since it is not valid")
                          .arg(item.command.toString()).arg(item.id.toString());
        } else {
            dbsToRegister.append(item);
        }
    }

    // Store manual debuggers
    DebuggerItemManager *manager = theDebuggerItemManager();
    for (int i = 0, n = dbsToRegister.size(); i != n; ++i) {
        DebuggerItem item = dbsToRegister.at(i);
        if (manager->findByCommand(item.command))
            continue;
        if (item.id.isNull())
            item.id = QUuid::createUuid().toString();
        manager->doAddDebugger(item);
    }

    // Auto detect current.
    manager->autoDetectDebuggers();

    // Add debuggers from pre-3.x profiles.xml
    manager->readLegacyDebuggers();
}

void DebuggerItemManager::saveDebuggers()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(DEBUGGER_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (const DebuggerItem &item, m_debuggers) {
        if (item.isValid()) {
            QVariantMap tmp = item.toMap();
            if (tmp.isEmpty())
                continue;
            data.insert(QString::fromLatin1(DEBUGGER_DATA_KEY) + QString::number(count), tmp);
            ++count;
        }
    }
    data.insert(QLatin1String(DEBUGGER_COUNT_KEY), count);
    m_writer->save(data, Core::ICore::mainWindow());

    // Do not save default debuggers as they are set by the SDK.
}

const DebuggerItem *DebuggerItemManager::debuggerFromKit(const Kit *kit)
{
    if (!kit)
        return 0;
    QVariant pathOrId = debuggerPathOrId(kit);
    foreach (const DebuggerItem &item, theDebuggerItemManager()->m_debuggers) {
        if (item.id == pathOrId)
            return &item;
        if (item.command == FileName::fromUserInput(pathOrId.toString()))
            return &item;
    }
    return 0;
}

void DebuggerItemManager::setDebugger(Kit *kit, Debugger::DebuggerEngineType type, const FileName &command)
{
    QTC_ASSERT(kit, return);
    // This should only operate on fresh kits.
    //QVariant id = k->value(DEBUGGER_INFORMATION);
    //QTC_CHECK(id.isNull());
    if (!findByCommand(command)) {
        DebuggerItem item;
        item.engineType = type;
        item.command = command;
        item.id = QUuid::createUuid().toString();
        item.displayName = uniqueDisplayName(tr("Created by tool chain."));
        doAddDebugger(item);
    }

    const DebuggerItem *it = findByCommand(command);
    QTC_ASSERT(it, return);
    QTC_ASSERT(it->id.isValid(), return);
    kit->setValue(DebuggerKitInformation::id(), it->id);
}

QModelIndex DebuggerItemManager::currentIndex() const
{
    QStandardItem *current = currentStandardItem();
    return current ? current->index() : QModelIndex();
}

void DebuggerItemManager::addDebugger()
{
    DebuggerItem item;
    item.engineType = NoEngineType;
    item.id = QUuid::createUuid().toString();
    item.displayName = uniqueDisplayName(tr("New Debugger"));
    item.isAutoDetected = false;
    doAddDebugger(item);
}

void DebuggerItemManager::cloneDebugger()
{
    const DebuggerItem *item = findById(m_currentDebugger);
    QTC_ASSERT(item, return);
    DebuggerItem newItem = *item;
    newItem.id = QUuid::createUuid().toString();
    newItem.displayName = uniqueDisplayName(tr("Clone of %1").arg(item->displayName));
    newItem.isAutoDetected = false;
    doAddDebugger(newItem);
}

void DebuggerItemManager::removeDebugger()
{
    QTC_ASSERT(m_currentDebugger.isValid(), return);
    QVariant id = m_currentDebugger;
    bool ok = false;
    for (int i = 0, n = m_debuggers.size(); i != n; ++i) {
        if (m_debuggers.at(i).id == id) {
            m_debuggers.removeAt(i);
            ok = true;
            break;
        }
    }
    QTC_ASSERT(ok, return);
    QStandardItem *sitem = currentStandardItem();
    QTC_ASSERT(sitem, return);
    QStandardItem *parent = sitem->parent();
    QTC_ASSERT(parent, return);
    // This will trigger a change of m_currentDebugger via changing the
    // view selection.
    parent->removeRow(sitem->row());
    emit debuggerRemoved(id);
}

void DebuggerItemManager::markCurrentDirty()
{
    QStandardItem *sitem = currentStandardItem();
    QTC_ASSERT(sitem, return);
    QFont font = sitem->font();
    font.setBold(true);
    sitem->setFont(font);
}

void DebuggerItemManager::setCurrentIndex(const QModelIndex &index)
{
    QStandardItem *sit = itemFromIndex(index);
    m_currentDebugger = sit ? sit->data() : QVariant();
}

void DebuggerItemManager::setCurrentData(const QString &displayName, const FileName &fileName)
{
    for (int i = 0, n = m_debuggers.size(); i != n; ++i) {
        DebuggerItem &item = m_debuggers[i];
        if (item.id == m_currentDebugger) {
            item.displayName = displayName;
            item.command = fileName;
            item.reinitializeFromFile();
            QStandardItem *sitem = currentStandardItem();
            QTC_ASSERT(sitem, return);
            QStandardItem *parent = sitem->parent();
            QTC_ASSERT(parent, return);
            int row = sitem->row();
            QFont font = sitem->font();
            font.setBold(false);
            parent->child(row, 0)->setData(item.displayName, Qt::DisplayRole);
            parent->child(row, 0)->setFont(font);
            parent->child(row, 1)->setData(item.command.toUserOutput(), Qt::DisplayRole);
            parent->child(row, 1)->setFont(font);
            parent->child(row, 2)->setData(item.engineTypeName(), Qt::DisplayRole);
            parent->child(row, 2)->setFont(font);
            emit debuggerUpdated(m_currentDebugger, displayName);
            return;
        }
    }
}

QVariant DebuggerItemManager::defaultDebugger(ToolChain *tc)
{
    QTC_ASSERT(tc, return QVariant());

    DebuggerItem result;
    result.isAutoDetected = true;
    result.displayName = tr("Auto-detected for Tool Chain %1").arg(tc->displayName());

    Abi abi = Abi::hostAbi();
    if (tc)
        abi = tc->targetAbi();

//        if (abis.first().wordWidth() == 32)
//            result.first = cdb.toString();
//        else if (abis.first().wordWidth() == 64)
//            result.second = cdb.toString();
//    // prefer 64bit debugger, even for 32bit binaries:
//    if (!result.second.isEmpty())
//        result.first = result.second;


    foreach (const DebuggerItem &item, m_debuggers)
        foreach (const Abi targetAbi, item.abis)
            if (targetAbi.isCompatibleWith(abi))
                return item.id;

    return QVariant();

    /*
    // CDB for windows:
    if (abi.os() == Abi::WindowsOS && abi.osFlavor() != Abi::WindowsMSysFlavor) {
        QPair<QString, QString> cdbs = autoDetectCdbDebugger();
        result.command = FileName::fromString(abi.wordWidth() == 32 ? cdbs.first : cdbs.second);
        result.engineType = CdbEngineType;
        return maybeAddDebugger(result, false);
    }

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

    // Default to GDB, system GDB
    result.engineType = GdbEngineType;
    QString gdb;
    const QString systemGdb = QLatin1String("gdb");
    // MinGW: Search for the python-enabled gdb first.
    if (abi.os() == Abi::WindowsOS && abi.osFlavor() == Abi::WindowsMSysFlavor)
        gdb = env.searchInPath(QLatin1String("gdb-i686-pc-mingw32"));
    if (gdb.isEmpty())
        gdb = env.searchInPath(systemGdb);
    result.command = FileName::fromString(env.searchInPath(gdb.isEmpty() ? systemGdb : gdb));
    return maybeAddDebugger(result, false);
    */
}

// -----------------------------------------------------------------------
// DebuggerKitConfigWidget
// -----------------------------------------------------------------------

DebuggerKitConfigWidget::DebuggerKitConfigWidget(Kit *workingCopy, const KitInformation *ki)
    : KitConfigWidget(workingCopy, ki)
{
    DebuggerItemManager *manager = theDebuggerItemManager();
    QTC_CHECK(manager);

    m_comboBox = new QComboBox;
    m_comboBox->setEnabled(true);
    m_comboBox->setToolTip(toolTip());
    m_comboBox->addItem(tr("None"), QString());
    foreach (const DebuggerItem &item, manager->m_debuggers)
        m_comboBox->addItem(item.displayName, item.id);

    refresh();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentDebuggerChanged(int)));

    m_manageButton = new QPushButton(tr("Manage..."));
    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageDebuggers()));

    connect(manager, SIGNAL(debuggerAdded(QVariant,QString)),
            this, SLOT(onDebuggerAdded(QVariant,QString)));
    connect(manager, SIGNAL(debuggerUpdated(QVariant,QString)),
            this, SLOT(onDebuggerUpdated(QVariant,QString)));
    connect(manager, SIGNAL(debuggerRemoved(QVariant)),
            this, SLOT(onDebuggerRemoved(QVariant)));
}

QString DebuggerKitConfigWidget::toolTip() const
{
    return tr("The debugger to use for this kit.");
}

QString DebuggerKitConfigWidget::displayName() const
{
    return tr("Debugger:");
}

void DebuggerKitConfigWidget::makeReadOnly()
{
    m_manageButton->setEnabled(false);
    m_comboBox->setEnabled(false);
}

void DebuggerKitConfigWidget::refresh()
{
    const DebuggerItem *item = DebuggerItemManager::debuggerFromKit(m_kit);
    updateComboBox(item ? item->id : QVariant());
}

QWidget *DebuggerKitConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

QWidget *DebuggerKitConfigWidget::mainWidget() const
{
    return m_comboBox;
}

void DebuggerKitConfigWidget::manageDebuggers()
{
    Core::ICore::showOptionsDialog(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                                   ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
}

void DebuggerKitConfigWidget::currentDebuggerChanged(int)
{
    m_kit->setValue(DebuggerKitInformation::id(), m_comboBox->itemData(m_comboBox->currentIndex()));
}

void DebuggerKitConfigWidget::onDebuggerAdded(const QVariant &id, const QString &displayName)
{
    m_comboBox->setEnabled(true);
    m_comboBox->addItem(displayName, id);
    updateComboBox(id);
}

void DebuggerKitConfigWidget::onDebuggerUpdated(const QVariant &id, const QString &displayName)
{
    m_comboBox->setEnabled(true);
    const int pos = indexOf(id);
    if (pos < 0)
        return;
    m_comboBox->setItemText(pos, displayName);
}

void DebuggerKitConfigWidget::onDebuggerRemoved(const QVariant &id)
{
    if (const int pos = indexOf(id)) {
        m_comboBox->removeItem(pos);
        updateComboBox(id);
    }
}

int DebuggerKitConfigWidget::indexOf(const QVariant &id)
{
    QTC_ASSERT(id.isValid(), return -1);
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i))
            return i;
    }
    return -1;
}

QVariant DebuggerKitConfigWidget::currentId() const
{
    return m_comboBox->itemData(m_comboBox->currentIndex());
}

void DebuggerKitConfigWidget::updateComboBox(const QVariant &id)
{
    for (int i = 0; i < m_comboBox->count(); ++i) {
        if (id == m_comboBox->itemData(i)) {
            m_comboBox->setCurrentIndex(i);
            return;
        }
    }
    m_comboBox->setCurrentIndex(0);
}

// -----------------------------------------------------------------------
// DebuggerItemConfigWidget
// -----------------------------------------------------------------------

class DebuggerItemConfigWidget : public QWidget
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::DebuggerItemConfigWidget)
public:
    explicit DebuggerItemConfigWidget();
    void loadItem();
    void saveItem();
    void connectDirty();
    void disconnectDirty();

private:
    QLineEdit *m_displayNameLineEdit;
    QLabel *m_cdbLabel;
    PathChooser *m_binaryChooser;
    QLineEdit *m_abis;
};

DebuggerItemConfigWidget::DebuggerItemConfigWidget()
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);

    m_cdbLabel = new QLabel(this);
    m_cdbLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_cdbLabel->setOpenExternalLinks(true);

    m_abis = new QLineEdit(this);
    m_abis->setEnabled(false);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
//    formLayout->addRow(new QLabel(tr("Type:")), m_engineTypeComboBox);
    formLayout->addRow(m_cdbLabel);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("Abis:")), m_abis);

    connectDirty();
}

void DebuggerItemConfigWidget::connectDirty()
{
    DebuggerItemManager *manager = theDebuggerItemManager();
    connect(m_displayNameLineEdit, SIGNAL(textChanged(QString)),
            manager, SLOT(markCurrentDirty()));
    connect(m_binaryChooser, SIGNAL(changed(QString)),
            manager, SLOT(markCurrentDirty()));
}

void DebuggerItemConfigWidget::disconnectDirty()
{
    DebuggerItemManager *manager = theDebuggerItemManager();
    disconnect(m_displayNameLineEdit, SIGNAL(textChanged(QString)),
            manager, SLOT(markCurrentDirty()));
    disconnect(m_binaryChooser, SIGNAL(changed(QString)),
            manager, SLOT(markCurrentDirty()));
}

void DebuggerItemConfigWidget::loadItem()
{
    DebuggerItemManager *manager = theDebuggerItemManager();
    const DebuggerItem *item = manager->findById(manager->m_currentDebugger);
    if (!item)
        return;

    disconnectDirty();
    m_displayNameLineEdit->setEnabled(!item->isAutoDetected);
    m_displayNameLineEdit->setText(item->displayName);

    m_binaryChooser->setEnabled(!item->isAutoDetected);
    m_binaryChooser->setFileName(item->command);
    connectDirty();

    QString text;
    QString versionCommand;
    if (item->engineType == CdbEngineType) {
#ifdef Q_OS_WIN
        const bool is64bit = winIs64BitSystem();
#else
        const bool is64bit = false;
#endif
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

    m_abis->setText(toList(item->abis).join(QLatin1String(", ")));
}

void DebuggerItemConfigWidget::saveItem()
{
    theDebuggerItemManager()->setCurrentData(m_displayNameLineEdit->text(), m_binaryChooser->fileName());
}

// --------------------------------------------------------------------------
// DebuggerOptionsPage
// --------------------------------------------------------------------------

DebuggerOptionsPage::DebuggerOptionsPage()
{
    m_manager = 0;
    m_debuggerView = 0;
    m_container = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;

    setId(ProjectExplorer::Constants::DEBUGGER_SETTINGS_PAGE_ID);
    setDisplayName(tr("Debuggers"));
    setCategory(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
        ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));
}

QWidget *DebuggerOptionsPage::createPage(QWidget *parent)
{
    m_configWidget = new QWidget(parent);

    m_addButton = new QPushButton(tr("Add"), m_configWidget);
    m_cloneButton = new QPushButton(tr("Clone"), m_configWidget);
    m_delButton = new QPushButton(tr("Remove"), m_configWidget);

    m_container = new DetailsWidget(m_configWidget);
    m_container->setState(DetailsWidget::NoSummary);
    m_container->setVisible(false);

    m_manager = theDebuggerItemManager();

    m_debuggerView = new QTreeView(m_configWidget);
    m_debuggerView->setModel(m_manager);
    m_debuggerView->setUniformRowHeights(true);
    m_debuggerView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_debuggerView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_debuggerView->expandAll();

    QHeaderView *header = m_debuggerView->header();
    header->setStretchLastSection(false);
    header->setResizeMode(0, QHeaderView::ResizeToContents);
    header->setResizeMode(1, QHeaderView::ResizeToContents);
    header->setResizeMode(2, QHeaderView::Stretch);

    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->setSpacing(6);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cloneButton);
    buttonLayout->addWidget(m_delButton);
    buttonLayout->addItem(new QSpacerItem(10, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QVBoxLayout *verticalLayout = new QVBoxLayout();
    verticalLayout->addWidget(m_debuggerView);
    verticalLayout->addWidget(m_container);

    QHBoxLayout *horizontalLayout = new QHBoxLayout(m_configWidget);
    horizontalLayout->addLayout(verticalLayout);
    horizontalLayout->addLayout(buttonLayout);

    connect(m_debuggerView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(debuggerSelectionChanged()));

    connect(m_addButton, SIGNAL(clicked()), this, SLOT(addDebugger()), Qt::QueuedConnection);
    connect(m_cloneButton, SIGNAL(clicked()), this, SLOT(cloneDebugger()), Qt::QueuedConnection);
    connect(m_delButton, SIGNAL(clicked()), this, SLOT(removeDebugger()), Qt::QueuedConnection);

    m_searchKeywords = tr("Debuggers");

    m_itemConfigWidget = new DebuggerItemConfigWidget;
    m_container->setWidget(m_itemConfigWidget);

    updateState();

    return m_configWidget;
}

void DebuggerOptionsPage::apply()
{
    m_itemConfigWidget->saveItem();
    debuggerModelChanged();
}

void DebuggerOptionsPage::cloneDebugger()
{
    m_manager->cloneDebugger();
    debuggerModelChanged();
}

void DebuggerOptionsPage::addDebugger()
{
    m_manager->addDebugger();
    debuggerModelChanged();
}

void DebuggerOptionsPage::removeDebugger()
{
    m_manager->removeDebugger();
    debuggerModelChanged();
}

void DebuggerOptionsPage::finish()
{
    // Deleted by settingsdialog.
    m_configWidget = 0;

    // Children of m_configWidget.
    m_container = 0;
    m_debuggerView = 0;
    m_addButton = 0;
    m_cloneButton = 0;
    m_delButton = 0;
}

bool DebuggerOptionsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

void DebuggerOptionsPage::debuggerSelectionChanged()
{
    QTC_ASSERT(m_container, return);

    QModelIndex mi = m_debuggerView->currentIndex();
    mi = mi.sibling(mi.row(), 0);
    m_manager->setCurrentIndex(mi);

    m_itemConfigWidget->loadItem();
    m_container->setVisible(m_manager->m_currentDebugger.isValid());
    updateState();
}

void DebuggerOptionsPage::debuggerModelChanged()
{
    QTC_ASSERT(m_container, return);

    m_itemConfigWidget->loadItem();
    m_container->setVisible(m_manager->m_currentDebugger.isValid());
    m_debuggerView->setCurrentIndex(m_manager->currentIndex());
    updateState();
}

void DebuggerOptionsPage::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;

    if (const DebuggerItem *item = m_manager->findById(m_manager->m_currentDebugger)) {
        canCopy = item->isValid() && item->canClone();
        canDelete = !item->isAutoDetected;
        canDelete = true; // Do we want to remove auto-detected items?
    }
    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

} // namespace Internal
} // namespace Debugger
