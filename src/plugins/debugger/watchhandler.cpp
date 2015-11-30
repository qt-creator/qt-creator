/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "watchhandler.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerprotocol.h"
#include "debuggertooltipmanager.h"
#include "simplifytype.h"
#include "imageviewer.h"
#include "watchutils.h"
#include "cdb/cdbengine.h" // Remove after string freeze

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/checkablemessagebox.h>
#include <utils/theme/theme.h>

#include <QDebug>
#include <QFile>
#include <QPainter>
#include <QProcess>
#include <QTabWidget>
#include <QTextEdit>
#include <QJsonArray>
#include <QJsonObject>

#include <QTimer>
#include <algorithm>
#include <cstring>
#include <ctype.h>

using namespace Utils;

namespace Debugger {
namespace Internal {

// Creates debug output for accesses to the model.
enum { debugModel = 0 };

#define MODEL_DEBUG(s) do { if (debugModel) qDebug() << s; } while (0)

static QHash<QByteArray, int> theWatcherNames;
static int theWatcherCount = 0;
static QHash<QByteArray, int> theTypeFormats;
static QHash<QByteArray, int> theIndividualFormats;
static int theUnprintableBase = -1;

const char INameProperty[] = "INameProperty";
const char KeyProperty[] = "KeyProperty";

static QByteArray stripForFormat(const QByteArray &ba)
{
    QByteArray res;
    res.reserve(ba.size());
    int inArray = 0;
    for (int i = 0; i != ba.size(); ++i) {
        const char c = ba.at(i);
        if (c == '<')
            break;
        if (c == '[')
            ++inArray;
        if (c == ']')
            --inArray;
        if (c == ' ')
            continue;
        if (c == '&') // Treat references like the referenced type.
            continue;
        if (inArray && c >= '0' && c <= '9')
            continue;
        res.append(c);
    }
    return res;
}

static void saveWatchers()
{
    setSessionValue("Watchers", WatchHandler::watchedExpressions());
}

static void loadFormats()
{
    QVariant value = sessionValue("DefaultFormats");
    QMapIterator<QString, QVariant> it(value.toMap());
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            theTypeFormats.insert(it.key().toUtf8(), it.value().toInt());
    }

    value = sessionValue("IndividualFormats");
    it = QMapIterator<QString, QVariant>(value.toMap());
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            theIndividualFormats.insert(it.key().toUtf8(), it.value().toInt());
    }
}

static void saveFormats()
{
    QMap<QString, QVariant> formats;
    QHashIterator<QByteArray, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != AutomaticFormat) {
            const QByteArray key = it.key().trimmed();
            if (!key.isEmpty())
                formats.insert(QString::fromLatin1(key), format);
        }
    }
    setSessionValue("DefaultFormats", formats);

    formats.clear();
    it = QHashIterator<QByteArray, int>(theIndividualFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        const QByteArray key = it.key().trimmed();
        if (!key.isEmpty())
            formats.insert(QString::fromLatin1(key), format);
    }
    setSessionValue("IndividualFormats", formats);
}

///////////////////////////////////////////////////////////////////////
//
// SeparatedView
//
///////////////////////////////////////////////////////////////////////

class SeparatedView : public QTabWidget
{
public:
    SeparatedView() : QTabWidget(Internal::mainWindow())
    {
        setTabsClosable(true);
        connect(this, &QTabWidget::tabCloseRequested, this, &SeparatedView::closeTab);
        setWindowFlags(windowFlags() | Qt::Window);
        setWindowTitle(WatchHandler::tr("Debugger - Qt Creator"));

        QVariant geometry = sessionValue("DebuggerSeparateWidgetGeometry");
        if (geometry.isValid()) {
            QRect rc = geometry.toRect();
            if (rc.width() < 200)
                rc.setWidth(200);
            if (rc.height() < 200)
                rc.setHeight(200);
            setGeometry(rc);
        }
    }

    void saveGeometry()
    {
        setSessionValue("DebuggerSeparateWidgetGeometry", geometry());
    }

    ~SeparatedView()
    {
        saveGeometry();
    }

    void removeObject(const QByteArray &key)
    {
        saveGeometry();
        if (QWidget *w = findWidget(key)) {
            removeTab(indexOf(w));
            sanitize();
        }
    }

    void closeTab(int index)
    {
        saveGeometry();
        if (QObject *o = widget(index)) {
            QByteArray iname = o->property(INameProperty).toByteArray();
            theIndividualFormats.remove(iname);
            saveFormats();
        }
        removeTab(index);
        sanitize();
    }

    void sanitize()
    {
        if (count() == 0)
            hide();
    }

    QWidget *findWidget(const QByteArray &needle)
    {
        for (int i = count(); --i >= 0; ) {
            QWidget *w = widget(i);
            QByteArray key = w->property(KeyProperty).toByteArray();
            if (key == needle)
                return w;
        }
        return 0;
    }

    template <class T> T *prepareObject(const QByteArray &key, const QString &tabName)
    {
        T *t = 0;
        if (QWidget *w = findWidget(key)) {
            t = qobject_cast<T *>(w);
            if (!t)
                removeTab(indexOf(w));
        }
        if (!t) {
            t = new T;
            t->setProperty(KeyProperty, key);
            addTab(t, tabName);
        }

        setCurrentWidget(t);
        show();
        raise();
        return t;
    }
};


///////////////////////////////////////////////////////////////////////
//
// WatchModel
//
///////////////////////////////////////////////////////////////////////

class WatchModel : public WatchModelBase
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::WatchModel)
public:
    WatchModel(WatchHandler *handler, DebuggerEngine *engine);

    static QString nameForFormat(int format);

    QVariant data(const QModelIndex &idx, int role) const;
    bool setData(const QModelIndex &idx, const QVariant &value, int role);

    QString displayForAutoTest(const QByteArray &iname) const;
    void reinitialize(bool includeInspectData = false);

    WatchItem *findItem(const QByteArray &iname) const;
    void insertItem(WatchItem *item);
    void reexpandItems();

    void showEditValue(const WatchItem *item);
    void setTypeFormat(const QByteArray &type, int format);
    void setIndividualFormat(const QByteArray &iname, int format);

    QString removeNamespaces(QString str) const;

public:
    WatchHandler *m_handler; // Not owned.
    DebuggerEngine *m_engine; // Not owned.

    bool m_contentsValid;
    bool m_resetLocationScheduled;

    WatchItem *root() const { return static_cast<WatchItem *>(rootItem()); }
    WatchItem *m_localsRoot; // Not owned.
    WatchItem *m_inspectorRoot; // Not owned.
    WatchItem *m_watchRoot; // Not owned.
    WatchItem *m_returnRoot; // Not owned.
    WatchItem *m_tooltipRoot; // Not owned.

    SeparatedView *m_separatedView; // Not owned.

    QSet<QByteArray> m_expandedINames;
    QTimer m_requestUpdateTimer;

    QHash<QString, DisplayFormats> m_reportedTypeFormats; // Type name -> Dumper Formats
    QHash<QByteArray, QString> m_valueCache;
};

