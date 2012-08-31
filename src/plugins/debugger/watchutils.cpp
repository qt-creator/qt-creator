/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "watchutils.h"
#include "watchdata.h"
#include "debuggerstringutils.h"
#include "gdb/gdbmi.h"

#include <utils/qtcassert.h>

#include <coreplugin/idocument.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <cpptools/cpptoolsconstants.h>
#include <cpptools/abstracteditorsupport.h>

#include <cpptools/ModelManagerInterface.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <Symbols.h>
#include <Scope.h>

#include <extensionsystem/pluginmanager.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QStringList>
#include <QTextStream>
#include <QTime>

#include <QTextCursor>
#include <QPlainTextEdit>

#include <string.h>
#include <ctype.h>

enum { debug = 0 };

// Debug helpers for code model. @todo: Move to some CppTools library?
namespace CPlusPlus {

static void debugCppSymbolRecursion(QTextStream &str, const Overview &o,
                                    const Symbol &s, bool doRecurse = true,
                                    int recursion = 0)
{
    for (int i = 0; i < recursion; i++)
        str << "  ";
    str << "Symbol: " << o.prettyName(s.name()) << " at line " << s.line();
    if (s.isFunction())
        str << " function";
    if (s.isClass())
        str << " class";
    if (s.isDeclaration())
        str << " declaration";
    if (s.isBlock())
        str << " block";
    if (doRecurse && s.isScope()) {
        const Scope *scoped = s.asScope();
        const int size =  scoped->memberCount();
        str << " scoped symbol of " << size << '\n';
        for (int m = 0; m < size; m++)
            debugCppSymbolRecursion(str, o, *scoped->memberAt(m), true, recursion + 1);
    } else {
        str << '\n';
    }
}

QDebug operator<<(QDebug d, const Symbol &s)
{
    QString output;
    CPlusPlus::Overview o;
    QTextStream str(&output);
    debugCppSymbolRecursion(str, o, s, true, 0);
    d.nospace() << output;
    return d;
}

QDebug operator<<(QDebug d, const Scope &scope)
{
    QString output;
    Overview o;
    QTextStream str(&output);
    const int size =  scope.memberCount();
    str << "Scope of " << size;
    if (scope.isNamespace())
        str << " namespace";
    if (scope.isClass())
        str << " class";
    if (scope.isEnum())
        str << " enum";
    if (scope.isBlock())
        str << " block";
    if (scope.isFunction())
        str << " function";
    if (scope.isFunction())
        str << " prototype";
#if 0 // ### port me
    if (const Symbol *owner = &scope) {
        str << " owner: ";
        debugCppSymbolRecursion(str, o, *owner, false, 0);
    } else {
        str << " 0-owner\n";
    }
#endif
    for (int s = 0; s < size; s++)
        debugCppSymbolRecursion(str, o, *scope.memberAt(s), true, 2);
    d.nospace() << output;
    return d;
}
} // namespace CPlusPlus

