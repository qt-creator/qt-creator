// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "peripheralregisterhandler.h"

#include "debuggercore.h"
#include "debuggertr.h"
#include "registerhandler.h"

#include <utils/basetreeview.h>

#include <QActionGroup>
#include <QFile>
#include <QItemDelegate>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QXmlStreamReader>

#ifdef WITH_TESTS
#include <QTemporaryFile>
#include <QTest>
#endif

#include <functional>
#include <numeric>

using namespace Utils;

namespace Debugger::Internal {

// Keys of a properties in SVD file.
constexpr char kAccess[] = "access";
constexpr char kAddressOffset[] = "addressOffset";
constexpr char kBaseAddress[] = "baseAddress";
constexpr char kBitOffset[] = "bitOffset";
constexpr char kBitRange[] = "bitRange";
constexpr char kBitWidth[] = "bitWidth";
constexpr char kCluster[] = "cluster";
constexpr char kDerivedFrom[] = "derivedFrom";
constexpr char kDescription[] = "description";
constexpr char kDevice[] = "device";
constexpr char kDim[] = "dim";
constexpr char kDimIncrement[] = "dimIncrement";
constexpr char kDimIndex[] = "dimIndex";
constexpr char kDisplayName[] = "displayName";
constexpr char kEnumeratedValue[] = "enumeratedValue";
constexpr char kEnumeratedValues[] = "enumeratedValues";
constexpr char kField[] = "field";
constexpr char kFields[] = "fields";
constexpr char kGroupName[] = "groupName";
constexpr char kLsb[] = "lsb";
constexpr char kMsb[] = "msb";
constexpr char kName[] = "name";
constexpr char kPeripheral[] = "peripheral";
constexpr char kPeripherals[] = "peripherals";
constexpr char kReadOnly[] = "read-only";
constexpr char kReadWrite[] = "read-write";
constexpr char kRegister[] = "register";
constexpr char kRegisters[] = "registers";
constexpr char kResetvalue[] = "resetValue";
constexpr char kSize[] = "size";
constexpr char kValue[] = "value";
constexpr char kWritOnlye[] = "write-only";

enum PeripheralRegisterColumns
{
    PeripheralRegisterNameColumn,
    PeripheralRegisterValueColumn,
    PeripheralRegisterAccessColumn,
    PeripheralRegisterColumnCount
};

enum PeripheralRegisterDataRole
{
    PeripheralRegisterChangedRole = Qt::UserRole
};

static QString accessName(PeripheralRegisterAccess access)
{
    switch (access) {
    case PeripheralRegisterAccess::ReadOnly:
        return Tr::tr("RO");
    case PeripheralRegisterAccess::WriteOnly:
        return Tr::tr("WO");
    case PeripheralRegisterAccess::ReadWrite:
        return Tr::tr("RW");
    default:
        return Tr::tr("N/A");
    }
}

static PeripheralRegisterAccess decodeAccess(const QString &accessText)
{
    if (accessText == QLatin1String(kReadWrite))
        return PeripheralRegisterAccess::ReadWrite;
    if (accessText == QLatin1String(kReadOnly))
        return PeripheralRegisterAccess::ReadOnly;
    if (accessText == QLatin1String(kWritOnlye))
        return PeripheralRegisterAccess::WriteOnly;
    return PeripheralRegisterAccess::Unknown;
}

static quint64 decodeNumeric(const QString &text, bool *ok = nullptr)
{
    bool parsed = false;
    if (!ok)
        ok = &parsed;
    if (text.startsWith('#'))
        return text.mid(1).toULongLong(ok, 2);
    if (text.startsWith("0b", Qt::CaseInsensitive))
        return text.mid(2).toULongLong(ok, 2);
    const quint64 result = text.toULongLong(ok, 10);
    if (*ok)
        return result;
    return text.toUInt(ok, 16);
}

static QStringList dimIndexes(const QString &dimIndex, int count)
{
    QStringList result;
    const QStringList range = dimIndex.split('-');
    if (range.size() == 2) {
        bool fromOk = false;
        bool toOk = false;
        const int from = range.at(0).toInt(&fromOk);
        const int to = range.at(1).toInt(&toOk);
        if (fromOk && toOk) {
            if (qint64(to) - from + 1 == count) {
                for (qint64 i = from; i <= to; ++i)
                    result.append(QString::number(i));
            }
        } else if (range.at(0).size() == 1 && range.at(1).size() == 1) {
            for (int c = range.at(0).at(0).unicode(); c <= range.at(1).at(0).unicode(); ++c)
                result.append(QChar(c));
        }
    } else if (!dimIndex.isEmpty()) {
        for (const QString &index : dimIndex.split(','))
            result.append(index.trimmed());
    }
    if (result.size() == count)
        return result;
    result.clear();
    for (int i = 0; i < count; ++i)
        result.append(QString::number(i));
    return result;
}

static QString instanceName(const QString &name, const QString &index)
{
    if (name.contains("%s") && !name.contains("[%s]"))
        return QString(name).replace("%s", index);
    return QStringLiteral("%1[%2]").arg(QString(name).remove("[%s]"), index);
}

static QString valueToString(quint64 value, int size, PeripheralRegisterFormat fmt)
{
    QString result;
    if (fmt == PeripheralRegisterFormat::Hexadecimal) {
        result = QString::number(value, 16);
        result.prepend("0x" + QString(size / 4 - result.size(), '0'));
    } else if (fmt == PeripheralRegisterFormat::Decimal) {
        result = QString::number(value, 10);
    } else if (fmt == PeripheralRegisterFormat::Octal) {
        result = QString::number(value, 8);
        result.prepend('0' + QString(size / 2 - result.size(), '0'));
    } else if (fmt == PeripheralRegisterFormat::Binary) {
        result = QString::number(value, 2);
        result.prepend("0b" + QString(size - result.size(), '0'));
    }
    return result;
}

static quint64 valueFromString(const QString &string, PeripheralRegisterFormat fmt,
                               bool *ok)
{
    if (fmt == PeripheralRegisterFormat::Hexadecimal) {
        static const QRegularExpression re("^(0x)?([0-9A-F]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(2).toULongLong(ok, 16);
    } else if (fmt == PeripheralRegisterFormat::Decimal) {
        static const QRegularExpression re("^([0-9]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(1).toULongLong(ok, 10);
    } else if (fmt == PeripheralRegisterFormat::Octal) {
        static const QRegularExpression re("^(0)?([0-7]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(2).toULongLong(ok, 8);
    } else if (fmt == PeripheralRegisterFormat::Binary) {
        static const QRegularExpression re("^(0b)?([0-1]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(2).toULongLong(ok, 2);
    }

    *ok = false;
    return 0;
}

// PeripheralFieldEnum

std::pair<QString, QString> PeripheralFieldEnum::enumerant(quint64 fieldValue) const
{
    for (auto &enumerant : enumerants) {
        if (enumerant.value == fieldValue) {
            return std::make_pair(enumerant.name, enumerant.description);
        }
    }

    return {};
}

// PeripheralRegisterField

QString PeripheralRegisterField::bitRangeString() const
{
    const int from = bitOffset;
    const int to = bitOffset + bitWidth - 1;
    return Tr::tr("[%1..%2]").arg(to).arg(from);
}

QString PeripheralRegisterField::bitValueString(quint64 regValue, PeripheralFieldFormat fieldFormat) const
{
    const quint64 value = bitValue(regValue);

    auto stringValue = valueToString(value, bitWidth, format);

    if (fieldEnum.isValid()) {
        const auto [enumValue, description] = fieldEnum.enumerant(value);
        if (!enumValue.isEmpty()) {
            stringValue += ": " + enumValue;
            if (fieldFormat == PeripheralFieldFormat::ToolTip) {
                if (!description.isEmpty()) {
                    stringValue += "\n" + description;
                }
            }
        }
    }

    return stringValue;
}

quint64 PeripheralRegisterField::bitValue(quint64 regValue) const
{
    const quint64 mask = bitMask();
    regValue &= mask;
    regValue >>= bitOffset;
    return regValue;
}

quint64 PeripheralRegisterField::bitMask() const
{
    quint64 mask = 0;
    for (auto pos = bitOffset; pos < bitOffset + bitWidth; ++pos)
        mask |= quint64(quint64(1) << pos);
    return mask;
}

// PeripheralRegisterValue

bool PeripheralRegisterValue::fromString(const QString &string,
                                         PeripheralRegisterFormat fmt)
{
    bool ok = false;
    const quint64 newVal = valueFromString(string, fmt, &ok);
    if (!ok)
        return false;
    v = newVal;
    return true;
}

QString PeripheralRegisterValue::toString(
        int size, PeripheralRegisterFormat fmt) const
{
    return valueToString(v, size, fmt);
}

// PeripheralRegister

QString PeripheralRegister::currentValueString() const
{
    return currentValue.toString(size, format);
}

QString PeripheralRegister::previousValueString() const
{
    return previousValue.toString(size, format);
}

QString PeripheralRegister::resetValueString() const
{
    return resetValue.toString(size, format);
}

QString PeripheralRegister::addressString(quint64 baseAddress) const
{
    return "0x" + QString::number(address(baseAddress), 16);
}

quint64 PeripheralRegister::address(quint64 baseAddress) const
{
    return baseAddress + addressOffset;
}

// PeripheralRegisterFieldItem

class PeripheralRegisterFieldItem final
        : public TypedTreeItem<PeripheralRegisterFieldItem>
{
public:
    explicit PeripheralRegisterFieldItem(
            DebuggerEngine *engine, const PeripheralRegisterGroup &group,
            PeripheralRegister &reg, PeripheralRegisterField &fld);

    QVariant data(int column, int role) const final;
    bool setData(int column, const QVariant &value, int role) final;
    Qt::ItemFlags flags(int column) const final;

    void triggerChange();

    DebuggerEngine *m_engine = nullptr;
    const PeripheralRegisterGroup &m_group;
    PeripheralRegister &m_reg;
    PeripheralRegisterField &m_fld;
};

PeripheralRegisterFieldItem::PeripheralRegisterFieldItem(
        DebuggerEngine *engine, const PeripheralRegisterGroup &group,
        PeripheralRegister &reg, PeripheralRegisterField &fld)
    : m_engine(engine), m_group(group), m_reg(reg), m_fld(fld)
{
}

QVariant PeripheralRegisterFieldItem::data(int column, int role) const
{
    switch (role) {
    case PeripheralRegisterChangedRole:
        return m_fld.bitValue(m_reg.currentValue.v)
                != m_fld.bitValue(m_reg.previousValue.v);

    case Qt::DisplayRole:
        switch (column) {
        case PeripheralRegisterNameColumn:
            return m_fld.name;
        case PeripheralRegisterValueColumn:
            return m_fld.bitValueString(m_reg.currentValue.v);
        case PeripheralRegisterAccessColumn:
            return accessName(m_fld.access);
        }
        break;

    case Qt::ToolTipRole:
        switch (column) {
        case PeripheralRegisterNameColumn:
            return QString("%1.%2\n%3\nBits: %4, %5")
                    .arg(m_reg.name)
                    .arg(m_fld.name)
                    .arg(m_fld.description)
                    .arg(m_fld.bitRangeString())
                    .arg(m_fld.bitWidth);
        case PeripheralRegisterValueColumn:
            return QString("Value: %1").arg(
                        m_fld.bitValueString(m_reg.currentValue.v, PeripheralFieldFormat::ToolTip));
        }
        break;

    case Qt::EditRole:
        return valueToString(m_fld.bitValue(m_reg.currentValue.v), m_fld.bitWidth, m_fld.format);

    case Qt::TextAlignmentRole: {
        if (column == PeripheralRegisterValueColumn)
            return Qt::AlignRight;
        return {};
    }

    default:
        break;
    }
    return {};
}

bool PeripheralRegisterFieldItem::setData(
        int column, const QVariant &value, int role)
{
    if (column == PeripheralRegisterValueColumn && role == Qt::EditRole) {
        bool ok = false;
        quint64 bitValue = valueFromString(value.toString(), m_fld.format, &ok);
        if (!ok)
            return false;
        bitValue <<= m_fld.bitOffset;
        const quint64 mask = m_fld.bitMask();
        m_reg.currentValue.v &= ~mask;
        m_reg.currentValue.v |= bitValue;
        triggerChange();
        return true;
    }
    return false;
}

Qt::ItemFlags PeripheralRegisterFieldItem::flags(int column) const
{
    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (column != PeripheralRegisterValueColumn)
        return notEditable;
    if (m_fld.access == PeripheralRegisterAccess::ReadWrite
            || m_fld.access == PeripheralRegisterAccess::WriteOnly) {
        return notEditable | Qt::ItemIsEditable;
    }
    return notEditable;
}

void PeripheralRegisterFieldItem::triggerChange()
{
    m_engine->setPeripheralRegisterValue(m_reg.address(m_group.baseAddress()),
                                         m_reg.currentValue.v);
}

// PeripheralRegisterItem

class PeripheralRegisterItem final
        : public TypedTreeItem<PeripheralRegisterItem>
{
public:
    explicit PeripheralRegisterItem(
            DebuggerEngine *engine,
            const PeripheralRegisterGroup &group,
            PeripheralRegister &reg);

    QVariant data(int column, int role) const final;
    bool setData(int column, const QVariant &value, int role) final;
    Qt::ItemFlags flags(int column) const final;

    void triggerChange();

    DebuggerEngine *m_engine = nullptr;
    const PeripheralRegisterGroup &m_group;
    PeripheralRegister &m_reg;
};

PeripheralRegisterItem::PeripheralRegisterItem(
        DebuggerEngine *engine,
        const PeripheralRegisterGroup &group,
        PeripheralRegister &reg)
    : m_engine(engine), m_group(group), m_reg(reg)
{
    for (auto &fld : m_reg.fields) {
        const auto item = new PeripheralRegisterFieldItem(
                    m_engine, m_group, m_reg, fld);
        appendChild(item);
    }
}

QVariant PeripheralRegisterItem::data(int column, int role) const
{
    switch (role) {
    case PeripheralRegisterChangedRole:
        return m_reg.currentValue != m_reg.previousValue;

    case Qt::DisplayRole:
        switch (column) {
        case PeripheralRegisterNameColumn:
            return m_reg.name;
        case PeripheralRegisterValueColumn:
            return m_reg.currentValueString();
        case PeripheralRegisterAccessColumn:
            return accessName(m_reg.access);
        }
        break;

    case Qt::ToolTipRole:
        switch (column) {
        case PeripheralRegisterNameColumn:
            return QStringLiteral("%1 / %2\n%3\n%4 @ %5, %6")
                    .arg(m_group.name)
                    .arg(m_reg.name)
                    .arg(m_reg.description)
                    .arg(accessName(m_reg.access))
                    .arg(m_reg.addressString(m_group.baseAddress()))
                    .arg(m_reg.size);
        case PeripheralRegisterValueColumn:
            return QStringLiteral("Current value: %1\n"
                                  "Previous value: %2\n"
                                  "Reset value: %3")
                    .arg(m_reg.currentValueString(),
                         m_reg.previousValueString(),
                         m_reg.resetValueString());
        }
        break;

    case Qt::EditRole:
        return m_reg.currentValueString();

    case Qt::TextAlignmentRole: {
        if (column == PeripheralRegisterValueColumn)
            return Qt::AlignRight;
        return {};
    }

    default:
        break;
    }
    return {};
}

bool PeripheralRegisterItem::setData(int column, const QVariant &value, int role)
{
    if (column == PeripheralRegisterValueColumn && role == Qt::EditRole) {
        if (m_reg.currentValue.fromString(value.toString(), m_reg.format)) {
            triggerChange();
            return true;
        }
    }
    return false;
}

Qt::ItemFlags PeripheralRegisterItem::flags(int column) const
{
    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (column != PeripheralRegisterValueColumn)
        return notEditable;
    if (m_reg.access == PeripheralRegisterAccess::ReadWrite
            || m_reg.access == PeripheralRegisterAccess::WriteOnly) {
        return notEditable | Qt::ItemIsEditable;
    }
    return notEditable;
}

void PeripheralRegisterItem::triggerChange()
{
    m_engine->setPeripheralRegisterValue(m_reg.address(m_group.baseAddress()),
                                         m_reg.currentValue.v);
}

// PeripheralRegisterHandler

PeripheralRegisterHandler::PeripheralRegisterHandler(DebuggerEngine *engine)
    : m_engine(engine)
{
    setObjectName("PeripheralRegisterModel");
    setHeader({Tr::tr("Name"), Tr::tr("Value"), Tr::tr("Access")});
}

static void handleField(QXmlStreamReader &in, PeripheralRegister &reg)
{
    PeripheralRegisterField fld;
    std::optional<int> from;
    std::optional<int> to;
    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == QLatin1String(kName)) {
            fld.name = in.readElementText();
        } else if (elementName == QLatin1String(kDescription)) {
            fld.description = in.readElementText();
        } else if (elementName == QLatin1String(kAccess)) {
            fld.access = decodeAccess(in.readElementText());
        } else if (elementName == QLatin1String(kBitRange)) {
            const QString elementText = in.readElementText();
            const int startBracket = elementText.indexOf('[');
            const int endBracket = elementText.indexOf(']');
            if (startBracket == -1 || endBracket == -1 || (endBracket - startBracket) <= 0)
                continue;
            const QString range = elementText.mid(startBracket + 1, endBracket - 1);
            const QStringList items = range.split(':');
            enum { MaxBit, MinBit, BitsCount };
            if (items.count() != BitsCount)
                continue;
            const int from = int(decodeNumeric(items.at(MinBit)));
            const int to = int(decodeNumeric(items.at(MaxBit)));
            fld.bitOffset = from;
            fld.bitWidth = to - from + 1;
        } else if (elementName == QLatin1String(kBitOffset)) {
            fld.bitOffset = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kBitWidth)) {
            fld.bitWidth = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kLsb)) {
            from = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kMsb)) {
            to = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kEnumeratedValues)) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == QLatin1String(kName)) {
                    fld.fieldEnum.name = in.readElementText();
                } else if (elementName == QLatin1String(kEnumeratedValue)) {
                    PeripheralFieldEnumerant enumerant;
                    bool valueOk = false;
                    while (in.readNextStartElement()) {
                        const auto elementName = in.name();
                        if (elementName == QLatin1String(kName)) {
                            enumerant.name = in.readElementText();
                        } else if (elementName == QLatin1String(kDescription)) {
                            enumerant.description = in.readElementText();
                        } else if (elementName == QLatin1String(kValue)) {
                            enumerant.value = decodeNumeric(in.readElementText(), &valueOk);
                        } else {
                            in.skipCurrentElement();
                        }
                    }
                    if (valueOk)
                        fld.fieldEnum.enumerants.push_back(enumerant);
                } else {
                    in.skipCurrentElement();
                }
            }
        } else {
            in.skipCurrentElement();
        }
    }

    if (from && to) {
        fld.bitOffset = *from;
        fld.bitWidth = *to - *from + 1;
    }

    // Inherit the field access from the register access if the filed
    // has not the access rights description.
    if (fld.access == PeripheralRegisterAccess::Unknown)
        fld.access = reg.access;

    reg.fields.push_back(fld);
}