WatchModel::WatchModel(WatchHandler *handler, DebuggerEngine *engine)
    : m_handler(handler), m_engine(engine), m_separatedView(new SeparatedView)
{
    setObjectName(QLatin1String("WatchModel"));

    m_contentsValid = false;
    m_contentsValid = true; // FIXME
    m_resetLocationScheduled = false;

    setHeader(QStringList() << tr("Name") << tr("Value") << tr("Type"));
    auto root = new WatchItem;
    root->appendChild(m_localsRoot = new WatchItem("local", tr("Locals")));
    root->appendChild(m_inspectorRoot = new WatchItem("inspect", tr("Inspector")));
    root->appendChild(m_watchRoot = new WatchItem("watch", tr("Expressions")));
    root->appendChild(m_returnRoot = new WatchItem("return", tr("Return Value")));
    root->appendChild(m_tooltipRoot = new WatchItem("tooltip", tr("Tooltip")));
    setRootItem(root);

    m_requestUpdateTimer.setSingleShot(true);
    connect(&m_requestUpdateTimer, &QTimer::timeout,
        this, &WatchModel::updateStarted);

    connect(action(SortStructMembers), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateLocals);
    connect(action(ShowStdNamespace), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateAll);
    connect(action(ShowQtNamespace), &SavedAction::valueChanged,
        m_engine, &DebuggerEngine::updateAll);
}

void WatchModel::reinitialize(bool includeInspectData)
{
    m_localsRoot->removeChildren();
    m_watchRoot->removeChildren();
    m_returnRoot->removeChildren();
    m_tooltipRoot->removeChildren();
    if (includeInspectData)
        m_inspectorRoot->removeChildren();
}

WatchItem *WatchModel::findItem(const QByteArray &iname) const
{
    return root()->findItem(iname);
}

WatchItem *WatchItem::findItem(const QByteArray &iname)
{
    if (this->iname == iname)
        return this;
    foreach (TreeItem *child, children()) {
        auto witem = static_cast<WatchItem *>(child);
        if (witem->iname == iname)
            return witem;
        if (witem->isAncestorOf(iname))
            return witem->findItem(iname);
    }
    return 0;
}

static QByteArray parentName(const QByteArray &iname)
{
    const int pos = iname.lastIndexOf('.');
    return pos == -1 ? QByteArray() : iname.left(pos);
}

static QString niceTypeHelper(const QByteArray &typeIn)
{
    typedef QMap<QByteArray, QString> Cache;
    static Cache cache;
    const Cache::const_iterator it = cache.constFind(typeIn);
    if (it != cache.constEnd())
        return it.value();
    const QString simplified = simplifyType(QLatin1String(typeIn));
    cache.insert(typeIn, simplified); // For simplicity, also cache unmodified types
    return simplified;
}

QString WatchModel::removeNamespaces(QString str) const
{
    if (!boolSetting(ShowStdNamespace))
        str.remove(QLatin1String("std::"));
    if (!boolSetting(ShowQtNamespace)) {
        const QString qtNamespace = QString::fromLatin1(m_engine->qtNamespace());
        if (!qtNamespace.isEmpty())
            str.remove(qtNamespace);
    }
    return str;
}

static int formatToIntegerBase(int format)
{
    switch (format) {
        case HexadecimalIntegerFormat:
            return 16;
        case BinaryIntegerFormat:
            return 2;
        case OctalIntegerFormat:
            return 8;
    }
    return 10;
}

template <class IntType> QString reformatInteger(IntType value, int format)
{
    switch (format) {
        case HexadecimalIntegerFormat:
            return QLatin1String("(hex) ") + QString::number(value, 16);
        case BinaryIntegerFormat:
            return QLatin1String("(bin) ") + QString::number(value, 2);
        case OctalIntegerFormat:
            return QLatin1String("(oct) ") + QString::number(value, 8);
    }
    return QString::number(value, 10); // not reached
}

static QString reformatInteger(quint64 value, int format, int size, bool isSigned)
{
    // Follow convention and don't show negative non-decimal numbers.
    if (format != AutomaticFormat && format != DecimalIntegerFormat)
        isSigned = false;

    switch (size) {
        case 1:
            value = value & 0xff;
            return isSigned
                ? reformatInteger<qint8>(value, format)
                : reformatInteger<quint8>(value, format);
        case 2:
            value = value & 0xffff;
            return isSigned
                ? reformatInteger<qint16>(value, format)
                : reformatInteger<quint16>(value, format);
        case 4:
            value = value & 0xffffffff;
            return isSigned
                ? reformatInteger<qint32>(value, format)
                : reformatInteger<quint32>(value, format);
        default:
        case 8: return isSigned
                ? reformatInteger<qint64>(value, format)
                : reformatInteger<quint64>(value, format);
    }
}

// Format printable (char-type) characters
static QString reformatCharacter(int code, int format, bool isSigned)
{
    const QString codeS = reformatInteger(code, format, 1, isSigned);
    if (code < 0) // Append unsigned value.
        return codeS + QLatin1String(" / ") + reformatInteger(256 + code, format, 1, false);
    const QChar c = QChar(uint(code));
    if (c.isPrint())
        return codeS + QLatin1String(" '") + c + QLatin1Char('\'');
    switch (code) {
    case 0:
        return codeS + QLatin1String(" '\\0'");
    case '\r':
        return codeS + QLatin1String(" '\\r'");
    case '\t':
        return codeS + QLatin1String(" '\\t'");
    case '\n':
        return codeS + QLatin1String(" '\\n'");
    }
    return codeS;
}

static QString quoteUnprintable(const QString &str)
{
    if (WatchHandler::unprintableBase() == 0)
        return str;

    QString encoded;
    if (WatchHandler::unprintableBase() == -1) {
        foreach (const QChar c, str) {
            int u = c.unicode();
            if (c.isPrint())
                encoded += c;
            else if (u == '\r')
                encoded += QLatin1String("\\r");
            else if (u == '\t')
                encoded += QLatin1String("\\t");
            else if (u == '\n')
                encoded += QLatin1String("\\n");
            else
                encoded += QString::fromLatin1("\\%1")
                    .arg(c.unicode(), 3, 8, QLatin1Char('0'));
        }
        return encoded;
    }

    foreach (const QChar c, str) {
        if (c.isPrint()) {
            encoded += c;
        } else if (WatchHandler::unprintableBase() == 8) {
            encoded += QString::fromLatin1("\\%1")
                .arg(c.unicode(), 3, 8, QLatin1Char('0'));
        } else {
            encoded += QString::fromLatin1("\\u%1")
                .arg(c.unicode(), 4, 16, QLatin1Char('0'));
        }
    }
    return encoded;
}

