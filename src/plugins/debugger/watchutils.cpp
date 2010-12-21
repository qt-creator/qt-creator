/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "watchutils.h"
#include "watchdata.h"
#include "debuggerstringutils.h"
#include "gdb/gdbmi.h"

#include <utils/qtcassert.h>

#include <coreplugin/ifile.h>

#include <texteditor/basetexteditor.h>
#include <texteditor/basetextmark.h>
#include <texteditor/itexteditor.h>
#include <texteditor/texteditorconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsconstants.h>

#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Overview.h>
#include <Symbols.h>
#include <Scope.h>

#include <extensionsystem/pluginmanager.h>

#include <QtCore/QDebug>
#include <QtCore/QTime>
#include <QtCore/QStringList>
#include <QtCore/QCoreApplication>
#include <QtCore/QTextStream>
#include <QtCore/QHash>

#include <QtGui/QTextCursor>
#include <QtGui/QPlainTextEdit>

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
    const int size = scope->memberCount();
    for (int s = 0; s < size; s++){
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

bool extractTemplate(const QByteArray &type, QByteArray *tmplate, QByteArray *inner)
{
    // Input "Template<Inner1,Inner2,...>::Foo" will return "Template::Foo" in
    // 'tmplate' and "Inner1@Inner2@..." etc in 'inner'. Result indicates
    // whether parsing was successful
    // Gdb inserts a blank after each comma which we would like to avoid
    tmplate->clear();
    inner->clear();
    if (!type.contains('<'))
        return  false;
    int level = 0;
    bool skipSpace = false;
    const int size = type.size();

    for (int i = 0; i != size; ++i) {
        const char c = type.at(i);
        switch (c) {
        case '<':
            *(level == 0 ? tmplate : inner) += c;
            ++level;
            break;
        case '>':
            --level;
            *(level == 0 ? tmplate : inner) += c;
            break;
        case ',':
            *inner += (level == 1) ? '@' : ',';
            skipSpace = true;
            break;
        default:
            if (!skipSpace || c != ' ') {
                *(level == 0 ? tmplate : inner) += c;
                skipSpace = false;
            }
            break;
        }
    }
    *tmplate = tmplate->trimmed();
    tmplate->replace("<>", "");
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

bool isSymbianIntType(const QByteArray &type)
{
    return type == "TInt" || type == "TBool";
}

QByteArray sizeofTypeExpression(const QByteArray &type, QtDumperHelper::Debugger debugger)
{
    if (type.endsWith('*'))
        return "sizeof(void*)";
    if (debugger != QtDumperHelper::GdbDebugger || type.endsWith('>'))
        return "sizeof(" + type + ')';
    return "sizeof(" + gdbQuoteTypes(type) + ')';
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
            rc += QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
            rc += doubleQuote;
            return rc;
        }
        case 3: { //  base64 encoded 32 bit data
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            const QChar doubleQuote(QLatin1Char('"'));
            QString rc = doubleQuote;
            rc += QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4);
            rc += doubleQuote;
            return rc;
        }
        case 4: { //  base64 encoded 16 bit data, without quotes (see 2)
            const QByteArray decodedBa = QByteArray::fromBase64(ba);
            return QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
        }
        case 5: { //  base64 encoded 8 bit data, without quotes (see 1)
            return quoteUnprintableLatin1(QByteArray::fromBase64(ba));
        }
        case 6: { //  %02x encoded 8 bit Latin1 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return doubleQuote + QString::fromLatin1(decodedBa) + doubleQuote;
        }
        case 7: { //  %04x encoded 16 bit data, Little Endian
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return doubleQuote + QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2) + doubleQuote;
        }
        case 8: { //  %08x encoded 32 bit data, Little Endian
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return doubleQuote + QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4) + doubleQuote;
        }
        case 9: { //  %02x encoded 8 bit Utf-8 data
            const QChar doubleQuote(QLatin1Char('"'));
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return doubleQuote + QString::fromUtf8(decodedBa) + doubleQuote;
        }
        case 10: { //  %08x encoded 32 bit data, Big Endian
            const QChar doubleQuote(QLatin1Char('"'));
            QByteArray decodedBa = QByteArray::fromHex(ba);
            for (int i = 0; i < decodedBa.size(); i += 4) {
                char c = decodedBa.at(i);
                decodedBa[i] = decodedBa.at(i + 3);
                decodedBa[i + 3] = c;
                c = decodedBa.at(i + 1);
                decodedBa[i + 1] = decodedBa.at(i + 2);
                decodedBa[i + 2] = c;
            }
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return doubleQuote + QString::fromUcs4(reinterpret_cast<const uint *>
                (decodedBa.data()), decodedBa.size() / 4) + doubleQuote;
        }
        case 11: { //  %04x encoded 16 bit data, Big Endian
            const QChar doubleQuote(QLatin1Char('"'));
            QByteArray decodedBa = QByteArray::fromHex(ba);
            for (int i = 0; i < decodedBa.size(); i += 2) {
                char c = decodedBa.at(i);
                decodedBa[i] = decodedBa.at(i + 1);
                decodedBa[i + 1] = c;
            }
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return doubleQuote + QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2) + doubleQuote;
        }
        case 12: { //  %04x encoded 16 bit data, Little Endian, without quotes (see 7)
            const QByteArray decodedBa = QByteArray::fromHex(ba);
            //qDebug() << quoteUnprintableLatin1(decodedBa) << "\n\n";
            return QString::fromUtf16(reinterpret_cast<const ushort *>
                (decodedBa.data()), decodedBa.size() / 2);
        }
    }
    qDebug() << "ENCODING ERROR: " << encoding;
    return QCoreApplication::translate("Debugger", "<Encoding error>");
}

