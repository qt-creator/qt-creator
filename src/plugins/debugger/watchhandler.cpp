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

#include "watchhandler.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "debuggerprotocol.h"
#include "debuggertooltipmanager.h"
#include "imageviewer.h"
#include "memoryagent.h"
#include "registerhandler.h"
#include "simplifytype.h"
#include "watchdelegatewidgets.h"
#include "watchutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/messagebox.h>

#include <texteditor/syntaxhighlighter.h>

#include <app/app_version.h>
#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/checkablemessagebox.h>
#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/theme/theme.h>

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QFile>
#include <QItemDelegate>
#include <QJsonArray>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QProcess>
#include <QSet>
#include <QTabWidget>
#include <QTextEdit>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QToolTip>

#include <algorithm>
#include <cstring>
#include <ctype.h>

using namespace Core;
using namespace Utils;

namespace Debugger {
namespace Internal {

// Creates debug output for accesses to the model.
enum { debugModel = 0 };

#define MODEL_DEBUG(s) do { if (debugModel) qDebug() << s; } while (0)

static QMap<QString, int> theWatcherNames; // Keep order, QTCREATORBUG-12308.
static QSet<QString> theTemporaryWatchers; // Used for 'watched widgets'.
static int theWatcherCount = 0;
static QHash<QString, int> theTypeFormats;
static QHash<QString, int> theIndividualFormats;
static int theUnprintableBase = -1;

const char INameProperty[] = "INameProperty";
const char KeyProperty[] = "KeyProperty";

static QVariant createItemDelegate();

typedef QList<MemoryMarkup> MemoryMarkupList;

// Helper functionality to indicate the area of a member variable in
// a vector representing the memory area by a unique color
// number and tooltip. Parts of it will be overwritten when recursing
// over the children.

typedef QPair<int, QString> ColorNumberToolTip;
typedef QVector<ColorNumberToolTip> ColorNumberToolTips;

struct TypeInfo
{
    TypeInfo(uint s = 0) : size(s) {}
    uint size;
};

static const WatchModel *watchModel(const WatchItem *item)
{
    return reinterpret_cast<const WatchModel *>(item->model());
}

template <class T>
void readNumericVectorHelper(std::vector<double> *v, const QByteArray &ba)
{
    const T *p = (const T *) ba.data();
    const int n = ba.size() / sizeof(T);
    v->resize(n);
    // Losing precision in case of 64 bit ints is ok here, as the result
    // is only used to plot data.
    for (int i = 0; i != n; ++i)
        (*v)[i] = static_cast<double>(p[i]);
}

static void readNumericVector(std::vector<double> *v, const QByteArray &rawData, DebuggerEncoding encoding)
{
    switch (encoding.type) {
        case DebuggerEncoding::HexEncodedSignedInteger:
            switch (encoding.size) {
                case 1:
                    readNumericVectorHelper<signed char>(v, rawData);
                    return;
                case 2:
                    readNumericVectorHelper<short>(v, rawData);
                    return;
                case 4:
                    readNumericVectorHelper<int>(v, rawData);
                    return;
                case 8:
                    readNumericVectorHelper<qint64>(v, rawData);
                    return;
                }
        case DebuggerEncoding::HexEncodedUnsignedInteger:
            switch (encoding.size) {
                case 1:
                    readNumericVectorHelper<char>(v, rawData);
                    return;
                case 2:
                    readNumericVectorHelper<unsigned short>(v, rawData);
                    return;
                case 4:
                    readNumericVectorHelper<unsigned int>(v, rawData);
                    return;
                case 8:
                    readNumericVectorHelper<quint64>(v, rawData);
                    return;
            }
        case DebuggerEncoding::HexEncodedFloat:
            switch (encoding.size) {
                case 4:
                    readNumericVectorHelper<float>(v, rawData);
                    return;
                case 8:
                    readNumericVectorHelper<double>(v, rawData);
                    return;
        }
        default:
            break;
    }
    qDebug() << "ENCODING ERROR: " << encoding.toString();
}

static QString stripForFormat(const QString &ba)
{
    QString res;
    res.reserve(ba.size());
    int inArray = 0;
    for (int i = 0; i != ba.size(); ++i) {
        const QChar c = ba.at(i);
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
            theTypeFormats.insert(it.key(), it.value().toInt());
    }

    value = sessionValue("IndividualFormats");
    it = QMapIterator<QString, QVariant>(value.toMap());
    while (it.hasNext()) {
        it.next();
        if (!it.key().isEmpty())
            theIndividualFormats.insert(it.key(), it.value().toInt());
    }
}

static void saveFormats()
{
    QMap<QString, QVariant> formats;
    QHashIterator<QString, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != AutomaticFormat) {
            const QString key = it.key().trimmed();
            if (!key.isEmpty())
                formats.insert(key, format);
        }
    }
    setSessionValue("DefaultFormats", formats);