QString WatchItem::formattedValue() const
{
    if (type == "bool") {
        if (value == QLatin1String("0"))
            return QLatin1String("false");
        if (value == QLatin1String("1"))
            return QLatin1String("true");
        return value;
    }

    const int format = itemFormat();

    // Append quoted, printable character also for decimal.
    // FIXME: This is unreliable.
    if (type.endsWith("char") || type.endsWith("QChar")) {
        bool ok;
        const int code = value.toInt(&ok);
        bool isUnsigned = type == "unsigned char" || type == "uchar";
        return ok ? reformatCharacter(code, format, !isUnsigned) : value;
    }

    if (format == HexadecimalIntegerFormat
            || format == DecimalIntegerFormat
            || format == OctalIntegerFormat
            || format == BinaryIntegerFormat) {
        bool isSigned = value.startsWith(QLatin1Char('-'));
        quint64 raw = isSigned ? quint64(value.toLongLong()) : value.toULongLong();
        return reformatInteger(raw, format, size, isSigned);
    }

    if (format == ScientificFloatFormat) {
        double dd = value.toDouble();
        return QString::number(dd, 'e');
    }

    if (format == CompactFloatFormat) {
        double dd = value.toDouble();
        return QString::number(dd, 'g');
    }

    if (type == "va_list")
        return value;

    if (!isPointerType(type) && !isVTablePointer()) {
        bool ok = false;
        qulonglong integer = value.toULongLong(&ok, 0);
        if (ok) {
            const int format = itemFormat();
            return reformatInteger(integer, format, size, false);
        }
    }

    if (elided) {
        QString v = value;
        v.chop(1);
        QString len = elided > 0 ? QString::number(elided) : QLatin1String("unknown length");
        return v + QLatin1String("\"... (") + len  + QLatin1Char(')');
    }

    return quoteUnprintable(value);
}

// Get a pointer address from pointer values reported by the debugger.
// Fix CDB formatting of pointers "0x00000000`000003fd class foo *", or
// "0x00000000`000003fd "Hallo"", or check gdb formatting of characters.
static inline quint64 pointerValue(QString data)
{
    const int blankPos = data.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        data.truncate(blankPos);
    data.remove(QLatin1Char('`'));
    return data.toULongLong(0, 0);
}

// Return the type used for editing
int WatchItem::editType() const
{
    if (type == "bool")
        return QVariant::Bool;
    if (isIntType(type))
        return type.contains('u') ? QVariant::ULongLong : QVariant::LongLong;
    if (isFloatType(type))
        return QVariant::Double;
    // Check for pointers using hex values (0xAD00 "Hallo")
    if (isPointerType(type) && value.startsWith(QLatin1String("0x")))
        return QVariant::ULongLong;
   return QVariant::String;
}

// Convert to editable (see above)
QVariant WatchItem::editValue() const
{
    switch (editType()) {
    case QVariant::Bool:
        return value != QLatin1String("0") && value != QLatin1String("false");
    case QVariant::ULongLong:
        if (isPointerType(type)) // Fix pointer values (0xAD00 "Hallo" -> 0xAD00)
            return QVariant(pointerValue(value));
        return QVariant(value.toULongLong());
    case QVariant::LongLong:
        return QVariant(value.toLongLong());
    case QVariant::Double:
        return QVariant(value.toDouble());
    default:
        break;
    }
    // Some string value: '0x434 "Hallo"':
    // Remove quotes and replace newlines, which will cause line edit troubles.
    QString stringValue = value;
    if (stringValue.endsWith(QLatin1Char('"'))) {
        const int leadingDoubleQuote = stringValue.indexOf(QLatin1Char('"'));
        if (leadingDoubleQuote != stringValue.size() - 1) {
            stringValue.truncate(stringValue.size() - 1);
            stringValue.remove(0, leadingDoubleQuote + 1);
            stringValue.replace(QLatin1String("\n"), QLatin1String("\\n"));
        }
    }
    return QVariant(quoteUnprintable(stringValue));
}

bool WatchItem::canFetchMore() const
{
    if (!wantsChildren)
        return false;
    const WatchModel *model = watchModel();
    if (!model)
        return false;
    if (!model->m_contentsValid && !isInspect())
        return false;
    return true;
}

void WatchItem::fetchMore()
{
    WatchModel *model = watchModel();
    model->m_expandedINames.insert(iname);
    if (children().isEmpty()) {
        setChildrenNeeded();
        model->m_engine->expandItem(iname);
    }
}

// Truncate value for item view, maintaining quotes.
static QString truncateValue(QString v)
{
    enum { maxLength = 512 };
    if (v.size() < maxLength)
        return v;
    const bool isQuoted = v.endsWith(QLatin1Char('"')); // check for 'char* "Hallo"'
    v.truncate(maxLength);
    v += isQuoted ? QLatin1String("...\"") : QLatin1String("...");
    return v;
}

int WatchItem::itemFormat() const
{
    const int individualFormat = theIndividualFormats.value(iname, AutomaticFormat);
    if (individualFormat != AutomaticFormat)
        return individualFormat;
    return theTypeFormats.value(stripForFormat(type), AutomaticFormat);
}

QString WatchItem::expression() const
{
    if (!exp.isEmpty())
         return QString::fromLatin1(exp);
    if (address && !type.isEmpty()) {
        return QString::fromLatin1("*(%1*)%2").
                arg(QLatin1String(type), QLatin1String(hexAddress()));
    }
    if (const WatchItem *p = parentItem()) {
        if (!p->exp.isEmpty())
           return QString::fromLatin1("(%1).%2").arg(QString::fromLatin1(p->exp), name);
    }
    return name;
}

QString WatchItem::displayName() const
{
    QString result;
    if (!parentItem())
        return result;
    if (iname.startsWith("return") && name.startsWith(QLatin1Char('$')))
        result = WatchModel::tr("returned value");
    else if (name == QLatin1String("*"))
        result = QLatin1Char('*') + parentItem()->name;
    else
        result = watchModel()->removeNamespaces(name);

    // Simplyfy names that refer to base classes.
    if (result.startsWith(QLatin1Char('['))) {
        result = simplifyType(result);
        if (result.size() > 30)
            result = result.left(27) + QLatin1String("...]");
    }

    return result;
}