TextEditor::ITextEditor *currentTextEditor()
{
    if (const Core::EditorManager *editorManager = Core::EditorManager::instance())
            if (Core::IEditor *editor = editorManager->currentEditor())
                return qobject_cast<TextEditor::ITextEditor*>(editor);
    return 0;
}

// Editor tooltip support
bool isCppEditor(Core::IEditor *editor)
{
    using namespace CppTools::Constants;
    const Core::IFile *file = editor->file();
    if (!file)
        return false;
    const QByteArray mimeType = file->mimeType().toLatin1();
    return mimeType == C_SOURCE_MIMETYPE
        || mimeType == CPP_SOURCE_MIMETYPE
        || mimeType == CPP_HEADER_MIMETYPE
        || mimeType == OBJECTIVE_CPP_SOURCE_MIMETYPE;
}

bool currentTextEditorPosition(QString *fileNameIn /* = 0 */,
                               int *lineNumberIn /* = 0 */)
{
    QString fileName;
    int  lineNumber = 0;
    if (TextEditor::ITextEditor *textEditor = currentTextEditor()) {
        if (const Core::IFile *file = textEditor->file()) {
            fileName = file->fileName();
            lineNumber = textEditor->currentLine();
        }
    }
    if (fileNameIn)
        *fileNameIn = fileName;
    if (lineNumberIn)
        *lineNumberIn = lineNumber;
    return !fileName.isEmpty();
}

static CppTools::CppModelManagerInterface *cppModelManager()
{
    using namespace CppTools;
    static QPointer<CppModelManagerInterface> modelManager;
    if (!modelManager.data())
        modelManager = ExtensionSystem::PluginManager::instance()->
                getObject<CppTools::CppModelManagerInterface>();
    return modelManager.data();
}

// Return the Cpp expression, and, if desired, the function
QString cppExpressionAt(TextEditor::ITextEditor *editor, int pos,
                        int *line, int *column, QString *function /* = 0 */)
{
    using namespace CppTools;
    *line = *column = 0;
    if (function)
        function->clear();

    const QPlainTextEdit *plaintext = qobject_cast<QPlainTextEdit*>(editor->widget());
    if (!plaintext)
        return QByteArray();

    QString expr = plaintext->textCursor().selectedText();
    CppModelManagerInterface *modelManager = cppModelManager();
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
        if (const Core::IFile *file = editor->file())
            if (modelManager)
                *function = AbstractEditorSupport::functionAt(modelManager,
                    file->fileName(), *line, *column);

    return expr.toUtf8();
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
    setQClassPrefixes(QByteArray());
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
    setQClassPrefixes(QByteArray());
}

QString QtDumperHelper::msgDumperOutdated(double requiredVersion, double currentVersion)
{
    return QCoreApplication::translate("QtDumperHelper",
       "Found an outdated version of the debugging helper library (%1); "
       "version %2 is required.").
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
            str << ' ' << it.key() << '=' << it.value() << '\n';
        }
        str << "\nExpression cache: (" << m_expressionCache.size() << ")\n";
        const ExpressionCache::const_iterator excend = m_expressionCache.constEnd();
        for (ExpressionCache::const_iterator it = m_expressionCache.constBegin(); it != excend; ++it)
            str << "    " << it.key() << ' ' << it.value() << '\n';
        return rc;
    }
    const QString nameSpace = m_qtNamespace.isEmpty()
        ? QCoreApplication::translate("QtDumperHelper", "<none>") : m_qtNamespace;
    return QCoreApplication::translate("QtDumperHelper",
       "%n known types, Qt version: %1, Qt namespace: %2 Dumper version: %3",
       0, QCoreApplication::CodecForTr,
       m_nameTypeMap.size()).arg(qtVersionString(), nameSpace).arg(m_dumperVersion);
}