static void handleRegister(QXmlStreamReader &in, PeripheralRegisterGroup &group)
{
    PeripheralRegister reg;
    QString dimIndex;
    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == QLatin1String(kName)) {
            reg.name = in.readElementText();
        } else if (elementName == QLatin1String(kDisplayName)) {
            reg.displayName = in.readElementText();
        } else if (elementName == QLatin1String(kDescription)) {
            reg.description = in.readElementText();
        } else if (elementName == QLatin1String(kDim)) {
            reg.dimensions = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kDimIncrement)) {
            reg.dimensionIncrement = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kDimIndex)) {
            dimIndex = in.readElementText();
        } else if (elementName == QLatin1String(kAddressOffset)) {
            reg.addressOffset = decodeNumeric(in.readElementText());
        } else if (elementName == QLatin1String(kSize)) {
            reg.size = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kAccess)) {
            reg.access = decodeAccess(in.readElementText());
        } else if (elementName == QLatin1String(kResetvalue)) {
            reg.resetValue = decodeNumeric(in.readElementText());
        } else if (elementName == QLatin1String(kFields)) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == QLatin1String(kField))
                    handleField(in, reg);
                else
                    in.skipCurrentElement();
            }
        } else {
            in.skipCurrentElement();
        }
    }

    if (reg.dimensions == 0) {
        group.registers.push_back(reg);
        return;
    }

    const QStringList indexes = dimIndexes(dimIndex, reg.instances());

    auto addressOffset = reg.addressOffset;

    for (auto instanceId = 0; instanceId < reg.instances(); instanceId++) {
        PeripheralRegister instance;

        instance.name = instanceName(reg.name, indexes.at(instanceId));
        instance.displayName = reg.displayName;
        instance.description = reg.description;
        instance.dimensions = 0;
        instance.dimensionIncrement = 0;
        instance.addressOffset = addressOffset;
        instance.size = reg.size;
        instance.access = reg.access;
        instance.resetValue = reg.resetValue;
        instance.fields = reg.fields;

        group.registers.push_back(instance);

        addressOffset += reg.dimensionIncrement;
    }
}

