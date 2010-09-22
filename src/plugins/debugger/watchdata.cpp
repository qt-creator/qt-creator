
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
    address(0),
    hasChildren(false),
    generation(-1),
    valueEnabled(true),
    valueEditable(true),
    error(false),
    source(0),
    objectId(0),
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
      && address == other.address
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
    if (value.endsWith(QLatin1Char('\'')) && type.endsWith("char")) {
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

void WatchData::setType(const QByteArray &str, bool guessChildrenFromType)
{
    type = str.trimmed();
    bool changed = true;
    while (changed) {
        if (type.endsWith("const"))
            type.chop(5);
        else if (type.endsWith(' '))
            type.chop(1);
        else if (type.endsWith('&'))
            type.chop(1);
        else if (type.startsWith("const "))
            type = type.mid(6);
        else if (type.startsWith("volatile "))
            type = type.mid(9);
        else if (type.startsWith("class "))
            type = type.mid(6);
        else if (type.startsWith("struct "))
            type = type.mid(6);
        else if (type.startsWith(' '))
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

void WatchData::setAddress(const quint64 &a)
{
    address = a;
}

void WatchData::setHexAddress(const QByteArray &a)
{
    bool ok;
    const qint64 av = a.toULongLong(&ok, 16);
    if (ok) {
        address = av;
    } else {
        qWarning("WatchData::setHexAddress(): Failed to parse address value '%s' for '%s', '%s'",
                 a.constData(), iname.constData(), type.constData());
        address = 0;
    }
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
    if (address) {
        str.setIntegerBase(16);
        str << "addr=\"0x" << address << doubleQuoteComma;
        str.setIntegerBase(10);
    }
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

    if (!dumperFlags.isEmpty())
        str << "dumperFlags=\"" << dumperFlags << doubleQuoteComma;

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
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Object Address"),
                     QString::fromAscii(hexAddress()));
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Internal ID"), iname);
    formatToolTipRow(str, QCoreApplication::translate("Debugger::Internal::WatchHandler", "Generation"),
        QString::number(generation));
    str << "</table></body></html>";
    return res;
}

QString WatchData::msgNotInScope()
{
    //: Value of variable in Debugger Locals display for variables out of scope (stopped above initialization).
    static const QString rc = QCoreApplication::translate("Debugger::Internal::WatchData", "<not in scope>");
    return rc;
}

const QString &WatchData::shadowedNameFormat()
{
    //: Display of variables shadowed by variables of the same name
    //: in nested scopes: Variable %1 is the variable name, %2 is a
    //: simple count.
    static const QString format = QCoreApplication::translate("Debugger::Internal::WatchData", "%1 <shadowed %2>");
    return format;
}

QString WatchData::shadowedName(const QString &name, int seen)
{
    if (seen <= 0)
        return name;
    return shadowedNameFormat().arg(name, seen);
}

quint64 WatchData::coreAddress() const
{
    return address;
}

QByteArray WatchData::hexAddress() const
{
    return address ? (QByteArray("0x") + QByteArray::number(address, 16)) : QByteArray();
}

} // namespace Internal
} // namespace Debugger

