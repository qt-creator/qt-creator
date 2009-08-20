/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "watchutils.h"
#include "watchhandler.h"
#include <utils/qtcassert.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>

#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsconstants.h>

#include <cplusplus/ExpressionUnderCursor.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>

#include <QtGui/QTextCursor>
#include <QtGui/QPlainTextEdit>

#include <string.h>
#include <ctype.h>

enum { debug = 0 };
namespace Debugger {
namespace Internal {

QString dotEscape(QString str)
{
    const QChar dot = QLatin1Char('.');
    str.replace(QLatin1Char(' '), dot);
    str.replace(QLatin1Char('\\'), dot);
    str.replace(QLatin1Char('/'), dot);
    return str;
}

QString currentTime()
{
    return QTime::currentTime().toString(QLatin1String("hh:mm:ss.zzz"));
}

bool isSkippableFunction(const QString &funcName, const QString &fileName)
{
    if (fileName.endsWith(QLatin1String("kernel/qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/moc_qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qmetaobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qmetaobject_p.h")))
        return true;
    if (fileName.endsWith(QLatin1String(".moc")))
        return true;

    if (funcName.endsWith("::qt_metacall"))
        return true;
    if (funcName.endsWith("::d_func"))
        return true;
    if (funcName.endsWith("::q_func"))
        return true;

    return false;
}

bool isLeavableFunction(const QString &funcName, const QString &fileName)
{
    if (funcName.endsWith(QLatin1String("QObjectPrivate::setCurrentSender")))
        return true;
    if (funcName.endsWith(QLatin1String("QMutexPool::get")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qmetaobject.cpp"))
            && funcName.endsWith(QLatin1String("QMetaObject::methodOffset")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qobject.h")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qobject.cpp"))
            && funcName.endsWith(QLatin1String("QObjectConnectionListVector::at")))
        return true;
    if (fileName.endsWith(QLatin1String("kernel/qobject.cpp"))
            && funcName.endsWith(QLatin1String("~QObject")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qmutex.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qthread.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qthread_unix.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("thread/qmutex.h")))
        return true;
    if (fileName.contains(QLatin1String("thread/qbasicatomic")))
        return true;
    if (fileName.contains(QLatin1String("thread/qorderedmutexlocker_p")))
        return true;
    if (fileName.contains(QLatin1String("arch/qatomic")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qvector.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qlist.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qhash.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qmap.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qshareddata.h")))
        return true;
    if (fileName.endsWith(QLatin1String("tools/qstring.h")))
        return true;
    if (fileName.endsWith(QLatin1String("global/qglobal.h")))
        return true;

    return false;
}

bool hasLetterOrNumber(const QString &exp)
{
    const QChar underscore = QLatin1Char('_');
    for (int i = exp.size(); --i >= 0; )
        if (exp.at(i).isLetterOrNumber() || exp.at(i) == underscore)
            return true;
    return false;
}

bool hasSideEffects(const QString &exp)
{
    // FIXME: complete?
    return exp.contains(QLatin1String("-="))
        || exp.contains(QLatin1String("+="))
        || exp.contains(QLatin1String("/="))
        || exp.contains(QLatin1String("%="))
        || exp.contains(QLatin1String("*="))
        || exp.contains(QLatin1String("&="))
        || exp.contains(QLatin1String("|="))
        || exp.contains(QLatin1String("^="))
        || exp.contains(QLatin1String("--"))
        || exp.contains(QLatin1String("++"));
}

bool isKeyWord(const QString &exp)
{
    // FIXME: incomplete
    return exp == QLatin1String("class")
        || exp == QLatin1String("const")
        || exp == QLatin1String("do")
        || exp == QLatin1String("if")
        || exp == QLatin1String("return")
        || exp == QLatin1String("struct")
        || exp == QLatin1String("template")
        || exp == QLatin1String("void")
        || exp == QLatin1String("volatile")
        || exp == QLatin1String("while");
}

bool isPointerType(const QString &type)
{
    return type.endsWith(QLatin1Char('*')) || type.endsWith(QLatin1String("* const"));
}

bool isAccessSpecifier(const QString &str)
{
    static const QStringList items = QStringList()
        << QLatin1String("private")
        << QLatin1String("protected")
        << QLatin1String("public");
    return items.contains(str);
}

bool startsWithDigit(const QString &str)
{
    return !str.isEmpty() && str.at(0).isDigit();
}

QString stripPointerType(QString type)
{
    if (type.endsWith(QLatin1Char('*')))
        type.chop(1);
    if (type.endsWith(QLatin1String("* const")))
        type.chop(7);
    if (type.endsWith(QLatin1Char(' ')))
        type.chop(1);
    return type;
}

QString gdbQuoteTypes(const QString &type)
{
    // gdb does not understand sizeof(Core::IFile*).
    // "sizeof('Core::IFile*')" is also not acceptable,
    // it needs to be "sizeof('Core::IFile'*)"
    //
    // We never will have a perfect solution here (even if we had a full blown
    // C++ parser as we do not have information on what is a type and what is
    // a variable name. So "a<b>::c" could either be two comparisons of values
    // 'a', 'b' and '::c', or a nested type 'c' in a template 'a<b>'. We
    // assume here it is the latter.
    //return type;

    // (*('myns::QPointer<myns::QObject>*'*)0x684060)" is not acceptable
    // (*('myns::QPointer<myns::QObject>'**)0x684060)" is acceptable
    if (isPointerType(type))
        return gdbQuoteTypes(stripPointerType(type)) + QLatin1Char('*');

    QString accu;
    QString result;
    int templateLevel = 0;

    const QChar colon = QLatin1Char(':');
    const QChar singleQuote = QLatin1Char('\'');
    const QChar lessThan = QLatin1Char('<');
    const QChar greaterThan = QLatin1Char('>');
    for (int i = 0; i != type.size(); ++i) {
        const QChar c = type.at(i);
        if (c.isLetterOrNumber() || c == QLatin1Char('_') || c == colon || c == QLatin1Char(' ')) {
            accu += c;
        } else if (c == lessThan) {
            ++templateLevel;
            accu += c;
        } else if (c == greaterThan) {
            --templateLevel;
            accu += c;
        } else if (templateLevel > 0) {
            accu += c;
        } else {
            if (accu.contains(colon) || accu.contains(lessThan))
                result += singleQuote + accu + singleQuote;
            else
                result += accu;
            accu.clear();
            result += c;
        }
    }
    if (accu.contains(colon) || accu.contains(lessThan))
        result += singleQuote + accu + singleQuote;
    else
        result += accu;
    //qDebug() << "GDB_QUOTING" << type << " TO " << result;

    return result;
}

bool extractTemplate(const QString &type, QString *tmplate, QString *inner)
{
    // Input "Template<Inner1,Inner2,...>::Foo" will return "Template::Foo" in
    // 'tmplate' and "Inner1@Inner2@..." etc in 'inner'. Result indicates
    // whether parsing was successful
    // Gdb inserts a blank after each comma which we would like to avoid
    tmplate->clear();
    inner->clear();
    if (!type.contains(QLatin1Char('<')))
        return  false;
    int level = 0;
    bool skipSpace = false;
    const int size = type.size();

    for (int i = 0; i != size; ++i) {
        const QChar c = type.at(i);
        const char asciiChar = c.toAscii();
        switch (asciiChar) {
        case '<':
            *(level == 0 ? tmplate : inner) += c;
            ++level;
            break;
        case '>':
            --level;
            *(level == 0 ? tmplate : inner) += c;
            break;
        case ',':
            *inner += (level == 1) ? QLatin1Char('@') : QLatin1Char(',');
            skipSpace = true;
            break;
        default:
            if (!skipSpace || asciiChar != ' ') {
                *(level == 0 ? tmplate : inner) += c;
                skipSpace = false;
            }
            break;
        }
    }
    *tmplate = tmplate->trimmed();
    *tmplate = tmplate->remove(QLatin1String("<>"));
    *inner = inner->trimmed();
    // qDebug() << "EXTRACT TEMPLATE: " << *tmplate << *inner << " FROM " << type;
    return !inner->isEmpty();
}

QString extractTypeFromPTypeOutput(const QString &str)
{
    int pos0 = str.indexOf(QLatin1Char('='));
    int pos1 = str.indexOf(QLatin1Char('{'));
    int pos2 = str.lastIndexOf(QLatin1Char('}'));
    QString res = str;
    if (pos0 != -1 && pos1 != -1 && pos2 != -1)
        res = str.mid(pos0 + 2, pos1 - 1 - pos0)
            + QLatin1String(" ... ") + str.right(str.size() - pos2);
    return res.simplified();
}

bool isIntType(const QString &type)
{
    static const QStringList types = QStringList()
        << QLatin1String("char") << QLatin1String("int") << QLatin1String("short")
        << QLatin1String("long") << QLatin1String("bool")
        << QLatin1String("signed char") << QLatin1String("unsigned")
        << QLatin1String("unsigned char")
        << QLatin1String("unsigned int") << QLatin1String("unsigned long")
        << QLatin1String("long long")  << QLatin1String("unsigned long long");
    return types.contains(type);
}

bool isIntOrFloatType(const QString &type)
{
    static const QStringList types = QStringList()
        << QLatin1String("float") << QLatin1String("double");
    return isIntType(type) || types.contains(type);
}

GuessChildrenResult guessChildren(const QString &type)
{
    if (isIntOrFloatType(type))
        return HasNoChildren;
    if (isPointerType(type))
        return HasChildren;
    if (type.endsWith(QLatin1String("QString")))
        return HasNoChildren;
    return HasPossiblyChildren;
}

QString sizeofTypeExpression(const QString &type)
{
    if (type.endsWith(QLatin1Char('*')))
        return QLatin1String("sizeof(void*)");
    if (type.endsWith(QLatin1Char('>')))
        return QLatin1String("sizeof(") + type + QLatin1Char(')');
    return QLatin1String("sizeof(") + gdbQuoteTypes(type) + QLatin1Char(')');
}

// Utilities to decode string data returned by the dumper helpers.

QString quoteUnprintableLatin1(const QByteArray &ba)
{
    QString res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const unsigned char c = ba.at(i);
        if (isprint(c)) {
            res += c;
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\%x", int(c));
            res += buf;
        }
    }
    return res;
}

QString decodeData(const QByteArray &ba, int encoding)
{
    switch (encoding) {
        case 0: // unencoded 8 bit data
            return quoteUnprintableLatin1(ba);
        case 1: { //  base64 encoded 8 bit data, used for QByteArray
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += quoteUnprintableLatin1(QByteArray::fromBase64(ba));
            rc += doubleQuote;
            return rc;
        }
        case 2: { //  base64 encoded 16 bit data, used for QString
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            QString rc = doubleQuote;
            rc += QString::fromUtf16(reinterpret_cast<const ushort *>(decodedBa.data()), decodedBa.size() / 2);
            rc += doubleQuote;
            return rc;
        }
        case 3: { //  base64 encoded 32 bit data
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += QString::fromUcs4(reinterpret_cast<const uint *>(decodedBa.data()), decodedBa.size() / 4);
            rc += doubleQuote;
            return rc;
        }
        case 4: { //  base64 encoded 16 bit data, without quotes (see 2)
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            return QString::fromUtf16(reinterpret_cast<const ushort *>(decodedBa.data()), decodedBa.size() / 2);
        }
        case 5: { //  base64 encoded 8 bit data, without quotes (see 1)
            return quoteUnprintableLatin1(QByteArray::fromBase64(ba));
        }
    }
    return QCoreApplication::translate("Debugger", "<Encoding error>");
}

// Editor tooltip support
bool isCppEditor(Core::IEditor *editor)
{
    static QStringList cppMimeTypes;
    if (cppMimeTypes.empty()) {
        cppMimeTypes << QLatin1String(CppTools::Constants::C_SOURCE_MIMETYPE)
                << QLatin1String(CppTools::Constants::CPP_SOURCE_MIMETYPE)
                << QLatin1String(CppTools::Constants::CPP_HEADER_MIMETYPE)
                << QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE);
    }
    if (const Core::IFile *file = editor->file())
        return cppMimeTypes.contains(file->mimeType());
    return  false;
}

// Find the function the cursor is in to use a scope.





// Return the Cpp expression, and, if desired, the function
QString cppExpressionAt(TextEditor::ITextEditor *editor, int pos,
                        int *line, int *column, QString *function /* = 0 */)
{

    *line = *column = 0;
    if (function)
        function->clear();

    const QPlainTextEdit *plaintext = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!plaintext)
        return QString();