namespace Debugger {
namespace Internal {

bool isEditorDebuggable(Core::IEditor *editor)
{
    // Only blacklist Qml. Whitelisting would fail on C++ code in files
    // with strange names, more harm would be done this way.
    //   IDocument *file = editor->document();
    //   return !(file && file->mimeType() == "application/x-qml");
    // Nowadays, even Qml is debuggable.
    return editor;
}

QByteArray dotEscape(QByteArray str)
{
    str.replace(' ', '.');
    str.replace('\\', '.');
    str.replace('/', '.');
    return str;
}

QString currentTime()
{
    return QTime::currentTime().toString(QLatin1String("hh:mm:ss.zzz"));
}

bool isSkippableFunction(const QString &funcName, const QString &fileName)
{
    if (fileName.endsWith(QLatin1String("/qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("/moc_qobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("/qmetaobject.cpp")))
        return true;
    if (fileName.endsWith(QLatin1String("/qmetaobject_p.h")))
        return true;
    if (fileName.endsWith(QLatin1String(".moc")))
        return true;

    if (funcName.endsWith(QLatin1String("::qt_metacall")))
        return true;
    if (funcName.endsWith(QLatin1String("::d_func")))
        return true;
    if (funcName.endsWith(QLatin1String("::q_func")))
        return true;

    return false;
}

bool isLeavableFunction(const QString &funcName, const QString &fileName)
{
    if (funcName.endsWith(QLatin1String("QObjectPrivate::setCurrentSender")))
        return true;
    if (funcName.endsWith(QLatin1String("QMutexPool::get")))
        return true;

    if (fileName.endsWith(QLatin1String(".cpp"))) {
        if (fileName.endsWith(QLatin1String("/qmetaobject.cpp"))
                && funcName.endsWith(QLatin1String("QMetaObject::methodOffset")))
            return true;
        if (fileName.endsWith(QLatin1String("/qobject.cpp"))
                && (funcName.endsWith(QLatin1String("QObjectConnectionListVector::at"))
                    || funcName.endsWith(QLatin1String("~QObject"))))
            return true;
        if (fileName.endsWith(QLatin1String("/qmutex.cpp")))
            return true;
        if (fileName.endsWith(QLatin1String("/qthread.cpp")))
            return true;
        if (fileName.endsWith(QLatin1String("/qthread_unix.cpp")))
            return true;
    } else if (fileName.endsWith(QLatin1String(".h"))) {

        if (fileName.endsWith(QLatin1String("/qobject.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qmutex.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qvector.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qlist.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qhash.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qmap.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qshareddata.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qstring.h")))
            return true;
        if (fileName.endsWith(QLatin1String("/qglobal.h")))
            return true;

    } else {

        if (fileName.contains(QLatin1String("/qbasicatomic")))
            return true;
        if (fileName.contains(QLatin1String("/qorderedmutexlocker_p")))
            return true;
        if (fileName.contains(QLatin1String("/qatomic")))
            return true;
    }

    return false;
}

bool isLetterOrNumber(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9');
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
    QTC_ASSERT(!exp.isEmpty(), return false);
    switch (exp.at(0).toAscii()) {
    case 'a':
        return exp == QLatin1String("auto");
    case 'b':
        return exp == QLatin1String("break");
    case 'c':
        return exp == QLatin1String("case") || exp == QLatin1String("class")
               || exp == QLatin1String("const") || exp == QLatin1String("constexpr")
               || exp == QLatin1String("catch") || exp == QLatin1String("continue")
               || exp == QLatin1String("const_cast");
    case 'd':
        return exp == QLatin1String("do") || exp == QLatin1String("default")
               || exp == QLatin1String("delete") || exp == QLatin1String("decltype")
               || exp == QLatin1String("dynamic_cast");
    case 'e':
        return exp == QLatin1String("else") || exp == QLatin1String("extern")
               || exp == QLatin1String("enum") || exp == QLatin1String("explicit");
    case 'f':
        return exp == QLatin1String("for") || exp == QLatin1String("friend");  // 'final'?
    case 'g':
        return exp == QLatin1String("goto");
    case 'i':
        return exp == QLatin1String("if") || exp == QLatin1String("inline");
    case 'n':
        return exp == QLatin1String("new") || exp == QLatin1String("namespace")
               || exp == QLatin1String("noexcept");
    case 'm':
        return exp == QLatin1String("mutable");
    case 'o':
        return exp == QLatin1String("operator"); // 'override'?
    case 'p':
        return exp == QLatin1String("public") || exp == QLatin1String("protected")
               || exp == QLatin1String("private");
    case 'r':
        return exp == QLatin1String("return") || exp == QLatin1String("register")
               || exp == QLatin1String("reinterpret_cast");
    case 's':
        return exp == QLatin1String("struct") || exp == QLatin1String("switch")
               || exp == QLatin1String("static_cast");
    case 't':
        return exp == QLatin1String("template") || exp == QLatin1String("typename")
               || exp == QLatin1String("try") || exp == QLatin1String("throw")
               || exp == QLatin1String("typedef");
    case 'u':
        return exp == QLatin1String("union") || exp == QLatin1String("using");
    case 'v':
        return exp == QLatin1String("void") || exp == QLatin1String("volatile")
               || exp == QLatin1String("virtual");
    case 'w':
        return exp == QLatin1String("while");
    }
    return false;
}

bool startsWithDigit(const QString &str)
{
    return !str.isEmpty() && str.at(0).isDigit();
}

QByteArray stripPointerType(QByteArray type)
{
    if (type.endsWith('*'))
        type.chop(1);
    if (type.endsWith("* const"))
        type.chop(7);
    if (type.endsWith(' '))
        type.chop(1);
    return type;
}

// Format a hex address with colons as in the memory editor.
QString formatToolTipAddress(quint64 a)
{
    QString rc = QString::number(a, 16);
    if (a) {
        if (const int remainder = rc.size() % 4)
            rc.prepend(QString(4 - remainder, QLatin1Char('0')));
        const QChar colon = QLatin1Char(':');
        switch (rc.size()) {
        case 16:
            rc.insert(12, colon);
        case 12:
            rc.insert(8, colon);
        case 8:
            rc.insert(4, colon);
        }
    }
    return QLatin1String("0x") + rc;
}

/* getUninitializedVariables(): Get variables that are not initialized
 * at a certain line of a function from the code model to be able to
 * indicate them as not in scope in the locals view.
 * Find document + function in the code model, do a double check and
 * collect declarative symbols that are in the function past or on
 * the current line. blockRecursion() recurses up the scopes
 * and collect symbols declared past or on the current line.
 * Recursion goes up from the innermost scope, keeping a map
 * of occurrences seen, to be able to derive the names of
 * shadowed variables as the debugger sees them:
\code
int x;             // Occurrence (1), should be reported as "x <shadowed 1>"
if (true) {
   int x = 5; (2)  // Occurrence (2), should be reported as "x"
}
\endcode
 */

typedef QHash<QString, int> SeenHash;

static void blockRecursion(const CPlusPlus::Overview &overview,
                           const CPlusPlus::Scope *scope,
                           unsigned line,
                           QStringList *uninitializedVariables,
                           SeenHash *seenHash,
                           int level = 0)
{
    // Go backwards in case someone has identical variables in the same scope.
    // Fixme: loop variables or similar are currently seen in the outer scope
    for (int s = scope->memberCount() - 1; s >= 0; --s){
        const CPlusPlus::Symbol *symbol = scope->memberAt(s);
        if (symbol->isDeclaration()) {
            // Find out about shadowed symbols by bookkeeping
            // the already seen occurrences in a hash.
            const QString name = overview.prettyName(symbol->name());
            SeenHash::iterator it = seenHash->find(name);
            if (it == seenHash->end()) {
                it = seenHash->insert(name, 0);
            } else {
                ++(it.value());
            }
            // Is the declaration on or past the current line, that is,
            // the variable not initialized.
            if (symbol->line() >= line)
                uninitializedVariables->push_back(WatchData::shadowedName(name, it.value()));
        }
    }
    // Next block scope.
    if (const CPlusPlus::Scope *enclosingScope = scope->enclosingBlock())
        blockRecursion(overview, enclosingScope, line, uninitializedVariables, seenHash, level + 1);
}

// Inline helper with integer error return codes.
static inline
int getUninitializedVariablesI(const CPlusPlus::Snapshot &snapshot,
                               const QString &functionName,
                               const QString &file,
                               int line,
                               QStringList *uninitializedVariables)
{
    uninitializedVariables->clear();
    // Find document
    if (snapshot.isEmpty() || functionName.isEmpty() || file.isEmpty() || line < 1)
        return 1;
    const CPlusPlus::Snapshot::const_iterator docIt = snapshot.find(file);
    if (docIt == snapshot.end())
        return 2;
    const CPlusPlus::Document::Ptr doc = docIt.value();
    // Look at symbol at line and find its function. Either it is the
    // function itself or some expression/variable.
    const CPlusPlus::Symbol *symbolAtLine = doc->lastVisibleSymbolAt(line, 0);
    if (!symbolAtLine)
        return 4;
    // First figure out the function to do a safety name check
    // and the innermost scope at cursor position
    const CPlusPlus::Function *function = 0;
    const CPlusPlus::Scope *innerMostScope = 0;
    if (symbolAtLine->isFunction()) {
        function = symbolAtLine->asFunction();
        if (function->memberCount() == 1) // Skip over function block
            if (CPlusPlus::Block *block = function->memberAt(0)->asBlock())
                innerMostScope = block;
    } else {
        if (const CPlusPlus::Scope *functionScope = symbolAtLine->enclosingFunction()) {
            function = functionScope->asFunction();
            innerMostScope = symbolAtLine->isBlock() ?
                             symbolAtLine->asBlock() :
                             symbolAtLine->enclosingBlock();
        }
    }
    if (!function || !innerMostScope)
        return 7;
    // Compare function names with a bit off fuzz,
    // skipping modules from a CDB symbol "lib!foo" or namespaces
    // that the code model does not show at this point
    CPlusPlus::Overview overview;
    const QString name = overview.prettyName(function->name());
    if (!functionName.endsWith(name))
        return 11;
    if (functionName.size() > name.size()) {
        const char previousChar = functionName.at(functionName.size() - name.size() - 1).toLatin1();
        if (previousChar != ':' && previousChar != '!' )
            return 11;
    }
    // Starting from the innermost block scope, collect declarations.
    SeenHash seenHash;
    blockRecursion(overview, innerMostScope, line, uninitializedVariables, &seenHash);
    return 0;
}

bool getUninitializedVariables(const CPlusPlus::Snapshot &snapshot,
                               const QString &function,
                               const QString &file,
                               int line,
                               QStringList *uninitializedVariables)
{
    const int rc = getUninitializedVariablesI(snapshot, function, file, line, uninitializedVariables);
    if (debug) {
        QString msg;
        QTextStream str(&msg);
        str << "getUninitializedVariables() " << function << ' ' << file << ':' << line
                << " returns (int) " << rc << " '"
                << uninitializedVariables->join(QString(QLatin1Char(','))) << '\'';
        if (rc)
            str << " of " << snapshot.size() << " documents";
        qDebug() << msg;
    }
    return rc == 0;
}

QByteArray gdbQuoteTypes(const QByteArray &type)
{
    // gdb does not understand sizeof(Core::IDocument*).
    // "sizeof('Core::IDocument*')" is also not acceptable,
    // it needs to be "sizeof('Core::IDocument'*)"
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
        return gdbQuoteTypes(stripPointerType(type)) + '*';

    QByteArray accu;
    QByteArray result;
    int templateLevel = 0;

    const char colon = ':';
    const char singleQuote = '\'';
    const char lessThan = '<';
    const char greaterThan = '>';
    for (int i = 0; i != type.size(); ++i) {
        const char c = type.at(i);
        if (isLetterOrNumber(c) || c == '_' || c == colon || c == ' ') {
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

// Utilities to decode string data returned by the dumper helpers.

QString quoteUnprintableLatin1(const QByteArray &ba)
{
    QString res;
    char buf[10];
    for (int i = 0, n = ba.size(); i != n; ++i) {
        const unsigned char c = ba.at(i);
        if (isprint(c)) {
            res += QLatin1Char(c);
        } else {
            qsnprintf(buf, sizeof(buf) - 1, "\\%x", int(c));
            res += QLatin1String(buf);
        }
    }
    return res;
}

static QDate dateFromData(int jd)
{
    return jd ? QDate::fromJulianDay(jd) : QDate();
}

static QTime timeFromData(int ms)
{
    return ms == -1 ? QTime() : QTime(0, 0, 0, 0).addMSecs(ms);
}

QString decodeData(const QByteArray &ba, int encoding)
{
    switch (encoding) {
        case Unencoded8Bit: // 0
            return quoteUnprintableLatin1(ba);
        case Base64Encoded8BitWithQuotes: { // 1, used for QByteArray
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += quoteUnprintableLatin1(QByteArray::fromBase64(ba));
            rc += doubleQuote;
            return rc;
        }
        case Base64Encoded16BitWithQuotes: { // 2, used for QString
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            QString rc = doubleQuote;
            rc += QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
            rc += doubleQuote;
            return rc;
        }
        case Base64Encoded32BitWithQuotes: { // 3
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4);
            rc += doubleQuote;
            return rc;
        }
        case Base64Encoded16Bit: { // 4, without quotes (see 2)
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            return QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
        }
        case Base64Encoded8Bit: { // 5, without quotes (see 1)
            return quoteUnprintableLatin1(QByteArray::fromBase64(ba));
        }
        case Hex2EncodedLatin1WithQuotes: { // 6, %02x encoded 8 bit Latin1 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromLatin1(decodedBa) + doubleQuote;
        }
        case Hex4EncodedLittleEndianWithQuotes: { // 7, %04x encoded 16 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2) + doubleQuote;
        }
        case Hex8EncodedLittleEndianWithQuotes: { // 8, %08x encoded 32 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4) + doubleQuote;
        }
        case Hex2EncodedUtf8WithQuotes: { // 9, %02x encoded 8 bit UTF-8 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromUtf8(decodedBa) + doubleQuote;
        }
        case Hex8EncodedBigEndian: { // 10, %08x encoded 32 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            QByteArray decodedBa = QByteArray::fromHex(ba);
            for (int i = 0; i < decodedBa.size() - 3; i += 4) {
                char c = decodedBa.at(i);
                decodedBa[i] = decodedBa.at(i + 3);
                decodedBa[i + 3] = c;
                c = decodedBa.at(i + 1);
                decodedBa[i + 1] = decodedBa.at(i + 2);
                decodedBa[i + 2] = c;
            }
            return doubleQuote + QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4) + doubleQuote;
        }
        case Hex4EncodedBigEndianWithQuotes: { // 11, %04x encoded 16 bit data
            const QChar doubleQuote(QLatin1Char('"'));
            QByteArray decodedBa = QByteArray::fromHex(ba);
            for (int i = 0; i < decodedBa.size(); i += 2) {
                char c = decodedBa.at(i);
                decodedBa[i] = decodedBa.at(i + 1);
                decodedBa[i + 1] = c;
            }
            return doubleQuote + QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2) + doubleQuote;
        }
        case Hex4EncodedLittleEndianWithoutQuotes: { // 12, see 7, without quotes
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
        }
        case Hex2EncodedLocal8BitWithQuotes: { // 13, %02x encoded 8 bit UTF-8 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            return doubleQuote + QString::fromLocal8Bit(decodedBa) + doubleQuote;
        }
        case JulianDate: { // 14, an integer count
            const QDate date = dateFromData(ba.toInt());
            return date.toString(Qt::TextDate);
        }
        case MillisecondsSinceMidnight: {
            const QTime time = timeFromData(ba.toInt());
            return time.toString(Qt::TextDate);
        }
        case JulianDateAndMillisecondsSinceMidnight: {
            const int p = ba.indexOf('/');
            const QDate date = dateFromData(ba.left(p).toInt());
            const QTime time = timeFromData(ba.mid(p + 1 ).toInt());
            return QDateTime(date, time).toString(Qt::TextDate);
        }
    }
    qDebug() << "ENCODING ERROR: " << encoding;
    return QCoreApplication::translate("Debugger", "<Encoding error>");
}

template <class T>
void decodeArrayHelper(QList<WatchData> *list, const WatchData &tmplate,
    const QByteArray &rawData)
{
    const QByteArray ba = QByteArray::fromHex(rawData);
    const T *p = (const T *) ba.data();
    WatchData data;
    const QByteArray exp = "*(" + gdbQuoteTypes(tmplate.type) + "*)0x";
    for (int i = 0, n = ba.size() / sizeof(T); i < n; ++i) {
        data = tmplate;
        data.sortId = i;
        data.iname += QByteArray::number(i);
        data.name = QString::fromLatin1("[%1]").arg(i);
        data.value = QString::number(p[i]);
        data.address += i * sizeof(T);
        data.exp = exp + QByteArray::number(data.address, 16);
        data.setAllUnneeded();
        list->append(data);
    }
}

void decodeArray(QList<WatchData> *list, const WatchData &tmplate,
    const QByteArray &rawData, int encoding)
{
    switch (encoding) {
        case Hex2EncodedInt1:
            decodeArrayHelper<uchar>(list, tmplate, rawData);
            break;
        case Hex2EncodedInt2:
            decodeArrayHelper<ushort>(list, tmplate, rawData);
            break;
        case Hex2EncodedInt4:
            decodeArrayHelper<uint>(list, tmplate, rawData);
            break;
        case Hex2EncodedInt8:
            decodeArrayHelper<quint64>(list, tmplate, rawData);
            break;
        case Hex2EncodedFloat4:
            decodeArrayHelper<float>(list, tmplate, rawData);
            break;
        case Hex2EncodedFloat8:
            decodeArrayHelper<double>(list, tmplate, rawData);
            break;
        default:
            qDebug() << "ENCODING ERROR: " << encoding;
    }
}

// Editor tooltip support
bool isCppEditor(Core::IEditor *editor)
{
    using namespace CppTools::Constants;
    const Core::IDocument *document= editor->document();
    if (!document)
        return false;
    const QByteArray mimeType = document->mimeType().toLatin1();
    return mimeType == C_SOURCE_MIMETYPE
        || mimeType == CPP_SOURCE_MIMETYPE
        || mimeType == CPP_HEADER_MIMETYPE
        || mimeType == OBJECTIVE_CPP_SOURCE_MIMETYPE;
}

// Return the Cpp expression, and, if desired, the function
QString cppExpressionAt(TextEditor::ITextEditor *editor, int pos,
                        int *line, int *column, QString *function /* = 0 */)
{
    using namespace CppTools;
    using namespace CPlusPlus;
    *line = *column = 0;
    if (function)
        function->clear();

    const QPlainTextEdit *plaintext = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!plaintext)
        return QString();

    QString expr = plaintext->textCursor().selectedText();
    CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
    if (expr.isEmpty() && modelManager) {
        QTextCursor tc(plaintext->document());
        tc.setPosition(pos);

        const QChar ch = editor->characterAt(pos);
        if (ch.isLetterOrNumber() || ch == QLatin1Char('_'))
            tc.movePosition(QTextCursor::EndOfWord);

        // Fetch the expression's code.
        CPlusPlus::ExpressionUnderCursor expressionUnderCursor;
        expr = expressionUnderCursor(tc);
        *column = tc.positionInBlock();
        *line = tc.blockNumber();
    } else {
        const QTextCursor tc = plaintext->textCursor();
        *column = tc.positionInBlock();
        *line = tc.blockNumber();
    }

    if (function && !expr.isEmpty())
        if (const Core::IDocument *document= editor->document())
            if (modelManager)
                *function = AbstractEditorSupport::functionAt(modelManager,
                    document->fileName(), *line, *column);

    return expr;
}

// Ensure an expression can be added as side-effect
// free debugger expression.
QString fixCppExpression(const QString &expIn)
{
    QString exp = expIn;
    // Extract the first identifier, everything else is considered
    // too dangerous.
    int pos1 = 0, pos2 = exp.size();
    bool inId = false;
    for (int i = 0; i != exp.size(); ++i) {
        const QChar c = exp.at(i);
        const bool isIdChar = c.isLetterOrNumber() || c.unicode() == '_';
        if (inId && !isIdChar) {
            pos2 = i;
            break;
        }
        if (!inId && isIdChar) {
            inId = true;
            pos1 = i;
        }
    }
    exp = exp.mid(pos1, pos2 - pos1);

    if (exp.isEmpty() || exp.startsWith(QLatin1Char('#')) || !hasLetterOrNumber(exp) || isKeyWord(exp))
        return QString();

    if (exp.startsWith(QLatin1Char('"')) && exp.endsWith(QLatin1Char('"')))
        return QString();

    if (exp.startsWith(QLatin1String("++")) || exp.startsWith(QLatin1String("--")))
        exp.remove(0, 2);

    if (exp.endsWith(QLatin1String("++")) || exp.endsWith(QLatin1String("--")))
        exp.truncate(exp.size() - 2);

    if (exp.startsWith(QLatin1Char('<')) || exp.startsWith(QLatin1Char('[')))
        return QString();

    if (hasSideEffects(exp) || exp.isEmpty())
        return QString();
    return exp;
}

QString cppFunctionAt(const QString &fileName, int line)
{
    using namespace CppTools;
    using namespace CPlusPlus;
    CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
    return AbstractEditorSupport::functionAt(modelManager,
                                             fileName, line, 1);
}

//////////////////////////////////////////////////////////////////////
//
// GdbMi interaction
//
//////////////////////////////////////////////////////////////////////

void setWatchDataValue(WatchData &data, const GdbMi &item)
{
    GdbMi value = item.findChild("value");
    if (value.isValid()) {
        int encoding = item.findChild("valueencoded").data().toInt();
        data.setValue(decodeData(value.data(), encoding));
    } else {
        data.setValueNeeded();
    }
}

void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
    int encoding)
{
    if (mi.isValid())
        data.setValueToolTip(decodeData(mi.data(), encoding));
}

void setWatchDataChildCount(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.setHasChildren(mi.data().toInt() > 0);
}

void setWatchDataValueEnabled(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEnabled = true;
    else if (mi.data() == "false")
        data.valueEnabled = false;
}

static void setWatchDataValueEditable(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEditable = true;
    else if (mi.data() == "false")
        data.valueEditable = false;
}

static void setWatchDataExpression(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.exp = mi.data();
}

static void setWatchDataAddress(WatchData &data, quint64 address, quint64 origAddress = 0)
{
    if (origAddress) { // Gdb dumpers reports the dereferenced address as origAddress
        data.address = origAddress;
        data.referencingAddress = address;
    } else {
        data.address = address;
    }
    if (data.exp.isEmpty() && !data.dumperFlags.startsWith('$')) {
        if (data.iname.startsWith("local.") && data.iname.count('.') == 1)
            // Solve one common case of adding 'class' in
            // *(class X*)0xdeadbeef for gdb.
            data.exp = data.name.toLatin1();
        else
            data.exp = "*(" + gdbQuoteTypes(data.type) + "*)" + data.hexAddress();
    }
}

void setWatchDataAddress(WatchData &data, const GdbMi &addressMi, const GdbMi &origAddressMi)
{
    if (!addressMi.isValid())
        return;
    const QByteArray addressBA = addressMi.data();
    if (!addressBA.startsWith("0x")) { // Item model dumpers pull tricks.
        data.dumperFlags = addressBA;
        return;
    }
    const quint64 address = addressMi.toAddress();
    const quint64 origAddress = origAddressMi.toAddress();
    setWatchDataAddress(data, address, origAddress);
}

static void setWatchDataSize(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid()) {
        bool ok = false;
        const unsigned size = mi.data().toUInt(&ok);
        if (ok)
            data.size = size;
    }
}