QtDumperHelper::Type QtDumperHelper::simpleType(const QByteArray &simpleType) const
{
    return m_nameTypeMap.value(simpleType, UnknownType);
}

int QtDumperHelper::qtVersion() const
{
    return m_qtVersion;
}

QByteArray QtDumperHelper::qtNamespace() const
{
    return m_qtNamespace;
}

int QtDumperHelper::typeCount() const
{
    return m_nameTypeMap.size();
}

// Look up unnamespaced 'std' types.
static QtDumperHelper::Type stdType(const QByteArray &type)
{
    if (type == "vector")
        return QtDumperHelper::StdVectorType;
    if (type == "deque")
        return QtDumperHelper::StdDequeType;
    if (type == "set")
        return QtDumperHelper::StdSetType;
    if (type == "stack")
        return QtDumperHelper::StdStackType;
    if (type == "map")
        return QtDumperHelper::StdMapType;
    if (type == "basic_string")
        return QtDumperHelper::StdStringType;
    return QtDumperHelper::UnknownType;
}

static QtDumperHelper::Type specialType(QByteArray type)
{
    // Std classes.
    if (type.startsWith("std::"))
        return stdType(type.mid(5));

    // Strip namespace
    // FIXME: that's not a good idea as it makes all namespaces equal.
    const int namespaceIndex = type.lastIndexOf("::");
    if (namespaceIndex == -1) {
        // None ... check for std..
        const QtDumperHelper::Type sType = stdType(type);
        if (sType != QtDumperHelper::UnknownType)
            return sType;
    } else {
        type = type.mid(namespaceIndex + 2);
    }

    if (type == "QAbstractItem")
        return QtDumperHelper::QAbstractItemType;
    if (type == "QMap")
        return QtDumperHelper::QMapType;
    if (type == "QMapNode")
        return QtDumperHelper::QMapNodeType;
    if (type == "QMultiMap")
        return QtDumperHelper::QMultiMapType;
    if (type == "QObject")
        return QtDumperHelper::QObjectType;
    if (type == "QObjectSignal")
        return QtDumperHelper::QObjectSignalType;
    if (type == "QObjectSlot")
        return QtDumperHelper::QObjectSlotType;
    if (type == "QStack")
        return QtDumperHelper::QStackType;
    if (type == "QVector")
        return QtDumperHelper::QVectorType;
    if (type == "QWidget")
        return QtDumperHelper::QWidgetType;
    return QtDumperHelper::UnknownType;
}

QByteArray QtDumperHelper::qtVersionString() const
{
    QString rc;
    QTextStream str(&rc);
    formatQtVersion(m_qtVersion, str);
    return rc.toLatin1();
}

// Parse a list of types.
typedef QList<QByteArray> QByteArrayList;

static inline QByteArray qClassName(const QByteArray &qtNamespace, const char *className)
{
    if (qtNamespace.isEmpty())
        return className;
    QByteArray rc = qtNamespace;
    rc += "::";
    rc += className;
    return rc;
}

void QtDumperHelper::setQClassPrefixes(const QByteArray &qNamespace)
{
    // Prefixes with namespaces
    m_qPointerPrefix = qClassName(qNamespace, "QPointer");
    m_qSharedPointerPrefix = qClassName(qNamespace, "QSharedPointer");
    m_qSharedDataPointerPrefix = qClassName(qNamespace, "QSharedDataPointer");
    m_qWeakPointerPrefix = qClassName(qNamespace, "QWeakPointer");
    m_qListPrefix = qClassName(qNamespace, "QList");
    m_qLinkedListPrefix = qClassName(qNamespace, "QLinkedList");
    m_qVectorPrefix = qClassName(qNamespace, "QVector");
    m_qQueuePrefix = qClassName(qNamespace, "QQueue");
}

static inline double getDumperVersion(const GdbMi &contents)
{
    const GdbMi dumperVersionG = contents.findChild("dumperversion");
    if (dumperVersionG.type() != GdbMi::Invalid) {
        bool ok;
        const double v = QString::fromAscii(dumperVersionG.data()).toDouble(&ok);
        if (ok)
            return v;
    }
    return 1.0;
}

