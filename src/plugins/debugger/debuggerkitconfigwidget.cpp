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

static const char DEBUGGER_INFORMATION[] = "Debugger.Information";
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

DebuggerItem::DebuggerItem()
{
    engineType = NoEngineType;
    isAutoDetected = false;
}

bool DebuggerItem::looksLike(const DebuggerItem &rhs) const
{
    return engineType == rhs.engineType
        && displayName == rhs.displayName
            && command == rhs.command;
}

QString DebuggerItem::engineTypeName() const
{
    switch (engineType) {
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

QString DebuggerItem::userOutput() const
{
    const QString binary = command.toUserOutput();
    const QString name = DebuggerKitInformation::tr("%1 Engine").arg(engineTypeName());

    return binary.isEmpty() ? DebuggerKitInformation::tr("%1 <None>").arg(name)
                            : DebuggerKitInformation::tr("%1 using \"%2\"").arg(name, binary);
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

// --------------------------------------------------------------------------
// DebuggerKitInformation
// --------------------------------------------------------------------------

DebuggerKitInformation::DebuggerKitInformation()
{
    setObjectName(QLatin1String("DebuggerKitInformation"));
    setDataId(DEBUGGER_INFORMATION);
    setPriority(28000);
}

QVariant DebuggerKitInformation::defaultValue(Kit *k) const
{
    if (isValidDebugger(k)) {
        DebuggerItem item = debuggerItem(k);
        return theDebuggerItemManager()->maybeAddDebugger(item, false);
    }

    ToolChain *tc = ToolChainKitInformation::toolChain(k);
    return theDebuggerItemManager()->defaultDebugger(tc);
}

void DebuggerKitInformation::setup(Kit *k)
{
    k->setValue(DEBUGGER_INFORMATION, defaultValue(k));
}

// Check the configuration errors and return a flag mask. Provide a quick check and
// a verbose one with a list of errors.

enum DebuggerConfigurationErrors {
    NoDebugger = 0x1,
    DebuggerNotFound = 0x2,
    DebuggerNotExecutable = 0x4,
    DebuggerNeedsAbsolutePath = 0x8
};

static unsigned debuggerConfigurationErrors(const Kit *k)
{
    unsigned result = 0;
    const DebuggerItem item = DebuggerKitInformation::debuggerItem(k);
    if (item.engineType == NoEngineType || item.command.isEmpty())
        return NoDebugger;

    const QFileInfo fi = item.command.toFileInfo();
    if (!fi.exists() || fi.isDir())
        result |= DebuggerNotFound;
    else if (!fi.isExecutable())
        result |= DebuggerNotExecutable;

    if (!fi.exists() || fi.isDir())
        // We need an absolute path to be able to locate Python on Windows.
        if (item.engineType == GdbEngineType)
            if (const ToolChain *tc = ToolChainKitInformation::toolChain(k))
                if (tc->targetAbi().os() == Abi::WindowsOS && !fi.isAbsolute())
                    result |= DebuggerNeedsAbsolutePath;
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

    const QString path = DebuggerKitInformation::debuggerCommand(k).toUserOutput();

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
    return new Internal::DebuggerKitConfigWidget(k, isSticky(k));
}

KitInformation::ItemList DebuggerKitInformation::toUserOutput(const Kit *k) const
{
    return ItemList() << qMakePair(tr("Debugger"), debuggerItem(k).userOutput());
}

DebuggerItem DebuggerKitInformation::debuggerItem(const ProjectExplorer::Kit *k)
{
    if (!k)
        return DebuggerItem();

    // We used to have:
    // <valuemap type="QVariantMap" key="Debugger.Information">
    //    <value type="QString" key="Binary">/data/dev/debugger/gdb-git/gdb/gdb</value>
    //    <value type="int" key="EngineType">1</value>
    //  </valuemap>
    // Starting with 3.0 we have:
    // <value type="QString" key="Debugger.Information">{75ecf347-f221-44c3-b613-ea1d29929cd4}</value>

    QVariant id = k->value(DEBUGGER_INFORMATION);

    QString pathOrUid;
    if (id.type() == QVariant::Map) // 2.x
        pathOrUid = id.toMap().value(QLatin1String("Binary")).toString();
    else if (id.type() == QVariant::String) // 3.x
        pathOrUid = id.toString();

    DebuggerItem *item;
    if (pathOrUid.startsWith(QLatin1Char('{')))
        item = DebuggerItemManager::debuggerFromId(id);
    else
        item = DebuggerItemManager::debuggerFromPath(pathOrUid);

    QTC_ASSERT(item, return DebuggerItem());
    return *item;
}

void DebuggerKitInformation::setDebuggerItem(Kit *k,
    DebuggerEngineType type, const Utils::FileName &command)
{
    QTC_ASSERT(k, return);
    DebuggerItem item;
    item.engineType = type;
    item.command = command;
    QVariant id = theDebuggerItemManager()->maybeAddDebugger(item);
    k->setValue(DEBUGGER_INFORMATION, id);
}

void DebuggerKitInformation::setDebuggerCommand(Kit *k, const FileName &command)
{
    DebuggerItem item = debuggerItem(k);
    item.command = command;
    QVariant id = theDebuggerItemManager()->maybeAddDebugger(item);
    k->setValue(DEBUGGER_INFORMATION, id);
}

void DebuggerKitInformation::makeSticky(Kit *k)
{
    k->makeSticky(DEBUGGER_INFORMATION);
}


namespace Internal {

static FileName userSettingsFileName()
{
    QFileInfo settingsLocation(Core::ICore::settings()->fileName());
    return FileName::fromString(settingsLocation.absolutePath() + QLatin1String(DEBUGGER_FILENAME));
}

static QList<QStandardItem *> describeItem(DebuggerItem *item)
{
    QList<QStandardItem *> row;
    row.append(new QStandardItem(item->displayName));
    row.append(new QStandardItem(item->command.toString()));
    row.append(new QStandardItem(item->engineTypeName()));
    row.at(0)->setData(item->id);
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
    m_currentDebugger = 0;
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
    qDeleteAll(m_debuggers);
    m_debuggers.clear();
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
        DebuggerItem item;
        item.isAutoDetected = true;
        item.abis = Abi::abisOfBinary(cdb);
        item.command = cdb;
        item.engineType = CdbEngineType;
        item.displayName = tr("Auto-detected CDB at %1").arg(cdb.toUserOutput());
        maybeAddDebugger(item, false);
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
            DebuggerItem item;
            item.command = FileName::fromString(fi.absoluteFilePath());
            item.reinitializeFromFile();
            item.displayName = tr("System %1 at %2")
                .arg(item.engineTypeName()).arg(fi.absoluteFilePath());
            item.isAutoDetected = true;
            maybeAddDebugger(item);
        }
    }
}

QVariant DebuggerItemManager::maybeAddDebugger(const DebuggerItem &testItem, bool makeCurrent)
{
    foreach (DebuggerItem *it, m_debuggers) {
        if (it->looksLike(testItem)) {
            m_currentDebugger = it;
            return it->id;
        }
    }

    DebuggerItem *item = new DebuggerItem(testItem);
    if (item->id.isNull())
        item->id = QUuid::createUuid().toString();

    QList<QStandardItem *> row = describeItem(item);
    (item->isAutoDetected ? m_autoRoot : m_manualRoot)->appendRow(row);
    m_debuggers.append(item);
    m_itemFromDebugger[item] = row.at(0);
    m_debuggerFromItem[row.at(0)] = item;

    if (makeCurrent)
        m_currentDebugger = item;

    emit debuggerAdded(item);
    return item->id;
}

void DebuggerItemManager::updateCurrentItem()
{
    QTC_ASSERT(m_currentDebugger, return);
    QStandardItem *item = m_itemFromDebugger.value(m_currentDebugger);
    QTC_ASSERT(item, return);
    m_currentDebugger->reinitializeFromFile();
    QStandardItem *parent = item->parent();
    QTC_ASSERT(parent, return);
    int row = item->row();
    parent->child(row, 0)->setData(m_currentDebugger->displayName, Qt::DisplayRole);
    parent->child(row, 1)->setData(m_currentDebugger->command.toString(), Qt::DisplayRole);
    parent->child(row, 2)->setData(m_currentDebugger->engineTypeName(), Qt::DisplayRole);
    emit debuggerUpdated(m_currentDebugger);
}

static QList<DebuggerItem *> readDebuggers(const FileName &fileName)
{
    QList<DebuggerItem *> result;

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
        DebuggerItem *debugger = new DebuggerItem;
        debugger->fromMap(dbMap);
        result.append(debugger);
    }

    return result;
}

void DebuggerItemManager::restoreDebuggers()
{
    QList<DebuggerItem *> dbsToCheck;

    // Read debuggers from SDK
    QFileInfo systemSettingsFile(Core::ICore::settings(QSettings::SystemScope)->fileName());
    QList<DebuggerItem *> dbsToRegister =
            readDebuggers(FileName::fromString(systemSettingsFile.absolutePath() + QLatin1String(DEBUGGER_FILENAME)));

    // These are autodetected.
    foreach (DebuggerItem *item, dbsToRegister)
        item->isAutoDetected = true;

    // SDK debuggers are always considered to be up-to-date, so no need to recheck them.

    // Read all debuggers from user file.
    foreach (DebuggerItem *item, readDebuggers(userSettingsFileName())) {
        if (item->isAutoDetected)
            dbsToCheck.append(item);
        else
            dbsToRegister.append(item);
    }

    // Remove debuggers configured by the SDK.
    foreach (DebuggerItem *item, dbsToRegister) {
        for (int i = dbsToCheck.count(); --i >= 0; ) {
            if (dbsToCheck.at(i)->id == item->id) {
                delete dbsToCheck.at(i);
                dbsToCheck.removeAt(i);
            }
        }
    }

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
    foreach (DebuggerItem *item, dbsToCheck) {
        if (!item->isValid()) {
            qWarning() << QString::fromLatin1("DebuggerItem \"%1\" (%2) dropped since it is not valid")
                          .arg(item->command.toString()).arg(item->id.toString());
            delete item;
        } else {
            dbsToRegister += item;
        }
    }

    // Store manual debuggers
    DebuggerItemManager *manager = theDebuggerItemManager();
    foreach (DebuggerItem *item, dbsToRegister)
        manager->maybeAddDebugger(*item);

    // Then auto detect
    manager->autoDetectDebuggers();
}

void DebuggerItemManager::saveDebuggers()
{
    QTC_ASSERT(m_writer, return);
    QVariantMap data;
    data.insert(QLatin1String(DEBUGGER_FILE_VERSION_KEY), 1);

    int count = 0;
    foreach (DebuggerItem *item, m_debuggers) {
        if (item->isValid()) {
            QVariantMap tmp = item->toMap();
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

QList<DebuggerItem *> DebuggerItemManager::findDebuggers(const Abi &abi) const
{
    QList<DebuggerItem *> result;
    foreach (DebuggerItem *item, m_debuggers)
        foreach (const Abi targetAbi, item->abis)
            if (targetAbi.isCompatibleWith(abi))
                result.append(item);
    return result;
}

DebuggerItem *DebuggerItemManager::debuggerFromId(const QVariant &id)
{
    foreach (DebuggerItem *item, theDebuggerItemManager()->m_debuggers)
        if (item->id == id)
            return item;
    return 0;
}

DebuggerItem *DebuggerItemManager::debuggerFromPath(const QString &path)
{
    foreach (DebuggerItem *item, theDebuggerItemManager()->m_debuggers)
        if (item->command.toString() == path)
            return item;
    return 0;
}

QModelIndex DebuggerItemManager::currentIndex() const
{
    QStandardItem *current = m_itemFromDebugger.value(m_currentDebugger);
    return current ? current->index() : QModelIndex();
}

bool DebuggerItemManager::isLoaded() const
{
    return m_writer;
}

void DebuggerItemManager::addDebugger()
{
    DebuggerItem item;
    item.displayName = tr("New Debugger");
    item.engineType = NoEngineType;
    item.isAutoDetected = false;
    maybeAddDebugger(item);
}

void DebuggerItemManager::cloneDebugger()
{
    DebuggerItem item;
    if (m_currentDebugger)
        item = *m_currentDebugger;
    item.displayName = tr("Clone of %1").arg(item.displayName);
    item.isAutoDetected = false;
    maybeAddDebugger(item);
}

void DebuggerItemManager::removeDebugger()
{
    if (!m_currentDebugger)
        return;

    emit debuggerRemoved(m_currentDebugger);
    bool ok = m_debuggers.removeOne(m_currentDebugger);
    QTC_ASSERT(ok, return);
    QStandardItem *item = m_itemFromDebugger.take(m_currentDebugger);
    DebuggerItem *debugger = m_debuggerFromItem.take(item);
    QTC_ASSERT(debugger == m_currentDebugger, return);
    QTC_ASSERT(item, return);
    QTC_ASSERT(debugger, return);
    QStandardItem *parent = item->parent();
    QTC_ASSERT(parent, return);
    // This will trigger a change of m_currentDebugger via changing the
    // view selection. So don't delete m_currentDebugger but debugger.
    parent->removeRow(item->row());
    QTC_ASSERT(debugger != m_currentDebugger, return);
    delete debugger;
}

void DebuggerItemManager::setCurrentIndex(const QModelIndex &index)
{
    m_currentDebugger = m_debuggerFromItem.value(itemFromIndex(index));
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


    QList<DebuggerItem *> compatible = findDebuggers(abi);
    if (!compatible.isEmpty())
        return compatible.front()->id;

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

DebuggerKitConfigWidget::DebuggerKitConfigWidget(Kit *workingCopy, bool sticky)
    : KitConfigWidget(workingCopy, sticky)
{
    DebuggerItemManager *manager = theDebuggerItemManager();
    QTC_CHECK(manager);

    m_comboBox = new QComboBox;
    m_comboBox->setEnabled(true);
    m_comboBox->setToolTip(toolTip());
    m_comboBox->addItem(tr("None"), QString());
    foreach (DebuggerItem *item, manager->debuggers())
        m_comboBox->addItem(item->displayName, item->id);

    refresh();
    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentDebuggerChanged(int)));

    m_manageButton = new QPushButton(tr("Manage..."));
    m_manageButton->setContentsMargins(0, 0, 0, 0);
    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageDebuggers()));

    connect(manager, SIGNAL(debuggerAdded(DebuggerItem*)),
            this, SLOT(onDebuggerAdded(DebuggerItem*)));
    connect(manager, SIGNAL(debuggerUpdated(DebuggerItem*)),
            this, SLOT(onDebuggerUpdated(DebuggerItem*)));
    connect(manager, SIGNAL(debuggerRemoved(DebuggerItem*)),
            this, SLOT(onDebuggerRemoved(DebuggerItem*)));
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
    DebuggerItem item = DebuggerKitInformation::debuggerItem(m_kit);
    updateComboBox(item.id);
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
    m_kit->setValue(DEBUGGER_INFORMATION, m_comboBox->itemData(m_comboBox->currentIndex()));
}

void DebuggerKitConfigWidget::onDebuggerAdded(DebuggerItem *item)
{
    m_comboBox->setEnabled(true);
    QVariant id = currentId();
    m_comboBox->addItem(item->displayName, item->id);
    updateComboBox(id);
}

void DebuggerKitConfigWidget::onDebuggerUpdated(DebuggerItem *item)
{
    m_comboBox->setEnabled(true);
    const int pos = indexOf(item);
    if (pos < 0)
        return;
    m_comboBox->setItemText(pos, item->displayName);
}

void DebuggerKitConfigWidget::onDebuggerRemoved(DebuggerItem *item)
{
    const int pos = indexOf(item);
    if (pos <= 0)
        return;
    QVariant id = currentId();
    m_comboBox->removeItem(pos);
    updateComboBox(id);
}

int DebuggerKitConfigWidget::indexOf(const DebuggerItem *debugger)
{
    QTC_ASSERT(debugger, return -1);
    const QVariant id = debugger->id;
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
public:
    explicit DebuggerItemConfigWidget();
    void loadItem(DebuggerItem *item);
    void saveItem(DebuggerItem *item);
private:
    QLineEdit *m_displayNameLineEdit;
    QComboBox *m_engineTypeComboBox;
    QLabel *m_cdbLabel;
    PathChooser *m_binaryChooser;
    QLineEdit *m_abis;
};

DebuggerItemConfigWidget::DebuggerItemConfigWidget()
{
    m_displayNameLineEdit = new QLineEdit(this);

    m_engineTypeComboBox = new QComboBox(this);

    m_binaryChooser = new PathChooser(this);
    m_binaryChooser->setExpectedKind(PathChooser::ExistingCommand);
    m_binaryChooser->setMinimumWidth(400);

    m_cdbLabel = new QLabel(this);
    m_cdbLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_cdbLabel->setOpenExternalLinks(true);

    m_abis = new QLineEdit(this);
    m_abis->setEnabled(false);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    nameLayout->addWidget(m_displayNameLineEdit);
    nameLayout->addWidget(new QLabel(tr("Type:")));
    nameLayout->addWidget(m_engineTypeComboBox);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    formLayout->addRow(new QLabel(tr("Name:")), nameLayout);
//    formLayout->addRow(new QLabel(tr("Name:")), m_displayNameLineEdit);
//    formLayout->addRow(new QLabel(tr("Type:")), m_engineTypeComboBox);
    formLayout->addRow(m_cdbLabel);
    formLayout->addRow(new QLabel(tr("Path:")), m_binaryChooser);
    formLayout->addRow(new QLabel(tr("Abis:")), m_abis);

    setLayout(formLayout);
}

void DebuggerItemConfigWidget::loadItem(DebuggerItem *item)
{
    if (!item)
        return;

    m_displayNameLineEdit->setEnabled(!item->isAutoDetected);
    m_displayNameLineEdit->setText(item->displayName);

    // This is immutable.
    m_engineTypeComboBox->clear();
    m_engineTypeComboBox->addItem(item->engineTypeName());
    m_engineTypeComboBox->setCurrentIndex(0);
    //m_engineTypeComboBox->setEnabled(false);

    m_binaryChooser->setEnabled(!item->isAutoDetected);
    m_binaryChooser->setFileName(item->command);

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

void DebuggerItemConfigWidget::saveItem(DebuggerItem *item)
{
    QTC_ASSERT(item, return);
    item->displayName = m_displayNameLineEdit->text();
    item->command = m_binaryChooser->fileName();
    // item->engineType is immutable
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
    m_itemConfigWidget->saveItem(m_manager->currentDebugger());
    m_manager->updateCurrentItem();
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

    DebuggerItem *item = m_manager->currentDebugger();
    m_itemConfigWidget->loadItem(item);
    m_container->setVisible(item != 0);
    updateState();
}

void DebuggerOptionsPage::debuggerModelChanged()
{
    QTC_ASSERT(m_container, return);

    DebuggerItem *item = m_manager->currentDebugger();
    m_itemConfigWidget->loadItem(item);
    m_container->setVisible(item != 0);
    m_debuggerView->setCurrentIndex(m_manager->currentIndex());
    updateState();
}

void DebuggerOptionsPage::updateState()
{
    if (!m_cloneButton)
        return;

    bool canCopy = false;
    bool canDelete = false;

    if (DebuggerItem *item = m_manager->currentDebugger()) {
        canCopy = item->isValid() && item->canClone();
        canDelete = !item->isAutoDetected;
        canDelete = true; // Do we want to remove auto-detected items?
    }
    m_cloneButton->setEnabled(canCopy);
    m_delButton->setEnabled(canDelete);
}

} // namespace Internal
} // namespace Debugger