    formats.clear();
    it = QHashIterator<QString, int>(theIndividualFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        const QString key = it.key().trimmed();
        if (!key.isEmpty())
            formats.insert(key, format);
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
        setWindowTitle(WatchHandler::tr("Debugger - %1").arg(Core::Constants::IDE_DISPLAY_NAME));

        QVariant geometry = sessionValue("DebuggerSeparateWidgetGeometry");
        if (geometry.isValid()) {
            QRect rc = geometry.toRect();
            if (rc.width() < 400)
                rc.setWidth(400);
            if (rc.height() < 400)
                rc.setHeight(400);
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

    void removeObject(const QString &key)
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
            QString iname = o->property(INameProperty).toString();
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

    QWidget *findWidget(const QString &needle)
    {
        for (int i = count(); --i >= 0; ) {
            QWidget *w = widget(i);
            QString key = w->property(KeyProperty).toString();
            if (key == needle)
                return w;
        }
        return 0;
    }

    template <class T> T *prepareObject(const WatchItem *item)
    {
        const QString key = item->key();
        T *t = 0;
        if (QWidget *w = findWidget(key)) {
            t = qobject_cast<T *>(w);
            if (!t)
                removeTab(indexOf(w));
        }
        if (!t) {
            t = new T;
            t->setProperty(KeyProperty, key);
            addTab(t, item->name);
        }
        setProperty(INameProperty, item->iname);

        setCurrentWidget(t);
        show();
        raise();
        return t;
    }
};

class TextEdit : public QTextEdit
{
    Q_OBJECT
public:
    bool event(QEvent *ev) override
    {
        if (ev->type() == QEvent::ToolTip) {
            auto hev = static_cast<QHelpEvent *>(ev);
            QTextCursor cursor = cursorForPosition(hev->pos());
            int nextPos = cursor.position();
            if (document() && nextPos + 1 < document()->characterCount())
                ++nextPos;
            cursor.setPosition(nextPos, QTextCursor::KeepAnchor);
            QString msg = QString("Position: %1  Character: %2")
                    .arg(cursor.anchor()).arg(cursor.selectedText());
            QToolTip::showText(hev->globalPos(), msg, this);
        }
        return QTextEdit::event(ev);
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

    QVariant data(const QModelIndex &idx, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &value, int role) override;

    Qt::ItemFlags flags(const QModelIndex &idx) const override;
    bool hasChildren(const QModelIndex &idx) const override;
    bool canFetchMore(const QModelIndex &idx) const override;
    void fetchMore(const QModelIndex &idx) override;

    QString displayForAutoTest(const QByteArray &iname) const;
    void reinitialize(bool includeInspectData = false);

    WatchItem *findItem(const QString &iname) const;

    void reexpandItems();

    void showEditValue(const WatchItem *item);
    void setTypeFormat(const QString &type, int format);
    void setIndividualFormat(const QString &iname, int format);

    QString removeNamespaces(QString str) const;

    bool contextMenuEvent(const ItemViewEvent &ev);
    QMenu *createFormatMenu(WatchItem *item);
    QMenu *createMemoryMenu(WatchItem *item);
    QMenu *createBreakpointMenu(WatchItem *item);

    void addStackLayoutMemoryView(bool separateView, const QPoint &p);

    void addVariableMemoryView(bool separateView, WatchItem *item, bool atPointerAddress,
                               const QPoint &pos);
    MemoryMarkupList variableMemoryMarkup(WatchItem *item, const QString &rootName,
                                          const QString &rootToolTip, quint64 address, quint64 size,
                                          const RegisterMap &registerMap, bool sizeIsEstimate);
    int memberVariableRecursion(WatchItem *item, const QString &name, quint64 start,
                                quint64 end, int *colorNumberIn, ColorNumberToolTips *cnmv);

    QString editorContents(const QModelIndexList &list = QModelIndexList());
    void clearWatches();
    void removeWatchItem(WatchItem *item);
    void inputNewExpression();

    void grabWidget();
    void ungrabWidget();
    void timerEvent(QTimerEvent *event) override;
    int m_grabWidgetTimerId = -1;

public:
    WatchHandler *m_handler; // Not owned.
    DebuggerEngine *m_engine; // Not owned.

    bool m_contentsValid;

    WatchItem *m_localsRoot; // Not owned.
    WatchItem *m_inspectorRoot; // Not owned.
    WatchItem *m_watchRoot; // Not owned.
    WatchItem *m_returnRoot; // Not owned.
    WatchItem *m_tooltipRoot; // Not owned.

    SeparatedView *m_separatedView; // Not owned.

    QSet<QString> m_expandedINames;
    QTimer m_requestUpdateTimer;

    QHash<QString, TypeInfo> m_reportedTypeInfo;
    QHash<QString, DisplayFormats> m_reportedTypeFormats; // Type name -> Dumper Formats
    QHash<QString, QString> m_valueCache;
};

WatchModel::WatchModel(WatchHandler *handler, DebuggerEngine *engine)
    : m_handler(handler), m_engine(engine), m_separatedView(new SeparatedView)
{
    setObjectName("WatchModel");

    m_contentsValid = true;

    setHeader({tr("Name"), tr("Value"), tr("Type")});
    m_localsRoot = new WatchItem;
    m_localsRoot->iname = "local";
    m_localsRoot->name = tr("Locals");
    m_inspectorRoot = new WatchItem;
    m_inspectorRoot->iname = "inspect";
    m_inspectorRoot->name = tr("Inspector");
    m_watchRoot = new WatchItem;
    m_watchRoot->iname = "watch";
    m_watchRoot->name = tr("Expressions");
    m_returnRoot = new WatchItem;
    m_returnRoot->iname = "return";
    m_returnRoot->name = tr("Return Value");
    m_tooltipRoot = new WatchItem;
    m_tooltipRoot->iname = "tooltip";
    m_tooltipRoot->name = tr("Tooltip");
    auto root = new WatchItem;
    root->appendChild(m_localsRoot);
    root->appendChild(m_inspectorRoot);
    root->appendChild(m_watchRoot);
    root->appendChild(m_returnRoot);
    root->appendChild(m_tooltipRoot);
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
    connect(action(ShowQObjectNames), &SavedAction::valueChanged,
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

WatchItem *WatchModel::findItem(const QString &iname) const
{
    return findNonRootItem([iname](WatchItem *item) { return item->iname == iname; });
}

static QString parentName(const QString &iname)
{
    const int pos = iname.lastIndexOf('.');
    return pos == -1 ? QString() : iname.left(pos);
}

static QString niceTypeHelper(const QString &typeIn)
{
    typedef QMap<QString, QString> Cache;
    static Cache cache;
    const Cache::const_iterator it = cache.constFind(typeIn);
    if (it != cache.constEnd())
        return it.value();
    const QString simplified = simplifyType(typeIn);
    cache.insert(typeIn, simplified); // For simplicity, also cache unmodified types
    return simplified;
}

QString WatchModel::removeNamespaces(QString str) const
{
    if (!boolSetting(ShowStdNamespace))
        str.remove("std::");
    if (!boolSetting(ShowQtNamespace)) {
        const QString qtNamespace = m_engine->qtNamespace();
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
            return "(hex) " + QString::number(value, 16);
        case BinaryIntegerFormat:
            return "(bin) " + QString::number(value, 2);
        case OctalIntegerFormat:
            return "(oct) " + QString::number(value, 8);
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
static QString reformatCharacter(int code, int size, bool isSigned)
{
    const QChar c = QChar(uint(code));
    QString out;
    if (c.isPrint())
        out = QString("'") + c + "' ";
    else if (code == 0)
        out = "'\\0'";
    else if (code == '\r')
        out = "'\\r'";
    else if (code == '\n')
        out = "'\\n'";
    else if (code == '\t')
        out = "'\\t'";
    else
        out = "    ";

    out += '\t';

    if (isSigned) {
        out += QString::number(code);
        if (code < 0)
            out += QString("/%1    ").arg((1ULL << (8*size)) + code).left(2 + 2 * size);
        else
            out += QString(2 + 2 * size, QLatin1Char(' '));
    } else {
        out += QString::number(unsigned(code));
    }

    out += '\t';

    out += QString("0x%1").arg(uint(code & ((1ULL << (8*size)) - 1)),
                               2 * size, 16, QLatin1Char('0'));
    return out;
}

static QString quoteUnprintable(const QString &str)
{
    if (theUnprintableBase == 0)
        return str;

    QString encoded;
    if (theUnprintableBase == -1) {
        foreach (const QChar c, str) {
            int u = c.unicode();
            if (c.isPrint())
                encoded += c;
            else if (u == '\r')
                encoded += "\\r";
            else if (u == '\t')
                encoded += "\\t";
            else if (u == '\n')
                encoded += "\\n";
            else
                encoded += QString("\\%1").arg(c.unicode(), 3, 8, QLatin1Char('0'));
        }
        return encoded;
    }

    foreach (const QChar c, str) {
        if (c.isPrint())
            encoded += c;
        else if (theUnprintableBase == 8)
            encoded += QString("\\%1").arg(c.unicode(), 3, 8, QLatin1Char('0'));
        else
            encoded += QString("\\u%1").arg(c.unicode(), 4, 16, QLatin1Char('0'));
    }
    return encoded;
}

static int itemFormat(const WatchItem *item)
{
    const int individualFormat = theIndividualFormats.value(item->iname, AutomaticFormat);
    if (individualFormat != AutomaticFormat)
        return individualFormat;
    return theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);
}

static QString formattedValue(const WatchItem *item)
{
    if (item->type == "bool") {
        if (item->value == "0")
            return QLatin1String("false");
        if (item->value == "1")
            return QLatin1String("true");
        return item->value;
    }

    const int format = itemFormat(item);

    // Append quoted, printable character also for decimal.
    // FIXME: This is unreliable.
    if (item->type.endsWith("char")) {
        bool ok;
        const int code = item->value.toInt(&ok);
        bool isUnsigned = item->type == "unsigned char" || item->type == "uchar";
        if (ok)
            return reformatCharacter(code, 1, !isUnsigned);
    } else if (item->type.endsWith("wchar_t")) {
        bool ok;
        const int code = item->value.toInt(&ok);
        if (ok)
            return reformatCharacter(code, 4, false);
    } else if (item->type.endsWith("QChar")) {
        bool ok;
        const int code = item->value.toInt(&ok);
        if (ok)
            return reformatCharacter(code, 2, false);
    }

    if (format == HexadecimalIntegerFormat
            || format == DecimalIntegerFormat
            || format == OctalIntegerFormat
            || format == BinaryIntegerFormat) {
        bool isSigned = item->value.startsWith('-');
        quint64 raw = isSigned ? quint64(item->value.toLongLong()) : item->value.toULongLong();
        return reformatInteger(raw, format, item->size, isSigned);
    }

    if (format == ScientificFloatFormat) {
        double dd = item->value.toDouble();
        return QString::number(dd, 'e');
    }

    if (format == CompactFloatFormat) {
        double dd = item->value.toDouble();
        return QString::number(dd, 'g');
    }

    if (item->type == "va_list")
        return item->value;

    if (!isPointerType(item->type) && !item->isVTablePointer()) {
        bool ok = false;
        qulonglong integer = item->value.toULongLong(&ok, 0);
        if (ok)
            return reformatInteger(integer, format, item->size, false);
    }

    if (item->elided) {
        QString v = item->value;
        v.chop(1);
        QString len = item->elided > 0 ? QString::number(item->elided) : "unknown length";
        return quoteUnprintable(v) + "\"... (" + len  + ')';
    }

    return quoteUnprintable(item->value);
}

// Get a pointer address from pointer values reported by the debugger.
// Fix CDB formatting of pointers "0x00000000`000003fd class foo *", or
// "0x00000000`000003fd "Hallo"", or check gdb formatting of characters.
static inline quint64 pointerValue(QString data)
{
    const int blankPos = data.indexOf(' ');
    if (blankPos != -1)
        data.truncate(blankPos);
    data.remove('`');
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
    if (isPointerType(type) && value.startsWith("0x"))
        return QVariant::ULongLong;
   return QVariant::String;
}

// Convert to editable (see above)
QVariant WatchItem::editValue() const
{
    switch (editType()) {
    case QVariant::Bool:
        return value != "0" && value != "false";
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
    if (stringValue.endsWith('"')) {
        const int leadingDoubleQuote = stringValue.indexOf('"');
        if (leadingDoubleQuote != stringValue.size() - 1) {
            stringValue.truncate(stringValue.size() - 1);
            stringValue.remove(0, leadingDoubleQuote + 1);
            stringValue.replace("\n", "\\n");
        }
    }
    return QVariant(quoteUnprintable(stringValue));
}

// Truncate value for item view, maintaining quotes.
static QString truncateValue(QString v)
{
    enum { maxLength = 512 };
    if (v.size() < maxLength)
        return v;
    const bool isQuoted = v.endsWith('"'); // check for 'char* "Hallo"'
    v.truncate(maxLength);
    v += QLatin1String(isQuoted ? "...\"" : "...");
    return v;
}

static QString displayName(const WatchItem *item)
{
    QString result;

    const WatchItem *p = item->parentItem();
    if (!p)
        return result;
    if (item->arrayIndex >= 0) {
        result = QString("[%1]").arg(item->arrayIndex);
        return result;
    }
    if (item->iname.startsWith("return") && item->name.startsWith('$'))
        result = WatchModel::tr("returned value");
    else if (item->name == "*")
        result = '*' + p->name;
    else
        result = watchModel(item)->removeNamespaces(item->name);

    // Simplify names that refer to base classes.
    if (result.startsWith('[')) {
        result = simplifyType(result);
        if (result.size() > 30)
            result = result.left(27) + "...]";
    }

    return result;
}

static QString displayValue(const WatchItem *item)
{
    QString result = watchModel(item)->removeNamespaces(truncateValue(formattedValue(item)));
    if (result.isEmpty() && item->address)
        result += QString::fromLatin1("@0x" + QByteArray::number(item->address, 16));
//    if (origaddr)
//        result += QString::fromLatin1(" (0x" + QByteArray::number(origaddr, 16) + ')');
    return result;
}

static QString displayType(const WatchItem *item)
{
    QString result = niceTypeHelper(item->type);
    if (item->bitsize)
        result += QString(":%1").arg(item->bitsize);
    result.remove('\'');
    result = watchModel(item)->removeNamespaces(result);
    return result;
}

static QColor valueColor(const WatchItem *item, int column)
{
    Theme::Color color = Theme::Debugger_WatchItem_ValueNormal;
    if (const WatchModel *model = watchModel(item)) {
        if (!model->m_contentsValid && !item->isInspect()) {
            color = Theme::Debugger_WatchItem_ValueInvalid;
        } else if (column == 1) {
            if (!item->valueEnabled)
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (!model->m_contentsValid && !item->isInspect())
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (column == 1 && item->value.isEmpty()) // This might still show 0x...
                color = Theme::Debugger_WatchItem_ValueInvalid;
            else if (column == 1 && item->value != model->m_valueCache.value(item->iname))
                color = Theme::Debugger_WatchItem_ValueChanged;
        }
    }
    return creatorTheme()->color(color);
}

static DisplayFormats typeFormatList(const WatchItem *item)
{
    DisplayFormats formats;

    // Types supported by dumpers:
    // Hack: Compensate for namespaces.
    QString t = stripForFormat(item->type);
    int pos = t.indexOf("::Q");
    if (pos >= 0 && t.count(':') == 2)
        t.remove(0, pos + 2);
    pos = t.indexOf('<');
    if (pos >= 0)
        t.truncate(pos);
    t.replace(':', '_');
    formats << watchModel(item)->m_reportedTypeFormats.value(t);

    if (t.contains(']'))
        formats.append(ArrayPlotFormat);

    // Fixed artificial string and pointer types.
    if (item->origaddr || isPointerType(item->type)) {
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
    } else if (item->type.contains("char[") || item->type.contains("char [")) {
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
    item->value.toDouble(&ok);
    if (ok) {
        formats.append(CompactFloatFormat);
        formats.append(ScientificFloatFormat);
    }

    // Fixed artificial integral types.
    QString v = item->value;
    if (v.startsWith('-'))
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

QVariant WatchModel::data(const QModelIndex &idx, int role) const
{
    if (role == BaseTreeView::ItemDelegateRole)
        return createItemDelegate();

    if (role == BaseTreeView::ExtraIndicesForColumnWidth) {
        QModelIndexList l;
        for (const TreeItem *item : *m_watchRoot)
            l.append(indexForItem(item));
        for (const TreeItem *item : *m_returnRoot)
            l.append(indexForItem(item));
        return QVariant::fromValue(l);
    }

    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return QVariant();

    const int column = idx.column();
    switch (role) {
        case LocalsNameRole:
            return item->name;

        case Qt::EditRole: {
            switch (column) {
                case 0:
                    return item->expression();
                case 1:
                    return item->editValue();
                case 2:
                    return item->type;
            }
        }

        case Qt::DisplayRole: {
            switch (column) {
                case 0:
                    return displayName(item);
                case 1:
                    return displayValue(item);
                case 2:
                    return displayType(item);
            }
        }

        case Qt::ToolTipRole:
            return boolSetting(UseToolTipsInLocalsView)
                ? item->toToolTip() : QVariant();

        case Qt::ForegroundRole:
            return valueColor(item, column);

        case LocalsINameRole:
            return item->iname;

        case LocalsExpandedRole:
            return m_expandedINames.contains(item->iname);

        case LocalsTypeFormatRole:
            return theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);

        case LocalsIndividualFormatRole:
            return theIndividualFormats.value(item->iname, AutomaticFormat);

        default:
            break;
    }
    return QVariant();
}

bool WatchModel::setData(const QModelIndex &idx, const QVariant &value, int role)
{
    WatchItem *item = itemForIndex(idx);

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = value.value<ItemViewEvent>();

        if (ev.as<QContextMenuEvent>())
            return contextMenuEvent(ev);

        if (auto dev = ev.as<QDragEnterEvent>()) {
            if (dev->mimeData()->hasText()) {
                dev->setDropAction(Qt::CopyAction);
                dev->accept();
            }
            return true;
        }

        if (auto dev = ev.as<QDragMoveEvent>()) {
            if (dev->mimeData()->hasText()) {
                dev->setDropAction(Qt::CopyAction);
                dev->accept();
            }
            return true;
        }

        if (auto dev = ev.as<QDropEvent>()) {
            if (dev->mimeData()->hasText()) {
                QString exp;
                QString data = dev->mimeData()->text();
                foreach (const QChar c, data)
                    exp.append(c.isPrint() ? c : QChar(' '));
                m_handler->watchVariable(exp);
                //ev->acceptProposedAction();
                dev->setDropAction(Qt::CopyAction);
                dev->accept();
            }
            return true;
        }

        if (ev.as<QMouseEvent>(QEvent::MouseButtonDblClick)) {
            if (item && !item->parent()) { // if item is the invisible root item
                inputNewExpression();
                return true;
            }
        }

        if (auto kev = ev.as<QKeyEvent>(QEvent::KeyPress)) {
            if (item && kev->key() == Qt::Key_Delete && item->isWatcher()) {
                foreach (const QModelIndex &idx, ev.selectedRows())
                    removeWatchItem(itemForIndex(idx));
                return true;
            }

            if (item && kev->key() == Qt::Key_Return
                    && kev->modifiers() == Qt::ControlModifier
                    && item->isLocal()) {
                m_handler->watchExpression(item->expression(), QString());
                return true;
            }
        }
    }

    if (!idx.isValid())
        return false; // Triggered by ModelTester.

    QTC_ASSERT(item, return false);

    switch (role) {
        case Qt::EditRole:
            switch (idx.column()) {
            case 0: {
                m_handler->updateWatchExpression(item, value.toString().trimmed());
                break;
            }
            case 1: // Change value
                m_engine->assignValueInDebugger(item, item->expression(), value);
                break;
            case 2: // TODO: Implement change type.
                m_engine->assignValueInDebugger(item, item->expression(), value);
                break;
            }
            return true;

        case LocalsExpandedRole:
            if (value.toBool()) {
                // Should already have been triggered by fetchMore()
                //QTC_CHECK(m_expandedINames.contains(item->iname));
                m_expandedINames.insert(item->iname);
            } else {
                m_expandedINames.remove(item->iname);
            }
            if (item->iname.contains('.'))
                m_handler->updateWatchersWindow();
            return true;

        case LocalsTypeFormatRole:
            setTypeFormat(item->type, value.toInt());
            m_engine->updateLocals();
            return true;

        case LocalsIndividualFormatRole: {
            setIndividualFormat(item->iname, value.toInt());
            m_engine->updateLocals();
            return true;
        }

        case BaseTreeView::ItemActivatedRole:
            m_engine->selectWatchData(item->iname);
            return true;
    }

    return false;
}

Qt::ItemFlags WatchModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 0;

    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return Qt::ItemIsEnabled|Qt::ItemIsSelectable;

    const int column = idx.column();

    QTC_ASSERT(m_engine, return Qt::ItemFlags());
    const DebuggerState state = m_engine->state();

    // Enabled, editable, selectable, checkable, and can be used both as the
    // source of a drag and drop operation and as a drop target.

    const Qt::ItemFlags notEditable = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    const Qt::ItemFlags editable = notEditable | Qt::ItemIsEditable;
    bool isRunning = true;
    switch (state) {
    case InferiorStopOk:
    case InferiorUnrunnable:
    case DebuggerNotReady:
    case DebuggerFinished:
        isRunning = false;
        break;
    default:
        break;
    }

    if (item->isWatcher()) {
        if (state == InferiorUnrunnable)
            return (column == 0 && item->iname.count('.') == 1) ? editable : notEditable;

        if (isRunning && !m_engine->hasCapability(AddWatcherWhileRunningCapability))
            return notEditable;
        if (column == 0 && item->iname.count('.') == 1)
            return editable; // Watcher names are editable.
        if (column == 1 && item->arrayIndex >= 0)
            return editable;

        if (!item->name.isEmpty()) {
            // FIXME: Forcing types is not implemented yet.
            //if (idx.column() == 2)
            //    return editable; // Watcher types can be set by force.
            if (column == 1 && item->valueEditable && !item->elided)
                return editable; // Watcher values are sometimes editable.
        }
    } else if (item->isLocal()) {
        if (state == InferiorUnrunnable)
            return notEditable;
        if (isRunning && !m_engine->hasCapability(AddWatcherWhileRunningCapability))
            return notEditable;
        if (column == 1 && item->valueEditable && !item->elided)
            return editable; // Locals values are sometimes editable.
        if (column == 1 && item->arrayIndex >= 0)
            return editable;
    } else if (item->isInspect()) {
        if (column == 1 && item->valueEditable)
            return editable; // Inspector values are sometimes editable.
    }
    return notEditable;
}

bool WatchModel::canFetchMore(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return false;

    // See "hasChildren" below.
    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return false;
    if (!item->wantsChildren)
        return false;
    if (!m_contentsValid && !item->isInspect())
        return false;
    return true;
}

void WatchModel::fetchMore(const QModelIndex &idx)
{
    if (!idx.isValid())
        return;

    WatchItem *item = nonRootItemForIndex(idx);
    if (item) {
        m_expandedINames.insert(item->iname);
        if (item->childCount() == 0)
            m_engine->expandItem(item->iname);
    }
}

bool WatchModel::hasChildren(const QModelIndex &idx) const
{
    const WatchItem *item = nonRootItemForIndex(idx);
    if (!item)
        return true;
    if (item->childCount() > 0)
        return true;

    // "Can fetch more", see above.
    if (!item->wantsChildren)
        return false;
    if (!m_contentsValid && !item->isInspect())
        return false;
    return true;
}

static QString variableToolTip(const QString &name, const QString &type, quint64 offset)
{
    return offset
        ? //: HTML tooltip of a variable in the memory editor
          WatchModel::tr("<i>%1</i> %2 at #%3").arg(type, name).arg(offset)
        : //: HTML tooltip of a variable in the memory editor
          WatchModel::tr("<i>%1</i> %2").arg(type, name);
}

void WatchModel::grabWidget()
{
   QGuiApplication::setOverrideCursor(Qt::CrossCursor);
   m_grabWidgetTimerId = startTimer(30);
   ICore::mainWindow()->grabMouse();
}

void WatchModel::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_grabWidgetTimerId) {
        QPoint pnt = QCursor::pos();
        Qt::KeyboardModifiers mods = QApplication::queryKeyboardModifiers();
        QString msg;
        if (mods == Qt::NoModifier) {
            msg = tr("Press Ctrl to select widget at (%1, %2). "
                     "Press any other keyboard modifier to stop selection.")
                        .arg(pnt.x()).arg(pnt.y());
        } else {
            if (mods == Qt::CTRL) {
                msg = tr("Selecting widget at (%1, %2).").arg(pnt.x()).arg(pnt.y());
                m_engine->watchPoint(pnt);
            } else {
                msg = tr("Selection aborted.");
            }
            ungrabWidget();
        }
        showMessage(msg, StatusBar);
    } else {
        WatchModelBase::timerEvent(event);
    }
}

void WatchModel::ungrabWidget()
{
    ICore::mainWindow()->releaseMouse();
    QGuiApplication::restoreOverrideCursor();
    killTimer(m_grabWidgetTimerId);
    m_grabWidgetTimerId = -1;
}

int WatchModel::memberVariableRecursion(WatchItem *item,
                                        const QString &name,
                                        quint64 start, quint64 end,
                                        int *colorNumberIn,
                                        ColorNumberToolTips *cnmv)
{
    int childCount = 0;
    QTC_ASSERT(item, return childCount);
    QModelIndex modelIndex = indexForItem(item);
    const int rows = rowCount(modelIndex);
    if (!rows)
        return childCount;
    const QString nameRoot = name.isEmpty() ? name : name + '.';
    for (int r = 0; r < rows; r++) {
        WatchItem *child = item->childAt(r);
        const quint64 childAddress = item->address;
        if (childAddress && childAddress >= start
                && (childAddress + item->size) <= end) { // Non-static, within area?
            const QString childName = nameRoot + child->name;
            const quint64 childOffset = childAddress - start;
            const QString toolTip = variableToolTip(childName, item->type, childOffset);
            const ColorNumberToolTip colorNumberNamePair((*colorNumberIn)++, toolTip);
            const ColorNumberToolTips::iterator begin = cnmv->begin() + childOffset;
            std::fill(begin, begin + item->size, colorNumberNamePair);
            childCount++;
            childCount += memberVariableRecursion(child, childName, start, end, colorNumberIn, cnmv);
        }
    }
    return childCount;
}

/*!
    Creates markup for a variable in the memory view.

    Marks the visible children with alternating colors in the parent, that is, for
    \code
    struct Foo {
    char c1
    char c2
    int x2;
    QPair<int, int> pair
    }
    \endcode
    create something like:
    \code
    0 memberColor1
    1 memberColor2
    2 base color (padding area of parent)
    3 base color
    4 member color1
    ...
    8 memberColor2 (pair.first)
    ...
    12 memberColor1 (pair.second)
    \endcode

    In addition, registers pointing into the area are shown as 1 byte-markers.

   Fixme: When dereferencing a pointer, the size of the pointee is not
   known, currently. So, we take an area of 1024 and fill the background
   with the default color so that just the members are shown
   (sizeIsEstimate=true). This could be fixed by passing the pointee size
   as well from the debugger, but would require expensive type manipulation.

   \note To recurse over the top level items of the model, pass an invalid model
   index.

    \sa Debugger::Internal::MemoryViewWidget
*/
MemoryMarkupList WatchModel::variableMemoryMarkup(WatchItem *item,
                         const QString &rootName,
                         const QString &rootToolTip,
                         quint64 address, quint64 size,
                         const RegisterMap &registerMap,
                         bool sizeIsEstimate)
{
    enum { debug = 0 };
    enum { registerColorNumber = 0x3453 };

    // Starting out from base, create an array representing the area
    // filled with base color. Fill children with some unique color numbers,
    // leaving the padding areas of the parent colored with the base color.
    MemoryMarkupList result;
    int colorNumber = 0;
    ColorNumberToolTips ranges(size, ColorNumberToolTip(colorNumber, rootToolTip));
    colorNumber++;
    const int childCount = memberVariableRecursion(item,
                                                   rootName, address, address + size,
                                                   &colorNumber, &ranges);
    if (sizeIsEstimate && !childCount)
        return result; // Fixme: Exact size not known, no point in filling if no children.
    // Punch in registers as 1-byte markers on top.
    for (auto it = registerMap.constBegin(), end = registerMap.constEnd(); it != end; ++it) {
        if (it.key() >= address) {
            const quint64 offset = it.key() - address;
            if (offset < size) {
                ranges[offset] = ColorNumberToolTip(registerColorNumber,
                           WatchModel::tr("Register <i>%1</i>").arg(it.value()));
            } else {
                break; // Sorted.
            }
        }
    } // for registers.
    if (debug) {
        QDebug dbg = qDebug().nospace();
        dbg << rootToolTip << ' ' << address << ' ' << size << '\n';
        QString name;
        for (unsigned i = 0; i < size; ++i)
            if (name != ranges.at(i).second) {
                dbg << ",[" << i << ' ' << ranges.at(i).first << ' '
                    << ranges.at(i).second << ']';
                name = ranges.at(i).second;
            }
    }

    // Assign colors from a list, use base color for 0 (contrast to black text).
    // Overwrite the first color (which is usually very bright) by the base color.
    QList<QColor> colors = TextEditor::SyntaxHighlighter::generateColors(colorNumber + 2,
                                                                         QColor(Qt::black));
    QWidget *parent = ICore::dialogParent();
    const QColor defaultBackground = parent->palette().color(QPalette::Normal, QPalette::Base);
    colors[0] = sizeIsEstimate ? defaultBackground : Qt::lightGray;
    const QColor registerColor = Qt::green;
    int lastColorNumber = 0;
    for (unsigned i = 0; i < size; ++i) {
        const ColorNumberToolTip &range = ranges.at(i);
        if (result.isEmpty() || lastColorNumber != range.first) {
            lastColorNumber = range.first;
            const QColor color = range.first == registerColorNumber ?
                         registerColor : colors.at(range.first);
            result.push_back(MemoryMarkup(address + i, 1, color, range.second));
        } else {
            result.back().length++;
        }
    }

    if (debug) {
        QDebug dbg = qDebug().nospace();
        dbg << rootName << ' ' << address << ' ' << size << '\n';
        QString name;
        for (unsigned i = 0; i < size; ++i)
            if (name != ranges.at(i).second) {
                dbg << ',' << i << ' ' << ranges.at(i).first << ' '
                    << ranges.at(i).second;
                name = ranges.at(i).second;
            }
        dbg << '\n';
        foreach (const MemoryMarkup &m, result)
            dbg << m.address <<  ' ' << m.length << ' '  << m.toolTip << '\n';
    }

    return result;
}

// Convenience to create a memory view of a variable.
void WatchModel::addVariableMemoryView(bool separateView,
    WatchItem *item, bool atPointerAddress, const QPoint &pos)
{
    MemoryViewSetupData data;
    data.startAddress = atPointerAddress ? item->origaddr : item->address;
    if (!data.startAddress)
         return;
    // Fixme: Get the size of pointee (see variableMemoryMarkup())?
    const QString rootToolTip = variableToolTip(item->name, item->type, 0);
    const bool sizeIsEstimate = atPointerAddress || item->size == 0;
    const quint64 size = sizeIsEstimate ? 1024 : item->size;
    data.markup = variableMemoryMarkup(item, item->name, rootToolTip,
                             data.startAddress, size,
                             m_engine->registerHandler()->registerMap(),
                             sizeIsEstimate);
    data.separateView = separateView;
    data.readOnly = separateView;
    QString pat = atPointerAddress
        ? tr("Memory at Pointer's Address \"%1\" (0x%2)")
        : tr("Memory at Object's Address \"%1\" (0x%2)");
    data.title = pat.arg(item->name).arg(data.startAddress, 0, 16);
    data.pos = pos;
    m_engine->openMemoryView(data);
}

// Add a memory view of the stack layout showing local variables and registers.
void WatchModel::addStackLayoutMemoryView(bool separateView, const QPoint &p)
{
    // Determine suitable address range from locals.
    quint64 start = Q_UINT64_C(0xFFFFFFFFFFFFFFFF);
    quint64 end = 0;

    // Note: Unsorted 'locals' by default. Exclude 'Automatically dereferenced
    // pointer' items as they are outside the address range.
    rootItem()->childAt(0)->forFirstLevelChildren([&start, &end](WatchItem *item) {
        if (item->origaddr == 0) {
            const quint64 address = item->address;
            if (address) {
                if (address < start)
                    start = address;
                const uint size = qMax(1u, item->size);
                if (address + size > end)
                    end = address + size;
            }
        }
    });

    if (const quint64 remainder = end % 8)
        end += 8 - remainder;
    // Anything found and everything in a sensible range (static data in-between)?
    if (end <= start || end - start > 100 * 1024) {
        AsynchronousMessageBox::information(
            tr("Cannot Display Stack Layout"),
            tr("Could not determine a suitable address range."));
        return;
    }
    // Take a look at the register values. Extend the range a bit if suitable
    // to show stack/stack frame pointers.
    const RegisterMap regMap = m_engine->registerHandler()->registerMap();
    for (auto it = regMap.constBegin(), cend = regMap.constEnd(); it != cend; ++it) {
        const quint64 value = it.key();
        if (value < start && start - value < 512)
            start = value;
        else if (value > end && value - end < 512)
            end = value + 1;
    }
    // Indicate all variables.
    MemoryViewSetupData data;
    data.startAddress = start;
    data.markup = variableMemoryMarkup(rootItem()->childAt(0), QString(),
                                       QString(), start, end - start,
                                       regMap, true);
    data.separateView  = separateView;
    data.readOnly = separateView;
    data.title = tr("Memory Layout of Local Variables at 0x%1").arg(start, 0, 16);
    data.pos = p;
    m_engine->openMemoryView(data);
}

// Text for add watch action with truncated expression.
static QString addWatchActionText(QString exp)
{
    if (exp.isEmpty())
        return WatchModel::tr("Add Expression Evaluator");
    if (exp.size() > 30) {
        exp.truncate(30);
        exp.append("...");
    }
    return WatchModel::tr("Add Expression Evaluator for \"%1\"").arg(exp);
}

// Text for add watch action with truncated expression.
static QString removeWatchActionText(QString exp)
{
    if (exp.isEmpty())
        return WatchModel::tr("Remove Expression Evaluator");
    if (exp.size() > 30) {
        exp.truncate(30);
        exp.append("...");
    }
    return WatchModel::tr("Remove Expression Evaluator for \"%1\"")
            .arg(exp.replace('&', "&&"));
}

static void copyToClipboard(const QString &clipboardText)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(clipboardText, QClipboard::Selection);
    clipboard->setText(clipboardText, QClipboard::Clipboard);
}

void WatchModel::inputNewExpression()
{
    QDialog dlg;

    auto label = new QLabel(tr("Enter an expression to evaluate."), &dlg);

    auto hint = new QLabel(QString("<html>%1</html>").arg(
                    tr("Note: Evaluators will be re-evaluated after each step. "
                       "For details, see the <a href=\""
                       "qthelp://org.qt-project.qtcreator/doc/creator-debug-mode.html#locals-and-expressions"
                       "\">documentation</a>.")), &dlg);

    auto lineEdit = new FancyLineEdit(&dlg);
    lineEdit->setHistoryCompleter("WatchItems");
    lineEdit->clear(); // Undo "convenient" population with history item.

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);

    auto layout = new QVBoxLayout;
    layout->addWidget(label, Qt::AlignLeft);
    layout->addWidget(hint, Qt::AlignLeft);
    layout->addWidget(lineEdit);
    layout->addSpacing(10);
    layout->addWidget(buttons);
    dlg.setLayout(layout);

    dlg.setWindowTitle(tr("New Evaluated Expression"));

    connect(buttons, &QDialogButtonBox::accepted, lineEdit, &FancyLineEdit::onEditingFinished);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(hint, &QLabel::linkActivated, [](const QString &link) {
            HelpManager::handleHelpRequest(link); });

    if (dlg.exec() == QDialog::Accepted)
        m_handler->watchExpression(lineEdit->text().trimmed());
}

bool WatchModel::contextMenuEvent(const ItemViewEvent &ev)
{
    WatchItem *item = itemForIndex(ev.index());

    const QString exp = item ? item->expression() : QString();
    const QString name = item ? item->name : QString();

    const bool canHandleWatches = m_engine->hasCapability(AddWatcherCapability);
    const DebuggerState state = m_engine->state();
    const bool canInsertWatches = state == InferiorStopOk
        || state == DebuggerNotReady
        || state == InferiorUnrunnable
        || (state == InferiorRunOk && m_engine->hasCapability(AddWatcherWhileRunningCapability));

    bool canRemoveWatches = ((canHandleWatches && canInsertWatches) || state == DebuggerNotReady)
                             && (item && item->isWatcher());

    auto menu = new QMenu;

    addAction(menu, tr("Add New Expression Evaluator..."),
              canHandleWatches && canInsertWatches,
              [this] { inputNewExpression(); });

    addAction(menu, addWatchActionText(exp),
              // Suppress for top-level watchers.
              canHandleWatches && !exp.isEmpty() && item && !(item->level() == 2 && item->isWatcher()),
              [this, exp, name] { m_handler->watchExpression(exp, name); });

    addAction(menu, removeWatchActionText(exp),
              canRemoveWatches && !exp.isEmpty() && item && item->isWatcher(),
              [this, item] { removeWatchItem(item); });

    addAction(menu, tr("Remove All Expression Evaluators"),
              canRemoveWatches && !m_handler->watchedExpressions().isEmpty(),
              [this] { clearWatches(); });

    addAction(menu, tr("Select Widget to Add into Expression Evaluator"),
              state == InferiorRunOk && m_engine->hasCapability(WatchWidgetsCapability),
              [this] { grabWidget(); });

    menu->addSeparator();
    menu->addMenu(createFormatMenu(item));
    menu->addMenu(createMemoryMenu(item));
    menu->addMenu(createBreakpointMenu(item));
    menu->addSeparator();

    addAction(menu, tr("Expand All Children"),
              item,
              [this, item] {
                m_expandedINames.insert(item->iname);
                item->forFirstLevelChildren([this](WatchItem *child) {
                    m_expandedINames.insert(child->iname);
                });
                m_engine->updateLocals();
              });

    addAction(menu, tr("Collapse All Children"),
              item,
              [this, item] {
                item->forFirstLevelChildren([this](WatchItem *child) {
                    m_expandedINames.remove(child->iname);
                });
                m_engine->updateLocals();
              });

    addAction(menu, tr("Close Editor Tooltips"),
              DebuggerToolTipManager::hasToolTips(),
              [] { DebuggerToolTipManager::closeAllToolTips(); });

    addAction(menu, tr("Copy View Contents to Clipboard"),
              true,
              [this] { copyToClipboard(editorContents()); });

    addAction(menu, tr("Copy Current Value to Clipboard"),
              item,
              [item] { copyToClipboard(item->value); });

//    addAction(menu, tr("Copy Selected Rows to Clipboard"),
//              selectionModel()->hasSelection(),
//              [this] { copyToClipboard(editorContents(selectionModel()->selectedRows())); });

    addAction(menu, tr("Open View Contents in Editor"),
              m_engine->debuggerActionsEnabled(),
              [this] { Internal::openTextEditor(tr("Locals & Expressions"), editorContents()); });

    menu->addSeparator();

    menu->addAction(action(UseDebuggingHelpers));
    menu->addAction(action(UseToolTipsInLocalsView));
    menu->addAction(action(AutoDerefPointers));
    menu->addAction(action(SortStructMembers));
    menu->addAction(action(UseDynamicType));
    menu->addAction(action(SettingsDialog));

    menu->addSeparator();
    menu->addAction(action(SettingsDialog));
    menu->popup(ev.globalPos());
    return true;
}

QMenu *WatchModel::createBreakpointMenu(WatchItem *item)
{
    auto menu = new QMenu(tr("Add Data Breakpoint"));
    if (!item) {
        menu->setEnabled(false);
        return menu;
    }

    QAction *act = nullptr;
    BreakHandler *bh = m_engine->breakHandler();

    const bool canSetWatchpoint = m_engine->hasCapability(WatchpointByAddressCapability);
    const bool createPointerActions = item->origaddr && item->origaddr != item->address;

    act = addAction(menu, tr("Add Data Breakpoint at Object's Address (0x%1)").arg(item->address, 0, 16),
                     tr("Add Data Breakpoint"),
                     canSetWatchpoint && item->address,
                     [bh, item] { bh->setWatchpointAtAddress(item->address, item->size); });
    BreakpointParameters bp(WatchpointAtAddress);
    bp.address = item->address;
    act->setChecked(bh->findWatchpoint(bp));
    act->setToolTip(tr("Stop the program when the data at the address is modified."));

    act = addAction(menu, tr("Add Data Breakpoint at Pointer's Address (0x%1)").arg(item->origaddr, 0, 16),
                     tr("Add Data Breakpoint at Pointer's Address"),
                     canSetWatchpoint && item->address && createPointerActions,
                     // FIXME: an approximation. This should be target's sizeof(void)
                     [bh, item] { bh->setWatchpointAtAddress(item->origaddr, sizeof(void *)); });
    if (isPointerType(item->type)) {
        BreakpointParameters bp(WatchpointAtAddress);
        bp.address = pointerValue(item->value);
        act->setChecked(bh->findWatchpoint(bp));
    }

    act = addAction(menu, tr("Add Data Breakpoint at Expression \"%1\"").arg(item->name),
                     tr("Add Data Breakpoint at Expression"),
                     m_engine->hasCapability(WatchpointByExpressionCapability) && !item->name.isEmpty(),
                     [bh, item] { bh->setWatchpointAtExpression(item->name); });
    act->setToolTip(tr("Stop the program when the data at the address given by the expression "
                       "is modified."));

    return menu;
}

QMenu *WatchModel::createMemoryMenu(WatchItem *item)
{
    auto menu = new QMenu(tr("Open Memory Editor"));
    if (!item || !m_engine->hasCapability(ShowMemoryCapability)) {
        menu->setEnabled(false);
        return menu;
    }

    const bool createPointerActions = item->origaddr && item->origaddr != item->address;

    QPoint pos = QPoint(100, 100); // ev->globalPos

    addAction(menu, tr("Open Memory View at Object's Address (0x%1)").arg(item->address, 0, 16),
               tr("Open Memory View at Object's Address"),
               item->address,
               [this, item, pos] { addVariableMemoryView(true, item, false, pos); });

    addAction(menu, tr("Open Memory View at Pointer's Address (0x%1)").arg(item->origaddr, 0, 16),
               tr("Open Memory View at Pointer's Address"),
               createPointerActions,
               [this, item, pos] { addVariableMemoryView(true, item, true, pos); });

    addAction(menu, tr("Open Memory Editor at Object's Address (0x%1)").arg(item->address, 0, 16),
               tr("Open Memory Editor at Object's Address"),
               item->address,
               [this, item, pos] { addVariableMemoryView(false, item, false, pos); });

    addAction(menu, tr("Open Memory Editor at Pointer's Address (0x%1)").arg(item->origaddr, 0, 16),
               tr("Open Memory Editor at Pointer's Address"),
               createPointerActions,
               [this, item, pos] { addVariableMemoryView(false, item, true, pos); });


    addAction(menu, tr("Open Memory Editor Showing Stack Layout"),
              item && item->isLocal(),
              [this, pos] { addStackLayoutMemoryView(false, pos); });

    addAction(menu, tr("Open Memory Editor..."),
              true,
              [this, item] {
                    AddressDialog dialog;
                    if (item->address)
                        dialog.setAddress(item->address);
                    if (dialog.exec() == QDialog::Accepted) {
                        MemoryViewSetupData data;
                        data.startAddress = dialog.address();
                        m_engine->openMemoryView(data);
                    }
               });

    return menu;
}

QMenu *WatchModel::createFormatMenu(WatchItem *item)
{
    auto menu = new QMenu(tr("Change Value Display Format"));
    if (!item) {
        menu->setEnabled(false);
        return menu;
    }

    const DisplayFormats alternativeFormats = typeFormatList(item);

    const QString iname = item->iname;
    const int typeFormat = theTypeFormats.value(stripForFormat(item->type), AutomaticFormat);
    const int individualFormat = theIndividualFormats.value(iname, AutomaticFormat);

    auto addBaseChangeAction = [this, menu](const QString &text, int base) {
        addCheckableAction(menu, text, true, theUnprintableBase == base, [this, base] {
            theUnprintableBase = base;
            layoutChanged(); // FIXME
        });
    };

    addBaseChangeAction(tr("Treat All Characters as Printable"), 0);
    addBaseChangeAction(tr("Show Unprintable Characters as Escape Sequences"), -1);
    addBaseChangeAction(tr("Show Unprintable Characters as Octal"), 8);
    addBaseChangeAction(tr("Show Unprintable Characters as Hexadecimal"), 16);

    const QString spacer = "     ";
    menu->addSeparator();

    addAction(menu, tr("Change Display for Object Named \"%1\":").arg(iname), false);

    QString msg = (individualFormat == AutomaticFormat && typeFormat != AutomaticFormat)
        ? tr("Use Format for Type (Currently %1)").arg(nameForFormat(typeFormat))
        : QString(tr("Use Display Format Based on Type") + ' ');

    addCheckableAction(menu, spacer + msg, true, individualFormat == AutomaticFormat,
                       [this, iname] {
                                // FIXME: Extend to multi-selection.
                                //const QModelIndexList active = activeRows();
                                //foreach (const QModelIndex &idx, active)
                                //    setModelData(LocalsIndividualFormatRole, AutomaticFormat, idx);
                                setIndividualFormat(iname, AutomaticFormat);
                                m_engine->updateLocals();
                       });

    for (int format : alternativeFormats) {
        addCheckableAction(menu, spacer + nameForFormat(format), true, format == individualFormat,
                           [this, format, iname] {
                                setIndividualFormat(iname, format);
                                m_engine->updateLocals();
                           });
    }

    menu->addSeparator();
    addAction(menu, tr("Change Display for Type \"%1\":").arg(item->type), false);

    addCheckableAction(menu, spacer + tr("Automatic"), true, typeFormat == AutomaticFormat,
                       [this, item] {
                            //const QModelIndexList active = activeRows();
                           //foreach (const QModelIndex &idx, active)
                           //    setModelData(LocalsTypeFormatRole, AutomaticFormat, idx);
                           setTypeFormat(item->type, AutomaticFormat);
                           m_engine->updateLocals();
                       });

    for (int format : alternativeFormats) {
        addCheckableAction(menu, spacer + nameForFormat(format), true, format == typeFormat,
                           [this, format, item] {
                                setTypeFormat(item->type, format);
                                m_engine->updateLocals();
                           });
    }

    return menu;
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
        case SimpleFormat: return tr("Normal");
        case EnhancedFormat: return tr("Enhanced");
        case SeparateFormat: return tr("Separate Window");

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
    theWatcherNames.remove(QString());
    for (const QString &exp : theTemporaryWatchers)
        theWatcherNames.remove(exp);
    theTemporaryWatchers.clear();
    saveWatchers();
    m_model->reinitialize();
    emit m_model->updateFinished();
    if (Internal::mainWindow())
        m_model->m_separatedView->hide();
}

static bool sortByName(const WatchItem *a, const WatchItem *b)
{
    return a->name < b->name;
}

void WatchHandler::insertItems(const GdbMi &data)
{
    QSet<WatchItem *> itemsToSort;

    const bool sortStructMembers = boolSetting(SortStructMembers);
    foreach (const GdbMi &child, data.children()) {
        auto item = new WatchItem;
        item->parse(child, sortStructMembers);
        const TypeInfo ti = m_model->m_reportedTypeInfo.value(item->type);
        if (ti.size && !item->size)
            item->size = ti.size;

        const bool added = insertItem(item);
        if (added && item->level() == 2)
            itemsToSort.insert(static_cast<WatchItem *>(item->parent()));
    }

    foreach (WatchItem *toplevel, itemsToSort)
        toplevel->sortChildren(&sortByName);
}

void WatchHandler::removeItemByIName(const QString &iname)
{
    m_model->removeWatchItem(m_model->findItem(iname));
}

bool WatchHandler::insertItem(WatchItem *item)
{
    QTC_ASSERT(!item->iname.isEmpty(), return false);

    WatchItem *parent = m_model->findItem(parentName(item->iname));
    QTC_ASSERT(parent, return false);

    bool found = false;
    const std::vector<TreeItem *> siblings(parent->begin(), parent->end());
    for (int row = 0, n = int(siblings.size()); row < n; ++row) {
        if (static_cast<WatchItem *>(siblings[row])->iname == item->iname) {
            m_model->destroyItem(parent->childAt(row));
            parent->insertChild(row, item);
            found = true;
            break;
        }
    }
    if (!found)
        parent->appendChild(item);

    item->update();

    m_model->showEditValue(item);
    item->forAllChildren([this](WatchItem *sub) { m_model->showEditValue(sub); });

    return !found;
}

void WatchModel::reexpandItems()
{
    foreach (const QString &iname, m_expandedINames) {
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
    m_model->forAllItems([this](WatchItem *item) {
        m_model->m_valueCache[item->iname] = item->value;
    });
}

void WatchHandler::resetWatchers()
{
    loadSessionData();
}

void WatchHandler::notifyUpdateStarted(const UpdateParameters &updateParameters)
{
    QStringList inames = updateParameters.partialVariables();
    if (inames.isEmpty())
        inames = QStringList({"local", "return"});

    auto marker = [](WatchItem *item) { item->outdated = true; };

    if (inames.isEmpty()) {
        m_model->forItemsAtLevel<1>([marker](WatchItem *item) {
            item->forAllChildren(marker);
        });
    } else {
        for (auto iname : inames) {
            if (WatchItem *item = m_model->findItem(iname))
                item->forAllChildren(marker);
        }
    }

    m_model->m_requestUpdateTimer.start(80);
    m_model->m_contentsValid = false;
    updateWatchersWindow();
}

void WatchHandler::notifyUpdateFinished()
{
    QList<WatchItem *> toRemove;
    m_model->forSelectedItems([&toRemove](WatchItem *item) {
        if (item->outdated) {
            toRemove.append(item);
            return false;
        }
        return true;
    });

    foreach (auto item, toRemove)
        m_model->destroyItem(item);

    m_model->forAllItems([this](WatchItem *item) {
        if (item->wantsChildren && isExpandedIName(item->iname)) {
            m_model->m_engine->showMessage(QString("ADJUSTING CHILD EXPECTATION FOR " + item->iname));
            item->wantsChildren = false;
        }
    });

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

void WatchModel::removeWatchItem(WatchItem *item)
{
    QTC_ASSERT(item, return);
    if (item->isWatcher()) {
        theWatcherNames.remove(item->exp);
        saveWatchers();
    }
    destroyItem(item);
    m_handler->updateWatchersWindow();
}

QString WatchHandler::watcherName(const QString &exp)
{
    return "watch." + QString::number(theWatcherNames[exp]);
}

// If \a name is empty, \a exp will be used as name.
void WatchHandler::watchExpression(const QString &exp, const QString &name, bool temporary)
{
    // Do not insert the same entry more then once.
    if (exp.isEmpty() || theWatcherNames.contains(exp))
        return;

    theWatcherNames[exp] = theWatcherCount++;
    if (temporary)
        theTemporaryWatchers.insert(exp);

    auto item = new WatchItem;
    item->exp = exp;
    item->name = name.isEmpty() ? exp : name;
    item->iname = watcherName(exp);
    insertItem(item);
    saveWatchers();

    if (m_model->m_engine->state() == DebuggerNotReady) {
        item->setValue(QString(QLatin1Char(' ')));
        item->update();
    } else {
        m_model->m_engine->updateWatchData(item->iname);
    }
    updateWatchersWindow();
}

void WatchHandler::updateWatchExpression(WatchItem *item, const QString &newExp)
{
    if (newExp.isEmpty())
        return;

    if (item->exp != newExp) {
        theWatcherNames.insert(newExp, theWatcherNames.value(item->exp));
        theWatcherNames.remove(item->exp);
        item->exp = newExp;
        item->name = newExp;
    }

    saveWatchers();
    if (m_model->m_engine->state() == DebuggerNotReady) {
        item->setValue(QString(QLatin1Char(' ')));
        item->update();
    } else {
        m_model->m_engine->updateWatchData(item->iname);
    }
    updateWatchersWindow();
}

// Watch something obtained from the editor.
// Prefer to watch an existing local variable by its expression
// (address) if it can be found. Default to watchExpression().
void WatchHandler::watchVariable(const QString &exp)
{
    if (const WatchItem *localVariable = findCppLocalVariable(exp))
        watchExpression(localVariable->exp, exp);
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

template <class T> void readOne(const char *p, QString *res, int size)
{
    T r = 0;
    memcpy(&r, p, size);
    res->setNum(r);
}

void WatchModel::showEditValue(const WatchItem *item)
{
    const QString &format = item->editformat;
    if (format.isEmpty()) {
        // Nothing
        m_separatedView->removeObject(item->key());
    } else if (format == DisplayImageData || format == DisplayImageFile) {
        // QImage
        int width = 0, height = 0, nbytes = 0, imformat = 0;
        QByteArray ba;
        uchar *bits = 0;
        if (format == DisplayImageData) {
            ba = QByteArray::fromHex(item->editvalue.toUtf8());
            QTC_ASSERT(ba.size() > 16, return);
            const int *header = (int *)(ba.data());
            if (!ba.at(0) && !ba.at(1)) // Check on 'width' for Python dumpers returning 4-byte swapped-data.
                swapEndian(ba.data(), 16);
            bits = 16 + (uchar *)(ba.data());
            width = header[0];
            height = header[1];
            nbytes = header[2];
            imformat = header[3];
        } else if (format == DisplayImageFile) {
            QTextStream ts(item->editvalue.toUtf8());
            QString fileName;
            ts >> width >> height >> nbytes >> imformat >> fileName;
            QFile f(fileName);
            f.open(QIODevice::ReadOnly);
            ba = f.readAll();
            bits = (uchar*)ba.data();
            nbytes = width * height;
        }
        QTC_ASSERT(0 < width && width < 10000, return);
        QTC_ASSERT(0 < height && height < 10000, return);
        QTC_ASSERT(0 < nbytes && nbytes < 10000 * 10000, return);
        QTC_ASSERT(0 < imformat && imformat < 32, return);
        QImage im(width, height, QImage::Format(imformat));
        std::memcpy(im.bits(), bits, nbytes);
        ImageViewer *v = m_separatedView->prepareObject<ImageViewer>(item);
        v->setInfo(item->address ?
            tr("%1 Object at %2").arg(item->type, item->hexAddress()) :
            tr("%1 Object at Unknown Address").arg(item->type) + "    " +
            ImageViewer::tr("Size: %1x%2, %3 byte, format: %4, depth: %5")
                .arg(width).arg(height).arg(nbytes).arg(im.format()).arg(im.depth())
        );
        v->setImage(im);
    } else if (format == DisplayLatin1String
            || format == DisplayUtf8String
            || format == DisplayUtf16String
            || format == DisplayUcs4String) {
         // String data.
        QByteArray ba = QByteArray::fromHex(item->editvalue.toUtf8());
        QString str;
        if (format == DisplayLatin1String)
            str = QString::fromLatin1(ba.constData(), ba.size());
        else if (format == DisplayUtf8String)
            str = QString::fromUtf8(ba.constData(), ba.size());
        else if (format == DisplayUtf16String)
            str = QString::fromUtf16((ushort *)ba.constData(), ba.size() / 2);
        else if (format == DisplayUcs4String)
            str = QString::fromUcs4((uint *)ba.constData(), ba.size() / 4);
        m_separatedView->prepareObject<TextEdit>(item)->setPlainText(str);
    } else if (format == DisplayPlotData) {
        // Plots
        std::vector<double> data;
        readNumericVector(&data, QByteArray::fromHex(item->editvalue.toUtf8()), item->editencoding);
        m_separatedView->prepareObject<PlotViewer>(item)->setData(data);
    } else if (format.startsWith(DisplayArrayData)) {
        QString innerType = format.section(':', 2, 2);
        int innerSize = format.section(':', 3, 3).toInt();
        QTC_ASSERT(0 <= innerSize && innerSize <= 8, return);
//        int hint = format.section(':', 4, 4).toInt();
        int ndims = format.section(':', 5, 5).toInt();
        int ncols = format.section(':', 6, 6).toInt();
        int nrows = format.section(':', 7, 7).toInt();
        QTC_ASSERT(ndims == 2, qDebug() << "Display format: " << format; return);
        QByteArray ba = QByteArray::fromHex(item->editvalue.toUtf8());

        void (*reader)(const char *p, QString *res, int size) = 0;
        if (innerType == "int")
            reader = &readOne<qlonglong>;
        else if (innerType == "uint")
            reader = &readOne<qulonglong>;
        else if (innerType == "float") {
            if (innerSize == 4)
                reader = &readOne<float>;
            else if (innerSize == 8)
                reader = &readOne<double>;
        }
        QTC_ASSERT(reader, return);
        auto table = m_separatedView->prepareObject<QTableWidget>(item);
        table->setRowCount(nrows);
        table->setColumnCount(ncols);
        QString s;
        const char *p = ba.constBegin();
        for (int i = 0; i < nrows; ++i) {
            for (int j = 0; j < ncols; ++j) {
                reader(p, &s, innerSize);
                table->setItem(i, j, new QTableWidgetItem(s));
                p += innerSize;
            }
        }
    } else {
        QTC_ASSERT(false, qDebug() << "Display format: " << format);
    }
}

void WatchModel::clearWatches()
{
    if (theWatcherNames.isEmpty())
        return;

    const QDialogButtonBox::StandardButton ret = CheckableMessageBox::doNotAskAgainQuestion(
                ICore::mainWindow(), tr("Remove All Expression Evaluators"),
                tr("Are you sure you want to remove all expression evaluators?"),
                ICore::settings(), "RemoveAllWatchers");
    if (ret != QDialogButtonBox::Yes)
        return;

    m_watchRoot->removeChildren();
    theWatcherNames.clear();
    theWatcherCount = 0;
    m_handler->updateWatchersWindow();
    saveWatchers();
}

void WatchHandler::updateWatchersWindow()
{
    // Force show/hide of watchers and return view.
    int showWatch = !theWatcherNames.isEmpty();
    int showReturn = m_model->m_returnRoot->childCount() != 0;
    Internal::updateWatchersWindow(showWatch, showReturn);
}

QStringList WatchHandler::watchedExpressions()
{
    // Filter out invalid watchers.
    QStringList watcherNames;
    QMapIterator<QString, int> it(theWatcherNames);
    while (it.hasNext()) {
        it.next();
        const QString &watcherName = it.key();
        if (!watcherName.isEmpty())
            watcherNames.push_back(watcherName);
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
    return m_model->itemForIndex(idx);
}

void WatchHandler::fetchMore(const QString &iname) const
{
    if (WatchItem *item = m_model->findItem(iname)) {
        m_model->m_expandedINames.insert(iname);
        if (item->childCount() == 0)
            m_model->m_engine->expandItem(iname);
    }
}

WatchItem *WatchHandler::findItem(const QString &iname) const
{
    return m_model->findItem(iname);
}

const WatchItem *WatchHandler::findCppLocalVariable(const QString &name) const
{
    // Can this be found as a local variable?
    const QString localsPrefix("local.");
    QString iname = localsPrefix + name;
    if (const WatchItem *item = findItem(iname))
        return item;
//    // Nope, try a 'local.this.m_foo'.
//    iname.insert(localsPrefix.size(), "this.");
//    if (const WatchData *wd = findData(iname))
//        return wd;
    return 0;
}

void WatchModel::setTypeFormat(const QString &type0, int format)
{
    const QString type = stripForFormat(type0);
    if (format == AutomaticFormat)
        theTypeFormats.remove(type);
    else
        theTypeFormats[type] = format;
    saveFormats();
    m_engine->updateAll();
}

void WatchModel::setIndividualFormat(const QString &iname, int format)
{
    if (format == AutomaticFormat)
        theIndividualFormats.remove(iname);
    else
        theIndividualFormats[iname] = format;
    saveFormats();
}

int WatchHandler::format(const QString &iname) const
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

static QString formatStringFromFormatCode(int code)
{
    switch (code) {
    // Taken from debuggerprotocol.h, DisplayFormat.
    case Latin1StringFormat:
        return QLatin1String("latin");
    case SeparateLatin1StringFormat:
        return QLatin1String("latin:separate");
    case Utf8StringFormat:
        return QLatin1String("utf8");
    case SeparateUtf8StringFormat:
        return QLatin1String("utf8:separate");
    case Utf16StringFormat:
        return QLatin1String("utf16");
    }
    return QString();
}

QString WatchHandler::typeFormatRequests() const
{
    QString ba;
    if (!theTypeFormats.isEmpty()) {
        QHashIterator<QString, int> it(theTypeFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format != AutomaticFormat) {
                ba.append(toHex(it.key()));
                ba.append('=');
                ba.append(formatStringFromFormatCode(format));
                ba.append(',');
            }
        }
        ba.chop(1);
    }
    return ba;
}

QString WatchHandler::individualFormatRequests() const
{
    QString res;
    if (!theIndividualFormats.isEmpty()) {
        QHashIterator<QString, int> it(theIndividualFormats);
        while (it.hasNext()) {
            it.next();
            const int format = it.value();
            if (format != AutomaticFormat) {
                res.append(it.key());
                res.append('=');
                res.append(formatStringFromFormatCode(it.value()));
                res.append(',');
            }
        }
        res.chop(1);
    }
    return res;
}

void WatchHandler::appendFormatRequests(DebuggerCommand *cmd)
{
    QJsonArray expanded;
    QSetIterator<QString> jt(m_model->m_expandedINames);
    while (jt.hasNext())
        expanded.append(jt.next());

    cmd->arg("expanded", expanded);

    QJsonObject typeformats;
    QHashIterator<QString, int> it(theTypeFormats);
    while (it.hasNext()) {
        it.next();
        const int format = it.value();
        if (format != AutomaticFormat)
            typeformats.insert(it.key(), format);
    }
    cmd->arg("typeformats", typeformats);

    QJsonObject formats;
    QHashIterator<QString, int> it2(theIndividualFormats);
    while (it2.hasNext()) {
        it2.next();
        const int format = it2.value();
        if (format != AutomaticFormat)
            formats.insert(it2.key(), format);
    }
    cmd->arg("formats", formats);
}

static inline QJsonObject watcher(const QString &iname, const QString &exp)
{
    QJsonObject watcher;
    watcher.insert("iname", iname);
    watcher.insert("exp", toHex(exp));
    return watcher;
}

void WatchHandler::appendWatchersAndTooltipRequests(DebuggerCommand *cmd)
{
    QJsonArray watchers;
    DebuggerToolTipContexts toolTips = DebuggerToolTipManager::pendingTooltips(m_model->m_engine);
    foreach (const DebuggerToolTipContext &p, toolTips)
        watchers.append(watcher(p.iname, p.expression));

    QMapIterator<QString, int> it(WatchHandler::watcherNames());
    while (it.hasNext()) {
        it.next();
        watchers.append(watcher("watch." + QString::number(it.value()), it.key()));
    }
    cmd->arg("watchers", watchers);
}

void WatchHandler::addDumpers(const GdbMi &dumpers)
{
    foreach (const GdbMi &dumper, dumpers.children()) {
        DisplayFormats formats;
        formats.append(RawFormat);
        QString reportedFormats = dumper["formats"].data();
        foreach (const QStringRef &format, reportedFormats.splitRef(',')) {
            if (int f = format.toInt())
                formats.append(DisplayFormat(f));
        }
        addTypeFormats(dumper["type"].data(), formats);
    }
}

void WatchHandler::addTypeFormats(const QString &type, const DisplayFormats &formats)
{
    m_model->m_reportedTypeFormats.insert(stripForFormat(type), formats);
}

QString WatchModel::editorContents(const QModelIndexList &list)
{
    QString contents;
    QTextStream ts(&contents);
    forAllItems([&ts, this, list](WatchItem *item) {
        if (list.isEmpty() || list.contains(indexForItem(item))) {
            const QChar tab = QLatin1Char('\t');
            const QChar nl = '\n';
            ts << QString(item->level(), tab) << item->name << tab << displayValue(item) << tab
               << item->type << nl;
        }
    });
    return contents;
}

void WatchHandler::scheduleResetLocation()
{
    m_model->m_contentsValid = false;
}

void WatchHandler::resetLocation()
{
}

void WatchHandler::setCurrentItem(const QString &iname)
{
    if (WatchItem *item = m_model->findItem(iname)) {
        QModelIndex idx = m_model->indexForItem(item);
        emit m_model->currentIndexRequested(idx);
    }
}

QMap<QString, int> WatchHandler::watcherNames()
{
    return theWatcherNames;
}

bool WatchHandler::isExpandedIName(const QString &iname) const
{
    return m_model->m_expandedINames.contains(iname);
}

QSet<QString> WatchHandler::expandedINames() const
{
    return m_model->m_expandedINames;
}

void WatchHandler::recordTypeInfo(const GdbMi &typeInfo)
{
    if (typeInfo.type() == GdbMi::List) {
        foreach (const GdbMi &s, typeInfo.children()) {
            QString typeName = fromHex(s["name"].data());
            TypeInfo ti(s["size"].data().toUInt());
            m_model->m_reportedTypeInfo.insert(typeName, ti);
        }
    }
}

/////////////////////////////////////////////////////////////////////
//
// WatchDelegate
//
/////////////////////////////////////////////////////////////////////

class WatchDelegate : public QItemDelegate
{
public:
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
        const QModelIndex &index) const override
    {
        const WatchModelBase *model = qobject_cast<const WatchModelBase *>(index.model());
        QTC_ASSERT(model, return 0);

        WatchItem *item = model->nonRootItemForIndex(index);
        QTC_ASSERT(item, return 0);

        // Value column: Custom editor. Apply integer-specific settings.
        if (index.column() == 1) {
            QVariant::Type editType = QVariant::Type(item->editType());
            if (editType == QVariant::Bool)
                return new BooleanComboBox(parent);

            WatchLineEdit *edit = WatchLineEdit::create(editType, parent);
            edit->setFrame(false);

            if (auto intEdit = qobject_cast<IntegerWatchLineEdit *>(edit)) {
                if (isPointerType(item->type))
                    intEdit->setBase(16); // Pointers using 0x-convention
                else
                    intEdit->setBase(formatToIntegerBase(itemFormat(item)));
            }

            return edit;
        }

        // Standard line edits for the rest.
        auto lineEdit = new FancyLineEdit(parent);
        lineEdit->setFrame(false);
        lineEdit->setHistoryCompleter("WatchItems");
        return lineEdit;
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option,
        const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
};

static QVariant createItemDelegate()
{
    return QVariant::fromValue(static_cast<QAbstractItemDelegate *>(new WatchDelegate));
}

} // namespace Internal
} // namespace Debugger

#include "watchhandler.moc"