    QString expr = plaintext->textCursor().selectedText();
    if (expr.isEmpty()) {
        QTextCursor tc(plaintext->document());
        tc.setPosition(pos);

        const QChar ch = editor->characterAt(pos);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
            tc.movePosition(QTextCursor::EndOfWord);

        // Fetch the expression's code.
        CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
        expr = expressionUnderCursor(tc);
        *column = tc.columnNumber();
        *line = tc.blockNumber();
    } else {
        const QTextCursor tc = plaintext->textCursor();
        *column = tc.columnNumber();
        *line = tc.blockNumber();
    }

    if (function && !expr.isEmpty())
        if (const Core::IFile *file = editor->file())
            if (CppTools::CppModelManagerInterface *modelManager = ExtensionSystem::PluginManager::instance()->getObject<CppTools::CppModelManagerInterface>())
                *function = CppTools::AbstractEditorSupport::functionAt(modelManager, file->fileName(), *line, *column);

    return expr;
}

// --------------- QtDumperResult

QtDumperResult::Child::Child() :
   keyEncoded(0),
   valueEncoded(0),
   childCount(-1),
   valuedisabled(false),
   valueEncountered(false)
{
}

QtDumperResult::QtDumperResult() :
    valueEncountered(false),
    valueEncoded(0),
    valuedisabled(false),
    childCount(-1),
    internal(false),
    childChildCount(-1)
{
}

void QtDumperResult::clear()
{
    iname.clear();
    value.clear();
    address.clear();
    addressInfo.clear();
    type.clear();
    extra.clear();
    displayedType.clear();
    valueEncoded = 0;
    valueEncountered = valuedisabled = false;
    childCount = -1;
    internal = false;
    childType.clear();
    children.clear();
    childChildCount = -1;
}

QList<WatchData> QtDumperResult::toWatchData(int source) const
{
    QList<WatchData> rc;
    rc.push_back(WatchData());
    WatchData &root = rc.front();
    root.iname = iname;
    const QChar dot = QLatin1Char('.');
    const int lastDotIndex = root.iname.lastIndexOf(dot);
    root.exp = root.name = lastDotIndex == -1 ? iname : iname.mid(lastDotIndex + 1);
    if (valueEncountered) {
        root.setValue(decodeData(value, valueEncoded));
        root.valuedisabled = valuedisabled;
    }
    root.setType(type);
    if (!displayedType.isEmpty())
        root.displayedType = displayedType;
    root.setAddress(address);
    root.source = source;
    if (childCount >= 0)
        root.setHasChildren(childCount > 0);
    // Children. Sanity check after parsing sets childcount to list size
    // if list is not empty
    if (children.empty()) {
        if (childCount > 0)
            root.setChildrenNeeded();
    } else {
        root.setChildrenUnneeded();
        for (int c = 0; c < childCount; c++) {
            const Child &dchild = children.at(c);
            rc.push_back(WatchData());
            WatchData &wchild = rc.back();
            wchild.source = source;
            wchild.iname = iname;
            // Name can be empty for array-like things
            const QString iname = dchild.name.isEmpty() ? QString::number(c) : dchild.name;
            // Use key entry as name (which is used for map nodes)
            if (dchild.key.isEmpty()) {
                wchild.name = iname;
            } else {
                // Do not use map keys as iname since they might contain quotes.
                wchild.name = decodeData(dchild.key, dchild.keyEncoded);
                if (wchild.name.size() > 13) {
                    wchild.name.truncate(12);
                    wchild.name += QLatin1String("...");
                }
            }
            // Append iname to total iname.
            wchild.iname += dot;
            wchild.iname += iname;
            wchild.exp = dchild.exp;
            if (dchild.valueEncountered) {
                wchild.valuedisabled = dchild.valuedisabled;
                wchild.setValue(decodeData(dchild.value, dchild.valueEncoded));
            }
            wchild.setAddress(dchild.address);
            // The type setter sets hasChildren for known types.
            wchild.setType(dchild.type.isEmpty() ? childType : dchild.type);
            if (!dchild.displayedType.isEmpty())
                wchild.displayedType = dchild.displayedType;
            // Child overrides.
            const int effectiveChildChildCount = dchild.childCount == -1 ?  childChildCount : dchild.childCount;
            switch (effectiveChildChildCount) {
            case -1: // In this case, trust WatchData::setType().
                break;
            case 0:
                wchild.setHasChildren(false);
                break;
            default:
                wchild.setHasChildren(true);
                wchild.setChildrenNeeded();
                break;
            }
        }
    }
    if (debug) {
        QDebug nospace = qDebug().nospace();
        nospace << "QtDumperResult::toWatchData" << *this << '\n';
        foreach(const WatchData &wd, rc)
            nospace << "  " << wd.toString() << '\n';
    }

    return rc;
}

QDebug operator<<(QDebug in, const QtDumperResult &d)
{
    QDebug nospace = in.nospace();
    nospace << " iname=" << d.iname << " type=" << d.type << " displayed=" << d.displayedType
            << " address=" << d.address;
    if (!d.addressInfo.isEmpty())
        nospace << " addressInfo=" << d.addressInfo;
    if (d.valueEncountered) {
        nospace << " encoded=" << d.valueEncoded
                << " value="  << d.value
                << " disabled=" << d.valuedisabled;
    } else {
        nospace  << " <no value>";
    }
    nospace << " childnumchild=" << d.childChildCount
            << " internal=" << d.internal
            << " extra='" << d.extra << "'\n";

    const int realChildCount = d.children.size();
    if (d.childCount || realChildCount) {
        nospace << "childCount=" << d.childCount << '/' << realChildCount
                << " childType=" << d.childType << '\n';
        for (int i = 0; i < realChildCount; i++) {
            const QtDumperResult::Child &c = d.children.at(i);
            nospace << "   #" << i << " addr=" << c.address
                    << " disabled=" << c.valuedisabled
                    << " type=" << c.type << " exp=" << c.exp
                    << " name=" << c.name;
            if (!c.key.isEmpty())
                nospace << " keyencoded=" << c.keyEncoded << " key=" << c.key;
            if (c.valueEncountered) {
                nospace << " valueencoded=" << c.valueEncoded << " value=" << c.value;
            } else {
                nospace  << " <no value>";
            }
            nospace  << "childcount=" << c.childCount << '\n';
        }
    }
    return in;
}

// ----------------- QtDumperHelper::TypeData
QtDumperHelper::TypeData::TypeData() :
    type(UnknownType),
    isTemplate(false)
{
}

void QtDumperHelper::TypeData::clear()
{
    isTemplate = false;
    type = UnknownType;
    tmplate.clear();
    inner.clear();
}

// ----------------- QtDumperHelper
QtDumperHelper::QtDumperHelper() :
    m_qtVersion(0),
    m_dumperVersion(1.0)
{
    qFill(m_specialSizes, m_specialSizes + SpecialSizeCount, 0);
    setQClassPrefixes(QString());
}

void QtDumperHelper::clear()
{
    m_nameTypeMap.clear();
    m_qtVersion = 0;
    m_dumperVersion = 1.0;
    m_qtNamespace.clear();
    m_sizeCache.clear();
    qFill(m_specialSizes, m_specialSizes + SpecialSizeCount, 0);
    m_expressionCache.clear();
    setQClassPrefixes(QString());
}

QString QtDumperHelper::msgDumperOutdated(double requiredVersion, double currentVersion)
{
    return QCoreApplication::translate("QtDumperHelper",
                                       "Found a too-old version of the debugging helper library (%1); version %2 is required.").
                                       arg(currentVersion).arg(requiredVersion);
}

static inline void formatQtVersion(int v, QTextStream &str)
{
    str  << ((v >> 16) & 0xFF) << '.' << ((v >> 8) & 0xFF) << '.' << (v & 0xFF);
}

QString QtDumperHelper::toString(bool debug) const
{
    if (debug)  {
        QString rc;
        QTextStream str(&rc);
        str << "version=";
        formatQtVersion(m_qtVersion, str);
        str << "dumperversion='" << m_dumperVersion <<  "' namespace='" << m_qtNamespace << "'," << m_nameTypeMap.size() << " known types <type enum>: ";
        const NameTypeMap::const_iterator cend = m_nameTypeMap.constEnd();
        for (NameTypeMap::const_iterator it = m_nameTypeMap.constBegin(); it != cend; ++it) {
            str <<",[" << it.key() << ',' << it.value() << ']';
        }
        str << "\nSpecial size: ";
        for (int i = 0; i < SpecialSizeCount; i++)
            str << ' ' << m_specialSizes[i];
        str << "\nSize cache: ";
        const SizeCache::const_iterator scend = m_sizeCache.constEnd();
        for (SizeCache::const_iterator it = m_sizeCache.constBegin(); it != scend; ++it) {
            str << ' ' << it.key() << '=' << it.value();
        }
        return rc;
    }
    const QString nameSpace = m_qtNamespace.isEmpty() ? QCoreApplication::translate("QtDumperHelper", "<none>") : m_qtNamespace;
    return QCoreApplication::translate("QtDumperHelper",
                                       "%n known types, Qt version: %1, Qt namespace: %2 Dumper version: %3",
                                       0, QCoreApplication::CodecForTr,
                                       m_nameTypeMap.size()).arg(qtVersionString(), nameSpace).arg(m_dumperVersion);
}

QtDumperHelper::Type QtDumperHelper::simpleType(const QString &simpleType) const
{
    return m_nameTypeMap.value(simpleType, UnknownType);
}

int QtDumperHelper::qtVersion() const
{
    return m_qtVersion;
}

QString QtDumperHelper::qtNamespace() const
{
    return m_qtNamespace;
}

void QtDumperHelper::setQtNamespace(const QString &qtNamespace)
{
    m_qtNamespace = qtNamespace;
}

int QtDumperHelper::typeCount() const
{
    return m_nameTypeMap.size();
}

// Look up unnamespaced 'std' types.
static inline QtDumperHelper::Type stdType(const QString &s)
{
    if (s == QLatin1String("vector"))
        return QtDumperHelper::StdVectorType;
    if (s == QLatin1String("deque"))
        return QtDumperHelper::StdDequeType;
    if (s == QLatin1String("set"))
        return QtDumperHelper::StdSetType;
    if (s == QLatin1String("stack"))
        return QtDumperHelper::StdStackType;
    if (s == QLatin1String("map"))
        return QtDumperHelper::StdMapType;
    if (s == QLatin1String("basic_string"))
        return QtDumperHelper::StdStringType;
    return QtDumperHelper::UnknownType;
}

QtDumperHelper::Type QtDumperHelper::specialType(QString s)
{
    // Std classes.
    if (s.startsWith(QLatin1String("std::")))
        return stdType(s.mid(5));
    // Strip namespace
    // FIXME: that's not a good idea as it makes all namespaces equal.
    const int namespaceIndex = s.lastIndexOf(QLatin1String("::"));
    if (namespaceIndex == -1) {
        // None ... check for std..
        const Type sType = stdType(s);
        if (sType != UnknownType)
            return sType;
    } else {
        s = s.mid(namespaceIndex + 2);
    }
    if (s == QLatin1String("QObject"))
        return QObjectType;
    if (s == QLatin1String("QWidget"))
        return QWidgetType;
    if (s == QLatin1String("QObjectSlot"))
        return QObjectSlotType;
    if (s == QLatin1String("QObjectSignal"))
        return QObjectSignalType;
    if (s == QLatin1String("QVector"))
        return QVectorType;
    if (s == QLatin1String("QAbstractItem"))
        return QAbstractItemType;
    if (s == QLatin1String("QMap"))
        return QMapType;
    if (s == QLatin1String("QMultiMap"))
        return QMultiMapType;
    if (s == QLatin1String("QMapNode"))
        return QMapNodeType;
    return UnknownType;
}

QtDumperHelper::ExpressionRequirement QtDumperHelper::expressionRequirements(Type t)
{

    switch (t) {
    case QAbstractItemType:
        return NeedsComplexExpression;
    case QMapType:
    case QMultiMapType:
    case QMapNodeType:
    case StdMapType:
        return NeedsCachedExpression;
    default:
        // QObjectSlotType, QObjectSignalType need the signal number, which is numeric
        break;
    }
    return NeedsNoExpression;
}

QString QtDumperHelper::qtVersionString() const
{
    QString rc;
    QTextStream str(&rc);
    formatQtVersion(m_qtVersion, str);
    return rc;
}

void QtDumperHelper::setQtVersion(int v)
{
    m_qtVersion = v;
}

void QtDumperHelper::setQtVersion(const QString &v)
{
    m_qtVersion = 0;
    const QStringList vl = v.split(QLatin1Char('.'));
    if (vl.size() == 3) {
        const int major = vl.at(0).toInt();
        const int minor = vl.at(1).toInt();
        const int patch = vl.at(2).toInt();
        m_qtVersion = (major << 16) | (minor << 8) | patch;
    }
}

// Parse a list of types.
void QtDumperHelper::parseQueryTypes(const QStringList &l, Debugger debugger)
{
    m_nameTypeMap.clear();
    const int count = l.count();
    for (int i = 0; i < count; i++) {
        const Type t = specialType(l.at(i));
        if (t != UnknownType) {
            // Exclude types that require expression syntax for CDB
            if (debugger == GdbDebugger || expressionRequirements(t) != NeedsComplexExpression)
                m_nameTypeMap.insert(l.at(i), t);
        } else {
            m_nameTypeMap.insert(l.at(i), SupportedType);
        }
    }
}

/*  A parse for dumper output:
 * "iname="local.sl",addr="0x0012BA84",value="<3 items>",valuedisabled="true",
 * numchild="3",childtype="QString",childnumchild="0",children=[{name="0",value="<binhex>",
 * valueencoded="2"},{name="1",value="dAB3AG8A",valueencoded="2"},{name="2",
 * value="dABoAHIAZQBlAA==",valueencoded="2"}]"
 * Default implementation can be used for debugging purposes. */

class DumperParser
{
public:
    explicit DumperParser(const char *s) : m_s(s) {}
    bool run();
    virtual ~DumperParser() {}

protected:
    // handle 'key="value"'
    virtual bool handleKeyword(const char *k, int size);
    virtual bool handleListStart();
    virtual bool handleListEnd();
    virtual bool handleHashStart();
    virtual bool handleHashEnd();
    virtual bool handleValue(const char *k, int size);

private:
    bool parseHash(int level, const char *&pos);
    bool parseValue(int level, const char *&pos);
    bool parseStringValue(const char *&ptr, int &size, const char *&pos) const;