static bool findGroup(QStringView groupName, const PeripheralRegisterGroups &groups, PeripheralRegisterGroup *&group)
{
    if (!groupName.isEmpty())
    {
        const auto groupEnd = groups.cend();
        const auto groupIt = std::find_if(groups.cbegin(), groupEnd,
                                          [groupName](const auto &group) {
            return groupName == group->name;
        });
        if (groupIt != groupEnd) {
            group = groupIt->get();
            return true;
        }
    }

    return false;
}

static void handleCluster(QXmlStreamReader &in, PeripheralRegisterGroup *parentGroup)
{
    auto cluster = std::make_unique<PeripheralRegisterGroup>();
    cluster->parentGroup = parentGroup;
    cluster->name = parentGroup->name;
    cluster->description = parentGroup->description;
    QString dimIndex;

    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == QLatin1String(kName)) {
            cluster->name = in.readElementText();
        } else if (elementName == QLatin1String(kDescription)) {
            cluster->description = in.readElementText();
        } else if (elementName == QLatin1String(kDim)) {
            cluster->dimensions = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kDimIncrement)) {
            cluster->dimensionIncrement = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kDimIndex)) {
            dimIndex = in.readElementText();
        } else if (elementName == QLatin1String(kAddressOffset)) {
            cluster->addressOffset = decodeNumeric(in.readElementText());
        } else if (elementName == QLatin1String(kRegister)) {
            handleRegister(in, *cluster);
        } else if (elementName == QLatin1String(kCluster)) {
            handleCluster(in, cluster.get());
        } else {
            in.skipCurrentElement();
        }
    }

    if (cluster->dimensions == 0) {
        parentGroup->clusters.push_back(std::move(cluster));
        return;
    }

    // Explode the cluster, creating an array of clones

    const QStringList indexes = dimIndexes(dimIndex, cluster->dimensions);

    quint64 addressOffset = cluster->addressOffset;

    for (auto instanceId = 0; instanceId < cluster->dimensions; instanceId++) {
        auto instance = std::make_unique<PeripheralRegisterGroup>(*cluster.get());

        instance->name = instanceName(cluster->name, indexes.at(instanceId));
        instance->dimensions = 0;
        instance->dimensionIncrement = 0;
        instance->addressOffset = addressOffset;

        addressOffset += cluster->dimensionIncrement;

        parentGroup->clusters.push_back(std::move(instance));
    }
}