QString WatchItem::displayValue() const
{
    QString result = watchModel()->removeNamespaces(truncateValue(formattedValue()));
    if (result.isEmpty() && address)
        result += QString::fromLatin1("@0x" + QByteArray::number(address, 16));
//    if (origaddr)
//        result += QString::fromLatin1(" (0x" + QByteArray::number(origaddr, 16) + ')');
    return result;
}

QString WatchItem::displayType() const
{
    QString result = displayedType.isEmpty()
        ? niceTypeHelper(type)
        : displayedType;
    if (bitsize)
        result += QString::fromLatin1(":%1").arg(bitsize);
    result.remove(QLatin1Char('\''));
    result = watchModel()->removeNamespaces(result);
    return result;
}

QColor WatchItem::valueColor(int column) const
{
    Theme::Color color = Theme::Debugger_WatchItem_ValueNormal;
    if (const WatchModel *model = watchModel()) {
        if (!model->m_contentsValid && !isInspect()) {
            color = Theme::Debugger_WatchItem_ValueInvalid;
        } else if (column == 1) {
            if (!valueEnabled)
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (!model->m_contentsValid && !isInspect())
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (column == 1 && value.isEmpty()) // This might still show 0x...
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (column == 1 && value != model->m_valueCache.value(iname))
                color = Theme::Debugger_WatchItem_ValueChanged;
        }
    }
    return creatorTheme()->color(color);
}

QVariant WatchItem::data(int column, int role) const
{
    switch (role) {
        case LocalsEditTypeRole:
            return QVariant(editType());

        case LocalsNameRole:
            return QVariant(name);

        case LocalsIntegerBaseRole:
            if (isPointerType(type)) // Pointers using 0x-convention
                return QVariant(16);
            return QVariant(formatToIntegerBase(itemFormat()));

        case Qt::EditRole: {
            switch (column) {
                case 0:
                    return expression();
                case 1:
                    return editValue();
                case 2:
                    // FIXME:: To be tested: Can debuggers handle those?
                    if (!displayedType.isEmpty())
                        return displayedType;
                    return QString::fromUtf8(type);
            }
        }

        case Qt::DisplayRole: {
            switch (column) {
                case 0:
                    return displayName();
                case 1:
                    return displayValue();
                case 2:
                    return displayType();
            }
        }

        case Qt::ToolTipRole:
            return boolSetting(UseToolTipsInLocalsView)
                ? toToolTip() : QVariant();

        case Qt::ForegroundRole:
            return valueColor(column);

        case LocalsExpressionRole:
            return expression();

        case LocalsRawExpressionRole:
            return exp;

        case LocalsINameRole:
            return iname;

        case LocalsExpandedRole:
            return watchModel()->m_expandedINames.contains(iname);

        case LocalsTypeFormatListRole:
            return QVariant::fromValue(typeFormatList());

        case LocalsTypeRole:
            return watchModel()->removeNamespaces(displayType());

        case LocalsRawTypeRole:
            return QString::fromLatin1(type);

        case LocalsTypeFormatRole:
            return theTypeFormats.value(stripForFormat(type), AutomaticFormat);

        case LocalsIndividualFormatRole:
            return theIndividualFormats.value(iname, AutomaticFormat);

        case LocalsRawValueRole:
            return value;

        case LocalsObjectAddressRole:
            return address;

        case LocalsPointerAddressRole:
            return origaddr;

        case LocalsIsWatchpointAtObjectAddressRole: {
            BreakpointParameters bp(WatchpointAtAddress);
            bp.address = address;
            return watchModel()->m_engine->breakHandler()->findWatchpoint(bp) != 0;
        }

        case LocalsSizeRole:
            return QVariant(size);

        case LocalsIsWatchpointAtPointerAddressRole:
            if (isPointerType(type)) {
                BreakpointParameters bp(WatchpointAtAddress);
                bp.address = pointerValue(value);
                return watchModel()->m_engine->breakHandler()->findWatchpoint(bp) != 0;
            }
            return false;

        default:
            break;
    }
    return QVariant();
}

QVariant WatchModel::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ExtraIndicesForColumnWidth) {
        QModelIndexList l;
        foreach (TreeItem *item, m_watchRoot->children())
            l.append(indexForItem(item));
        foreach (TreeItem *item, m_returnRoot->children())
            l.append(indexForItem(item));
        return QVariant::fromValue(l);
    }
    return WatchModelBase::data(idx, role);
}

bool WatchModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (!idx.isValid())
        return false; // Triggered by ModelTester.

    WatchItem *item = static_cast<WatchItem *>(itemForIndex(idx));
    QTC_ASSERT(item, return false);

    switch (role) {
        case Qt::EditRole:
            switch (idx.column()) {
            case 0: {
                m_handler->updateWatchExpression(item, value.toString().trimmed().toUtf8());
                break;
            }
            case 1: // Change value
                m_engine->assignValueInDebugger(item, item->expression(), value);
                break;
            case 2: // TODO: Implement change type.
                m_engine->assignValueInDebugger(item, item->expression(), value);
                break;
            }
        case LocalsExpandedRole:
            if (value.toBool()) {
                // Should already have been triggered by fetchMore()
                //QTC_CHECK(m_expandedINames.contains(item->iname));
                m_expandedINames.insert(item->iname);
            } else {
                m_expandedINames.remove(item->iname);
            }
            if (item->iname.contains('.'))
                emit columnAdjustmentRequested();
            break;

        case LocalsTypeFormatRole:
            setTypeFormat(item->type, value.toInt());
            m_engine->updateLocals();
            break;

        case LocalsIndividualFormatRole: {
            setIndividualFormat(item->iname, value.toInt());
            m_engine->updateLocals();
            break;
        }
    }

    //emit dataChanged(idx, idx);
    return true;
}

Qt::ItemFlags WatchItem::flags(int column) const
{
    QTC_ASSERT(model(), return Qt::ItemFlags());
    DebuggerEngine *engine = watchModel()->m_engine;
    QTC_ASSERT(engine, return Qt::ItemFlags());
    const DebuggerState state = engine->state();

    // Enabled, editable, selectable, checkable, and can be used both as the
    // source of a drag and drop operation and as a drop target.

    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    const Qt::ItemFlags editable = notEditable | Qt::ItemIsEditable;

    if (state == InferiorUnrunnable)
        return (isWatcher() && column == 0 && iname.count('.') == 1) ? editable : notEditable;

    if (isWatcher()) {
        if (state != InferiorStopOk
                && state != DebuggerNotReady
                && state != DebuggerFinished
                && !engine->hasCapability(AddWatcherWhileRunningCapability))
            return Qt::ItemFlags();
        if (column == 0 && iname.count('.') == 1)
            return editable; // Watcher names are editable.

        if (!name.isEmpty()) {
            // FIXME: Forcing types is not implemented yet.
            //if (idx.column() == 2)
            //    return editable; // Watcher types can be set by force.
            if (column == 1 && valueEditable && !elided)
                return editable; // Watcher values are sometimes editable.
        }
    } else if (isLocal()) {
        if (state != InferiorStopOk && !engine->hasCapability(AddWatcherWhileRunningCapability))
           return Qt::ItemFlags();
        if (column == 1 && valueEditable && !elided)
            return editable; // Locals values are sometimes editable.
    } else if (isInspect()) {
        if (column == 1 && valueEditable)
            return editable; // Inspector values are sometimes editable.
    }
    return notEditable;
}