    const char *m_s;
};

// get a string value with pos at the opening double quote
bool DumperParser::parseStringValue(const char *&ptr, int &size, const char *&pos) const
{
    pos++;
    const char *endValuePtr = strchr(pos, '"');
    if (!endValuePtr)
        return false;
    size = endValuePtr - pos;
    ptr = pos;
    pos = endValuePtr + 1;
    return true;
}

bool DumperParser::run()
{
    const char *ptr = m_s;
    const bool rc = parseHash(0, ptr);
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << m_s << rc;
    return rc;
}

// Parse a non-empty hash with pos at the first keyword.
// Curly braces are present at level 0 only.
// '{a="X", b="X"}'
bool DumperParser::parseHash(int level, const char *&pos)
{
    while (true) {
        switch (*pos) {
        case '\0': // EOS is acceptable at level 0 only
            return level == 0;
        case '}':
            pos++;
            return true;
        default:
            break;
        }
        const char *equalsPtr = strchr(pos, '=');
        if (!equalsPtr)
            return false;
        const int keywordLen = equalsPtr - pos;
        if (!handleKeyword(pos, keywordLen))
            return false;
        pos = equalsPtr + 1;
        if (!*pos)
            return false;
        if (!parseValue(level + 1, pos))
            return false;
        if (*pos == ',')
            pos++;
    }
    return false;
}

bool DumperParser::parseValue(int level, const char *&pos)
{
    // Simple string literal
    switch (*pos) {
    case '"': {
            const char *valuePtr;
            int valueSize;
            return parseStringValue(valuePtr, valueSize, pos) && handleValue(valuePtr, valueSize);
        }
        // A List. Note that it has a trailing comma '["a",]'
    case '[': {
            if (!handleListStart())
                return false;
            pos++;
            while (true) {
                switch (*pos) {
                case ']':
                    pos++;
                    return handleListEnd();
                case '\0':
                    return false;
                default:
                    break;
                }
                if (!parseValue(level + 1, pos))
                    return false;
                if (*pos == ',')
                    pos++;
            }
        }
        return false;
        // A hash '{a="b",b="c"}'
    case '{': {
            if (!handleHashStart())
                return false;
            pos++;
            if (!parseHash(level + 1, pos))
                return false;
            return handleHashEnd();
        }
        return false;
    }
    return false;
}

bool DumperParser::handleKeyword(const char *k, int size)
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << QByteArray(k, size);
    return true;
}