static void handleGroup(QXmlStreamReader &in, PeripheralRegisterGroups &groups)
{
    PeripheralRegisterGroup *derivedFrom = nullptr;

    const auto fromGroupName = in.attributes().value(kDerivedFrom);

    auto group = findGroup(fromGroupName, groups, derivedFrom)
            ? std::make_unique<PeripheralRegisterGroup>(*derivedFrom)
            : std::make_unique<PeripheralRegisterGroup>();

    group->parentGroup = nullptr;

    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == QLatin1String(kName)) {
            group->name = in.readElementText();
        } else if (elementName == QLatin1String(kDescription)) {
            group->description = in.readElementText();
        } else if (elementName == QLatin1String(kGroupName)) {
            group->displayName = in.readElementText();
        } else if (elementName == QLatin1String(kBaseAddress)) {
            group->setBaseAddress(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kSize)) {
            group->size = int(decodeNumeric(in.readElementText()));
        } else if (elementName == QLatin1String(kAccess)) {
            group->access = decodeAccess(in.readElementText());
        } else if (elementName == QLatin1String(kRegisters)) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == QLatin1String(kRegister)) {
                    handleRegister(in, *group);
                } else if (elementName == QLatin1String(kCluster)) {
                    handleCluster(in, group.get());
                } else {
                    in.skipCurrentElement();
                }
            }
        } else {
            in.skipCurrentElement();
        }
    }

    groups.push_back(std::move(group));
}