static inline QString msgArrayFormat(int n)
{
    return WatchModel::tr("Array of %n items", 0, n);
}

QString WatchModel::nameForFormat(int format)
{
    switch (format) {
        case AutomaticFormat: return tr("Automatic");

        case RawFormat: return tr("Raw Data");
        case SimpleFormat: return CdbEngine::tr("Normal");  // FIXME: String
        case EnhancedFormat: return tr("Enhanced");
        case SeparateFormat: return CdbEngine::tr("Separate Window");  // FIXME: String

        case Latin1StringFormat: return tr("Latin1 String");
        case SeparateLatin1StringFormat: return tr("Latin1 String in Separate Window");
        case Utf8StringFormat: return tr("UTF-8 String");
        case SeparateUtf8StringFormat: return tr("UTF-8 String in Separate Window");
        case Local8BitStringFormat: return tr("Local 8-Bit String");
        case Utf16StringFormat: return tr("UTF-16 String");
        case Ucs4StringFormat: return tr("UCS-4 String");

        case Array10Format: return msgArrayFormat(10);
        case Array100Format: return msgArrayFormat(100);
        case Array1000Format: return msgArrayFormat(1000);
        case Array10000Format: return msgArrayFormat(10000);
        case ArrayPlotFormat: return tr("Plot in Separate Window");

        case CompactMapFormat: return tr("Display Keys and Values Side by Side");
        case DirectQListStorageFormat: return tr("Force Display as Direct Storage Form");
        case IndirectQListStorageFormat: return tr("Force Display as Indirect Storage Form");

        case BoolTextFormat: return tr("Display Boolean Values as True or False");
        case BoolIntegerFormat: return tr("Display Boolean Values as 1 or 0");

        case DecimalIntegerFormat: return tr("Decimal Integer");
        case HexadecimalIntegerFormat: return tr("Hexadecimal Integer");
        case BinaryIntegerFormat: return tr("Binary Integer");
        case OctalIntegerFormat: return tr("Octal Integer");

        case CompactFloatFormat: return tr("Compact Float");
        case ScientificFloatFormat: return tr("Scientific Float");
    }

    QTC_CHECK(false);
    return QString();
}

DisplayFormats WatchItem::typeFormatList() const
{
    DisplayFormats formats;

    // Types supported by dumpers:
    // Hack: Compensate for namespaces.
    QString t = QLatin1String(stripForFormat(type));
    int pos = t.indexOf(QLatin1String("::Q"));
    if (pos >= 0 && t.count(QLatin1Char(':')) == 2)
        t.remove(0, pos + 2);
    pos = t.indexOf(QLatin1Char('<'));
    if (pos >= 0)
        t.truncate(pos);
    t.replace(QLatin1Char(':'), QLatin1Char('_'));
    formats << watchModel()->m_reportedTypeFormats.value(t);

    if (t.contains(QLatin1Char(']')))
        formats.append(ArrayPlotFormat);

    // Fixed artificial string and pointer types.
    if (origaddr || isPointerType(type)) {
        formats.append(RawFormat);
        formats.append(Latin1StringFormat);
        formats.append(SeparateLatin1StringFormat);
        formats.append(Utf8StringFormat);
        formats.append(SeparateUtf8StringFormat);
        formats.append(Local8BitStringFormat);
        formats.append(Utf16StringFormat);
        formats.append(Ucs4StringFormat);
        formats.append(Array10Format);
        formats.append(Array100Format);
        formats.append(Array1000Format);
        formats.append(Array10000Format);
    } else if (type.contains("char[") || type.contains("char [")) {
        formats.append(RawFormat);
        formats.append(Latin1StringFormat);
        formats.append(SeparateLatin1StringFormat);
        formats.append(Utf8StringFormat);
        formats.append(SeparateUtf8StringFormat);
        formats.append(Local8BitStringFormat);
        formats.append(Utf16StringFormat);
        formats.append(Ucs4StringFormat);
    }

    // Fixed artificial floating point types.
    bool ok = false;
    value.toDouble(&ok);
    if (ok) {
        formats.append(CompactFloatFormat);
        formats.append(ScientificFloatFormat);
    }

    // Fixed artificial integral types.
    QString v = value;
    if (v.startsWith(QLatin1Char('-')))
        v = v.mid(1);
    v.toULongLong(&ok, 10);
    if (!ok)
        v.toULongLong(&ok, 16);
    if (!ok)
        v.toULongLong(&ok, 8);
    if (ok) {
        formats.append(DecimalIntegerFormat);
        formats.append(HexadecimalIntegerFormat);
        formats.append(BinaryIntegerFormat);
        formats.append(OctalIntegerFormat);
    }

    return formats;
}

int WatchItem::requestedFormat() const
{
    int format = theIndividualFormats.value(iname, AutomaticFormat);
    if (format == AutomaticFormat)
        format = theTypeFormats.value(stripForFormat(type), AutomaticFormat);
    return format;
}

///////////////////////////////////////////////////////////////////////
//
// WatchHandler
//
///////////////////////////////////////////////////////////////////////

WatchHandler::WatchHandler(DebuggerEngine *engine)
{
    m_model = new WatchModel(this, engine);
}

WatchHandler::~WatchHandler()
{
    // Do it manually to prevent calling back in model destructors
    // after m_cache is destroyed.
    delete m_model;
    m_model = 0;
}

void WatchHandler::cleanup()
{
    m_model->m_expandedINames.clear();
    theWatcherNames.remove(QByteArray());
    saveWatchers();
    m_model->reinitialize();
    emit m_model->updateFinished();
    m_model->m_separatedView->hide();
}

void WatchHandler::insertItem(WatchItem *item)
{
    m_model->insertItem(item);
}