bool QtDumperHelper::parseQuery(const GdbMi &contents)
{
    clear();
    if (debug > 1)
        qDebug() << "parseQuery" << contents.toString(true, 2);

    // Common info, dumper version, etc
    m_qtNamespace = contents.findChild("namespace").data();
    int qtv = 0;
    const GdbMi qtversion = contents.findChild("qtversion");
    if (qtversion.children().size() == 3) {
        qtv = (qtversion.childAt(0).data().toInt() << 16)
                    + (qtversion.childAt(1).data().toInt() << 8)
                    + qtversion.childAt(2).data().toInt();
    }
    m_qtVersion = qtv;
    // Get list of helpers
    QByteArrayList availableSimpleDebuggingHelpers;
    foreach (const GdbMi &item, contents.findChild("dumpers").children())
        availableSimpleDebuggingHelpers.append(item.data());

    // Parse types
    m_nameTypeMap.clear();
    foreach (const QByteArray &type, availableSimpleDebuggingHelpers) {
        const Type t = specialType(type);
        m_nameTypeMap.insert(type, t != UnknownType ? t : SupportedType);
    }

    m_dumperVersion = getDumperVersion(contents);
    // Parse sizes
    foreach (const GdbMi &sizesList, contents.findChild("sizes").children()) {
        const int childCount = sizesList.childCount();
        if (childCount > 1) {
            const int size = sizesList.childAt(0).data().toInt();
            for (int c = 1; c < childCount; c++)
                addSize(sizesList.childAt(c).data(), size);
        }
    }
    // Parse expressions
    foreach (const GdbMi &exprList, contents.findChild("expressions").children())
        if (exprList.childCount() == 2)
            m_expressionCache.insert(exprList.childAt(0).data(),
                                     exprList.childAt(1).data());
    return true;
}

// parse a query
bool QtDumperHelper::parseQuery(const char *data)
{
    GdbMi root;
    root.fromStringMultiple(QByteArray(data));
    if (!root.isValid())
        return false;
    return parseQuery(root);
}

void QtDumperHelper::addSize(const QByteArray &name, int size)
{
    // Special interest cases
    if (name == "char*") {
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
        if (name == "std::string") {
            m_sizeCache.insert("std::basic_string<char,std::char_traits<char>,std::allocator<char> >", size);
            m_sizeCache.insert("basic_string<char,char_traits<char>,allocator<char> >", size);
            break;
        }
        if (name == "std::wstring") {
            m_sizeCache.insert("basic_string<unsigned short,char_traits<unsignedshort>,allocator<unsignedshort> >", size);
            m_sizeCache.insert("std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >", size);
            break;
        }
    } while (false);
    m_sizeCache.insert(name, size);
}

QtDumperHelper::Type QtDumperHelper::type(const QByteArray &typeName) const
{
    const QtDumperHelper::TypeData td = typeData(typeName);
    return td.type;
}

QtDumperHelper::TypeData QtDumperHelper::typeData(const QByteArray &typeName) const
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
QByteArray QtDumperHelper::evaluationSizeofTypeExpression(const QByteArray &typeName,
                                                       Debugger debugger) const
{
    // Look up special size types
    const SpecialSizeType st = specialSizeType(typeName);
    if (st != SpecialSizeCount) {
        if (const int size = m_specialSizes[st])
            return QByteArray::number(size);
    }
    // Look up size cache
    const SizeCache::const_iterator sit = m_sizeCache.constFind(typeName);
    if (sit != m_sizeCache.constEnd())
        return QByteArray::number(sit.value());
    // Finally have the debugger evaluate
    return sizeofTypeExpression(typeName, debugger);
}