bool DumperParser::handleListStart()
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO;
    return true;
}

bool DumperParser::handleListEnd()
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO;
    return true;
}

bool DumperParser::handleHashStart()
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO;
    return true;
}

bool DumperParser::handleHashEnd()
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO;

    return true;
}

bool DumperParser::handleValue(const char *k, int size)
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << QByteArray(k, size);
    return true;
}

/* Parse 'query' (1) protocol response of the custom dumpers:
 * "'dumpers=["QByteArray","QDateTime",..."std::basic_string",],
 * qtversion=["4","5","1"],namespace="""' */

class QueryDumperParser : public DumperParser {
public:
    typedef QPair<QString, int> SizeEntry;
    explicit QueryDumperParser(const char *s);

    struct Data {
        QString qtNameSpace;
        QString qtVersion;
        QString dumperVersion;
        QStringList types;
        QList<SizeEntry> sizes;
        QMap<QString, QString> expressionCache;
    };

    inline Data data() const { return m_data; }

protected:
    virtual bool handleKeyword(const char *k, int size);
    virtual bool handleListStart();
    virtual bool handleListEnd();
    virtual bool handleHashEnd();
    virtual bool handleValue(const char *k, int size);

private:
    enum Mode { None, ExpectingDumpers, ExpectingQtVersion, ExpectingDumperVersion,
                ExpectingNameSpace, ExpectingSizes, ExpectingExpressionCache };
    Mode m_mode;
    Data m_data;
    QString m_lastSizeType;
    QString m_lastExpression;
};

