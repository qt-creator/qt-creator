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
#include "simplifytype.h"
#include "imageviewer.h"
#include "watchutils.h"

#include <coreplugin/icore.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/checkablemessagebox.h>

#include <QDebug>
#include <QFile>
#include <QProcess>
#include <QTabWidget>
#include <QTextEdit>

#include <QTimer>
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

///////////////////////////////////////////////////////////////////////
//
// SeparatedView
//
///////////////////////////////////////////////////////////////////////

class SeparatedView : public QTabWidget
{
    Q_OBJECT

public:
    SeparatedView() : QTabWidget(Internal::mainWindow())
    {
        setTabsClosable(true);
        connect(this, &QTabWidget::tabCloseRequested, this, &SeparatedView::closeTab);
        setWindowFlags(windowFlags() | Qt::Window);
        setWindowTitle(WatchHandler::tr("Debugger - Qt Creator"));

        QVariant geometry = sessionValue("DebuggerSeparateWidgetGeometry");
        if (geometry.isValid())
            setGeometry(geometry.toRect());
    }

    ~SeparatedView()
    {
        setSessionValue("DebuggerSeparateWidgetGeometry", geometry());
    }

    void removeObject(const QByteArray &key)
    {
        if (QWidget *w = findWidget(key)) {
            removeTab(indexOf(w));
            sanitize();
        }
    }

