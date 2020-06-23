/****************************************************************************
**
** Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "peripheralregisterhandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"

#include <utils/basetreeview.h>
#include <utils/savedaction.h>

#include <QActionGroup>
#include <QFile>
#include <QItemDelegate>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>

using namespace Utils;

namespace Debugger {
namespace Internal {

namespace {
// Keys of a properties in SVD file.
constexpr char kAccess[] = "access";
constexpr char kAddressOffset[] = "addressOffset";
constexpr char kBaseAddress[] = "baseAddress";
constexpr char kBitOffset[] = "bitOffset";
constexpr char kBitRange[] = "bitRange";
constexpr char kBitWidth[] = "bitWidth";
constexpr char kDerivedFrom[] = "derivedFrom";
constexpr char kDescription[] = "description";
constexpr char kDevice[] = "device";
constexpr char kDisplayName[] = "displayName";
constexpr char kField[] = "field";
constexpr char kFields[] = "fields";
constexpr char kGroupName[] = "groupName";
constexpr char kName[] = "name";
constexpr char kPeripheral[] = "peripheral";
constexpr char kPeripherals[] = "peripherals";
constexpr char kReadOnly[] = "read-only";
constexpr char kReadWrite[] = "read-write";
constexpr char kRegister[] = "register";
constexpr char kRegisters[] = "registers";
constexpr char kResetvalue[] = "resetValue";
constexpr char kSize[] = "size";
constexpr char kWritOnlye[] = "write-only";
} // namespace

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
        return PeripheralRegisterHandler::tr("RO");
    case PeripheralRegisterAccess::WriteOnly:
        return PeripheralRegisterHandler::tr("WO");
    case PeripheralRegisterAccess::ReadWrite:
        return PeripheralRegisterHandler::tr("RW");
    default:
        return PeripheralRegisterHandler::tr("N/A");
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

static quint64 decodeNumeric(const QString &text)
{
    bool ok = false;
    const quint64 result = text.toULongLong(&ok, 10);
    if (ok)
        return result;
    return text.toUInt(&ok, 16);
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
        const QRegularExpression re("^(0x)?([0-9A-F]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(2).toULongLong(ok, 16);
    } else if (fmt == PeripheralRegisterFormat::Decimal) {
        const QRegularExpression re("^([0-9]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(1).toULongLong(ok, 10);
    } else if (fmt == PeripheralRegisterFormat::Octal) {
        const QRegularExpression re("^(0)?([0-7]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(2).toULongLong(ok, 8);
    } else if (fmt == PeripheralRegisterFormat::Binary) {
        const QRegularExpression re("^(0b)?([0-1]+)$");
        const QRegularExpressionMatch m = re.match(string);
        if (m.hasMatch())
            return m.captured(2).toULongLong(ok, 2);
    }

    *ok = false;
    return 0;
}

// PeripheralRegisterField

QString PeripheralRegisterField::bitRangeString() const
{
    const int from = bitOffset;
    const int to = bitOffset + bitWidth - 1;
    return PeripheralRegisterHandler::tr("[%1..%2]").arg(to).arg(from);
}

QString PeripheralRegisterField::bitValueString(quint64 regValue) const
{
    const quint64 value = bitValue(regValue);
    return valueToString(value, bitWidth, format);
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
                        m_fld.bitValueString(m_reg.currentValue.v));
        }
        break;

    case Qt::EditRole:
        return m_fld.bitValueString(m_reg.currentValue.v);

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
    m_engine->setPeripheralRegisterValue(m_reg.address(m_group.baseAddress),
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
                    .arg(m_reg.addressString(m_group.baseAddress))
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
    m_engine->setPeripheralRegisterValue(m_reg.address(m_group.baseAddress),
                                         m_reg.currentValue.v);
}

// PeripheralRegisterDelegate

class PeripheralRegisterDelegate final : public QItemDelegate
{
public:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const final
    {
        if (index.column() == PeripheralRegisterValueColumn) {
            const auto lineEdit = new QLineEdit(parent);
            lineEdit->setAlignment(Qt::AlignLeft);
            lineEdit->setFrame(false);
            return lineEdit;
        }
        return nullptr;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const final
    {
        const auto lineEdit = qobject_cast<QLineEdit *>(editor);
        QTC_ASSERT(lineEdit, return);
        lineEdit->setText(index.data(Qt::EditRole).toString());
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,
        const QModelIndex &index) const final
    {
        if (index.column() == PeripheralRegisterValueColumn) {
            const auto lineEdit = qobject_cast<QLineEdit *>(editor);
            QTC_ASSERT(lineEdit, return);
            model->setData(index, lineEdit->text(), Qt::EditRole);
        }
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const final
    {
        editor->setGeometry(option.rect);
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
        const QModelIndex &index) const final
    {
        if (index.column() == PeripheralRegisterValueColumn) {
            const bool paintRed = index.data(PeripheralRegisterChangedRole).toBool();
            const QPen oldPen = painter->pen();
            const QColor lightColor(140, 140, 140);
            if (paintRed)
                painter->setPen(QColor(200, 0, 0));
            else
                painter->setPen(lightColor);
            // FIXME: performance? this changes only on real font changes.
            const QFontMetrics fm(option.font);
            const int charWidth = qMax(fm.horizontalAdvance('x'),
                                       fm.horizontalAdvance('0'));
            const QString str = index.data(Qt::DisplayRole).toString();
            int x = option.rect.x();
            bool light = !paintRed;
            for (int i = 0; i < str.size(); ++i) {
                const QChar c = str.at(i);
                const int uc = c.unicode();
                if (light && (uc != 'x' && uc != '0')) {
                    light = false;
                    painter->setPen(oldPen.color());
                }
                if (uc == ' ') {
                    light = true;
                    painter->setPen(lightColor);
                } else {
                    QRect r = option.rect;
                    r.setX(x);
                    r.setWidth(charWidth);
                    painter->drawText(r, Qt::AlignHCenter, c);
                }
                x += charWidth;
            }
            painter->setPen(oldPen);
        } else {
            QItemDelegate::paint(painter, option, index);
        }
    }
};

// PeripheralRegisterHandler

PeripheralRegisterHandler::PeripheralRegisterHandler(DebuggerEngine *engine)
    : m_engine(engine)
{
    setObjectName("PeripheralRegisterModel");
    setHeader({tr("Name"), tr("Value"), tr("Access")});
}

static void handleField(QXmlStreamReader &in, PeripheralRegister &reg)
{
    PeripheralRegisterField fld;
    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == kName) {
            fld.name = in.readElementText();
        } else if (elementName == kDescription) {
            fld.description = in.readElementText();
        } else if (elementName == kAccess) {
            fld.access = decodeAccess(in.readElementText());
        } else if (elementName == kBitRange) {
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
        } else if (elementName == kBitOffset) {
            fld.bitOffset = int(decodeNumeric(in.readElementText()));
        } else if (elementName == kBitWidth) {
            fld.bitWidth = int(decodeNumeric(in.readElementText()));
        } else {
            in.skipCurrentElement();
        }
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
    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == kName) {
            reg.name = in.readElementText();
        } else if (elementName == kDisplayName) {
            reg.displayName = in.readElementText();
        } else if (elementName == kDescription) {
            reg.description = in.readElementText();
        } else if (elementName == kAddressOffset) {
            reg.addressOffset = decodeNumeric(in.readElementText());
        } else if (elementName == kSize) {
            reg.size = int(decodeNumeric(in.readElementText()));
        } else if (elementName == kAccess) {
            reg.access = decodeAccess(in.readElementText());
        } else if (elementName == kResetvalue) {
            reg.resetValue = decodeNumeric(in.readElementText());
        } else if (elementName == kFields) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == kField)
                    handleField(in, reg);
                else
                    in.skipCurrentElement();
            }
        } else {
            in.skipCurrentElement();
        }
    }
    group.registers.push_back(reg);
}

static void handleGroup(QXmlStreamReader &in, PeripheralRegisterGroups &groups)
{
    PeripheralRegisterGroup group;

    const auto fromGroupName = in.attributes().value(kDerivedFrom);
    if (!fromGroupName.isEmpty()) {
        const auto groupEnd = groups.cend();
        const auto groupIt = std::find_if(groups.cbegin(), groupEnd,
                                          [fromGroupName](const PeripheralRegisterGroup &group) {
            return fromGroupName == group.name;
        });
        if (groupIt != groupEnd)
            group = *groupIt;
    }

    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == kName) {
            group.name = in.readElementText();
        } else if (elementName == kDescription) {
            group.description = in.readElementText();
        } else if (elementName == kGroupName) {
            group.displayName = in.readElementText();
        } else if (elementName == kBaseAddress) {
            group.baseAddress = decodeNumeric(in.readElementText());
        } else if (elementName == kSize) {
            group.size = int(decodeNumeric(in.readElementText()));
        } else if (elementName == kAccess) {
            group.access = decodeAccess(in.readElementText());
        } else if (elementName == kRegisters) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == kRegister)
                    handleRegister(in, group);
                else
                    in.skipCurrentElement();
            }
        } else {
            in.skipCurrentElement();
        }
    }
    groups.push_back(group);
}

static PeripheralRegisterGroups availablePeripheralRegisterGroups(const FilePath &filePath)
{
    QFile f(filePath.toString());
    if (!f.open(QIODevice::ReadOnly))
        return {};

    QXmlStreamReader in(&f);
    PeripheralRegisterGroups groups;
    while (in.readNextStartElement()) {
        const auto elementName = in.name();
        if (elementName == kDevice) {
            while (in.readNextStartElement()) {
                const auto elementName = in.name();
                if (elementName == kPeripherals) {
                    while (in.readNextStartElement()) {
                        const auto elementName = in.name();
                        if (elementName == kPeripheral)
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
    const FilePath peripheralDescriptionFile = FilePath::fromVariant(
                rp.inferior.extraData.value(Debugger::Constants::kPeripheralDescriptionFile));

    if (!peripheralDescriptionFile.exists())
        return;
    m_peripheralRegisterGroups = availablePeripheralRegisterGroups(peripheralDescriptionFile);
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
        return QVariant::fromValue(static_cast<QAbstractItemDelegate *>(
                                       new PeripheralRegisterDelegate));
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

    Internal::addHideColumnActions(menu, ev.view());
    menu->addAction(action(SettingsDialog));
    menu->popup(ev.globalPos());
    return true;
}

QMenu *PeripheralRegisterHandler::createRegisterGroupsMenu(DebuggerState state) const
{
    const auto groupMenu = new QMenu(tr("View Groups"));
    const auto actionGroup = new QActionGroup(groupMenu);
    bool hasActions = false;
    for (const PeripheralRegisterGroup &group : qAsConst(m_peripheralRegisterGroups)) {
        const QString actName = QStringLiteral("%1: %2")
                .arg(group.name, group.description);
        QAction *act = groupMenu->addAction(actName);
        const bool on = m_engine->hasCapability(RegisterCapability)
                && (state == InferiorStopOk || state == InferiorUnrunnable);
        act->setEnabled(on);
        act->setData(group.name);
        act->setCheckable(true);
        act->setChecked(group.active);
        actionGroup->addAction(act);
        QObject::connect(act, &QAction::triggered,
                         this, &PeripheralRegisterHandler::setActiveGroup);
        hasActions = true;
    }
    groupMenu->setEnabled(hasActions);
    groupMenu->setStyleSheet("QMenu { menu-scrollable: 1; }");
    return groupMenu;
}

QMenu *PeripheralRegisterHandler::createRegisterFormatMenu(
        DebuggerState state, PeripheralRegisterItem *item) const
{
    const auto fmtMenu = new QMenu(tr("Format"));
    const auto actionGroup = new QActionGroup(fmtMenu);

    const bool on = m_engine->hasCapability(RegisterCapability)
            && (state == InferiorStopOk || state == InferiorUnrunnable);

    const PeripheralRegisterFormat fmt = item->m_reg.format;

    // Hexadecimal action.
    const auto hexAct = addCheckableAction(
                fmtMenu, tr("Hexadecimal"), on,
                fmt == PeripheralRegisterFormat::Hexadecimal,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Hexadecimal;
        item->update();
    });
    actionGroup->addAction(hexAct);

    // Decimal action.
    const auto decAct = addCheckableAction(
                fmtMenu, tr("Decimal"), on,
                fmt == PeripheralRegisterFormat::Decimal,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Decimal;
        item->update();
    });
    actionGroup->addAction(decAct);

    // Octal action.
    const auto octAct = addCheckableAction(
                fmtMenu, tr("Octal"), on,
                fmt == PeripheralRegisterFormat::Octal,
                [item] {
        item->m_reg.format = PeripheralRegisterFormat::Octal;
        item->update();
    });
    actionGroup->addAction(octAct);

    // Binary action.
    const auto binAct = addCheckableAction(
                fmtMenu, tr("Binary"), on,
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
    const auto fmtMenu = new QMenu(tr("Format"));
    const auto actionGroup = new QActionGroup(fmtMenu);

    const bool on = m_engine->hasCapability(RegisterCapability)
            && (state == InferiorStopOk || state == InferiorUnrunnable);

    const PeripheralRegisterFormat fmt = item->m_fld.format;

    // Hexadecimal action.
    const auto hexAct = addCheckableAction(
                fmtMenu, tr("Hexadecimal"), on,
                fmt == PeripheralRegisterFormat::Hexadecimal,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Hexadecimal;
        item->update();
    });
    actionGroup->addAction(hexAct);

    // Decimal action.
    const auto decAct = addCheckableAction(
                fmtMenu, tr("Decimal"), on,
                fmt == PeripheralRegisterFormat::Decimal,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Decimal;
        item->update();
    });
    actionGroup->addAction(decAct);

    // Octal action.
    const auto octAct = addCheckableAction(
                fmtMenu, tr("Octal"), on,
                fmt == PeripheralRegisterFormat::Octal,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Octal;
        item->update();
    });
    actionGroup->addAction(octAct);

    // Binary action.
    const auto binAct = addCheckableAction(
                fmtMenu, tr("Binary"), on,
                fmt == PeripheralRegisterFormat::Binary,
                [item] {
        item->m_fld.format = PeripheralRegisterFormat::Binary;
        item->update();
    });
    actionGroup->addAction(binAct);

    return fmtMenu;
}

void PeripheralRegisterHandler::setActiveGroup(bool checked)
{
    if (!checked)
        return;
    deactivateGroups();
    if (const auto act = qobject_cast<QAction *>(sender())) {
        const QString groupName = act->data().toString();
        const auto groupEnd = m_peripheralRegisterGroups.end();
        const auto groupIt = std::find_if(
                    m_peripheralRegisterGroups.begin(), groupEnd,
                    [groupName](const PeripheralRegisterGroup &group){
            return group.name == groupName;
        });
        if (groupIt == groupEnd)
            return; // Group not found.
        // Set active group.
        groupIt->active = true;

        // Add all register items of active register group.
        m_activeRegisters.reserve(groupIt->registers.count());
        for (PeripheralRegister &reg : groupIt->registers) {
            const auto item = new PeripheralRegisterItem(m_engine, *groupIt, reg);
            rootItem()->appendChild(item);

            const quint64 address = reg.address(groupIt->baseAddress);
            m_activeRegisters.insert(address, item);
        }

        m_engine->reloadPeripheralRegisters();
    }
}

void PeripheralRegisterHandler::deactivateGroups()
{
    clear();

    for (PeripheralRegisterGroup &group : m_peripheralRegisterGroups)
        group.active = false;

    m_activeRegisters.clear();
}

} // namespace Internal
} // namespace Debugger