static PeripheralRegisterGroups availablePeripheralRegisterGroups(const FilePath &filePath)
{
    QFile f(filePath.toFSPathString());
    if (!f.open(QIODevice::ReadOnly))
        return {};

    QXmlStreamReader in(&f);
    PeripheralRegisterGroups groups;
    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == QLatin1String(kDevice)) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == QLatin1String(kPeripherals)) {
                    while (in.readNextStartElement()) {
                        const auto elementName = in.name();
                        if (elementName == QLatin1String(kPeripheral))
                            handleGroup(in, groups);
                        else
                            in.skipCurrentElement();
                    }
                } else {
                    in.skipCurrentElement();
                }
            }
        }
    }
    return groups;
}

void PeripheralRegisterHandler::updateRegisterGroups()
{
    clear();

    const DebuggerRunParameters &rp = m_engine->runParameters();
    if (!rp.peripheralDescriptionFile().exists())
        return;
    m_peripheralRegisterGroups = availablePeripheralRegisterGroups(rp.peripheralDescriptionFile());
}

void PeripheralRegisterHandler::updateRegister(quint64 address, quint64 value)
{
    const auto regItem = m_activeRegisters.value(address);
    if (!regItem)
        return;

    regItem->m_reg.previousValue = regItem->m_reg.currentValue;
    regItem->m_reg.currentValue = value;

    commitUpdates();
}

QList<quint64> PeripheralRegisterHandler::activeRegisters() const
{
    return m_activeRegisters.keys();
}

QVariant PeripheralRegisterHandler::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ItemDelegateRole) {
        return QVariant::fromValue(createRegisterDelegate(PeripheralRegisterValueColumn));
    }
    return PeripheralRegisterModel::data(idx, role);
}