QueryDumperParser::QueryDumperParser(const char *s) :
    DumperParser(s),
    m_mode(None)
{
}

bool QueryDumperParser::handleKeyword(const char *k, int size)
{
    switch (m_mode) {
    case ExpectingSizes:
        m_lastSizeType = QString::fromLatin1(k, size);
        return true;
    case ExpectingExpressionCache:
        m_lastExpression = QString::fromLatin1(k, size);
        return true;
    default:
       break;
    }
    if (!qstrncmp(k, "dumpers", size)) {
        m_mode = ExpectingDumpers;
        return true;
    }
    if (!qstrncmp(k, "qtversion", size)) {
        m_mode = ExpectingQtVersion;
        return true;
    }
    if (!qstrncmp(k, "dumperversion", size)) {
        m_mode = ExpectingDumperVersion;
        return true;
    }
    if (!qstrncmp(k, "namespace", size)) {
        m_mode = ExpectingNameSpace;
        return true;
    }
    if (!qstrncmp(k, "sizes", size)) {
        m_mode = ExpectingSizes;
        return true;
    }
    if (!qstrncmp(k, "expressions", size)) {
        m_mode = ExpectingExpressionCache;
        return true;
    }
    qWarning("%s Unexpected keyword %s.\n", Q_FUNC_INFO, QByteArray(k, size).constData());
    return false;
}

bool QueryDumperParser::handleListStart()
{
    return m_mode == ExpectingDumpers || m_mode == ExpectingQtVersion;
}

bool QueryDumperParser::handleListEnd()
{
    m_mode = None;
    return true;
}

bool QueryDumperParser::handleHashEnd()
{
    m_mode = None; // Size hash
    return true;
}

bool QueryDumperParser::handleValue(const char *k, int size)
{
    switch (m_mode) {
    case None:
        return false;
    case ExpectingDumpers:
        m_data.types.push_back(QString::fromLatin1(k, size));
        break;
    case ExpectingNameSpace:
        m_data.qtNameSpace = QString::fromLatin1(k, size);
        break;
    case ExpectingDumperVersion:
        m_data.dumperVersion = QString::fromLatin1(k, size);
        break;
    case ExpectingQtVersion: // ["4","1","5"]
        if (!m_data.qtVersion.isEmpty())
            m_data.qtVersion += QLatin1Char('.');
        m_data.qtVersion += QString::fromLatin1(k, size);
        break;
    case ExpectingSizes:
        m_data.sizes.push_back(SizeEntry(m_lastSizeType, QString::fromLatin1(k, size).toInt()));
        break;
    case ExpectingExpressionCache:
        m_data.expressionCache.insert(m_lastExpression, QString::fromLatin1(k, size));
        break;
    }
    return true;
}