QtDumperHelper::SpecialSizeType QtDumperHelper::specialSizeType(const QByteArray &typeName) const
{
    if (isPointerType(typeName))
        return PointerSize;
    if (typeName == "int")
        return IntSize;
    if (typeName.startsWith("std::allocator"))
        return StdAllocatorSize;
    if (typeName.startsWith(m_qPointerPrefix))
        return QPointerSize;
    if (typeName.startsWith(m_qSharedPointerPrefix))
        return QSharedPointerSize;
    if (typeName.startsWith(m_qSharedDataPointerPrefix))
        return QSharedDataPointerSize;
    if (typeName.startsWith(m_qWeakPointerPrefix))
        return QWeakPointerSize;
    if (typeName.startsWith(m_qListPrefix))
        return QListSize;
    if (typeName.startsWith(m_qLinkedListPrefix))
        return QLinkedListSize;
    if (typeName.startsWith(m_qVectorPrefix))
        return QVectorSize;
    if (typeName.startsWith(m_qQueuePrefix))
        return QQueueSize;
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
    const TypeData &td, Debugger debugger,
    QByteArray *inBuffer, QByteArrayList *extraArgsIn) const
{
    enum { maxExtraArgCount = 4 };

    QByteArrayList &extraArgs = *extraArgsIn;

    // See extractTemplate for parameters
    QByteArrayList inners = td.inner.split('@');
    if (inners.at(0).isEmpty())
        inners.clear();
    for (int i = 0; i != inners.size(); ++i)
        inners[i] = inners[i].simplified();

    QString outertype = td.isTemplate ? td.tmplate : data.type;
    // adjust the data extract
    if (outertype == m_qtNamespace + "QWidget")
        outertype = m_qtNamespace + "QObject";

    QString inner = td.inner;
    const QByteArray zero = "0";

    extraArgs.clear();

    if (!inners.empty()) {
        // "generic" template dumpers: passing sizeof(argument)
        // gives already most information the dumpers need
        const int count = qMin(int(maxExtraArgCount), inners.size());
        for (int i = 0; i < count; i++)
            extraArgs.push_back(evaluationSizeofTypeExpression(inners.at(i), debugger));
    }

    // Pad with zeros
    while (extraArgs.size() < maxExtraArgCount)
        extraArgs.push_back("0");

    // in rare cases we need more or less:
    switch (td.type) {
    case QAbstractItemType:
        if (data.dumperFlags.isEmpty()) {
            qWarning("Internal error: empty dumper state '%s'.", data.iname.constData());
        } else {
            inner = data.dumperFlags.mid(1);
        }
        break;
    case QObjectSlotType:
    case QObjectSignalType: {
            // we need the number out of something like
            // iname="local.ob.slots.2" // ".deleteLater()"?
            const int pos = data.iname.lastIndexOf('.');
            const QByteArray slotNumber = data.iname.mid(pos + 1);
            QTC_ASSERT(slotNumber.toInt() != -1, /**/);
            extraArgs[0] = slotNumber;
        }
        break;
    case QMapType:
    case QMultiMapType: {
            QByteArray nodetype;
            if (m_qtVersion >= 0x040500) {
                nodetype = m_qtNamespace + "QMapNode";
                nodetype += data.type.mid(outertype.size());
            } else {
                // FIXME: doesn't work for QMultiMap
                nodetype  = data.type + "::Node";
            }
            //qDebug() << "OUTERTYPE: " << outertype << " NODETYPE: " << nodetype
            //    << "QT VERSION" << m_qtVersion << ((4 << 16) + (5 << 8) + 0);
            extraArgs[2] = evaluationSizeofTypeExpression(nodetype, debugger);
            extraArgs[3] = qMapNodeValueOffsetExpression(nodetype, data.hexAddress(), debugger);
        }
        break;
    case QMapNodeType:
        extraArgs[2] = evaluationSizeofTypeExpression(data.type, debugger);
        extraArgs[3] = qMapNodeValueOffsetExpression(data.type, data.hexAddress(), debugger);
        break;
    case StdVectorType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == "bool")
            outertype = "std::vector::bool";
        break;
    case StdDequeType:
        extraArgs[1] = "0";
        break;
    case StdStackType:
        // remove 'std::allocator<...>':
        extraArgs[1] = "0";
        break;
    case StdSetType:
        // remove 'std::less<...>':
        extraArgs[1] = "0";
        // remove 'std::allocator<...>':
        extraArgs[2] = "0";
        break;
    case StdMapType: {
            // We need the offset of the second item in the value pair.
            // We read the type of the pair from the allocator argument because
            // that gets the constness "right" (in the sense that gdb/cdb can
            // read it back: "std::allocator<std::pair<Key,Value> >"
            // -> "std::pair<Key,Value>". Different debuggers have varying
            // amounts of terminating blanks...
            extraArgs[2].clear();
            extraArgs[3] = "0";
            QByteArray pairType = inners.at(3);
            int bracketPos = pairType.indexOf('<');
            if (bracketPos != -1)
                pairType.remove(0, bracketPos + 1);
            // We don't want the comparator and the allocator confuse gdb.
            const char closingBracket = '>';
            bracketPos = pairType.lastIndexOf(closingBracket);
            if (bracketPos != -1)
                bracketPos = pairType.lastIndexOf(closingBracket, bracketPos - pairType.size() - 1);
            if (bracketPos != -1)
                pairType.truncate(bracketPos + 1);
            if (debugger == GdbDebugger) {
                extraArgs[2] = "(size_t)&(('";
                extraArgs[2] += pairType;
                extraArgs[2] += "'*)0)->second";
            } else {
                // Cdb: The std::pair is usually in scope. Still, this expression
                // occasionally fails for complex types (std::string).
                // We need an address as CDB cannot do the 0-trick.
                // Use data address or try at least cache if missing.
                const QByteArray address = data.address ?
                                           data.hexAddress() :
                                           "DUMMY_ADDRESS";
                QByteArray offsetExpr = "(size_t)&(((" + pairType + " *)" + address
                        + ")->second)" + '-' + address;
                extraArgs[2] = lookupCdbDummyAddressExpression(offsetExpr, address);
            }
        }
        break;
    case StdStringType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == "char")
            outertype = "std::string";
        else if (inners.at(0) == "wchar_t")
            outertype = "std::wstring";
        qFill(extraArgs, zero);
        break;
    case UnknownType:
        qWarning("Unknown type encountered in %s.\n", Q_FUNC_INFO);
        break;
    case SupportedType:
    case QVectorType:
    case QStackType:
    case QObjectType:
    case QWidgetType:
        break;
    }

    // Look up expressions in the cache
    if (!m_expressionCache.empty()) {
        const ExpressionCache::const_iterator excCend = m_expressionCache.constEnd();
        const QByteArrayList::iterator eend = extraArgs.end();
        for (QByteArrayList::iterator it = extraArgs.begin(); it != eend; ++it) {
            QByteArray &e = *it;
            if (!e.isEmpty() && e != zero && !isInteger(e)) {
                const ExpressionCache::const_iterator eit = m_expressionCache.constFind(e);
                if (eit != excCend)
                    e = eit.value();
            }
        }
    }

    inBuffer->clear();
    inBuffer->append(outertype.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.iname);
    inBuffer->append('\0');
    inBuffer->append(data.exp);
    inBuffer->append('\0');
    inBuffer->append(inner.toUtf8());
    inBuffer->append('\0');
    inBuffer->append(data.iname);
    inBuffer->append('\0');

    if (debug)
        qDebug() << '\n' << Q_FUNC_INFO << '\n' << data.toString() << "\n-->" << outertype << td.type << extraArgs;
}