bool PeripheralRegisterHandler::setData(const QModelIndex &idx,
                                        const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemViewEventRole) {
        const auto ev = data.value<ItemViewEvent>();
        if (ev.type() == QEvent::ContextMenu)
            return contextMenuEvent(ev);
    }
    return PeripheralRegisterModel::setData(idx, data, role);
}

bool PeripheralRegisterHandler::contextMenuEvent(const ItemViewEvent &ev)
{
    const DebuggerState state = m_engine->state();
    const auto menu = new QMenu;

    QMenu *groupMenu = createRegisterGroupsMenu(state);
    menu->addMenu(groupMenu);

    if (const auto regItem = itemForIndexAtLevel<PeripheralRegisterLevel>(
                ev.sourceModelIndex())) {
        // Show the register value format menu only
        // if a register item chose.
        QMenu *fmtMenu = createRegisterFormatMenu(state, regItem);
        menu->addMenu(fmtMenu);
    } else if (const auto fldItem = itemForIndexAtLevel<PeripheralRegisterFieldLevel>(
                   ev.sourceModelIndex())) {
        // Show the register field value format menu only
        // if a register field item chose.
        QMenu *fmtMenu = createRegisterFieldFormatMenu(state, fldItem);
        menu->addMenu(fmtMenu);
    }

    addStandardActions(ev.view(), menu);

    menu->setAttribute(Qt::WA_DeleteOnClose);
    menu->popup(ev.globalPos());
    return true;
}

QMenu *PeripheralRegisterHandler::createRegisterGroupsMenu(DebuggerState state)
{
    const auto groupMenu = new QMenu(Tr::tr("View Groups"));
    const auto actionGroup = new QActionGroup(groupMenu);
    const bool on = m_engine->hasCapability(RegisterCapability)
            && (state == InferiorStopOk || state == InferiorUnrunnable);
    const auto addGroupAction = [this, on](QMenu *menu, QActionGroup *actions,
                                           const QString &text, bool active,
                                           const QString &groupName,
                                           const QStringList &clusterPath) {
        QAction *act = menu->addAction(text);
        act->setEnabled(on);
        act->setCheckable(true);
        act->setChecked(active);
        actions->addAction(act);
        QObject::connect(act, &QAction::triggered, this,
                         [this, groupName, clusterPath](bool checked) {
            if (checked)
                setActiveGroup(groupName, clusterPath);
        });
    };
    const std::function<void(QMenu *, QActionGroup *, const PeripheralRegisterGroup &,
                             const QString &, const QStringList &)> addClusters
        = [&](QMenu *menu, QActionGroup *actions, const PeripheralRegisterGroup &parent,
              const QString &groupName, const QStringList &parentPath) {
        for (const auto &cluster : parent.clusters) {
            const QString text = QStringLiteral("%1: %2").arg(cluster->name, cluster->description);
            const QStringList clusterPath = parentPath + QStringList(cluster->name);
            if (cluster->clusters.empty()) {
                addGroupAction(menu, actions, text, cluster->active, groupName, clusterPath);
                continue;
            }
            const auto subMenu = menu->addMenu(text);
            subMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");
            if (!cluster->registers.empty()) {
                addGroupAction(subMenu, actions, Tr::tr("%1 registers").arg(cluster->name),
                               cluster->active, groupName, clusterPath);
                subMenu->addSeparator();
            }
            addClusters(subMenu, actions, *cluster, groupName, clusterPath);
        }
    };
    bool hasActions = false;
    for (const auto &group : std::as_const(m_peripheralRegisterGroups)) {
        const QString groupText = QStringLiteral("%1: %2").arg(group->name, group->description);
        hasActions = true;
        if (group->clusters.empty()) {
            addGroupAction(groupMenu, actionGroup, groupText, group->active, group->name, {});
            continue;
        }
        const auto clusterMenu = groupMenu->addMenu(groupText);
        clusterMenu->setEnabled(true);
        clusterMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");
        if (!group->registers.empty()) {
            addGroupAction(clusterMenu, actionGroup, Tr::tr("%1 registers").arg(group->name),
                           group->active, group->name, {});
            clusterMenu->addSeparator();
        }
        addClusters(clusterMenu, new QActionGroup(clusterMenu), *group, group->name, {});
    }
    groupMenu->setEnabled(hasActions);
    groupMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");
    return groupMenu;
}

QMenu *PeripheralRegisterHandler::createRegisterFormatMenu(
        DebuggerState state, PeripheralRegisterItem *item) const
{
    const auto fmtMenu = new QMenu(Tr::tr("Format"));
    const auto actionGroup = new QActionGroup(fmtMenu);

    const bool on = m_engine->hasCapability(RegisterCapability)
            && (state == InferiorStopOk || state == InferiorUnrunnable);

    const PeripheralRegisterFormat fmt = item->m_reg.format;

    // Hexadecimal action.
    const auto hexAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Hexadecimal"), on,
                fmt == PeripheralRegisterFormat::Hexadecimal,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Hexadecimal;
        item->update();
    });
    actionGroup->addAction(hexAct);

    // Decimal action.
    const auto decAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Decimal"), on,
                fmt == PeripheralRegisterFormat::Decimal,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Decimal;
        item->update();
    });
    actionGroup->addAction(decAct);

    // Octal action.
    const auto octAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Octal"), on,
                fmt == PeripheralRegisterFormat::Octal,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Octal;
        item->update();
    });
    actionGroup->addAction(octAct);

    // Binary action.
    const auto binAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Binary"), on,
                fmt == PeripheralRegisterFormat::Binary,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Binary;
        item->update();
    });
    actionGroup->addAction(binAct);

    return fmtMenu;
}