static inline QString qClassName(const QString &qtNamespace, const char *className)
{
    if (qtNamespace.isEmpty())
        return QString::fromAscii(className);
    QString rc = qtNamespace;
    rc += QLatin1String("::");
    rc += QString::fromAscii(className);
    return rc;
}

void QtDumperHelper::setQClassPrefixes(const QString &qNamespace)
{
    // Prefixes with namespaces
    m_qPointerPrefix = qClassName(qNamespace, "QPointer");
    m_qSharedPointerPrefix = qClassName(qNamespace, "QSharedPointer");
    m_qSharedDataPointerPrefix = qClassName(qNamespace, "QSharedDataPointer");
    m_qWeakPointerPrefix = qClassName(qNamespace, "QWeakPointer");
}

// parse a query
bool QtDumperHelper::parseQuery(const char *data, Debugger debugger)
{
    QueryDumperParser parser(data);
    if (!parser.run())
        return false;
    clear();
    m_qtNamespace = parser.data().qtNameSpace;
    setQtVersion(parser.data().qtVersion);
    setQClassPrefixes(m_qtNamespace);
    parseQueryTypes(parser.data().types, debugger);
    foreach (const QueryDumperParser::SizeEntry &se, parser.data().sizes)
        addSize(se.first, se.second);
    m_expressionCache = parser.data().expressionCache;
    // Version
    if (!parser.data().dumperVersion.isEmpty()) {
        double dumperVersion;
        bool ok;
        dumperVersion = parser.data().dumperVersion.toDouble(&ok);
        if (ok)
            m_dumperVersion = dumperVersion;
    }
    return true;
}

void QtDumperHelper::addExpression(const QString &expression, const QString &value)
{
    m_expressionCache.insert(expression, value);
}

void QtDumperHelper::addSize(const QString &name, int size)
{
    // Special interest cases
    if (name == QLatin1String("char*")) {
        m_specialSizes[PointerSize] = size;
        return;
    }
    const SpecialSizeType st = specialSizeType(name);
    if (st != SpecialSizeCount) {
        m_specialSizes[st] = size;
        return;
    }
    do {
        // CDB helpers
        if (name == QLatin1String("std::string")) {
            m_sizeCache.insert(QLatin1String("std::basic_string<char,std::char_traits<char>,std::allocator<char> >"), size);
            m_sizeCache.insert(QLatin1String("basic_string<char,char_traits<char>,allocator<char> >"), size);
            break;
        }
        if (name == QLatin1String("std::wstring")) {
            m_sizeCache.insert(QLatin1String("basic_string<unsigned short,char_traits<unsignedshort>,allocator<unsignedshort> >"), size);
            m_sizeCache.insert(QLatin1String("std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >"), size);
            break;
        }
    } while (false);
    m_sizeCache.insert(name, size);
}

QtDumperHelper::Type QtDumperHelper::type(const QString &typeName) const
{
    const QtDumperHelper::TypeData td = typeData(typeName);
    return td.type;
}

QtDumperHelper::TypeData QtDumperHelper::typeData(const QString &typeName) const
{
    TypeData td;
    td.type = UnknownType;
    const Type st = simpleType(typeName);
    if (st != UnknownType) {
        td.isTemplate = false;
        td.type = st;
        return td;
    }
    // Try template
    td.isTemplate = extractTemplate(typeName, &td.tmplate, &td.inner);
    if (!td.isTemplate)
        return td;
    // Check the template type QMap<X,Y> -> 'QMap'
    td.type = simpleType(td.tmplate);
    return td;
}

// Format an expression to have the debugger query the
// size. Use size cache if possible
QString QtDumperHelper::evaluationSizeofTypeExpression(const QString &typeName,
                                                       Debugger /* debugger */) const
{
    // Look up special size types
    const SpecialSizeType st = specialSizeType(typeName);
    if (st != SpecialSizeCount) {
        if (const int size = m_specialSizes[st])
            return QString::number(size);
    }
    // Look up size cache
    const SizeCache::const_iterator sit = m_sizeCache.constFind(typeName);
    if (sit != m_sizeCache.constEnd())
        return QString::number(sit.value());
    // Finally have the debugger evaluate
    return sizeofTypeExpression(typeName);
}

QtDumperHelper::SpecialSizeType QtDumperHelper::specialSizeType(const QString &typeName) const
{
    if (isPointerType(typeName))
        return PointerSize;
    static const QString intType = QLatin1String("int");
    static const QString stdAllocatorPrefix = QLatin1String("std::allocator");
    if (typeName == intType)
        return IntSize;
    if (typeName.startsWith(stdAllocatorPrefix))
        return StdAllocatorSize;
    if (typeName.startsWith(m_qPointerPrefix))
        return QPointerSize;
    if (typeName.startsWith(m_qSharedPointerPrefix))
        return QSharedPointerSize;
    if (typeName.startsWith(m_qSharedDataPointerPrefix))
        return QSharedDataPointerSize;
    if (typeName.startsWith(m_qWeakPointerPrefix))
        return QWeakPointerSize;
    return SpecialSizeCount;
}

static inline bool isInteger(const QString &n)
{
    const int size = n.size();
    if (!size)
        return false;
    for (int i = 0; i < size; i++)
        if (!n.at(i).isDigit())
            return false;
    return true;
}