// Return debugger expression to get the offset of a map node.
QByteArray QtDumperHelper::qMapNodeValueOffsetExpression
    (const QByteArray &type, const QByteArray &addressIn, Debugger debugger) const
{
    switch (debugger) {
    case GdbDebugger:
        return "(size_t)&(('" + type + "'*)0)->value";
    case CdbDebugger: {
            // Cdb: This will only work if a QMapNode is in scope.
            // We need an address as CDB cannot do the 0-trick.
            // Use data address or try at least cache if missing.
            const QByteArray address = addressIn.isEmpty() ? "DUMMY_ADDRESS" : addressIn;
            QByteArray offsetExpression = "(size_t)&(((" + type
                    + " *)" + address + ")->value)-" + address;
            return lookupCdbDummyAddressExpression(offsetExpression, address);
        }
    }
    return QByteArray();
}

/* Cdb cannot do tricks like ( "&(std::pair<int,int>*)(0)->second)",
 * that is, use a null pointer to determine the offset of a member.
 * It tries to dereference the address at some point and fails with
 * "memory access error". As a trick, use the address of the watch item
 * to do this. However, in the expression cache, 0 is still used, so,
 * for cache lookups,  use '0' as address. */
QByteArray QtDumperHelper::lookupCdbDummyAddressExpression
    (const QByteArray &expr, const QByteArray &address) const
{
    QByteArray nullExpr = expr;
    nullExpr.replace(address, "0");
    const QByteArray rc = m_expressionCache.value(nullExpr, expr);
    if (debug)
        qDebug() << "lookupCdbDummyAddressExpression" << expr << rc;
    return rc;
}

// GdbMi parsing helpers for parsing dumper value results

static bool gdbMiGetIntValue(int *target, const GdbMi &node, const char *child)
{
    *target = -1;
    const GdbMi childNode = node.findChild(child);
    if (!childNode.isValid())
        return false;
    bool ok;
    *target = childNode.data().toInt(&ok);
    return ok;
}