QMenu *PeripheralRegisterHandler::createRegisterFieldFormatMenu(
        DebuggerState state, PeripheralRegisterFieldItem *item) const
{
    const auto fmtMenu = new QMenu(Tr::tr("Format"));
    const auto actionGroup = new QActionGroup(fmtMenu);

    const bool on = m_engine->hasCapability(RegisterCapability)
            && (state == InferiorStopOk || state == InferiorUnrunnable);

    const PeripheralRegisterFormat fmt = item->m_fld.format;

    // Hexadecimal action.
    const auto hexAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Hexadecimal"), on,
                fmt == PeripheralRegisterFormat::Hexadecimal,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Hexadecimal;
        item->update();
    });
    actionGroup->addAction(hexAct);

    // Decimal action.
    const auto decAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Decimal"), on,
                fmt == PeripheralRegisterFormat::Decimal,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Decimal;
        item->update();
    });
    actionGroup->addAction(decAct);

    // Octal action.
    const auto octAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Octal"), on,
                fmt == PeripheralRegisterFormat::Octal,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Octal;
        item->update();
    });
    actionGroup->addAction(octAct);

    // Binary action.
    const auto binAct = addCheckableAction(
                this, fmtMenu, Tr::tr("Binary"), on,
                fmt == PeripheralRegisterFormat::Binary,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Binary;
        item->update();
    });
    actionGroup->addAction(binAct);

    return fmtMenu;
}

void PeripheralRegisterHandler::setActiveGroup(const QString &groupName,
                                               const QStringList &clusterPath)
{
    deactivateGroups();
    auto groupEnd = m_peripheralRegisterGroups.end();
    auto groupIt = std::find_if(
                m_peripheralRegisterGroups.begin(), groupEnd,
                [groupName](const auto &group){
        return group->name == groupName;
    });
    if (groupIt == groupEnd)
        return; // Group not found.

    auto group = groupIt->get();
    for (const QString &clusterName : clusterPath) {
        groupEnd = group->clusters.end();
        groupIt = std::find_if(
                    group->clusters.begin(), groupEnd,
                    [clusterName](const auto &cluster){
            return cluster->name == clusterName;
        });
        if (groupIt == groupEnd)
            return; // Group not found.
        group = groupIt->get();
    }

    // Set active group.
    group->active = true;

    // Add all register items of active register group.
    m_activeRegisters.reserve(group->registers.count());
    for (PeripheralRegister &reg : group->registers) {
        const auto item = new PeripheralRegisterItem(m_engine, *group, reg);
        rootItem()->appendChild(item);

        const quint64 address = reg.address(group->baseAddress());
        m_activeRegisters.insert(address, item);
    }

    m_engine->reloadPeripheralRegisters();
}

static void deactivateGroup(PeripheralRegisterGroup &group)
{
    group.active = false;
    for (auto &cluster : group.clusters)
        deactivateGroup(*cluster);
}

void PeripheralRegisterHandler::deactivateGroups()
{
    clear();

    for (auto &group : m_peripheralRegisterGroups)
        deactivateGroup(*group);

    m_activeRegisters.clear();
}

quint64 PeripheralRegisterGroup::baseAddress() const
{
    auto address = parentGroup && parentGroup->baseAddress() > 0
        ? parentGroup->baseAddress()
        : m_baseAddress;

    return address + addressOffset;
}

void PeripheralRegisterGroup::setBaseAddress(quint64 address)
{
    m_baseAddress = address;
}

PeripheralRegisterGroup::PeripheralRegisterGroup(const PeripheralRegisterGroup &other)
    : name(other.name),
      displayName(other.displayName),
      description(other.description),
      addressOffset(other.addressOffset),
      dimensions(other.dimensions),
      dimensionIncrement(other.dimensionIncrement),
      size(other.size),
      access(other.access),
      active(other.active),
      parentGroup(other.parentGroup),
      registers(other.registers),
      m_baseAddress(other.m_baseAddress)
{
    for (auto &cluster : other.clusters) {
        clusters.push_back(std::make_unique<PeripheralRegisterGroup>(*cluster));
        clusters.back()->parentGroup = this;
    }
}

#ifdef WITH_TESTS

class PeripheralRegisterTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testDimIndex();
    void testEnumeratedValues();
    void testFieldEditValue();
    void testClusterArray();
    void testNestedCluster();
    void testDerivedFrom();

private:
    PeripheralRegisterGroups m_groups;
};