    void closeTab(int index)
    {
        if (QObject *o = widget(index)) {
            QByteArray iname = o->property(INameProperty).toByteArray();
            theIndividualFormats.remove(iname);
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

    template <class T> T *prepareObject(const QByteArray &key, const QString &title)
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
            addTab(t, title);
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
public:
    explicit WatchModel(WatchHandler *handler);

    static QString nameForFormat(int format);
    TypeFormatList typeFormatList(const WatchData &value) const;

    bool setData(const QModelIndex &idx, const QVariant &value, int role);

    void insertDataItem(const WatchData &data);
    void reinsertAllData();
    void reinsertAllDataHelper(WatchItem *item, QList<WatchData> *data);
    QString displayForAutoTest(const QByteArray &iname) const;
    void reinitialize(bool includeInspectData = false);

    friend QDebug operator<<(QDebug d, const WatchModel &m);

    void showInEditorHelper(QString *contents, WatchItem *item, int level);
    void setCurrentItem(const QByteArray &iname);

    QString removeNamespaces(QString str) const;
    DebuggerEngine *engine() const;
    bool contentIsValid() const;

    WatchHandler *m_handler; // Not owned.

    WatchItem *root() const { return static_cast<WatchItem *>(rootItem()); }
    WatchItem *m_localsRoot; // Not owned.
    WatchItem *m_inspectorRoot; // Not owned.
    WatchItem *m_watchRoot; // Not owned.
    WatchItem *m_returnRoot; // Not owned.
    WatchItem *m_tooltipRoot; // Not owned.

    QSet<QByteArray> m_expandedINames;

    TypeFormatList builtinTypeFormatList(const WatchData &data) const;
    QStringList dumperTypeFormatList(const WatchData &data) const;
    DumperTypeFormats m_reportedTypeFormats;

    WatchItem *createItem(const QByteArray &iname);
    WatchItem *findItem(const QByteArray &iname) const;
    friend class WatchItem;
    typedef QHash<QByteArray, QString> ValueCache;
    ValueCache m_valueCache;

    void insertItem(WatchItem *item);
    void reexpandItems();
};

WatchModel::WatchModel(WatchHandler *handler)
    : m_handler(handler)
{
    setObjectName(QLatin1String("WatchModel"));

    setHeader(QStringList() << tr("Name") << tr("Value") << tr("Type"));
    auto root = new WatchItem;
    root->appendChild(m_localsRoot = new WatchItem("local", tr("Locals")));
    root->appendChild(m_inspectorRoot = new WatchItem("inspect", tr("Inspector")));
    root->appendChild(m_watchRoot = new WatchItem("watch", tr("Expressions")));
    root->appendChild(m_returnRoot = new WatchItem("return", tr("Return Value")));
    root->appendChild(m_tooltipRoot = new WatchItem("tooltip", tr("Tooltip")));
    setRootItem(root);

    connect(action(SortStructMembers), &SavedAction::valueChanged,
        this, &WatchModel::reinsertAllData);
    connect(action(ShowStdNamespace), &SavedAction::valueChanged,
        this, &WatchModel::reinsertAllData);
    connect(action(ShowQtNamespace), &SavedAction::valueChanged,
        this, &WatchModel::reinsertAllData);
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

DebuggerEngine *WatchModel::engine() const
{
    return m_handler->m_engine;
}

WatchItem *WatchModel::findItem(const QByteArray &iname) const
{
    return root()->findItem(iname);
}

WatchItem *WatchItem::findItem(const QByteArray &iname)
{
    if (d.iname == iname)
        return this;
    foreach (TreeItem *child, children()) {
        auto witem = static_cast<WatchItem *>(child);
        if (witem->d.iname == iname)
            return witem;
        if (witem->d.isAncestorOf(iname))
            return witem->findItem(iname);
    }
    return 0;
}

void WatchModel::reinsertAllData()
{
    QList<WatchData> list;
    foreach (TreeItem *child, rootItem()->children())
        reinsertAllDataHelper(static_cast<WatchItem *>(child), &list);
    reinitialize(true);
    for (int i = 0, n = list.size(); i != n; ++i)
        insertDataItem(list.at(i));
}

void WatchModel::reinsertAllDataHelper(WatchItem *item, QList<WatchData> *data)
{
    data->append(item->d);
    data->back().setAllUnneeded();
    foreach (TreeItem *child, item->children())
        reinsertAllDataHelper(static_cast<WatchItem *>(child), data);
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
        const QString qtNamespace = QString::fromLatin1(engine()->qtNamespace());
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
static QString reformatCharacter(int code, int format)
{
    const QString codeS = reformatInteger(code, format, 1, true);
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

static QString translate(const QString &str)
{
    if (str.startsWith(QLatin1Char('<'))) {
        if (str == QLatin1String("<empty>"))
            return WatchHandler::tr("<empty>");
        if (str == QLatin1String("<uninitialized>"))
            return WatchHandler::tr("<uninitialized>");
        if (str == QLatin1String("<invalid>"))
            return WatchHandler::tr("<invalid>");
        if (str == QLatin1String("<not accessible>"))
            return WatchHandler::tr("<not accessible>");
        if (str.endsWith(QLatin1String(" items>"))) {
            // '<10 items>' or '<>10 items>' (more than)
            bool ok;
            const bool moreThan = str.at(1) == QLatin1Char('>');
            const int numberPos = moreThan ? 2 : 1;
            const int len = str.indexOf(QLatin1Char(' ')) - numberPos;
            const int size = str.mid(numberPos, len).toInt(&ok);
            QTC_ASSERT(ok, qWarning("WatchHandler: Invalid item count '%s'",
                qPrintable(str)));
            return moreThan ?
                     WatchHandler::tr("<more than %n items>", 0, size) :
                     WatchHandler::tr("<%n items>", 0, size);
        }
    }
    return quoteUnprintable(str);
}

QString WatchItem::formattedValue() const
{
    if (d.type == "bool") {
        if (d.value == QLatin1String("0"))
            return QLatin1String("false");
        if (d.value == QLatin1String("1"))
            return QLatin1String("true");
        return d.value;
    }

    const int format = itemFormat();

    // Append quoted, printable character also for decimal.
    if (d.type.endsWith("char") || d.type.endsWith("QChar")) {
        bool ok;
        const int code = d.value.toInt(&ok);
        return ok ? reformatCharacter(code, format) : d.value;
    }

    if (format == HexadecimalIntegerFormat
            || format == DecimalIntegerFormat
            || format == OctalIntegerFormat
            || format == BinaryIntegerFormat) {
        bool isSigned = d.value.startsWith(QLatin1Char('-'));
        quint64 raw = isSigned ? quint64(d.value.toLongLong()) : d.value.toULongLong();
        return reformatInteger(raw, format, d.size, isSigned);
    }

    if (format == ScientificFloatFormat) {
        double dd = d.value.toDouble();
        return QString::number(dd, 'e');
    }

    if (format == CompactFloatFormat) {
        double dd = d.value.toDouble();
        return QString::number(dd, 'g');
    }

    if (d.type == "va_list")
        return d.value;

    if (!isPointerType(d.type) && !d.isVTablePointer()) {
        bool ok = false;
        qulonglong integer = d.value.toULongLong(&ok, 0);
        if (ok) {
            const int format = itemFormat();
            return reformatInteger(integer, format, d.size, false);
        }
    }

    if (d.elided) {
        QString v = d.value;
        v.chop(1);
        v = translate(v);
        QString len = d.elided > 0 ? QString::number(d.elided)
                                      : QLatin1String("unknown length");
        return v + QLatin1String("\"... (") + len  + QLatin1Char(')');
    }

    return translate(d.value);
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
    if (d.type == "bool")
        return QVariant::Bool;
    if (isIntType(d.type))
        return d.type.contains('u') ? QVariant::ULongLong : QVariant::LongLong;
    if (isFloatType(d.type))
        return QVariant::Double;
    // Check for pointers using hex values (0xAD00 "Hallo")
    if (isPointerType(d.type) && d.value.startsWith(QLatin1String("0x")))
        return QVariant::ULongLong;
   return QVariant::String;
}

// Convert to editable (see above)
QVariant WatchItem::editValue() const
{
    switch (editType()) {
    case QVariant::Bool:
        return d.value != QLatin1String("0") && d.value != QLatin1String("false");
    case QVariant::ULongLong:
        if (isPointerType(d.type)) // Fix pointer values (0xAD00 "Hallo" -> 0xAD00)
            return QVariant(pointerValue(d.value));
        return QVariant(d.value.toULongLong());
    case QVariant::LongLong:
        return QVariant(d.value.toLongLong());
    case QVariant::Double:
        return QVariant(d.value.toDouble());
    default:
        break;
    }
    // Some string value: '0x434 "Hallo"':
    // Remove quotes and replace newlines, which will cause line edit troubles.
    QString stringValue = d.value;
    if (stringValue.endsWith(QLatin1Char('"'))) {
        const int leadingDoubleQuote = stringValue.indexOf(QLatin1Char('"'));
        if (leadingDoubleQuote != stringValue.size() - 1) {
            stringValue.truncate(stringValue.size() - 1);
            stringValue.remove(0, leadingDoubleQuote + 1);
            stringValue.replace(QLatin1String("\n"), QLatin1String("\\n"));
        }
    }
    return QVariant(translate(stringValue));
}

bool WatchItem::canFetchMore() const
{
    if (!d.hasChildren)
        return false;
    if (!watchModel())
        return false;
    if (!watchModel()->contentIsValid() && !d.isInspect())
        return false;
    return !fetchTriggered;
}

void WatchItem::fetchMore()
{
    if (fetchTriggered)
        return;

    watchModel()->m_expandedINames.insert(d.iname);
    fetchTriggered = true;
    if (children().isEmpty()) {
        d.setChildrenNeeded();
        watchModel()->engine()->updateWatchData(d);
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
    const int individualFormat = theIndividualFormats.value(d.iname, AutomaticFormat);
    if (individualFormat != AutomaticFormat)
        return individualFormat;
    return theTypeFormats.value(stripForFormat(d.type), AutomaticFormat);
}

bool WatchModel::contentIsValid() const
{
    // FIXME:
    // inspector doesn't follow normal beginCycle()/endCycle()
    //if (m_type == InspectWatch)
    //    return true;
    return m_handler->m_contentsValid;
}

QString WatchItem::expression() const
{
    if (!d.exp.isEmpty())
         return QString::fromLatin1(d.exp);
    if (d.address && !d.type.isEmpty()) {
        return QString::fromLatin1("*(%1*)%2").
                arg(QLatin1String(d.type), QLatin1String(d.hexAddress()));
    }
    if (const WatchItem *p = parentItem()) {
        if (!p->d.exp.isEmpty())
           return QString::fromLatin1("(%1).%2").arg(QString::fromLatin1(p->d.exp), d.name);
    }
    return d.name;
}

QString WatchItem::displayName() const
{
    QString result;
    if (!parentItem())
        return result;
    if (d.iname.startsWith("return"))
        result = WatchModel::tr("returned value");
    else if (d.name == QLatin1String("*"))
        result = QLatin1Char('*') + parentItem()->d.name;
    else
        result = watchModel()->removeNamespaces(d.name);

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
    if (result.isEmpty() && d.address)
        result += QString::fromLatin1("@0x" + QByteArray::number(d.address, 16));
//    if (d.origaddr)
//        result += QString::fromLatin1(" (0x" + QByteArray::number(d.origaddr, 16) + ')');
    return result;
}

QString WatchItem::displayType() const
{
    QString result = d.displayedType.isEmpty()
        ? niceTypeHelper(d.type)
        : d.displayedType;
    if (d.bitsize)
        result += QString::fromLatin1(":%1").arg(d.bitsize);
    result.remove(QLatin1Char('\''));
    result = watchModel()->removeNamespaces(result);
    return result;
}

QColor WatchItem::valueColor() const
{
    static const QColor red(200, 0, 0);
    static const QColor gray(140, 140, 140);
    if (watchModel()) {
        if (!d.valueEnabled)
            return gray;
        if (!watchModel()->contentIsValid() && !d.isInspect())
            return gray;
        if (d.value.isEmpty()) // This might still show 0x...
            return gray;
        if (d.value != watchModel()->m_valueCache.value(d.iname))
            return red;
    }
    return QColor();
}

QVariant WatchItem::data(int column, int role) const
{
    switch (role) {
        case LocalsEditTypeRole:
            return QVariant(editType());

        case LocalsNameRole:
            return QVariant(d.name);

        case LocalsIntegerBaseRole:
            if (isPointerType(d.type)) // Pointers using 0x-convention
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
                    if (!d.displayedType.isEmpty())
                        return d.displayedType;
                    return QString::fromUtf8(d.type);
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
                ? d.toToolTip() : QVariant();

        case Qt::ForegroundRole:
            if (column == 1)
                return valueColor();

        case LocalsExpressionRole:
            return expression();

        case LocalsRawExpressionRole:
            return d.exp;

        case LocalsINameRole:
            return d.iname;

        case LocalsExpandedRole:
            return watchModel()->m_expandedINames.contains(d.iname);

        case LocalsTypeFormatListRole:
            return QVariant::fromValue(watchModel()->typeFormatList(d));

        case LocalsTypeRole:
            return watchModel()->removeNamespaces(displayType());

        case LocalsRawTypeRole:
            return QString::fromLatin1(d.type);

        case LocalsTypeFormatRole:
            return theTypeFormats.value(stripForFormat(d.type), AutomaticFormat);

        case LocalsIndividualFormatRole:
            return theIndividualFormats.value(d.iname, AutomaticFormat);

        case LocalsRawValueRole:
            return d.value;

        case LocalsObjectAddressRole:
            return d.address;

        case LocalsPointerAddressRole:
            return d.origaddr;

        case LocalsIsWatchpointAtObjectAddressRole: {
            BreakpointParameters bp(WatchpointAtAddress);
            bp.address = d.address;
            return watchModel()->engine()->breakHandler()->findWatchpoint(bp) != 0;
        }

        case LocalsSizeRole:
            return QVariant(d.size);

        case LocalsIsWatchpointAtPointerAddressRole:
            if (isPointerType(d.type)) {
                BreakpointParameters bp(WatchpointAtAddress);
                bp.address = pointerValue(d.value);
                return watchModel()->engine()->breakHandler()->findWatchpoint(bp) != 0;
            }
            return false;

        default:
            break;
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    if (!idx.isValid())
        return false; // Triggered by ModelTester.

    WatchItem *item = static_cast<WatchItem *>(itemFromIndex(idx));
    QTC_ASSERT(item, return false);
    const WatchData &data = item->d;

    switch (role) {
        case Qt::EditRole:
            switch (idx.column()) {
            case 0: {
                QByteArray exp = value.toByteArray();
                if (!exp.isEmpty()) {
                    theWatcherNames.remove(item->d.exp);
                    item->d.exp = exp;
                    item->d.name = QString::fromLatin1(exp);
                    theWatcherNames[exp] = theWatcherCount++;
                    m_handler->saveWatchers();
                    if (engine()->state() == DebuggerNotReady) {
                        item->d.setAllUnneeded();
                        item->d.setValue(QString(QLatin1Char(' ')));
                        item->d.setHasChildren(false);
                    } else {
                        engine()->updateWatchData(item->d);
                    }
                }
                m_handler->updateWatchersWindow();
                break;
            }
            case 1: // Change value
                engine()->assignValueInDebugger(&data, item->expression(), value);
                break;
            case 2: // TODO: Implement change type.
                engine()->assignValueInDebugger(&data, item->expression(), value);
                break;
            }
        case LocalsExpandedRole:
            if (value.toBool()) {
                // Should already have been triggered by fetchMore()
                //QTC_CHECK(m_expandedINames.contains(data.iname));
                m_expandedINames.insert(data.iname);
            } else {
                m_expandedINames.remove(data.iname);
            }
            emit columnAdjustmentRequested();
            break;

        case LocalsTypeFormatRole:
            m_handler->setFormat(data.type, value.toInt());
            engine()->updateWatchData(data);
            break;

        case LocalsIndividualFormatRole: {
            const int format = value.toInt();
            if (format == AutomaticFormat)
                theIndividualFormats.remove(data.iname);
            else
                theIndividualFormats[data.iname] = format;
            engine()->updateWatchData(data);
            break;
        }
    }

    //emit dataChanged(idx, idx);
    return true;
}

Qt::ItemFlags WatchItem::flags(int column) const
{
    QTC_ASSERT(model(), return Qt::ItemFlags());
    if (!watchModel()->contentIsValid() && !d.isInspect())
        return Qt::ItemFlags();

    // Enabled, editable, selectable, checkable, and can be used both as the
    // source of a drag and drop operation and as a drop target.

    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    const Qt::ItemFlags editable = notEditable | Qt::ItemIsEditable;

    // Disable editing if debuggee is positively running except for Inspector data
    DebuggerEngine *engine = watchModel()->engine();
    const bool isRunning = engine && engine->state() == InferiorRunOk;
    if (isRunning && engine && !engine->hasCapability(AddWatcherWhileRunningCapability) &&
            !d.isInspect())
        return notEditable;

    if (d.isWatcher()) {
        if (column == 0 && d.iname.count('.') == 1)
            return editable; // Watcher names are editable.

        if (!d.name.isEmpty()) {
            // FIXME: Forcing types is not implemented yet.
            //if (idx.column() == 2)
            //    return editable; // Watcher types can be set by force.
            if (column == 1 && d.valueEditable)
                return editable; // Watcher values are sometimes editable.
        }
    } else if (d.isLocal()) {
        if (column == 1 && d.valueEditable)
            return editable; // Locals values are sometimes editable.
    } else if (d.isInspect()) {
        if (column == 1 && d.valueEditable)
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
        case RawFormat: return tr("Raw Data");
        case Latin1StringFormat: return tr("Latin1 String");
        case Utf8StringFormat: return tr("UTF-8 String");
        case Local8BitStringFormat: return tr("Local 8-Bit String");
        case Utf16StringFormat: return tr("UTF-16 String");
        case Ucs4StringFormat: return tr("UCS-4 String");
        case Array10Format: return msgArrayFormat(10);
        case Array100Format: return msgArrayFormat(100);
        case Array1000Format: return msgArrayFormat(1000);
        case Array10000Format: return msgArrayFormat(10000);
        case SeparateLatin1StringFormat: return tr("Latin1 String in Separate Window");
        case SeparateUtf8StringFormat: return tr("UTF-8 String in Separate Window");
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

TypeFormatList WatchModel::typeFormatList(const WatchData &data) const
{
    TypeFormatList formats;

    // Types supported by dumpers:
    // Hack: Compensate for namespaces.
    QString type = QLatin1String(stripForFormat(data.type));
    int pos = type.indexOf(QLatin1String("::Q"));
    if (pos >= 0 && type.count(QLatin1Char(':')) == 2)
        type.remove(0, pos + 2);
    pos = type.indexOf(QLatin1Char('<'));
    if (pos >= 0)
        type.truncate(pos);
    type.replace(QLatin1Char(':'), QLatin1Char('_'));
    QStringList reported = m_reportedTypeFormats.value(type);
    for (int i = 0, n = reported.size(); i != n; ++i)
        formats.append(TypeFormatItem(reported.at(i), i));

    // Fixed artificial string and pointer types.
    if (data.origaddr || isPointerType(data.type)) {
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
    } else if (data.type.contains("char[") || data.type.contains("char [")) {
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
    data.value.toDouble(&ok);
    if (ok) {
        formats.append(CompactFloatFormat);
        formats.append(ScientificFloatFormat);
    }

    // Fixed artificial integral types.
    QString v = data.value;
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

// Determine sort order of watch items by sort order or alphabetical inames
// according to setting 'SortStructMembers'. We need a map key for insertBulkData
// and a predicate for finding the insertion position of a single item.

// Set this before using any of the below according to action
static bool sortWatchDataAlphabetically = true;

static bool watchDataLessThan(const QByteArray &iname1, int sortId1,
    const QByteArray &iname2, int sortId2)
{
    if (!sortWatchDataAlphabetically)
        return sortId1 < sortId2;
    // Get positions of last part of iname 'local.this.i1" -> "i1"
    int cmpPos1 = iname1.lastIndexOf('.');
    if (cmpPos1 == -1)
        cmpPos1 = 0;
    else
        cmpPos1++;
    int cmpPos2 = iname2.lastIndexOf('.');
    if (cmpPos2 == -1)
        cmpPos2 = 0;
    else
        cmpPos2++;
    // Are we looking at an array with numerical inames 'local.this.i1.0" ->
    // Go by sort id.
    if (cmpPos1 < iname1.size() && cmpPos2 < iname2.size()
            && isdigit(iname1.at(cmpPos1)) && isdigit(iname2.at(cmpPos2)))
        return sortId1 < sortId2;
    // Alphabetically
    return qstrcmp(iname1.constData() + cmpPos1, iname2.constData() + cmpPos2) < 0;
}

// Sort key for watch data consisting of iname and numerical sort id.
struct WatchDataSortKey
{
    explicit WatchDataSortKey(const WatchData &wd)
        : iname(wd.iname), sortId(wd.sortId) {}
    QByteArray iname;
    int sortId;
};

inline bool operator<(const WatchDataSortKey &k1, const WatchDataSortKey &k2)
{
    return watchDataLessThan(k1.iname, k1.sortId, k2.iname, k2.sortId);
}

bool watchItemSorter(const TreeItem *item1, const TreeItem *item2)
{
    const WatchItem *it1 = static_cast<const WatchItem *>(item1);
    const WatchItem *it2 = static_cast<const WatchItem *>(item2);
    return watchDataLessThan(it1->d.iname, it1->d.sortId, it2->d.iname, it2->d.sortId);
}

static int findInsertPosition(const QVector<TreeItem *> &list, const WatchItem *item)
{
    sortWatchDataAlphabetically = boolSetting(SortStructMembers);
    const auto it = qLowerBound(list.begin(), list.end(), item, watchItemSorter);
    return it - list.begin();
}

int WatchItem::requestedFormat() const
{
    int format = theIndividualFormats.value(d.iname, AutomaticFormat);
    if (format == AutomaticFormat)
        format = theTypeFormats.value(stripForFormat(d.type), AutomaticFormat);
    return format;
}

void WatchItem::showInEditorHelper(QString *contents, int depth) const
{
    const QChar tab = QLatin1Char('\t');
    const QChar nl = QLatin1Char('\n');
    contents->append(QString(depth, tab));
    contents->append(d.name);
    contents->append(tab);
    contents->append(d.value);
    contents->append(tab);
    contents->append(QString::fromLatin1(d.type));
    contents->append(nl);
    foreach (const TreeItem *child, children())
        static_cast<const WatchItem *>(child)->showInEditorHelper(contents, depth + 1);
}

void WatchModel::setCurrentItem(const QByteArray &iname)
{
    if (WatchItem *item = findItem(iname)) {
        QModelIndex idx = indexFromItem(item);
        emit currentIndexRequested(idx);
    }
}

///////////////////////////////////////////////////////////////////////
//
// WatchHandler
//
///////////////////////////////////////////////////////////////////////

WatchHandler::WatchHandler(DebuggerEngine *engine)
{
    m_engine = engine;
    m_model = new WatchModel(this);
    m_contentsValid = false;
    m_contentsValid = true; // FIXME
    m_resetLocationScheduled = false;
    m_separatedView = new SeparatedView;
    m_requestUpdateTimer = new QTimer(this);
    m_requestUpdateTimer->setSingleShot(true);
    connect(m_requestUpdateTimer, &QTimer::timeout, m_model, &WatchModel::updateRequested);
}

WatchHandler::~WatchHandler()
{
    delete m_separatedView;
    m_separatedView = 0;
    // Do it manually to prevent calling back in model destructors
    // after m_cache is destroyed.
    delete m_model;
    m_model = 0;
}

void WatchHandler::cleanup()
{
    m_model->m_expandedINames.clear();
    theWatcherNames.remove(QByteArray());
    m_model->reinitialize();
    emit m_model->updateFinished();
    m_separatedView->hide();
}

void WatchHandler::insertItem(WatchItem *item)
{
    m_model->insertItem(item);
}

void WatchModel::insertItem(WatchItem *item)
{
    WatchItem *existing = findItem(item->d.iname);
    if (existing)
        removeItem(existing);

    WatchItem *parent = findItem(parentName(item->d.iname));
    QTC_ASSERT(parent, return);
    const int row = findInsertPosition(parent->children(), item);
    parent->insertChild(row, item);
}

void WatchModel::insertDataItem(const WatchData &data)
{
    QTC_ASSERT(!data.iname.isEmpty(), qDebug() << data.toString(); return);

    if (WatchItem *item = findItem(data.iname)) {
        // Remove old children.
        item->fetchTriggered = false;
        item->removeChildren();

        // Overwrite old entry.
        item->d = data;
        item->update();
    } else {
        // Add new entry.
        WatchItem *parent = findItem(parentName(data.iname));
        QTC_ASSERT(parent, return);
        WatchItem *newItem = new WatchItem;
        newItem->d = data;
        const int row = findInsertPosition(parent->children(), newItem);
        parent->insertChild(row, newItem);
        if (m_expandedINames.contains(parent->d.iname)) {
            emit inameIsExpanded(parent->d.iname);
            emit itemIsExpanded(indexFromItem(parent));
        }
    }
    m_handler->showEditValue(data);
}

void WatchModel::reexpandItems()
{
    foreach (const QByteArray &iname, m_expandedINames) {
        if (WatchItem *item = findItem(iname)) {
            emit itemIsExpanded(indexFromItem(item));
            emit inameIsExpanded(iname);
        } else {
            // Can happen. We might have stepped into another frame
            // not containing that iname, but we still like to
            // remember the expanded state of iname in case we step
            // out of the frame again.
        }
    }
}

void WatchHandler::insertData(const WatchData &data)
{
    m_model->insertDataItem(data);
    m_contentsValid = true;
    updateWatchersWindow();
}

void WatchHandler::insertDataList(const QList<WatchData> &list)
{
    for (int i = 0, n = list.size(); i != n; ++i)
        m_model->insertDataItem(list.at(i));
    m_contentsValid = true;
    updateWatchersWindow();
}

void WatchHandler::removeAllData(bool includeInspectData)
{
    m_model->reinitialize(includeInspectData);
    updateWatchersWindow();
}

void WatchHandler::resetValueCache()
{
    m_model->m_valueCache.clear();
    TreeItem *root = m_model->rootItem();
    root->walkTree([this, root](TreeItem *item) {
        auto watchItem = static_cast<WatchItem *>(item);
        m_model->m_valueCache[watchItem->d.iname] = watchItem->d.value;
    });
}

void WatchHandler::updateRequested()
{
    m_requestUpdateTimer->start(80);
}

void WatchHandler::updateFinished()
{
    m_requestUpdateTimer->stop();
    emit m_model->updateFinished();
}

void WatchHandler::purgeOutdatedItems(const QSet<QByteArray> &inames)
{
    foreach (const QByteArray &iname, inames) {
        WatchItem *item = findItem(iname);
        m_model->removeItem(item);
    }

    m_model->layoutChanged();
    m_model->reexpandItems();
    m_contentsValid = true;
    updateWatchersWindow();
}

void WatchHandler::removeItemByIName(const QByteArray &iname)
{
    WatchItem *item = m_model->findItem(iname);
    if (!item)
        return;
    if (item->d.isWatcher()) {
        theWatcherNames.remove(item->d.exp);
        saveWatchers();
    }
    m_model->removeItem(item);
    delete item;
    updateWatchersWindow();
}

QByteArray WatchHandler::watcherName(const QByteArray &exp)
{
    return "watch." + QByteArray::number(theWatcherNames[exp]);
}

void WatchHandler::watchExpression(const QString &exp0, const QString &name)
{
    QString exp = exp0;

    QTC_ASSERT(m_engine, return);
    // Do not insert the same entry more then once.
    if (theWatcherNames.value(exp.toLatin1()))
        return;

    // FIXME: 'exp' can contain illegal characters
    exp.replace(QLatin1Char('#'), QString());

    WatchData data;
    data.exp = exp.toLatin1();
    data.name = name.isEmpty() ? exp : name;
    theWatcherNames[data.exp] = theWatcherCount++;
    saveWatchers();

    if (exp.isEmpty())
        data.setAllUnneeded();
    data.iname = watcherName(data.exp);
    if (m_engine->state() == DebuggerNotReady) {
        data.setAllUnneeded();
        data.setValue(QString(QLatin1Char(' ')));
        data.setHasChildren(false);
        m_model->insertDataItem(data);
    } else {
        m_engine->updateWatchData(data);
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

void WatchHandler::showEditValue(const WatchData &data)
{
    const QByteArray key  = data.address ? data.hexAddress() : data.iname;
    switch (data.editformat) {
    case StopDisplay:
        m_separatedView->removeObject(data.iname);
        break;
    case DisplayImageData:
    case DisplayImageFile: {  // QImage
        int width = 0, height = 0, nbytes = 0, format = 0;
        QByteArray ba;
        uchar *bits = 0;
        if (data.editformat == DisplayImageData) {
            ba = QByteArray::fromHex(data.editvalue);
            QTC_ASSERT(ba.size() > 16, return);
            const int *header = (int *)(ba.data());
            if (!ba.at(0) && !ba.at(1)) // Check on 'width' for Python dumpers returning 4-byte swapped-data.
                swapEndian(ba.data(), 16);
            bits = 16 + (uchar *)(ba.data());
            width = header[0];
            height = header[1];
            nbytes = header[2];
            format = header[3];
        } else if (data.editformat == DisplayImageFile) {
            QTextStream ts(data.editvalue);
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
        const QString title = data.address ?
            tr("%1 Object at %2").arg(QLatin1String(data.type),
                QLatin1String(data.hexAddress())) :
            tr("%1 Object at Unknown Address").arg(QLatin1String(data.type));
        ImageViewer *v = m_separatedView->prepareObject<ImageViewer>(key, title);
        v->setProperty(INameProperty, data.iname);
        v->setImage(im);
        break;
    }
    case DisplayUtf16String:
    case DisplayLatin1String:
    case DisplayUtf8String: { // String data.
        QByteArray ba = QByteArray::fromHex(data.editvalue);
        QString str;
        if (data.editformat == DisplayUtf16String)
            str = QString::fromUtf16((ushort *)ba.constData(), ba.size()/2);
        else if (data.editformat == DisplayLatin1String)
            str = QString::fromLatin1(ba.constData(), ba.size());
        else if (data.editformat == DisplayUtf8String)
            str = QString::fromUtf8(ba.constData(), ba.size());
        QTextEdit *t = m_separatedView->prepareObject<QTextEdit>(key, data.name);
        t->setProperty(INameProperty, data.iname);
        t->setText(str);
        break;
    }
    default:
        QTC_ASSERT(false, qDebug() << "Display format: " << data.editformat);
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
    static int previousShowWatch = -1;
    static int previousShowReturn = -1;
    int showWatch = !m_model->m_watchRoot->children().isEmpty();
    int showReturn = !m_model->m_returnRoot->children().isEmpty();
    if (showWatch == previousShowWatch && showReturn == previousShowReturn)
        return;
    previousShowWatch = showWatch;
    previousShowReturn = showReturn;
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

void WatchHandler::saveWatchers()
{
    setSessionValue("Watchers", watchedExpressions());
}

void WatchHandler::loadFormats()
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

void WatchHandler::saveFormats()
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
        watchExpression(exp);
}

WatchModelBase *WatchHandler::model() const
{
    return m_model;
}

const WatchData *WatchHandler::watchData(const QModelIndex &idx) const
{
    TreeItem *item = m_model->itemFromIndex(idx);
    return item ? &static_cast<WatchItem *>(item)->d : 0;
}

void WatchHandler::fetchMore(const QByteArray &iname) const
{
    WatchItem *item = m_model->findItem(iname);
    if (item)
        item->fetchMore();
}

const WatchData *WatchHandler::findData(const QByteArray &iname) const
{
    const WatchItem *item = m_model->findItem(iname);
    return item ? &item->d : 0;
}

WatchItem *WatchHandler::findItem(const QByteArray &iname) const
{
    return m_model->findItem(iname);
}

const WatchData *WatchHandler::findCppLocalVariable(const QString &name) const
{
    // Can this be found as a local variable?
    const QByteArray localsPrefix("local.");
    QByteArray iname = localsPrefix + name.toLatin1();
    if (const WatchData *wd = findData(iname))
        return wd;
//    // Nope, try a 'local.this.m_foo'.
//    iname.insert(localsPrefix.size(), "this.");
//    if (const WatchData *wd = findData(iname))
//        return wd;
    return 0;
}

bool WatchHandler::hasItem(const QByteArray &iname) const
{
    return m_model->findItem(iname);
}

void WatchHandler::setFormat(const QByteArray &type0, int format)
{
    const QByteArray type = stripForFormat(type0);
    if (format == AutomaticFormat)
        theTypeFormats.remove(type);
    else
        theTypeFormats[type] = format;
    saveFormats();
    m_model->reinsertAllData();
}

int WatchHandler::format(const QByteArray &iname) const
{
    int result = AutomaticFormat;
    if (const WatchItem *item = m_model->findItem(iname)) {
        result = theIndividualFormats.value(item->d.iname, AutomaticFormat);
        if (result == AutomaticFormat)
            result = theTypeFormats.value(stripForFormat(item->d.type), AutomaticFormat);
    }
    return result;
}

QByteArray WatchHandler::typeFormatRequests() const
{
    QByteArray ba;
    if (!theTypeFormats.isEmpty()) {
        QHashIterator<QByteArray, int> it(theTypeFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format >= RawFormat && format < ArtificialFormatBase) {
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
            if (format >= RawFormat && format < ArtificialFormatBase) {
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
    cmd->beginList("expanded");
    QSetIterator<QByteArray> jt(m_model->m_expandedINames);
    while (jt.hasNext()) {
        QByteArray iname = jt.next();
        //WatchItem *item = m_model->findItem(iname);
        cmd->arg(iname);
        //cmd->arg("format", item->requestedFormat());
    }
    cmd->endList();

    cmd->beginGroup("typeformats");
    QHashIterator<QByteArray, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format >= RawFormat && format < ArtificialFormatBase)
            cmd->arg(it.key(), format);
    }
    cmd->endGroup();

    cmd->beginGroup("formats");
    QHashIterator<QByteArray, int> it2(theIndividualFormats);
    while (it2.hasNext()) {
        it2.next();
        const int format = it2.value();
        if (format >= RawFormat && format < ArtificialFormatBase)
            cmd->arg(it2.key(), format);
    }
    cmd->endGroup();
}

void WatchHandler::addDumpers(const GdbMi &dumpers)
{
    foreach (const GdbMi &dumper, dumpers.children()) {
        QStringList formats(tr("Raw structure"));
        foreach (const QByteArray &format, dumper["formats"].data().split(',')) {
            if (format == "Normal")
                formats.append(tr("Normal"));
            else if (format == "Displayed")
                formats.append(tr("Displayed"));
            else if (!format.isEmpty())
                formats.append(QString::fromLatin1(format));
        }
        addTypeFormats(dumper["type"].data(), formats);
    }
}

void WatchHandler::addTypeFormats(const QByteArray &type, const QStringList &formats)
{
    m_model->m_reportedTypeFormats.insert(QLatin1String(stripForFormat(type)), formats);
}

QString WatchHandler::editorContents()
{
    QString contents;
    m_model->root()->showInEditorHelper(&contents, 0);
    return contents;
}

void WatchHandler::setTypeFormats(const DumperTypeFormats &typeFormats)
{
    m_model->m_reportedTypeFormats = typeFormats;
}

DumperTypeFormats WatchHandler::typeFormats() const
{
    return m_model->m_reportedTypeFormats;
}

void WatchHandler::editTypeFormats(bool includeLocals, const QByteArray &iname)
{
    Q_UNUSED(includeLocals);
    TypeFormatsDialog dlg(0);

    //QHashIterator<QString, QStringList> it(m_reportedTypeFormats);
    QList<QString> l = m_model->m_reportedTypeFormats.keys();
    Utils::sort(l);
    foreach (const QString &ba, l) {
        int f = iname.isEmpty() ? AutomaticFormat : format(iname);
        dlg.addTypeFormats(ba, m_model->m_reportedTypeFormats.value(ba), f);
    }
    if (dlg.exec())
        setTypeFormats(dlg.typeFormats());
}

void WatchHandler::scheduleResetLocation()
{
    m_contentsValid = false;
    //m_contentsValid = true; // FIXME
    m_resetLocationScheduled = true;
}

void WatchHandler::resetLocation()
{
    m_resetLocationScheduled = false;
}

void WatchHandler::setCurrentItem(const QByteArray &iname)
{
    m_model->setCurrentItem(iname);
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
// TypeFormatItem/List
//
////////////////////////////////////////////////////////////////////

TypeFormatItem::TypeFormatItem(const QString &display, int format)
    : display(display), format(format)
{}

void TypeFormatList::append(int format)
{
    append(TypeFormatItem(WatchModel::nameForFormat(format), format));
}

TypeFormatItem TypeFormatList::find(int format) const
{
    for (int i = 0; i != size(); ++i)
        if (at(i).format == format)
            return at(i);
    return TypeFormatItem();
}

////////////////////////////////////////////////////////////////////
//
// WatchItem
//
////////////////////////////////////////////////////////////////////

WatchItem::WatchItem()
    : fetchTriggered(false)
{}

WatchItem::WatchItem(const QByteArray &i, const QString &n)
{
    fetchTriggered = false;
    d.iname = i;
    d.name = n;
}

WatchItem::WatchItem(const WatchData &data)
    : d(data), fetchTriggered(false)
{
}

WatchItem::WatchItem(const GdbMi &data)
    : fetchTriggered(false)
{
    d.iname = data["iname"].data();

    GdbMi wname = data["wname"];
    if (wname.isValid()) // Happens (only) for watched expressions.
        d.name = QString::fromUtf8(QByteArray::fromHex(wname.data()));
    else
        d.name = QString::fromLatin1(data["name"].data());

    parseWatchData(data);

    if (wname.isValid())
        d.exp = d.name.toUtf8();
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
        d = data;
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

    parseChildrenData(d, input, itemHandler, childHandler, arrayDecoder);
}

} // namespace Internal
} // namespace Debugger

#include "watchhandler.moc"
