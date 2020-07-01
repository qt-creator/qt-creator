/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "uvscclient.h"
#include "uvscutils.h"

#include <QMutexLocker>

#include <memory>
#include <vector>

using namespace Utils;

namespace Debugger {
namespace Internal {

constexpr int kMaximumAflmapSize = 65536;
constexpr int kMaximumTaskEnumsCount = 512;
constexpr int kMaximumStackEnumsCount = 512;
constexpr int kMaximumRegisterGroupsCount = 128;
constexpr int kMaximumRegisterEnumsCount = 512;
constexpr int kMaximumVarinfosCount = 256;
constexpr int kMaximumValueBitsSize = 32;
constexpr int kMaximumBreakpointEnumsCount = 128;
constexpr int kMaximumDisassembledBytesCount = 1024;

const QEvent::Type kUvscMsgEventType = static_cast<QEvent::Type>(QEvent::User + 1);

class UvscMsgEvent final : public QEvent
{
public:
    explicit UvscMsgEvent() : QEvent(QEvent::Type(kUvscMsgEventType))
    {}
    UV_OPERATION operation = UV_NULL_CMD;
    UV_STATUS status = UV_STATUS_SUCCESS;
    QByteArray data;
};

Q_GLOBAL_STATIC(QLibrary, gUvscLibrary)
Q_GLOBAL_STATIC(QVector<UvscClient *>, gUvscClients)

static QMutex gUvscsGuard;

static QStringList childrenINamesOf(const QString &parentIName, const QStringList &allINames)
{
    QStringList childrenINames;
    for (const QString &iname : allINames) {
        const int dotIndex = iname.lastIndexOf('.');
        if (dotIndex < 0)
            continue;
        const QString parentPart = iname.mid(0, dotIndex);
        if (parentIName == parentPart)
            childrenINames.push_back(iname);
    }
    return childrenINames;
}

static void uvsc_handle_command_response(UvscClient *client, const UVSOCK_CMD_RESPONSE &resp)
{
    std::unique_ptr<UvscMsgEvent> event(new UvscMsgEvent);

    switch (resp.command) {
    case UV_DBG_START_EXECUTION:
        break;
    case UV_DBG_STOP_EXECUTION:
        event->data = QByteArray(reinterpret_cast<const char *>(&resp.bpreason),
                                 sizeof(resp.bpreason));
        break;
    case UV_PRJ_CLOSE:
        break;
    default:
        return;
    }

    event->operation = resp.command;
    event->status = resp.status;
    qApp->postEvent(client, event.release());
}

static void uvsc_handle_async_message(UvscClient *client, const UVSOCK_CMD &cmd)
{
    switch (cmd.command) {
    case UV_ASYNC_MSG:
        uvsc_handle_command_response(client, cmd.data.cmdrsp);
        break;
    default:
        break;
    }
}

static void uvsc_callback(void *cb_custom, UVSC_CB_TYPE type, UVSC_CB_DATA *data)
{
    const QMutexLocker lock(&gUvscsGuard);
    const auto client = reinterpret_cast<UvscClient *>(cb_custom);
    if (!gUvscClients->contains(client))
        return;

    switch (type) {
    case UVSC_CB_ASYNC_MSG:
        uvsc_handle_async_message(client, data->msg);
        break;
    default:
        break;
    }
}

// UvscClient

UvscClient::UvscClient(const QDir &uvscDir)
{
    const QMutexLocker lock(&gUvscsGuard);
    gUvscClients->append(this);

    // Try to resolve the UVSC library symbols.
    const bool symbolsResolved = resolveUvscSymbols(uvscDir, gUvscLibrary());
    if (!symbolsResolved) {
        const QString errorReason = gUvscLibrary()->errorString();
        setError(InitializationError, errorReason);
    }
}

UvscClient::~UvscClient()
{
    const QMutexLocker lock(&gUvscsGuard);
    gUvscClients->removeAll(this);

    closeProject();
    disconnectSession();
}

void UvscClient::version(QString &uvscVersion, QString &uvsockVersion)
{
    quint32 uvsc = 0;
    quint32 uvsock = 0;
    ::UVSC_Version(&uvsc, &uvsock);
    uvscVersion = tr("%1.%2").arg(uvsc / 100).arg(uvsc % 100);
    uvsockVersion = tr("%1.%2").arg(uvsock / 100).arg(uvsock % 100);
}

bool UvscClient::connectSession(qint32 uvscPort)
{
    if (m_descriptor != -1)
        return true; // Connection already open.

    // Initializing the UVSC library.
    UVSC_STATUS st = ::UVSC_Init(uvscPort, uvscPort + 1);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(ConnectionError);
        return false;
    }