void PeripheralRegisterTest::initTestCase()
{
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write(R"(<device><peripherals><peripheral>
<name>P</name><baseAddress>0x40000000</baseAddress>
<registers>
  <register><name>GPIO%s</name><addressOffset>0x10</addressOffset><size>32</size>
    <dim>3</dim><dimIncrement>4</dimIncrement><dimIndex>A,B,C</dimIndex></register>
  <register><name>R</name><addressOffset>0</addressOffset><size>32</size>
    <access>read-write</access>
    <fields><field><name>F</name><bitOffset>0</bitOffset><bitWidth>2</bitWidth>
      <enumeratedValues>
        <enumeratedValue><name>ONE</name><value>#01</value></enumeratedValue>
        <enumeratedValue><name>TWO</name><value>0b10</value></enumeratedValue>
        <enumeratedValue><name>ANY</name><value>#1x</value></enumeratedValue>
      </enumeratedValues>
    </field></fields>
  </register>
  <register><name>DATA[%s]</name><addressOffset>0x20</addressOffset><size>32</size>
    <dim>1</dim><dimIncrement>4</dimIncrement></register>
  <register><name>BIG%s</name><addressOffset>0x30</addressOffset><size>32</size>
    <dim>2</dim><dimIncrement>4</dimIncrement><dimIndex>0-2147483647</dimIndex></register>
  <register><name>TOP%s</name><addressOffset>0x40</addressOffset><size>32</size>
    <dim>2</dim><dimIncrement>4</dimIncrement><dimIndex>2147483646-2147483647</dimIndex></register>
  <cluster><name>CH[%s]</name><addressOffset>0x100</addressOffset>
    <dim>2</dim><dimIncrement>0x20</dimIncrement>
    <register><name>CTRL</name><addressOffset>0x4</addressOffset><size>32</size></register>
    <cluster><name>CC[%s]</name><addressOffset>0x10</addressOffset>
      <dim>2</dim><dimIncrement>4</dimIncrement>
      <register><name>VAL</name><addressOffset>0</addressOffset><size>32</size></register>
    </cluster>
  </cluster>
</registers>
</peripheral>
<peripheral derivedFrom="P"><name>Q</name><baseAddress>0x50000000</baseAddress></peripheral>
</peripherals></device>)");
    file.close();

    m_groups = availablePeripheralRegisterGroups(FilePath::fromString(file.fileName()));
    QCOMPARE(m_groups.size(), size_t(2));
    QCOMPARE(m_groups.front()->registers.size(), qsizetype(9));
}

void PeripheralRegisterTest::testDimIndex()
{
    const PeripheralRegisterGroup &group = *m_groups.front();
    QCOMPARE(group.registers.at(0).name, QString("GPIOA"));
    QCOMPARE(group.registers.at(2).name, QString("GPIOC"));
    QCOMPARE(group.registers.at(2).address(group.baseAddress()), quint64(0x40000018));
    QCOMPARE(group.registers.at(4).name, QString("DATA[0]"));
    QCOMPARE(group.registers.at(6).name, QString("BIG1"));
    QCOMPARE(group.registers.at(7).name, QString("TOP2147483646"));
    QCOMPARE(group.registers.at(8).name, QString("TOP2147483647"));
}

void PeripheralRegisterTest::testEnumeratedValues()
{
    const PeripheralFieldEnum &fieldEnum = m_groups.front()->registers.at(3).fields.at(0).fieldEnum;
    QCOMPARE(fieldEnum.enumerants.size(), qsizetype(2));
    QCOMPARE(fieldEnum.enumerant(1).first, QString("ONE"));
    QCOMPARE(fieldEnum.enumerant(2).first, QString("TWO"));
}

void PeripheralRegisterTest::testFieldEditValue()
{
    const PeripheralRegisterGroup &group = *m_groups.front();
    PeripheralRegister reg = group.registers.at(3);
    PeripheralRegisterField &fld = reg.fields[0];
    fld.fieldEnum.enumerants = {{"ONE", {}, 1}};
    reg.currentValue = 1;
    const PeripheralRegisterFieldItem item(nullptr, group, reg, fld);
    QCOMPARE(item.data(PeripheralRegisterValueColumn, Qt::DisplayRole).toString(),
             QString("0x1: ONE"));
    QCOMPARE(item.data(PeripheralRegisterValueColumn, Qt::EditRole).toString(), QString("0x1"));
}

void PeripheralRegisterTest::testClusterArray()
{
    const PeripheralRegisterGroup &group = *m_groups.front();
    QCOMPARE(group.clusters.size(), size_t(2));
    const PeripheralRegisterGroup &ch1 = *group.clusters.at(1);
    QCOMPARE(ch1.name, QString("CH[1]"));
    QCOMPARE(ch1.registers.at(0).address(ch1.baseAddress()), quint64(0x40000124));
}

void PeripheralRegisterTest::testNestedCluster()
{
    const PeripheralRegisterGroup &ch1 = *m_groups.front()->clusters.at(1);
    QCOMPARE(ch1.clusters.size(), size_t(2));
    const PeripheralRegisterGroup &cc1 = *ch1.clusters.at(1);
    QCOMPARE(cc1.name, QString("CC[1]"));
    QCOMPARE(cc1.parentGroup, &ch1);
    QCOMPARE(cc1.registers.at(0).address(cc1.baseAddress()), quint64(0x40000134));
}

void PeripheralRegisterTest::testDerivedFrom()
{
    const PeripheralRegisterGroup &q = *m_groups.at(1);
    QCOMPARE(q.name, QString("Q"));
    const PeripheralRegisterGroup &qch1 = *q.clusters.at(1);
    QCOMPARE(qch1.parentGroup, &q);
    QCOMPARE(qch1.registers.at(0).address(qch1.baseAddress()), quint64(0x50000124));
    const PeripheralRegisterGroup &qcc1 = *qch1.clusters.at(1);
    QCOMPARE(qcc1.parentGroup, &qch1);
    QCOMPARE(qcc1.registers.at(0).address(qcc1.baseAddress()), quint64(0x50000134));
}

QObject *createPeripheralRegisterTest()
{
    return new PeripheralRegisterTest;
}

#endif // WITH_TESTS

} // namespace Debugger::Internal

#ifdef WITH_TESTS
#include "peripheralregisterhandler.moc"
#endif