void QtDumperHelper::evaluationParameters(const WatchData &data,
                                          const TypeData &td,
                                          Debugger debugger,
                                          QByteArray *inBuffer,
                                          QStringList *extraArgsIn) const
{
    enum { maxExtraArgCount = 4 };

    QStringList &extraArgs = *extraArgsIn;

    // See extractTemplate for parameters
    QStringList inners = td.inner.split(QLatin1Char('@'));
    if (inners.at(0).isEmpty())
        inners.clear();
    for (int i = 0; i != inners.size(); ++i)
        inners[i] = inners[i].simplified();

    QString outertype = td.isTemplate ? td.tmplate : data.type;
    // adjust the data extract
    if (outertype == m_qtNamespace + QLatin1String("QWidget"))
        outertype = m_qtNamespace + QLatin1String("QObject");

    QString inner = td.inner;

    extraArgs.clear();

    if (!inners.empty()) {
        // "generic" template dumpers: passing sizeof(argument)
        // gives already most information the dumpers need
        const int count = qMin(int(maxExtraArgCount), inners.size());
        for (int i = 0; i < count; i++)
            extraArgs.push_back(evaluationSizeofTypeExpression(inners.at(i), debugger));
    }
    int extraArgCount = extraArgs.size();
    // Pad with zeros
    const QString zero = QString(QLatin1Char('0'));
    const int extraPad = maxExtraArgCount - extraArgCount;
    for (int i = 0; i < extraPad; i++)
        extraArgs.push_back(zero);

    // in rare cases we need more or less:
    switch (td.type) {
    case QAbstractItemType:
        inner = data.addr.mid(1);
        break;
    case QObjectSlotType:
    case QObjectSignalType: {
            // we need the number out of something like
            // iname="local.ob.slots.2" // ".deleteLater()"?
            const int pos = data.iname.lastIndexOf('.');
            const QString slotNumber = data.iname.mid(pos + 1);
            QTC_ASSERT(slotNumber.toInt() != -1, /**/);
            extraArgs[0] = slotNumber;
        }
        break;
    case QMapType:
    case QMultiMapType: {
            QString nodetype;
            if (m_qtVersion >= 0x040500) {
                nodetype = m_qtNamespace + QLatin1String("QMapNode");
                nodetype += data.type.mid(outertype.size());
            } else {
                // FIXME: doesn't work for QMultiMap
                nodetype  = data.type + QLatin1String("::Node");
            }
            //qDebug() << "OUTERTYPE: " << outertype << " NODETYPE: " << nodetype
            //    << "QT VERSION" << m_qtVersion << ((4 << 16) + (5 << 8) + 0);
            extraArgs[2] = evaluationSizeofTypeExpression(nodetype, debugger);
            extraArgs[3] = QLatin1String("(size_t)&(('");
            extraArgs[3] += nodetype;
            extraArgs[3] += QLatin1String("'*)0)->value");
        }
        break;
    case QMapNodeType:
        extraArgs[2] = evaluationSizeofTypeExpression(data.type, debugger);
        extraArgs[3] = QLatin1String("(size_t)&(('");
        extraArgs[3] += data.type;
        extraArgs[3] += QLatin1String("'*)0)->value");
        break;
    case StdVectorType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == QLatin1String("bool")) {
            outertype = QLatin1String("std::vector::bool");
        } else {
            //extraArgs[extraArgCount++] = evaluationSizeofTypeExpression(data.type, debugger);
            //extraArgs[extraArgCount++] = "(size_t)&(('" + data.type + "'*)0)->value";
        }
        break;
    case StdDequeType:
        extraArgs[1] = zero;
    case StdStackType:
        // remove 'std::allocator<...>':
        extraArgs[1] = zero;
        break;
    case StdSetType:
        // remove 'std::less<...>':
        extraArgs[1] = zero;
        // remove 'std::allocator<...>':
        extraArgs[2] = zero;
        break;
    case StdMapType: {
            // We don't want the comparator and the allocator confuse gdb.
            // But we need the offset of the second item in the value pair.
            // We read the type of the pair from the allocator argument because
            // that gets the constness "right" (in the sense that gdb can
            // read it back: "std::allocator<std::pair<Key,Value> >"
            // -> "std::pair<Key,Value>". Different debuggers have varying
            // amounts of terminating blanks...
            QString pairType = inners.at(3);
            int bracketPos = pairType.indexOf(QLatin1Char('<'));
            if (bracketPos != -1)
                pairType.remove(0, bracketPos + 1);
            const QChar closingBracket = QLatin1Char('>');
            bracketPos = pairType.lastIndexOf(closingBracket);
            if (bracketPos != -1)
                bracketPos = pairType.lastIndexOf(closingBracket, bracketPos - pairType.size() - 1);
            if (bracketPos != -1)
                pairType.truncate(bracketPos + 1);
            extraArgs[2] = QLatin1String("(size_t)&(('");
            extraArgs[2] += pairType;
            extraArgs[2] += QLatin1String("'*)0)->second");
            extraArgs[3] = zero;
        }
        break;
    case StdStringType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == QLatin1String("char")) {
            outertype = QLatin1String("std::string");
        } else if (inners.at(0) == QLatin1String("wchar_t")) {
            outertype = QLatin1String("std::wstring");
        }
        qFill(extraArgs, zero);
        break;
    case UnknownType:
        qWarning("Unknown type encountered in %s.\n", Q_FUNC_INFO);
        break;
    case SupportedType:
    case QVectorType:
    case QObjectType:
    case QWidgetType:
        break;
    }

    // Look up expressions in the cache
    if (!m_expressionCache.empty()) {
        const QMap<QString, QString>::const_iterator excCend = m_expressionCache.constEnd();
        const QStringList::iterator eend = extraArgs.end();
        for (QStringList::iterator it = extraArgs.begin(); it != eend; ++it) {
            QString &e = *it;
            if (!e.isEmpty() && e != zero && !isInteger(e)) {
                const QMap<QString, QString>::const_iterator eit = m_expressionCache.constFind(e);
                if (eit != excCend)
                    e = eit.value();
            }
        }
    }

    inBuffer->clear();
    inBuffer->append(outertype.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.iname.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.exp.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(inner.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.iname.toUtf8());
    inBuffer->append('\0');

    if (debug)
        qDebug() << '\n' << Q_FUNC_INFO << '\n' << data.toString() << "\n-->" << outertype << td.type << extraArgs;
}

/* Parse value:
 * "iname="local.sl",addr="0x0012BA84",value="<3 items>",valuedisabled="true",
 * numchild="3",childtype="QString",childnumchild="0",
 * children=[{name="0",value="<binhex>",valueencoded="2"},
 * {name="1",value="dAB3AG8A",valueencoded="2"},
 * {name="2",value="dABoAHIAZQBlAA==",valueencoded="2"}]" */

class ValueDumperParser : public DumperParser
{
public:
    explicit ValueDumperParser(const char *s);

    inline QtDumperResult result() const { return m_result; }

protected:
    virtual bool handleKeyword(const char *k, int size);
    virtual bool handleHashStart();
    virtual bool handleValue(const char *k, int size);

private:
    enum Mode { None, ExpectingIName, ExpectingAddress, ExpectingValue,
                ExpectingType, ExpectingDisplayedType, ExpectingInternal,
                ExpectingValueDisabled,  ExpectingValueEncoded,
                ExpectingCommonChildType, ExpectingChildCount,
                ExpectingChildChildOverrideCount,
                ExpectingExtra,
                IgnoreNext,
                ChildModeStart,
                ExpectingChildren,ExpectingChildName, ExpectingChildAddress,
                ExpectingChildExpression, ExpectingChildType,
                ExpectingChildDisplayedType,
                ExpectingChildKey, ExpectingChildKeyEncoded,
                ExpectingChildValue, ExpectingChildValueEncoded,
                ExpectingChildValueDisabled, ExpectingChildChildCount,
                IgnoreNextChildMode
              };

    static inline Mode nextMode(Mode in, const char *keyword, int size);

    Mode m_mode;
    QtDumperResult m_result;
};