void WatchModel::insertItem(WatchItem *item)
{
    QTC_ASSERT(!item->iname.isEmpty(), return);

    WatchItem *parent = findItem(parentName(item->iname));
    QTC_ASSERT(parent, return);

    bool found = false;
    const QVector<TreeItem *> siblings = parent->children();
    for (int row = 0, n = siblings.size(); row < n; ++row) {
        if (static_cast<WatchItem *>(siblings.at(row))->iname == item->iname) {
            delete takeItem(parent->children().at(row));
            parent->insertChild(row, item);
            found = true;
            break;
        }
    }
    if (!found)
        parent->appendChild(item);

    item->update();

    item->walkTree([this](TreeItem *sub) { showEditValue(static_cast<WatchItem *>(sub)); });
}

void WatchModel::reexpandItems()
{
    foreach (const QByteArray &iname, m_expandedINames) {
        if (WatchItem *item = findItem(iname)) {
            emit itemIsExpanded(indexForItem(item));
            emit inameIsExpanded(iname);
        } else {
            // Can happen. We might have stepped into another frame
            // not containing that iname, but we still like to
            // remember the expanded state of iname in case we step
            // out of the frame again.
        }
    }
}

void WatchHandler::removeAllData(bool includeInspectData)
{
    m_model->reinitialize(includeInspectData);
}

/*!
  If a displayed item differs from the cached entry it is considered
  "new", and correspondingly marked in red. Calling \c resetValueCache()
  stores the currently displayed items in the cache, effectively
  marking the value as known, and consequently painted black.
 */
void WatchHandler::resetValueCache()
{
    m_model->m_valueCache.clear();
    TreeItem *root = m_model->rootItem();
    root->walkTree([this, root](TreeItem *item) {
        auto watchItem = static_cast<WatchItem *>(item);
        m_model->m_valueCache[watchItem->iname] = watchItem->value;
    });
}

void WatchHandler::resetWatchers()
{
    loadSessionData();
}

void WatchHandler::notifyUpdateStarted(const QList<QByteArray> &inames)
{
    auto marker = [](TreeItem *it) { static_cast<WatchItem *>(it)->outdated = true; };

    if (inames.isEmpty()) {
        foreach (auto item, m_model->itemsAtLevel<WatchItem *>(2))
            item->walkTree(marker);
    } else {
        foreach (auto iname, inames) {
            if (WatchItem *item = m_model->findItem(iname))
                item->walkTree(marker);
        }
    }

    m_model->m_requestUpdateTimer.start(80);
    m_model->m_contentsValid = false;
    updateWatchersWindow();
}

void WatchHandler::notifyUpdateFinished()
{
    struct OutDatedItemsFinder : public TreeItemVisitor
    {
        bool preVisit(TreeItem *item)
        {
            auto watchItem = static_cast<WatchItem *>(item);
            if (level() <= 1 || !watchItem->outdated)
                return true;
            toRemove.append(watchItem);
            return false;
        }

        QList<WatchItem *> toRemove;
    } finder;

    m_model->root()->walkTree(&finder);

    foreach (auto item, finder.toRemove)
        delete m_model->takeItem(item);

    m_model->m_contentsValid = true;
    updateWatchersWindow();
    m_model->reexpandItems();
    m_model->m_requestUpdateTimer.stop();
    emit m_model->updateFinished();
}

void WatchHandler::reexpandItems()
{
    m_model->reexpandItems();
}

void WatchHandler::removeItemByIName(const QByteArray &iname)
{
    WatchItem *item = m_model->findItem(iname);
    if (!item)
        return;
    if (item->isWatcher()) {
        theWatcherNames.remove(item->exp);
        saveWatchers();
    }
    delete m_model->takeItem(item);
    updateWatchersWindow();
}

QByteArray WatchHandler::watcherName(const QByteArray &exp)
{
    return "watch." + QByteArray::number(theWatcherNames[exp]);
}

void WatchHandler::watchExpression(const QString &exp0, const QString &name)
{
    // Do not insert the same entry more then once.
    QByteArray exp = exp0.toLatin1();
    if (exp.isEmpty() || theWatcherNames.contains(exp))
        return;

    theWatcherNames[exp] = theWatcherCount++;

    auto item = new WatchItem;
    item->exp = exp;
    item->name = name.isEmpty() ? exp0 : name;
    item->iname = watcherName(exp);
    m_model->insertItem(item);
    saveWatchers();

    if (m_model->m_engine->state() == DebuggerNotReady) {
        item->setAllUnneeded();
        item->setValue(QString(QLatin1Char(' ')));
        item->update();
    } else {
        m_model->m_engine->updateItem(item->iname);
    }
    updateWatchersWindow();
}

void WatchHandler::updateWatchExpression(WatchItem *item, const QByteArray &newExp)
{
    if (newExp.isEmpty())
        return;

    if (item->exp != newExp) {
        theWatcherNames.insert(newExp, theWatcherNames.value(item->exp));
        theWatcherNames.remove(item->exp);
        item->exp = newExp;
        item->name = QString::fromUtf8(item->exp);
    }

    saveWatchers();
    if (m_model->m_engine->state() == DebuggerNotReady) {
        item->setAllUnneeded();
        item->setValue(QString(QLatin1Char(' ')));
        item->update();
    } else {
        m_model->m_engine->updateItem(item->iname);
    }
    updateWatchersWindow();
}

// Watch something obtained from the editor.
// Prefer to watch an existing local variable by its expression
// (address) if it can be found. Default to watchExpression().
void WatchHandler::watchVariable(const QString &exp)
{
    if (const WatchData *localVariable = findCppLocalVariable(exp))
        watchExpression(QLatin1String(localVariable->exp), exp);
    else
        watchExpression(exp);
}

static void swapEndian(char *d, int nchar)
{
    QTC_ASSERT(nchar % 4 == 0, return);
    for (int i = 0; i < nchar; i += 4) {
        char c = d[i];
        d[i] = d[i + 3];
        d[i + 3] = c;
        c = d[i + 1];
        d[i + 1] = d[i + 2];
        d[i + 2] = c;
    }
}