// Find the "type" and "displayedtype" children of root and set up type.
void setWatchDataType(WatchData &data, const GdbMi &item)
{
    if (item.isValid())
        data.setType(item.data());
    else if (data.type.isEmpty())
        data.setTypeNeeded();
}

void setWatchDataDisplayedType(WatchData &data, const GdbMi &item)
{
    if (item.isValid())
        data.displayedType = _(item.data());
}

void parseWatchData(const QSet<QByteArray> &expandedINames,
    const WatchData &data0, const GdbMi &item,
    QList<WatchData> *list)
{
    //qDebug() << "HANDLE CHILDREN: " << data0.toString() << item.toString();
    WatchData data = data0;
    bool isExpanded = expandedINames.contains(data.iname);
    if (!isExpanded)
        data.setChildrenUnneeded();

    GdbMi children = item.findChild("children");
    if (children.isValid() || !isExpanded)
        data.setChildrenUnneeded();

    setWatchDataType(data, item.findChild("type"));
    GdbMi mi = item.findChild("editvalue");
    if (mi.isValid())
        data.editvalue = mi.data();
    mi = item.findChild("editformat");
    if (mi.isValid())
        data.editformat = mi.data().toInt();
    mi = item.findChild("typeformats");
    if (mi.isValid())
        data.typeFormats = QString::fromUtf8(mi.data());
    mi = item.findChild("bitpos");
    if (mi.isValid())
        data.bitpos = mi.data().toInt();
    mi = item.findChild("bitsize");
    if (mi.isValid())
        data.bitsize = mi.data().toInt();

    setWatchDataValue(data, item);
    setWatchDataAddress(data, item.findChild("addr"), item.findChild("origaddr"));
    setWatchDataSize(data, item.findChild("size"));
    setWatchDataExpression(data, item.findChild("exp"));
    setWatchDataValueEnabled(data, item.findChild("valueenabled"));
    setWatchDataValueEditable(data, item.findChild("valueeditable"));
    setWatchDataChildCount(data, item.findChild("numchild"));
    //qDebug() << "\nAPPEND TO LIST: " << data.toString() << "\n";
    list->append(data);

    bool ok = false;
    qulonglong addressBase = item.findChild("addrbase").data().toULongLong(&ok, 0);
    qulonglong addressStep = item.findChild("addrstep").data().toULongLong(&ok, 0);

    // Try not to repeat data too often.
    WatchData childtemplate;
    setWatchDataType(childtemplate, item.findChild("childtype"));
    setWatchDataChildCount(childtemplate, item.findChild("childnumchild"));
    //qDebug() << "CHILD TEMPLATE:" << childtemplate.toString();

    mi = item.findChild("arraydata");
    if (mi.isValid()) {
        int encoding = item.findChild("arrayencoding").data().toInt();
        childtemplate.iname = data.iname + '.';
        childtemplate.address = addressBase;
        decodeArray(list, childtemplate, mi.data(), encoding);
    } else {
        for (int i = 0, n = children.children().size(); i != n; ++i) {
            const GdbMi &child = children.children().at(i);
            WatchData data1 = childtemplate;
            data1.sortId = i;
            GdbMi name = child.findChild("name");
            if (name.isValid())
                data1.name = _(name.data());
            else
                data1.name = QString::number(i);
            GdbMi iname = child.findChild("iname");
            if (iname.isValid()) {
                data1.iname = iname.data();
            } else {
                data1.iname = data.iname;
                data1.iname += '.';
                data1.iname += data1.name.toLatin1();
            }
            if (!data1.name.isEmpty() && data1.name.at(0).isDigit())
                data1.name = QLatin1Char('[') + data1.name + QLatin1Char(']');
            if (addressStep) {
                setWatchDataAddress(data1, addressBase);
                addressBase += addressStep;
            }
            QByteArray key = child.findChild("key").data();
            if (!key.isEmpty()) {
                int encoding = child.findChild("keyencoded").data().toInt();
                QString skey = decodeData(key, encoding);
                if (skey.size() > 13) {
                    skey = skey.left(12);
                    skey += _("...");
                }
                //data1.name += " (" + skey + ")";
                data1.name = skey;
            }
            parseWatchData(expandedINames, data1, child, list);
        }
    }
}


} // namespace Internal
} // namespace Debugger