ValueDumperParser::ValueDumperParser(const char *s) :
   DumperParser(s),
   m_mode(None)
{
}

// Check key words
ValueDumperParser::Mode ValueDumperParser::nextMode(Mode in, const char *keyword, int size)
{
    // Careful with same prefix
    switch (size) {
    case 3:
        if (!qstrncmp(keyword, "exp", size))
            return ExpectingChildExpression;
        if (!qstrncmp(keyword, "key", size))
            return ExpectingChildKey;
        break;
    case 4:
        if (!qstrncmp(keyword, "addr", size))
            return in > ChildModeStart ? ExpectingChildAddress : ExpectingAddress;
        if (!qstrncmp(keyword, "type", size))
            return in > ChildModeStart ? ExpectingChildType : ExpectingType;
        if (!qstrncmp(keyword, "name", size))
            return ExpectingChildName;
        break;
    case 5:
        if (!qstrncmp(keyword, "iname", size))
            return ExpectingIName;
        if (!qstrncmp(keyword, "value", size))
            return in > ChildModeStart ? ExpectingChildValue : ExpectingValue;
        if (!qstrncmp(keyword, "extra", size))
            return ExpectingExtra;
        break;
    case 8:
        if (!qstrncmp(keyword, "children", size))
            return ExpectingChildren;
        if (!qstrncmp(keyword, "numchild", size))
            return in > ChildModeStart ?  ExpectingChildChildCount : ExpectingChildCount;
        if (!qstrncmp(keyword, "internal", size))
            return ExpectingInternal;
        break;
    case 9:
        if (!qstrncmp(keyword, "childtype", size))
            return ExpectingCommonChildType;
        break;
    case 10:
        if (!qstrncmp(keyword, "keyencoded", size))
            return ExpectingChildKeyEncoded;
        break;
    case 12:
        if (!qstrncmp(keyword, "valueencoded", size))
            return in > ChildModeStart ? ExpectingChildValueEncoded : ExpectingValueEncoded;
        break;
    case 13:
        if (!qstrncmp(keyword, "valuedisabled", size))
            return in > ChildModeStart ? ExpectingChildValueDisabled : ExpectingValueDisabled;
        if (!qstrncmp(keyword, "displayedtype", size))
            return in > ChildModeStart ? ExpectingChildDisplayedType : ExpectingDisplayedType;
        if (!qstrncmp(keyword, "childnumchild", size))
            return ExpectingChildChildOverrideCount;
        break;
    }
    return in > ChildModeStart ? IgnoreNextChildMode : IgnoreNext;
}

bool ValueDumperParser::handleKeyword(const char *k, int size)
{
    const Mode newMode = nextMode(m_mode, k, size);
    if (debug && newMode == IgnoreNext)
        qWarning("%s Unexpected keyword %s.\n", Q_FUNC_INFO, QByteArray(k, size).constData());
    m_mode = newMode;
    return true;
}

bool ValueDumperParser::handleHashStart()
{
    m_result.children.push_back(QtDumperResult::Child());
    return true;
}

bool ValueDumperParser::handleValue(const char *k, int size)
{
    const QByteArray valueBA(k, size);
    switch (m_mode) {
    case None:
    case ChildModeStart:
        return false;
    case ExpectingIName:
        m_result.iname = QString::fromLatin1(valueBA);
        break;
        case ExpectingAddress: {
                const QString address = QString::fromLatin1(valueBA);
                if (address.startsWith(QLatin1String("0x"))) {
                    m_result.address = address;
                } else {
                    m_result.addressInfo = address;
                }
            }
            break;
    case ExpectingValue:
        m_result.valueEncountered = true;
        m_result.value = valueBA;
        break;
    case ExpectingValueDisabled:
        m_result.valuedisabled = valueBA == "true";
        break;
    case ExpectingValueEncoded:
        m_result.valueEncoded = QString::fromLatin1(valueBA).toInt();
        break;
    case ExpectingType:
        m_result.type = QString::fromLatin1(valueBA);
        break;
    case ExpectingDisplayedType:
        m_result.displayedType = QString::fromLatin1(valueBA);
        break;
    case ExpectingExtra:
        m_result.extra = valueBA;
        break;
    case ExpectingInternal:
        m_result.internal = valueBA == "true";
        break;
    case ExpectingCommonChildType:
        m_result.childType = QString::fromLatin1(valueBA);
        break;
    case ExpectingChildCount:
        m_result.childCount = QString::fromLatin1(valueBA).toInt();
        break;
    case ExpectingChildChildOverrideCount:
        m_result.childChildCount = QString::fromLatin1(valueBA).toInt();
        break;
    case ExpectingChildren:
    case IgnoreNextChildMode:
    case IgnoreNext:
        break;
    case ExpectingChildName:
        m_result.children.back().name = QString::fromLatin1(valueBA);
        break;
    case ExpectingChildAddress:
        m_result.children.back().address = QString::fromLatin1(valueBA);
        break;
    case ExpectingChildKeyEncoded:
        m_result.children.back().keyEncoded = QString::fromLatin1(valueBA).toInt();
        break;
    case ExpectingChildKey:
        m_result.children.back().key = valueBA;
        break;
    case ExpectingChildValue:
        m_result.children.back().valueEncountered = true;
        m_result.children.back().value = valueBA;
        break;
    case ExpectingChildExpression:
        m_result.children.back().exp = QString::fromLatin1(valueBA);
        break;
    case ExpectingChildValueEncoded:
        m_result.children.back().valueEncoded = QString::fromLatin1(valueBA).toInt();
        break;
    case ExpectingChildValueDisabled:
        m_result.children.back().valuedisabled = valueBA == "true";
        break;
    case ExpectingChildType:
        m_result.children.back().type = QString::fromLatin1(valueBA);
        break;
    case ExpectingChildDisplayedType:
        m_result.children.back().displayedType = QString::fromLatin1(valueBA);
        break;
    case ExpectingChildChildCount:
        m_result.children.back().childCount = QString::fromLatin1(valueBA).toInt();
        break;
    }
    return true;
}

bool QtDumperHelper::parseValue(const char *data, QtDumperResult *r)
{
    ValueDumperParser parser(data);

    if (!parser.run())
        return false;
    *r = parser.result();
    // Sanity
    if (!r->children.empty() && r->childCount != r->children.size())
        r->childCount = r->children.size();
    if (debug > 1)
        qDebug() << '\n' << data << '\n' << *r;
    return true;
}

QDebug operator<<(QDebug in, const QtDumperHelper::TypeData &d)
{
    QDebug nsp = in.nospace();
    nsp << " type=" << d.type << " tpl=" << d.isTemplate;
    if (d.isTemplate)
        nsp << d.tmplate << '<' << d.inner << '>';
    return in;
}

} // namespace Internal
} // namespace Debugger