void WatchModel::showEditValue(const WatchItem *item)
{
    const QByteArray key = item->address ? item->hexAddress() : item->iname;
    switch (item->editformat) {
    case StopDisplay:
        m_separatedView->removeObject(key);
        break;
    case DisplayImageData:
    case DisplayImageFile: {  // QImage
        int width = 0, height = 0, nbytes = 0, format = 0;
        QByteArray ba;
        uchar *bits = 0;
        if (item->editformat == DisplayImageData) {
            ba = QByteArray::fromHex(item->editvalue);
            QTC_ASSERT(ba.size() > 16, return);
            const int *header = (int *)(ba.data());
            if (!ba.at(0) && !ba.at(1)) // Check on 'width' for Python dumpers returning 4-byte swapped-data.
                swapEndian(ba.data(), 16);
            bits = 16 + (uchar *)(ba.data());
            width = header[0];
            height = header[1];
            nbytes = header[2];
            format = header[3];
        } else if (item->editformat == DisplayImageFile) {
            QTextStream ts(item->editvalue);
            QString fileName;
            ts >> width >> height >> nbytes >> format >> fileName;
            QFile f(fileName);
            f.open(QIODevice::ReadOnly);
            ba = f.readAll();
            bits = (uchar*)ba.data();
            nbytes = width * height;
        }
        QTC_ASSERT(0 < width && width < 10000, return);
        QTC_ASSERT(0 < height && height < 10000, return);
        QTC_ASSERT(0 < nbytes && nbytes < 10000 * 10000, return);
        QTC_ASSERT(0 < format && format < 32, return);
        QImage im(width, height, QImage::Format(format));
        std::memcpy(im.bits(), bits, nbytes);
        ImageViewer *v = m_separatedView->prepareObject<ImageViewer>(key, item->name);
        v->setProperty(INameProperty, item->iname);
        v->setInfo(item->address ?
            tr("%1 Object at %2").arg(QLatin1String(item->type),
                QLatin1String(item->hexAddress())) :
            tr("%1 Object at Unknown Address").arg(QLatin1String(item->type))
            + QLatin1String("    ") +
            ImageViewer::tr("Size: %1x%2, %3 byte, format: %4, depth: %5")
                .arg(width).arg(height).arg(nbytes).arg(im.format()).arg(im.depth())
        );
        v->setImage(im);
        break;
    }
    case DisplayUtf16String:
    case DisplayLatin1String:
    case DisplayUtf8String: { // String data.
        QByteArray ba = QByteArray::fromHex(item->editvalue);
        QString str;
        if (item->editformat == DisplayUtf16String)
            str = QString::fromUtf16((ushort *)ba.constData(), ba.size()/2);
        else if (item->editformat == DisplayLatin1String)
            str = QString::fromLatin1(ba.constData(), ba.size());
        else if (item->editformat == DisplayUtf8String)
            str = QString::fromUtf8(ba.constData(), ba.size());
        QTextEdit *t = m_separatedView->prepareObject<QTextEdit>(key, item->name);
        t->setProperty(INameProperty, item->iname);
        t->setText(str);
        break;
    }
    case DisplayPlotData: { // Plots
        std::vector<double> data;
        readNumericVector(&data, QByteArray::fromHex(item->editvalue), item->editencoding);
        PlotViewer *v = m_separatedView->prepareObject<PlotViewer>(key, item->name);
        v->setProperty(INameProperty, item->iname);
        v->setData(data);
        break;
    }
    default:
        QTC_ASSERT(false, qDebug() << "Display format: " << item->editformat);
        break;
    }
}

void WatchHandler::clearWatches()
{
    if (theWatcherNames.isEmpty())
        return;

    const QDialogButtonBox::StandardButton ret = CheckableMessageBox::doNotAskAgainQuestion(
                Core::ICore::mainWindow(), tr("Remove All Expression Evaluators"),
                tr("Are you sure you want to remove all expression evaluators?"),
                Core::ICore::settings(), QLatin1String("RemoveAllWatchers"));
    if (ret != QDialogButtonBox::Yes)
        return;

    m_model->m_watchRoot->removeChildren();
    theWatcherNames.clear();
    theWatcherCount = 0;
    updateWatchersWindow();
    saveWatchers();
}

void WatchHandler::updateWatchersWindow()
{
    emit m_model->columnAdjustmentRequested();

    // Force show/hide of watchers and return view.
    int showWatch = !theWatcherNames.isEmpty();
    int showReturn = m_model->m_returnRoot->childCount() != 0;
    Internal::updateWatchersWindow(showWatch, showReturn);
}

QStringList WatchHandler::watchedExpressions()
{
    // Filter out invalid watchers.
    QStringList watcherNames;
    QHashIterator<QByteArray, int> it(theWatcherNames);
    while (it.hasNext()) {
        it.next();
        const QByteArray &watcherName = it.key();
        if (!watcherName.isEmpty())
            watcherNames.push_back(QLatin1String(watcherName));
    }
    return watcherNames;
}

void WatchHandler::saveSessionData()
{
    saveWatchers();
    saveFormats();
}

void WatchHandler::loadSessionData()
{
    loadFormats();
    theWatcherNames.clear();
    theWatcherCount = 0;
    QVariant value = sessionValue("Watchers");
    m_model->m_watchRoot->removeChildren();
    foreach (const QString &exp, value.toStringList())
        watchExpression(exp.trimmed());
}

WatchModelBase *WatchHandler::model() const
{
    return m_model;
}

const WatchItem *WatchHandler::watchItem(const QModelIndex &idx) const
{
    return static_cast<WatchItem *>(m_model->itemForIndex(idx));
}

void WatchHandler::fetchMore(const QByteArray &iname) const
{
    if (WatchItem *item = m_model->findItem(iname))
        item->fetchMore();
}

WatchItem *WatchHandler::findItem(const QByteArray &iname) const
{
    return m_model->findItem(iname);
}

const WatchItem *WatchHandler::findCppLocalVariable(const QString &name) const
{
    // Can this be found as a local variable?
    const QByteArray localsPrefix("local.");
    QByteArray iname = localsPrefix + name.toLatin1();
    if (const WatchItem *item = findItem(iname))
        return item;
//    // Nope, try a 'local.this.m_foo'.
//    iname.insert(localsPrefix.size(), "this.");
//    if (const WatchData *wd = findData(iname))
//        return wd;
    return 0;
}

void WatchModel::setTypeFormat(const QByteArray &type0, int format)
{
    const QByteArray type = stripForFormat(type0);
    if (format == AutomaticFormat)
        theTypeFormats.remove(type);
    else
        theTypeFormats[type] = format;
    saveFormats();
    m_engine->updateAll();
}

void WatchModel::setIndividualFormat(const QByteArray &iname, int format)
{
    if (format == AutomaticFormat)
        theIndividualFormats.remove(iname);
    else
        theIndividualFormats[iname] = format;
    saveFormats();
}

int WatchHandler::format(const QByteArray &iname) const
{
    int result = AutomaticFormat;
    if (const WatchItem *item = m_model->findItem(iname)) {
        result = theIndividualFormats.value(item->iname, AutomaticFormat);
        if (result == AutomaticFormat)
            result = theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);
    }
    return result;
}

QString WatchHandler::nameForFormat(int format)
{
    return WatchModel::nameForFormat(format);
}

QByteArray WatchHandler::typeFormatRequests() const
{
    QByteArray ba;
    if (!theTypeFormats.isEmpty()) {
        QHashIterator<QByteArray, int> it(theTypeFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format != AutomaticFormat) {
                ba.append(it.key().toHex());
                ba.append('=');
                ba.append(QByteArray::number(format));
                ba.append(',');
            }
        }
        ba.chop(1);
    }
    return ba;
}