// Find a string child node and assign value if it exists.
// Optionally decode.
static bool gdbMiGetStringValue(QString *target,
                             const GdbMi &node,
                             const char *child,
                             const char *encodingChild = 0)
{
    target->clear();
    const GdbMi childNode = node.findChild(child);
    if (!childNode.isValid())
        return false;
    // Encoded data
    if (encodingChild) {
        int encoding;
        if (!gdbMiGetIntValue(&encoding, node, encodingChild))
            encoding = 0;
        *target = decodeData(childNode.data(), encoding);
        return true;
    }
    // Plain data
    *target = QLatin1String(childNode.data());
    return true;
}

static bool gdbMiGetByteArrayValue(QByteArray *target,
                             const GdbMi &node,
                             const char *child,
                             const char *encodingChild = 0)
{
    QString str;
    const bool success = gdbMiGetStringValue(&str, node, child, encodingChild);
    *target = str.toLatin1();
    return success;
}

static bool gdbMiGetBoolValue(bool *target,
                             const GdbMi &node,
                             const char *child)
{
    *target = false;
    const GdbMi childNode = node.findChild(child);
    if (!childNode.isValid())
        return false;
    *target = childNode.data() == "true";
    return true;
}

/* Context to store parameters that influence the next level children.
 *  (next level only, it is not further inherited). For example, the root item
 * can provide a "childtype" node that specifies the type of the children. */

struct GdbMiRecursionContext
{
    enum Type
    {
        Debugger,    // Debugger symbol dump, recursive/symmetrical
        GdbMacrosCpp // old gdbmacros.cpp format, unsymmetrical
    };

    GdbMiRecursionContext(Type t, int recursionLevelIn = 0) :
            type(t), recursionLevel(recursionLevelIn), childNumChild(-1), childIndex(0) {}

    const Type type;
    int recursionLevel;
    int childNumChild;
    int childIndex;
    QString childType;
    QByteArray parentIName;
};

static void gbdMiToWatchData(const GdbMi &root,
                             const GdbMiRecursionContext &ctx,
                             QList<WatchData> *wl)
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << root.toString(false, 0);
    WatchData w;
    QString v;
    QByteArray b;
    // Check for name/iname and use as expression default
    w.sortId = ctx.childIndex;
    // Fully symmetrical
    if (ctx.type == GdbMiRecursionContext::Debugger) {
        gdbMiGetByteArrayValue(&w.iname, root, "iname");
        gdbMiGetStringValue(&w.name, root, "name");
        gdbMiGetByteArrayValue(&w.exp, root, "exp");
    } else {
        // gdbmacros.cpp: iname/name present according to recursion level
        // Check for name/iname and use as expression default
        if (ctx.recursionLevel == 0) {
            // parents have only iname, from which name is derived
            QString iname;
            if (!gdbMiGetStringValue(&iname, root, "iname"))
                qWarning("Internal error: iname missing");
            w.iname = iname.toLatin1();
            w.name = iname;
            const int lastDotPos = w.name.lastIndexOf(QLatin1Char('.'));
            if (lastDotPos != -1)
                w.name.remove(0, lastDotPos + 1);
            w.exp = w.name.toLatin1();
        } else {
            // Children can have a 'name' attribute. If missing, assume array index
            // For display purposes, it can be overridden by "key"
            if (!gdbMiGetStringValue(&w.name, root, "name")) {
                w.name = QString::number(ctx.childIndex);
            }
            // Set iname
            w.iname = ctx.parentIName;
            w.iname += '.';
            w.iname += w.name.toLatin1();
            // Key?
            QString key;
            if (gdbMiGetStringValue(&key, root, "key", "keyencoded")) {
                w.name = key.size() > 13 ? key.mid(0, 13) + QLatin1String("...") : key;
            }
        }
    }
    if (w.name.isEmpty()) {
        const QString msg = QString::fromLatin1(
            "Internal error: Unable to determine name at level %1/%2 for %3")
            .arg(ctx.recursionLevel).arg(w.iname, QLatin1String(root.toString(true, 2)));
        qWarning("%s\n", qPrintable(msg));
    }
    gdbMiGetStringValue(&w.displayedType, root, "displayedtype");
    if (gdbMiGetByteArrayValue(&b, root, "editvalue"))
        w.editvalue = b;
    if (gdbMiGetByteArrayValue(&b, root, "exp"))
        w.exp = b;
    QByteArray addressBA;
    gdbMiGetByteArrayValue(&addressBA, root, "addr");
    if (addressBA.startsWith("0x")) { // Item model dumper pulls tricks
        w.setHexAddress(addressBA);
    } else {
        w.dumperFlags = addressBA;
    }
    gdbMiGetBoolValue(&w.valueEnabled, root, "valueenabled");
    gdbMiGetBoolValue(&w.valueEditable, root, "valueeditable");
    if (gdbMiGetStringValue(&v, root, "valuetooltip", "valuetooltipencoded"))
        w.setValue(v);
    if (gdbMiGetStringValue(&v, root, "value", "valueencoded"))
        w.setValue(v);
    // Type from context or self
    if (ctx.childType.isEmpty()) {
        if (gdbMiGetStringValue(&v, root, "type"))
            w.setType(v.toUtf8());
    } else {
        w.setType(ctx.childType.toUtf8());
    }
    // child count?
    int numChild = -1;
    if (ctx.childNumChild >= 0) {
        numChild = ctx.childNumChild;
    } else {
        gdbMiGetIntValue(&numChild, root, "numchild");
    }
    if (numChild >= 0)
        w.setHasChildren(numChild > 0);
    wl->push_back(w);
    // Parse children with a new context
    if (numChild == 0)
        return;
    const GdbMi childrenNode = root.findChild("children");
    if (!childrenNode.isValid())
        return;
    const QList<GdbMi> children =childrenNode.children();
    if (children.empty())
        return;
    wl->back().setChildrenUnneeded();
    GdbMiRecursionContext nextLevelContext(ctx.type, ctx.recursionLevel + 1);
    nextLevelContext.parentIName = w.iname;
    gdbMiGetStringValue(&nextLevelContext.childType, root, "childtype");
    if (!gdbMiGetIntValue(&nextLevelContext.childNumChild, root, "childnumchild"))
        nextLevelContext.childNumChild = -1;
    foreach (const GdbMi &child, children) {
        gbdMiToWatchData(child, nextLevelContext, wl);
        nextLevelContext.childIndex++;
    }
}

