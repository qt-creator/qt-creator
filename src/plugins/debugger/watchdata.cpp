
#include "watchdata.h"
#include "watchutils.h"

#include <QtCore/QTextStream>
#include <QtCore/QDebug>

#include <QtGui/QApplication>
#include <QtGui/QTextDocument> // Qt::escape

////////////////////////////////////////////////////////////////////
//
// WatchData
//
////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

WatchData::WatchData() :
    editformat(0),
    hasChildren(false),
    generation(-1),
    valueEnabled(true),
    valueEditable(true),
    error(false),
    source(0),
    state(InitialState),
    changed(false)
{
}

bool WatchData::isEqual(const WatchData &other) const
{
    return iname == other.iname
      && exp == other.exp
      && name == other.name
      && value == other.value
      && editvalue == other.editvalue
      && valuetooltip == other.valuetooltip
      && type == other.type
      && displayedType == other.displayedType
      && variable == other.variable
      && addr == other.addr
      && framekey == other.framekey
      && hasChildren == other.hasChildren
      && valueEnabled == other.valueEnabled
      && valueEditable == other.valueEditable
      && error == other.error;
}

void WatchData::setError(const QString &msg)
{
    setAllUnneeded();
    value = msg;
    setHasChildren(false);
    valueEnabled = false;
    valueEditable = false;
    error = true;
}

void WatchData::setValue(const QString &value0)
{
    value = value0;
    if (value == "{...}") {
        value.clear();
        hasChildren = true; // at least one...
    }
    // strip off quoted characters for chars.
    if (value.endsWith(QLatin1Char('\'')) && type.endsWith(QLatin1String("char"))) {
        const int blankPos = value.indexOf(QLatin1Char(' '));
        if (blankPos != -1)
            value.truncate(blankPos);
    }

    // avoid duplicated information
    if (value.startsWith(QLatin1Char('(')) && value.contains(") 0x"))
        value = value.mid(value.lastIndexOf(") 0x") + 2);

    // doubles are sometimes displayed as "@0x6141378: 1.2".
    // I don't want that.
    if (/*isIntOrFloatType(type) && */ value.startsWith("@0x")
         && value.contains(':')) {
        value = value.mid(value.indexOf(':') + 2);
        setHasChildren(false);
    }

    // "numchild" is sometimes lying
    //MODEL_DEBUG("\n\n\nPOINTER: " << type << value);
    if (isPointerType(type))
        setHasChildren(value != "0x0" && value != "<null>"
            && !isCharPointerType(type));

    // pointer type information is available in the 'type'
    // column. No need to duplicate it here.
    if (value.startsWith(QLatin1Char('(') + type + ") 0x"))
        value = value.section(QLatin1Char(' '), -1, -1);

    setValueUnneeded();
}

void WatchData::setValueToolTip(const QString &tooltip)
{
    valuetooltip = tooltip;
}

void WatchData::setType(const QString &str, bool guessChildrenFromType)
{
    type = str.trimmed();
    bool changed = true;
    while (changed) {
        if (type.endsWith(QLatin1String("const")))
            type.chop(5);
        else if (type.endsWith(QLatin1Char(' ')))
            type.chop(1);
        else if (type.endsWith(QLatin1Char('&')))
            type.chop(1);
        else if (type.startsWith(QLatin1String("const ")))
            type = type.mid(6);
        else if (type.startsWith(QLatin1String("volatile ")))
            type = type.mid(9);
        else if (type.startsWith(QLatin1String("class ")))
            type = type.mid(6);
        else if (type.startsWith(QLatin1String("struct ")))
            type = type.mid(6);
        else if (type.startsWith(QLatin1Char(' ')))
            type = type.mid(1);
        else
            changed = false;
    }
    setTypeUnneeded();
    if (guessChildrenFromType) {
        switch (guessChildren(type)) {
        case HasChildren:
            setHasChildren(true);
            break;
        case HasNoChildren:
            setHasChildren(false);
            break;
        case HasPossiblyChildren:
            setHasChildren(true); // FIXME: bold assumption
            break;
        }
    }
}

void WatchData::setAddress(const QByteArray &a)
{
    addr = a;
}

QString WatchData::toString() const
{
    const char *doubleQuoteComma = "\",";
    QString res;
    QTextStream str(&res);
    str << QLatin1Char('{');
    if (!iname.isEmpty())
        str << "iname=\"" << iname << doubleQuoteComma;
    if (!name.isEmpty() && name != iname)
        str << "name=\"" << name << doubleQuoteComma;
    if (error)
        str << "error,";
    if (!addr.isEmpty())
        str << "addr=\"" << addr << doubleQuoteComma;
    if (!exp.isEmpty())
        str << "exp=\"" << exp << doubleQuoteComma;

    if (!variable.isEmpty())
        str << "variable=\"" << variable << doubleQuoteComma;

    if (isValueNeeded())
        str << "value=<needed>,";
    if (isValueKnown() && !value.isEmpty())
        str << "value=\"" << value << doubleQuoteComma;

    if (!editvalue.isEmpty())
        str << "editvalue=\"<...>\",";
    //    str << "editvalue=\"" << editvalue << doubleQuoteComma;

    if (isTypeNeeded())
        str << "type=<needed>,";
    if (isTypeKnown() && !type.isEmpty())
        str << "type=\"" << type << doubleQuoteComma;

    if (isHasChildrenNeeded())
        str << "hasChildren=<needed>,";
    if (isHasChildrenKnown())
        str << "hasChildren=\"" << (hasChildren ? "true" : "false") << doubleQuoteComma;

    if (isChildrenNeeded())
        str << "children=<needed>,";
    if (source)
        str << "source=" << source;
    str.flush();
    if (res.endsWith(QLatin1Char(',')))
        res.truncate(res.size() - 1);
    return res + QLatin1Char('}');
}

// Format a tooltip fow with aligned colon.
static void formatToolTipRow(QTextStream &str,
    const QString &category, const QString &value)
{
    str << "<tr><td>" << category << "</td><td> : </td><td>"
        << Qt::escape(value) << "</td></tr>";
}

static QString typeToolTip(const WatchData &wd)
{
    if (wd.displayedType.isEmpty())
        return wd.type;
    QString rc = wd.displayedType;
    rc += QLatin1String(" (");
    rc += wd.type;
    rc += QLatin1Char(')');
    return rc;
}

QString WatchData::toToolTip() const
{
    if (!valuetooltip.isEmpty())
        return QString::number(valuetooltip.size());
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>";
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Name"), name);
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Expression"), exp);
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Type"), typeToolTip(*this));
    QString val = value;
    if (value.size() > 1000) {
        val.truncate(1000);
        val += QCoreApplication::translate("Debugger::Internal::WatchHandler", " ... <cut off>");
    }
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Value"), val);
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Object Address"), addr);
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Internal ID"), iname);
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Generation"),
        QString::number(generation));
    str << "</table></body></html>";
    return res;
}

QString WatchData::msgNotInScope()
{
    static const QString rc = QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");
    return rc;
}

const QString &WatchData::shadowedNameFormat()
{
    static const QString format = QCoreApplication::translate("Debugger::Internal::WatchData", "%1 <shadowed %2>");
    return format;
}

QString WatchData::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    return shadowedNameFormat().arg(name, seen);
}

} // namespace Internal
} // namespace Debugger