QByteArray WatchHandler::individualFormatRequests() const
{
    QByteArray ba;
    if (!theIndividualFormats.isEmpty()) {
        QHashIterator<QByteArray, int> it(theIndividualFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format != AutomaticFormat) {
                ba.append(it.key());
                ba.append('=');
                ba.append(QByteArray::number(it.value()));
                ba.append(',');
            }
        }
        ba.chop(1);
    }
    return ba;
}

void WatchHandler::appendFormatRequests(DebuggerCommand *cmd)
{
    QJsonArray expanded;
    QSetIterator<QByteArray> jt(m_model->m_expandedINames);
    while (jt.hasNext())
        expanded.append(QLatin1String(jt.next()));

    cmd->arg("expanded", expanded);

    QJsonObject typeformats;
    QHashIterator<QByteArray, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != AutomaticFormat)
            typeformats.insert(QLatin1String(it.key()), format);
    }
    cmd->arg("typeformats", typeformats);

    QJsonObject formats;
    QHashIterator<QByteArray, int> it2(theIndividualFormats);
    while (it2.hasNext()) {
        it2.next();
        const int format = it2.value();
        if (format != AutomaticFormat)
            formats.insert(QLatin1String(it2.key()), format);
    }
    cmd->arg("formats", formats);
}

static inline QJsonObject watcher(const QByteArray &iname, const QByteArray &exp)
{
    QJsonObject watcher;
    watcher.insert(QStringLiteral("iname"), QLatin1String(iname));
    watcher.insert(QStringLiteral("exp"), QLatin1String(exp.toHex()));
    return watcher;
}

void WatchHandler::appendWatchersAndTooltipRequests(DebuggerCommand *cmd)
{
    QJsonArray watchers;
    DebuggerToolTipContexts toolTips = DebuggerToolTipManager::pendingTooltips(m_model->m_engine);
    foreach (const DebuggerToolTipContext &p, toolTips)
        watchers.append(watcher(p.iname, p.expression.toLatin1()));

    QHashIterator<QByteArray, int> it(WatchHandler::watcherNames());
    while (it.hasNext()) {
        it.next();
        watchers.append(watcher("watch." + QByteArray::number(it.value()), it.key()));
    }
    cmd->arg("watchers", watchers);
}

void WatchHandler::addDumpers(const GdbMi &dumpers)
{
    foreach (const GdbMi &dumper, dumpers.children()) {
        DisplayFormats formats;
        formats.append(RawFormat);
        QByteArray reportedFormats = dumper["formats"].data();
        foreach (const QByteArray &format, reportedFormats.split(',')) {
            if (int f = format.toInt())
                formats.append(DisplayFormat(f));
        }
        addTypeFormats(dumper["type"].data(), formats);
    }
}

void WatchHandler::addTypeFormats(const QByteArray &type, const DisplayFormats &formats)
{
    m_model->m_reportedTypeFormats.insert(QLatin1String(stripForFormat(type)), formats);
}

static void showInEditorHelper(const WatchItem *item, QTextStream &ts, int depth)
{
    const QChar tab = QLatin1Char('\t');
    const QChar nl = QLatin1Char('\n');
    ts << QString(depth, tab) << item->name << tab << item->value << tab
       << item->type << nl;
    foreach (const TreeItem *child, item->children())
        showInEditorHelper(static_cast<const WatchItem *>(child), ts, depth + 1);
}

QString WatchHandler::editorContents()
{
    QString contents;
    QTextStream ts(&contents);
    showInEditorHelper(m_model->root(), ts, 0);
    return contents;
}

void WatchHandler::scheduleResetLocation()
{
    m_model->m_contentsValid = false;
    m_model->m_resetLocationScheduled = true;
}

void WatchHandler::resetLocation()
{
    m_model->m_resetLocationScheduled = false;
}

void WatchHandler::setCurrentItem(const QByteArray &iname)
{
    if (WatchItem *item = m_model->findItem(iname)) {
        QModelIndex idx = m_model->indexForItem(item);
        emit m_model->currentIndexRequested(idx);
    }
}

QHash<QByteArray, int> WatchHandler::watcherNames()
{
    return theWatcherNames;
}

void WatchHandler::setUnprintableBase(int base)
{
    theUnprintableBase = base;
    m_model->layoutChanged();
}

int WatchHandler::unprintableBase()
{
    return theUnprintableBase;
}

bool WatchHandler::isExpandedIName(const QByteArray &iname) const
{
    return m_model->m_expandedINames.contains(iname);
}

QSet<QByteArray> WatchHandler::expandedINames() const
{
    return m_model->m_expandedINames;
}

////////////////////////////////////////////////////////////////////
//
// WatchItem
//
////////////////////////////////////////////////////////////////////

WatchItem::WatchItem(const QByteArray &i, const QString &n)
{
    iname = i;
    name = n;
}

WatchItem::WatchItem(const WatchData &data)
    : WatchData(data)
{
}

WatchItem::WatchItem(const GdbMi &data)
{
    iname = data["iname"].data();

    GdbMi wname = data["wname"];
    if (wname.isValid()) // Happens (only) for watched expressions.
        name = QString::fromUtf8(QByteArray::fromHex(wname.data()));
    else
        name = QString::fromLatin1(data["name"].data());

    parseWatchData(data);

    if (wname.isValid())
        exp = name.toUtf8();
}

WatchItem *WatchItem::parentItem() const
{
    return dynamic_cast<WatchItem *>(parent());
}

const WatchModel *WatchItem::watchModel() const
{
    return static_cast<const WatchModel *>(model());
}

WatchModel *WatchItem::watchModel()
{
    return static_cast<WatchModel *>(model());
}

void WatchItem::parseWatchData(const GdbMi &input)
{
    auto itemHandler = [this](const WatchData &data) {
        static_cast<WatchData *>(this)->operator=(data); // FIXME with 3.5
    };

    auto childHandler = [this](const WatchData &innerData, const GdbMi &innerInput) {
        WatchItem *item = new WatchItem(innerData);
        item->parseWatchData(innerInput);
        appendChild(item);
    };

    auto itemAdder = [this](const WatchData &data) {
        appendChild(new WatchItem(data));
    };

    auto arrayDecoder = [itemAdder](const WatchData &childTemplate,
            const QByteArray &encodedData, int encoding) {
        decodeArrayData(itemAdder, childTemplate, encodedData, encoding);
    };

    parseChildrenData(*this, input, itemHandler, childHandler, arrayDecoder);
}

} // namespace Internal
} // namespace Debugger