bool QtDumperHelper::parseValue(const char *data, QList<WatchData> *l)
{
    l->clear();
    GdbMi root;
    // Array (CDB2)
    if (*data == '[') {
        root.fromString(data);
        if (!root.isValid())
            return false;
        foreach(const GdbMi &child, root.children())
            gbdMiToWatchData(child, GdbMiRecursionContext(GdbMiRecursionContext::Debugger), l);
    } else {
        root.fromStringMultiple(QByteArray(data));
        if (!root.isValid())
            return false;
        gbdMiToWatchData(root, GdbMiRecursionContext(GdbMiRecursionContext::GdbMacrosCpp), l);
    }
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

void setWatchDataValueEditable(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valueEditable = true;
    else if (mi.data() == "false")
        data.valueEditable = false;
}

void setWatchDataExpression(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.exp = mi.data();
}

void setWatchDataAddress(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        setWatchDataAddressHelper(data, mi.data());
}

void setWatchDataAddressHelper(WatchData &data, const QByteArray &addr)
{
    if (addr.startsWith("0x")) { // Item model dumpers pull tricks
       data.setHexAddress(addr);
    } else {
        data.dumperFlags = addr;
    }
    if (data.exp.isEmpty() && !data.dumperFlags.startsWith('$'))
        data.exp = "*(" + gdbQuoteTypes(data.type) + "*)" +data.hexAddress();
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

    setWatchDataValue(data, item);
    setWatchDataAddress(data, item.findChild("addr"));
    setWatchDataExpression(data, item.findChild("exp"));
    setWatchDataValueEnabled(data, item.findChild("valueenabled"));
    setWatchDataValueEditable(data, item.findChild("valueeditable"));
    setWatchDataChildCount(data, item.findChild("numchild"));
    //qDebug() << "\nAPPEND TO LIST: " << data.toString() << "\n";
    list->append(data);

    bool ok = false;
    qulonglong addressBase = item.findChild("addrbase").data().toULongLong(&ok, 0);
    qulonglong addressStep = item.findChild("addrstep").data().toULongLong();

    // Try not to repeat data too often.
    WatchData childtemplate;
    setWatchDataType(childtemplate, item.findChild("childtype"));
    setWatchDataChildCount(childtemplate, item.findChild("childnumchild"));
    //qDebug() << "CHILD TEMPLATE:" << childtemplate.toString();

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
            data1.name = _c('[') + data1.name + _c(']');
        if (addressStep) {
            const QByteArray addr = "0x" + QByteArray::number(addressBase, 16);
            setWatchDataAddressHelper(data1, addr);
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


} // namespace Internal
} // namespace Debugger