    // Opening the UVSC connection.
    st = ::UVSC_OpenConnection(nullptr, &m_descriptor, &uvscPort, nullptr,
                               UVSC_RUNMODE_NORMAL, uvsc_callback, this,
                               nullptr, false, nullptr);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(ConnectionError);
        return false;
    }
    return true;
}

void UvscClient::disconnectSession()
{
    if (m_descriptor == -1)
        return; // Connection already closed.

    // Closing the UVSC conenction.
    const bool terminateUVision = true;
    UVSC_STATUS st = ::UVSC_CloseConnection(m_descriptor, terminateUVision);
    if (st != UVSC_STATUS_SUCCESS)
        setError(ConnectionError);
    m_descriptor = -1;

    // De-initializing the UVSC library.
    st = ::UVSC_UnInit();
    if (st != UVSC_STATUS_SUCCESS)
        setError(ConnectionError);
}

bool UvscClient::startSession(bool extendedStack)
{
    if (!checkConnection())
        return false;

    // Set extended stack option.
    UVSOCK_OPTIONS opts = {};
    opts.extendedStack = extendedStack;
    UVSC_STATUS st = ::UVSC_GEN_SET_OPTIONS(m_descriptor, &opts);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    // Enter debugger.
    st = ::UVSC_DBG_ENTER(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::stopSession()
{
    if (!checkConnection())
        return false;

    // Exit debugger.
    const UVSC_STATUS st = ::UVSC_DBG_EXIT(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::executeStepOver()
{
    if (!checkConnection())
        return false;

    const UVSC_STATUS st = ::UVSC_DBG_STEP_HLL(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::executeStepIn()
{
    if (!checkConnection())
        return false;

    const UVSC_STATUS st = ::UVSC_DBG_STEP_INTO(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::executeStepOut()
{
    if (!checkConnection())
        return false;

    const UVSC_STATUS st = ::UVSC_DBG_STEP_OUT(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::executeStepInstruction()
{
    if (!checkConnection())
        return false;

    const UVSC_STATUS st = ::UVSC_DBG_STEP_INSTRUCTION(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::startExecution()
{
    if (!checkConnection())
        return false;

    const UVSC_STATUS st = ::UVSC_DBG_START_EXECUTION(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::stopExecution()
{
    if (!checkConnection())
        return false;

    const UVSC_STATUS st = ::UVSC_DBG_STOP_EXECUTION(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::fetchThreads(bool showNames, GdbMi &data)
{
    if (!checkConnection())
        return false;

    std::vector<TASKENUM> taskenums(kMaximumTaskEnumsCount);
    qint32 taskenumsCount = qint32(taskenums.size());
    const UVSC_STATUS st = ::UVSC_DBG_ENUM_TASKS(m_descriptor, taskenums.data(),
                                                 &taskenumsCount);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    } else {
        taskenums.resize(taskenumsCount);
    }

    // Build threads entry.
    GdbMi threads = UvscUtils::buildEntry("threads", "", GdbMi::List);
    for (const TASKENUM &taskenum : taskenums) {
        const QString id = QString::number(taskenum.id);
        const QString state = QString::number(taskenum.state);
        const QString name = UvscUtils::decodeSstr(taskenum.name);
        const QString address = QString::number(taskenum.entryAddress);

        QString file;
        QString function;
        quint32 line = -1;
        addressToFileLine(taskenum.entryAddress, file, function, line);

        // Build frame entry.
        GdbMi frame = UvscUtils::buildEntry("frame", "", GdbMi::Tuple);
        frame.addChild(UvscUtils::buildEntry("addr", address, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("func", function, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("fullname", file, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("line", QString::number(line), GdbMi::Const));

        // Build thread entry.
        GdbMi thread = UvscUtils::buildEntry("", "", GdbMi::Tuple);
        if (showNames)
            thread.addChild(UvscUtils::buildEntry("name", name, GdbMi::Const));
        thread.addChild(UvscUtils::buildEntry("id", id, GdbMi::Const));
        thread.addChild(UvscUtils::buildEntry("state", state, GdbMi::Const));
        thread.addChild(frame);

        threads.addChild(thread);
    }

    // Build data entry.
    data = UvscUtils::buildEntry("data", "", GdbMi::Tuple);
    data.addChild(threads);
    return true;
}

bool UvscClient::fetchStackFrames(quint32 taskId, quint64 address, GdbMi &data)
{
    if (!checkConnection())
        return false;

    std::vector<STACKENUM> stackenums;
    const bool success = enumerateStack(taskId, stackenums);
    if (!success) {
        setError(RuntimeError);
        return false;
    } else {
        // Fix the current stack address.
        STACKENUM &first = stackenums.front();
        if (first.currentAddress == 0 && address != 0)
            first.currentAddress = address;
    }

    // Build frames entry.
    GdbMi frames = UvscUtils::buildEntry("frames", "", GdbMi::List);
    // We need to iterate in a reverse order, because we has
    // enabled the 'stack extent' fearure, which returns a
    // frames in a reverse order!
    const auto stackenumBegin = std::crbegin(stackenums);
    const auto stackenumEnd = std::crend(stackenums);
    for (auto stackenumIt = stackenumBegin; stackenumIt < stackenumEnd; ++stackenumIt) {
        const QString level = QString::number(stackenumIt->number);
        const QString address = QStringLiteral("0x%1")
                .arg(QString::number(stackenumIt->currentAddress, 16));
        const QString context = QStringLiteral("0x%1")
                .arg(QString::number(stackenumIt->returnAddress, 16));

        QString file;
        QString function;
        quint32 line = -1;
        addressToFileLine(stackenumIt->currentAddress, file, function, line);

        // Build frame entry.
        GdbMi frame = UvscUtils::buildEntry("frame", "", GdbMi::Tuple);
        frame.addChild(UvscUtils::buildEntry("level", level, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("address", address, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("context", context, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("function", function, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("file", file, GdbMi::Const));
        frame.addChild(UvscUtils::buildEntry("line", QString::number(line), GdbMi::Const));

        frames.addChild(frame);
    }

    // Build stack entry.
    GdbMi stack = UvscUtils::buildEntry("stack", "", GdbMi::Tuple);
    stack.addChild(frames);
    // Build data entry.
    data = UvscUtils::buildEntry("result", "", GdbMi::Tuple);
    data.addChild(stack);
    return true;
}

bool UvscClient::fetchRegisters(Registers &registers)
{
    if (!checkConnection())
        return false;

    // Enumerate the register groups names.
    std::vector<SSTR> sstrs(kMaximumRegisterGroupsCount);
    qint32 sstrsCount = qint32(sstrs.size());
    UVSC_STATUS st = ::UVSC_DBG_ENUM_REGISTER_GROUPS(m_descriptor, sstrs.data(),
                                                     &sstrsCount);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    } else {
        sstrs.resize(sstrsCount);
    }

    std::map<int, QString> groups;
    int groupIndex = 0;
    for (const SSTR &sstr : sstrs)
        groups[groupIndex++] = UvscUtils::decodeSstr(sstr);

    // Enumerate the registers.
    std::vector<REGENUM> regenums(kMaximumRegisterEnumsCount);
    qint32 regenumsCount = qint32(regenums.size());
    st = ::UVSC_DBG_ENUM_REGISTERS(m_descriptor, regenums.data(), &regenumsCount);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    } else {
        regenums.resize(regenumsCount);
    }

    int registerIndex = 0;
    for (const REGENUM &regenum : regenums) {
        if (!UvscUtils::isKnownRegister(regenum.item))
            continue;
        Register reg;
        reg.kind = FlagRegister;
        reg.name = UvscUtils::decodeAscii(regenum.name);
        reg.description = groups.at(regenum.groupIndex);
        registers[registerIndex++] = reg;
    }

    // Read the register values.
    std::vector<qint8> values(kMaximumRegisterEnumsCount * kMaximumValueBitsSize);
    qint32 length = qint32(values.size());
    st = ::UVSC_DBG_READ_REGISTERS( m_descriptor, values.data(), &length);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    } else {
        values.resize(length);
    }

    const auto valueBegin = std::cbegin(values);
    const auto valueEnd = std::cend(values);
    const auto registerEnd = std::cend(registers);
    registerIndex = 0;
    for (auto valueIt = valueBegin; valueIt < valueEnd;
         valueIt += kMaximumValueBitsSize) {
        QString value = UvscUtils::decodeAscii(&(*valueIt));
        const auto &registerIt = registers.find(registerIndex++);
        if (registerIt == registerEnd)
            continue;
        Register &reg = registerIt->second;
        reg.size = 4; // We assume that all registers has 32-bit width.
        if (value.startsWith("0x")) {
            // For ARM architecture.
            reg.value.fromString(value, HexadecimalFormat);
        } else if (value.startsWith("C:0x")) {
            // For MCS51 architecture.
            value.remove(0, 2); // Remove 'C:' prefix.
            reg.value.fromString(value, HexadecimalFormat);
        } else {
            reg.value.fromString(value, SignedDecimalFormat);
        }
    }

    return true;
}

bool UvscClient::fetchLocals(const QStringList &expandedLocalINames,
                             qint32 taskId, qint32 frameId, GdbMi &data)
{
    if (!checkConnection())
        return false;

    // Always start inspection from the root 'local'.
    if (!inspectLocal(expandedLocalINames, "local", 0, taskId, frameId, data))
        return false;
    return true;
}

bool UvscClient::inspectLocal(const QStringList &expandedLocalINames,
                              const QString &localIName, qint32 localId,
                              qint32 taskId, qint32 frameId, GdbMi &data)
{
    IVARENUM ivarenum = {};
    ivarenum.id = localId;
    ivarenum.task = taskId;
    ivarenum.frame = frameId;
    ivarenum.count = kMaximumVarinfosCount / 2;
    std::vector<VARINFO> varinfos(kMaximumVarinfosCount);
    qint32 varinfosCount = qint32(varinfos.size());
    const UVSC_STATUS st = ::UVSC_DBG_ENUM_VARIABLES(m_descriptor, &ivarenum,
                                                     varinfos.data(), &varinfosCount);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    } else {
        varinfos.resize(varinfosCount);
    }

    const QStringList childrenINames = childrenINamesOf(localIName, expandedLocalINames);

    std::vector<GdbMi> children;
    for (const VARINFO &varinfo : varinfos) {
        const QString id = UvscUtils::buildLocalId(varinfo);
        const QString valueeditable = UvscUtils::buildLocalEditable(varinfo);
        const QString numchild = UvscUtils::buildLocalNumchild(varinfo);
        const QString name = UvscUtils::buildLocalName(varinfo);
        const QString iname = UvscUtils::buildLocalIName(localIName, name);
        const QString type = UvscUtils::buildLocalType(varinfo);
        const QString value = UvscUtils::buildLocalValue(varinfo, type);

        GdbMi child = UvscUtils::buildEntry("", "", GdbMi::Tuple);
        child.addChild(UvscUtils::buildEntry("id", id, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("iname", iname, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("name", name, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("numchild", numchild, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("type", type, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("value", value, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("valueeditable", valueeditable, GdbMi::Const));
        // We need also to add and the expression value (that allows to drag & drop
        // the watcher items).
        child.addChild(UvscUtils::buildEntry("exp", name, GdbMi::Const));

        if (childrenINames.contains(iname)) {
            if (!inspectLocal(expandedLocalINames, iname, varinfo.id, taskId, frameId, child))
                return false;
        }

        children.push_back(child);
    }

    if (localIName == "local") {
        for (const GdbMi &child : qAsConst(children))
            data.addChild(child);
    } else {
        const GdbMi childrenEntry = UvscUtils::buildChildrenEntry(children);
        data.addChild(childrenEntry);
    }

    return true;
}

bool UvscClient::fetchWatchers(const QStringList &expandedWatcherINames,
                               const std::vector<std::pair<QString, QString>> &rootWatchers,
                               GdbMi &data)
{
    if (!checkConnection())
        return false;

    for (const std::pair<QString, QString> &rootWatcher : rootWatchers) {
        if (!fetchWatcher(expandedWatcherINames, rootWatcher, data))
            return false;
    }

    return true;
}

bool UvscClient::fetchWatcher(const QStringList &expandedWatcherINames,
                              const std::pair<QString, QString> &rootWatcher, GdbMi &data)
{
    const QString rootIName = rootWatcher.first;
    const QString rootExp = QString::fromLatin1(QByteArray::fromHex(
                                                    rootWatcher.second.toLatin1()));
    const quint64 frameAddress = 0;
    VSET vset = UvscUtils::encodeU64Vset(frameAddress, rootExp);
    VARINFO varinfo = {};
    const UVSC_STATUS st = ::UVSC_DBG_EVAL_WATCH_EXPRESSION(m_descriptor, &vset,
                                                            sizeof(vset), &varinfo);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    const QString id = UvscUtils::buildLocalId(varinfo);
    const QString valueeditable = UvscUtils::buildLocalEditable(varinfo);
    const QString numchild = UvscUtils::buildLocalNumchild(varinfo);
    const QString iname = UvscUtils::buildLocalIName(rootIName);
    const QString wname = UvscUtils::buildLocalWName(rootExp);
    const QString type = UvscUtils::buildLocalType(varinfo);
    const QString value = UvscUtils::buildLocalValue(varinfo, type);

    GdbMi watcher = UvscUtils::buildEntry("", "", GdbMi::Tuple);
    watcher.addChild(UvscUtils::buildEntry("id", id, GdbMi::Const));
    watcher.addChild(UvscUtils::buildEntry("iname", iname, GdbMi::Const));
    watcher.addChild(UvscUtils::buildEntry("wname", wname, GdbMi::Const));
    watcher.addChild(UvscUtils::buildEntry("numchild", numchild, GdbMi::Const));
    watcher.addChild(UvscUtils::buildEntry("type", type, GdbMi::Const));
    watcher.addChild(UvscUtils::buildEntry("value", value, GdbMi::Const));
    watcher.addChild(UvscUtils::buildEntry("valueeditable", valueeditable, GdbMi::Const));

    if (expandedWatcherINames.contains(rootIName)) {
        const qint32 watcherId = varinfo.id;
        if (!inspectWatcher(expandedWatcherINames, watcherId, iname, watcher))
            return false;
    }

    data.addChild(watcher);
    return true;
}

bool UvscClient::inspectWatcher(const QStringList &expandedWatcherINames,
                                qint32 watcherId, const QString &watcherIName, GdbMi &parent)
{
    IVARENUM ivarenum = {};
    ivarenum.id = watcherId;
    ivarenum.count = kMaximumVarinfosCount / 2;
    std::vector<VARINFO> varinfos(kMaximumVarinfosCount);
    qint32 varinfosCount = qint32(varinfos.size());
    const UVSC_STATUS st = ::UVSC_DBG_ENUM_VARIABLES(m_descriptor, &ivarenum,
                                                     varinfos.data(), &varinfosCount);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    } else {
        varinfos.resize(varinfosCount);
    }

    const QStringList childrenINames = childrenINamesOf(watcherIName, expandedWatcherINames);

    std::vector<GdbMi> children;
    for (const VARINFO &varinfo : varinfos) {
        const QString id = UvscUtils::buildLocalId(varinfo);
        const QString valueeditable = UvscUtils::buildLocalEditable(varinfo);
        const QString numchild = UvscUtils::buildLocalNumchild(varinfo);
        const QString name = UvscUtils::buildLocalName(varinfo);
        const QString type = UvscUtils::buildLocalType(varinfo);
        const QString value = UvscUtils::buildLocalValue(varinfo, type);

        GdbMi child = UvscUtils::buildEntry("", "", GdbMi::Tuple);
        child.addChild(UvscUtils::buildEntry("id", id, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("name", name, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("numchild", numchild, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("type", type, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("value", value, GdbMi::Const));
        child.addChild(UvscUtils::buildEntry("valueeditable", valueeditable, GdbMi::Const));

        const QString iname = UvscUtils::buildLocalIName(watcherIName, name);
        if (childrenINames.contains(iname)) {
            const qint32 watcherId = varinfo.id;
            if (!inspectWatcher(expandedWatcherINames, watcherId, iname, child))
                return false;
        }

        children.push_back(child);
    }

    const GdbMi childrenEntry = UvscUtils::buildChildrenEntry(children);
    parent.addChild(childrenEntry);
    return true;
}

bool UvscClient::fetchMemory(quint64 address, QByteArray &data)
{
    if (data.isEmpty())
        data.resize(sizeof(quint8));

    QByteArray amem = UvscUtils::encodeAmem(address, data);
    const auto amemPtr = reinterpret_cast<AMEM *>(amem.data());
    const UVSC_STATUS st = ::UVSC_DBG_MEM_READ(m_descriptor, amemPtr, amem.size());
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    data = QByteArray(reinterpret_cast<char *>(&amemPtr->bytes),
                      amemPtr->bytesCount);
    return true;
}

bool UvscClient::changeMemory(quint64 address, const QByteArray &data)
{
    if (data.isEmpty()) {
        setError(RuntimeError);
        return false;
    }

    QByteArray amem = UvscUtils::encodeAmem(address, data);
    const auto amemPtr = reinterpret_cast<AMEM *>(amem.data());
    const UVSC_STATUS st = ::UVSC_DBG_MEM_WRITE(m_descriptor, amemPtr, amem.size());
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::disassemblyAddress(quint64 address, QByteArray &result)
{
    if (!checkConnection())
        return false;

    QByteArray amem = UvscUtils::encodeAmem(address, kMaximumDisassembledBytesCount);
    const auto amemPtr = reinterpret_cast<AMEM *>(amem.data());
    const UVSC_STATUS st = ::UVSC_DBG_DSM_READ(m_descriptor, amemPtr, amem.size());
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    result = QByteArray(reinterpret_cast<char *>(&amemPtr->bytes),
                        amemPtr->bytesCount);
    return true;
}

bool UvscClient::setRegisterValue(int index, const QString &value)
{
    if (!checkConnection())
        return false;

    VSET vset = UvscUtils::encodeIntVset(index, value);
    const UVSC_STATUS st = ::UVSC_DBG_REGISTER_SET(m_descriptor, &vset, sizeof(vset));
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::setLocalValue(qint32 localId, qint32 taskId, qint32 frameId,
                               const QString &value)
{
    if (!checkConnection())
        return false;

    VARVAL varval = {};
    varval.variable.id = localId;
    varval.variable.task = taskId;
    varval.variable.frame = frameId;
    varval.value = UvscUtils::encodeSstr(value);
    const UVSC_STATUS st = ::UVSC_DBG_VARIABLE_SET(m_descriptor, &varval, sizeof(varval));
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::setWatcherValue(qint32 watcherId, const QString &value)
{
    if (!checkConnection())
        return false;

    VARVAL varval = {};
    varval.variable.id = watcherId;
    varval.value = UvscUtils::encodeSstr(value);
    const UVSC_STATUS st = ::UVSC_DBG_VARIABLE_SET(m_descriptor, &varval, sizeof(varval));
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::createBreakpoint(const QString &exp, quint32 &tickMark, quint64 &address,
                                  quint32 &line, QString &function, QString &fileName)
{
    if (!checkConnection())
        return false;

    // Magic workaround to prevent the stalling.
    if (!controlHiddenBreakpoint(exp))
        return false;

    // Execute command to create the BP.
    const QString setCmd = QStringLiteral("BS %1").arg(exp);
    QString setCmdOutput;
    if (!executeCommand(setCmd, setCmdOutput))
        return false;

    std::vector<BKRSP> bpenums;
    if (!enumerateBreakpoints(bpenums))
        return false;

    const auto bpenumBegin = bpenums.cbegin();
    const auto bpenumEnd = bpenums.cend();
    const auto bpenumIt = std::find_if(bpenumBegin, bpenumEnd, [exp](const BKRSP &bpenum) {
        const QString bpexp = QString::fromLatin1(reinterpret_cast<const char *>(bpenum.expressionBuffer),
                                                  bpenum.expressionLength).trimmed();
        return bpexp.contains(exp);
    });
    if (bpenumIt == bpenumEnd)
        return false;

    tickMark = bpenumIt->tickMark;
    address = bpenumIt->address;

    if (!addressToFileLine(address, fileName, function, line))
        return false;
    return true;
}

bool UvscClient::deleteBreakpoint(quint32 tickMark)
{
    if (!checkConnection())
        return false;

    BKCHG bkchg = {};
    bkchg.type = CHG_KILLBP;
    bkchg.tickMark = tickMark;
    BKRSP bkrsp = {};
    qint32 bkrspLength = sizeof(bkrsp);
    const UVSC_STATUS st = ::UVSC_DBG_CHANGE_BP(m_descriptor, &bkchg, sizeof(bkchg), &bkrsp, &bkrspLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::enableBreakpoint(quint32 tickMark)
{
    if (!checkConnection())
        return false;

    BKCHG bkchg = {};
    bkchg.type = CHG_ENABLEBP;
    bkchg.tickMark = tickMark;
    BKRSP bkrsp = {};
    qint32 bkrspLength = sizeof(bkrsp);
    const UVSC_STATUS st = ::UVSC_DBG_CHANGE_BP(m_descriptor, &bkchg, sizeof(bkchg), &bkrsp, &bkrspLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::disableBreakpoint(quint32 tickMark)
{
    if (!checkConnection())
        return false;

    BKCHG bkchg = {};
    bkchg.type = CHG_DISABLEBP;
    bkchg.tickMark = tickMark;
    BKRSP bkrsp = {};
    qint32 bkrspLength = sizeof(bkrsp);
    const UVSC_STATUS st = ::UVSC_DBG_CHANGE_BP(m_descriptor, &bkchg, sizeof(bkchg), &bkrsp, &bkrspLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::controlHiddenBreakpoint(const QString &exp)
{
    if (!checkConnection())
        return false;

    // It is a magic workaround to prevent the UVSC bug when the break-point
    // creation may stall. A problem is that sometime the UVSC_DBG_CREATE_BP
    // function blocks and returns then with the timeout error when the original
    // break-point contains the full expression including the line number.
    //
    // It can be avoided with helps of creation and then deletion of the
    // 'fake hidden' break-point with the same expression excluding the line
    // number, before creation of an original break-point.

    const int slashIndex = exp.lastIndexOf('\\');
    if (slashIndex == -1 || (slashIndex + 1) == exp.size())
        return true;

    BKRSP bkrsp = {};

    const QString hiddenExp = exp.mid(0, slashIndex);
    QByteArray bkparm = UvscUtils::encodeBreakPoint(BRKTYPE_EXEC, hiddenExp);
    qint32 bkrspLength = sizeof(bkrsp);
    UVSC_STATUS st = ::UVSC_DBG_CREATE_BP(m_descriptor,
                                          reinterpret_cast<BKPARM *>(bkparm.data()),
                                          bkparm.size(),
                                          &bkrsp, &bkrspLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    BKCHG bkchg = {};
    bkchg.type = CHG_KILLBP;
    bkchg.tickMark = bkrsp.tickMark;
    bkrspLength = sizeof(bkrsp);
    st = ::UVSC_DBG_CHANGE_BP(m_descriptor, &bkchg, sizeof(bkchg), &bkrsp, &bkrspLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    return true;
}

bool UvscClient::enumerateBreakpoints(std::vector<BKRSP> &bpenums)
{
    if (!checkConnection())
        return false;

    bpenums.resize(kMaximumBreakpointEnumsCount);
    qint32 bpenumsCount = kMaximumBreakpointEnumsCount;
    std::vector<qint32> indexes(bpenumsCount, 0);
    const UVSC_STATUS st = ::UVSC_DBG_ENUMERATE_BP(m_descriptor, bpenums.data(),
                                                   indexes.data(), &bpenumsCount);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    bpenums.resize(bpenumsCount);
    return true;
}

bool UvscClient::calculateExpression(const QString &exp, QByteArray &)
{
    if (!checkConnection())
        return false;

    VSET vset = UvscUtils::encodeVoidVset(exp);
    const UVSC_STATUS st = ::UVSC_DBG_CALC_EXPRESSION(m_descriptor, &vset, sizeof(vset));
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }
    return true;
}

bool UvscClient::openProject(const Utils::FilePath &projectFile)
{
    if (!checkConnection())
        return false;

    QByteArray prjdata = UvscUtils::encodeProjectData({projectFile.toString()});
    const UVSC_STATUS st = ::UVSC_PRJ_LOAD(m_descriptor,
                                           reinterpret_cast<PRJDATA *>(prjdata.data()),
                                           prjdata.size());
    if (st != UVSC_STATUS_SUCCESS) {
        setError(ConfigurationError);
        return false;
    }
    return true;
}

void UvscClient::closeProject()
{
    if (!checkConnection())
        return;

    const UVSC_STATUS st = ::UVSC_PRJ_CLOSE(m_descriptor);
    if (st != UVSC_STATUS_SUCCESS)
        setError(ConfigurationError);
}

bool UvscClient::setProjectSources(const FilePath &sourceDirectory,
                                   const FilePaths &sourceFiles)
{
    if (!checkConnection())
        return false;

    QStringList knownGroupNames;
    for (const FilePath &sourceFile : sourceFiles) {
        if (sourceFile.isDir())
            continue;
        if (sourceFile.endsWith(".cpp") || sourceFile.endsWith(".hpp")
                || sourceFile.endsWith(".c") || sourceFile.endsWith(".h")
                || sourceFile.endsWith(".s")) {
            const FilePath parentDir = sourceFile.parentDir();
            QString groupName = parentDir.relativeChildPath(
                        sourceDirectory).toString();
            if (groupName.isEmpty())
                groupName = "default";

            // Add new group if it not exists yet.
            if (!knownGroupNames.contains(groupName)) {
                QByteArray prjdata = UvscUtils::encodeProjectData({groupName});
                const UVSC_STATUS st = ::UVSC_PRJ_ADD_GROUP(m_descriptor,
                                                            reinterpret_cast<PRJDATA *>(prjdata.data()),
                                                            prjdata.size());
                if (st != UVSC_STATUS_SUCCESS) {
                    setError(ConfigurationError);
                    return false;
                }
                knownGroupNames.push_back(groupName);
            }

            // Add new source file to a group.
            QByteArray prjdata = UvscUtils::encodeProjectData({groupName, sourceFile.toString()});
            const UVSC_STATUS st = ::UVSC_PRJ_ADD_FILE(m_descriptor,
                                                       reinterpret_cast<PRJDATA *>(prjdata.data()),
                                                       prjdata.size());
            if (st != UVSC_STATUS_SUCCESS) {
                setError(ConfigurationError);
                return false;
            }
        }
    }
    return true;
}

bool UvscClient::setProjectDebugTarget(bool simulator)
{
    if (!checkConnection())
        return false;

    DBGTGTOPT opt = {};
    opt.targetType = (simulator) ? UV_TARGET_SIM : UV_TARGET_HW;
    const UVSC_STATUS st = ::UVSC_PRJ_SET_DEBUG_TARGET(m_descriptor, &opt);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(ConfigurationError);
        return false;
    }
    return true;
}

bool UvscClient::setProjectOutputTarget(const Utils::FilePath &outputFile)
{
    if (!checkConnection())
        return false;

    QByteArray prjdata = UvscUtils::encodeProjectData({outputFile.toString()});
    const UVSC_STATUS st = ::UVSC_PRJ_SET_OUTPUTNAME(m_descriptor,
                                                     reinterpret_cast<PRJDATA *>(prjdata.data()),
                                                     prjdata.size());
    if (st != UVSC_STATUS_SUCCESS) {
        setError(ConfigurationError);
        return false;
    }
    return true;
}

void UvscClient::showWindow()
{
    ::UVSC_GEN_SHOW(m_descriptor);
}

void UvscClient::hideWindow()
{
    ::UVSC_GEN_HIDE(m_descriptor);
}

void UvscClient::setError(UvscError error, const QString &errorString)
{
    m_error = error;

    if (errorString.isEmpty()) {
        UV_OPERATION op = UV_NULL_CMD;
        UV_STATUS status = UV_STATUS_SUCCESS;
        QByteArray buffer(UVSC_MAX_API_STR_SIZE, 0);
        const UVSC_STATUS st = ::UVSC_GetLastError(m_descriptor, &op, &status,
                                                   reinterpret_cast<qint8 *>(buffer.data()),
                                                   buffer.size());
        m_errorString = (st == UVSC_STATUS_SUCCESS)
                ? QString::fromLocal8Bit(buffer) : tr("Unknown error.");
    } else {
        m_errorString = errorString;
    }

    m_errorString = m_errorString.trimmed();
    emit errorOccurred(m_error);
}

UvscClient::UvscError UvscClient::error() const
{
    return m_error;
}

QString UvscClient::errorString() const
{
    return m_errorString;
}

void UvscClient::customEvent(QEvent *event)
{
    const QEvent::Type et = event->type();
    if (et == kUvscMsgEventType)
        handleMsgEvent(event);
}

bool UvscClient::checkConnection()
{
    if (m_descriptor == -1) {
        setError(ConfigurationError, tr("Connection is not open."));
        return false;
    }
    return true;
}

bool UvscClient::enumerateStack(quint32 taskId, std::vector<STACKENUM> &stackenums)
{
    iSTKENUM istkenum = {};
    istkenum.task = taskId;
    istkenum.isFull = true;
    istkenum.hasExtended = true;
    stackenums.resize(kMaximumStackEnumsCount);
    qint32 stackenumsCount = qint32(stackenums.size());
    const UVSC_STATUS st = ::UVSC_DBG_ENUM_STACK(m_descriptor, &istkenum,
                                                 stackenums.data(), &stackenumsCount);
    if (st != UVSC_STATUS_SUCCESS)
        return false;
    stackenums.resize(stackenumsCount);
    return true;
}

void UvscClient::handleMsgEvent(QEvent *event)
{
    const auto me = static_cast<UvscMsgEvent *>(event);
    if (me->status != UV_STATUS_SUCCESS)
        return;
    switch (me->operation) {
    case UV_DBG_START_EXECUTION:
        emit executionStarted();
        break;
    case UV_DBG_STOP_EXECUTION:
        updateLocation(me->data);
        emit executionStopped();
        break;
    case UV_PRJ_CLOSE:
        emit projectClosed();
    default:
        break;
    }
}

void UvscClient::updateLocation(const QByteArray &bpreason)
{
    const auto bpr = reinterpret_cast<const BPREASON *>(bpreason.constData());
    quint64 address = bpr->reasonAddress;
    std::vector<STACKENUM> stackenums;
    enumerateStack(0, stackenums);
    if (stackenums.size() == 2) {
        m_exitAddress = stackenums.back().returnAddress;
    } else if (stackenums.size() == 1 && m_exitAddress != 0) {
        address = m_exitAddress;
        m_exitAddress = 0;
    }

    emit locationUpdated(address);
}

bool UvscClient::addressToFileLine(quint64 address, QString &fileName,
                                   QString &function, quint32 &line)
{
    if (!checkConnection())
        return false;

    ADRMTFL adrmtfl = {};
    adrmtfl.address = address;
    adrmtfl.isFullPath = true;
    QByteArray aflmap(kMaximumAflmapSize, 0);
    qint32 aflmapLength = aflmap.size();
    auto aflmapPtr = reinterpret_cast<AFLMAP *>(aflmap.data());
    const UVSC_STATUS st = ::UVSC_DBG_ADR_TOFILELINE(m_descriptor, &adrmtfl,
                                                     aflmapPtr, &aflmapLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    fileName = UvscUtils::decodeAscii(aflmapPtr->fileName + aflmapPtr->fileNameIndex);
    function = UvscUtils::decodeAscii(aflmapPtr->fileName + aflmapPtr->functionNameIndex);
    line = aflmapPtr->codeLineNumber;
    return true;
}

bool UvscClient::executeCommand(const QString &cmd, QString &output)
{
    if (!checkConnection())
        return false;

    EXECCMD exeCmd = UvscUtils::encodeCommand(cmd);
    UVSC_STATUS st = ::UVSC_DBG_EXEC_CMD(m_descriptor, &exeCmd, sizeof(exeCmd.command));
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    qint32 outputLength = 0;
    st = ::UVSC_GetCmdOutputSize(m_descriptor, &outputLength);
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    QByteArray data(outputLength, 0);
    st = UVSC_GetCmdOutput(m_descriptor, reinterpret_cast<qint8 *>(data.data()), data.size());
    if (st != UVSC_STATUS_SUCCESS) {
        setError(RuntimeError);
        return false;
    }

    // Note: UVSC API support only ASCII!
    output = QString::fromLatin1(data);
    return true;
}

} // namespace Internal
} // namespace Debugger
