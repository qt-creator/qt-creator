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
#include "gdb/gdbmi.h"
#include <utils/qtcassert.h>

#include <coreplugin/ifile.h>

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
        << QLatin1String("unsigned char") << QLatin1String("unsigned long")
        << QLatin1String("long long")  << QLatin1String("unsigned long long");
    return type.endsWith(QLatin1String(" int")) || types.contains(type);
}

bool isSymbianIntType(const QString &type)
{
    static const QStringList types = QStringList()
        << QLatin1String("TInt") << QLatin1String("TBool");
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

QString sizeofTypeExpression(const QString &type, QtDumperHelper::Debugger debugger)
{
    if (type.endsWith(QLatin1Char('*')))
        return QLatin1String("sizeof(void*)");
    if (debugger != QtDumperHelper::GdbDebugger || type.endsWith(QLatin1Char('>')))
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
            str << ' ' << it.key() << '=' << it.value() << '\n';
        }
        str << "\nExpression cache: (" << m_expressionCache.size() << ")\n";
        const QMap<QString, QString>::const_iterator excend = m_expressionCache.constEnd();
        for (QMap<QString, QString>::const_iterator it = m_expressionCache.constBegin(); it != excend; ++it)
            str << "    " << it.key() << ' ' << it.value() << '\n';
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
    if (s == QLatin1String("QAbstractItem"))
        return QAbstractItemType;
    if (s == QLatin1String("QMap"))
        return QMapType;
    if (s == QLatin1String("QMapNode"))
        return QMapNodeType;
    if (s == QLatin1String("QMultiMap"))
        return QMultiMapType;
    if (s == QLatin1String("QObject"))
        return QObjectType;
    if (s == QLatin1String("QObjectSignal"))
        return QObjectSignalType;
    if (s == QLatin1String("QObjectSlot"))
        return QObjectSlotType;
    if (s == QLatin1String("QStack"))
        return QStackType;
    if (s == QLatin1String("QVector"))
        return QVectorType;
    if (s == QLatin1String("QWidget"))
        return QWidgetType;
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

bool QtDumperHelper::parseQuery(const GdbMi &contents, Debugger debugger)
{
    clear();
    if (debug > 1)
        qDebug() << "parseQuery" << contents.toString(true, 2);

    // Common info, dumper version, etc
    m_qtNamespace = QLatin1String(contents.findChild("namespace").data());
    int qtv = 0;
    const GdbMi qtversion = contents.findChild("qtversion");
    if (qtversion.children().size() == 3) {
        qtv = (qtversion.childAt(0).data().toInt() << 16)
                    + (qtversion.childAt(1).data().toInt() << 8)
                    + qtversion.childAt(2).data().toInt();
    }
    m_qtVersion = qtv;
    // Get list of helpers
    QStringList availableSimpleDebuggingHelpers;
    foreach (const GdbMi &item, contents.findChild("dumpers").children())
        availableSimpleDebuggingHelpers.append(QLatin1String(item.data()));
    parseQueryTypes(availableSimpleDebuggingHelpers, debugger);
    m_dumperVersion = getDumperVersion(contents);
    // Parse sizes
    foreach (const GdbMi &sizesList, contents.findChild("sizes").children()) {
        const int childCount = sizesList.childCount();
        if (childCount > 1) {
            const int size = sizesList.childAt(0).data().toInt();
            for (int c = 1; c < childCount; c++)
                addSize(QLatin1String(sizesList.childAt(c).data()), size);
        }
    }
    // Parse expressions
    foreach (const GdbMi &exprList, contents.findChild("expressions").children())
        if (exprList.childCount() == 2)
            m_expressionCache.insert(QLatin1String(exprList.childAt(0).data()),
                                     QLatin1String(exprList.childAt(1).data()));
    return true;
}

// parse a query
bool QtDumperHelper::parseQuery(const char *data, Debugger debugger)
{
    QByteArray fullData = data;
    fullData.insert(0, '{');
    fullData.append(data);
    fullData.append('}');
    GdbMi root(fullData);
    if (!root.isValid())
        return false;
    return parseQuery(root, debugger);
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
                                                       Debugger debugger) const
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
    return sizeofTypeExpression(typeName, debugger);
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
            extraArgs[3] = qMapNodeValueOffsetExpression(nodetype, data.addr, debugger);
        }
        break;
    case QMapNodeType:
        extraArgs[2] = evaluationSizeofTypeExpression(data.type, debugger);
        extraArgs[3] = qMapNodeValueOffsetExpression(data.type, data.addr, debugger);
        break;
    case StdVectorType:
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == QLatin1String("bool")) {
            outertype = QLatin1String("std::vector::bool");
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
            // We need the offset of the second item in the value pair.
            // We read the type of the pair from the allocator argument because
            // that gets the constness "right" (in the sense that gdb/cdb can
            // read it back: "std::allocator<std::pair<Key,Value> >"
            // -> "std::pair<Key,Value>". Different debuggers have varying
            // amounts of terminating blanks...
            extraArgs[2].clear();
            extraArgs[3] = zero;
            QString pairType = inners.at(3);
            int bracketPos = pairType.indexOf(QLatin1Char('<'));
            if (bracketPos != -1)
                pairType.remove(0, bracketPos + 1);
            // We don't want the comparator and the allocator confuse gdb.
            const QChar closingBracket = QLatin1Char('>');
            bracketPos = pairType.lastIndexOf(closingBracket);
            if (bracketPos != -1)
                bracketPos = pairType.lastIndexOf(closingBracket, bracketPos - pairType.size() - 1);
            if (bracketPos != -1)
                pairType.truncate(bracketPos + 1);
            if (debugger == GdbDebugger) {
                extraArgs[2] = QLatin1String("(size_t)&(('");
                extraArgs[2] += pairType;
                extraArgs[2] += QLatin1String("'*)0)->second");
            } else {
                // Cdb: The std::pair is usually in scope. Still, this expression
                // occasionally fails for complex types (std::string).
                // We need an address as CDB cannot do the 0-trick.
                // Use data address or try at least cache if missing.
                const QString address = data.addr.isEmpty() ? QString::fromLatin1("DUMMY_ADDRESS") : data.addr;
                QString offsetExpr;
                QTextStream str(&offsetExpr);
                str << "(size_t)&(((" << pairType << " *)" << address << ")->second)" << '-' << address;
                extraArgs[2] = lookupCdbDummyAddressExpression(offsetExpr, address);
            }
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
    case QStackType:
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

// Return debugger expression to get the offset of a map node.
QString QtDumperHelper::qMapNodeValueOffsetExpression(const QString &type,
                                                      const QString &addressIn,
                                                      Debugger debugger) const
{
    switch (debugger) {
    case GdbDebugger:
        return QLatin1String("(size_t)&(('") + type + QLatin1String("'*)0)->value");
    case CdbDebugger: {
            // Cdb: This will only work if a QMapNode is in scope.
            // We need an address as CDB cannot do the 0-trick.
            // Use data address or try at least cache if missing.
            const QString address = addressIn.isEmpty() ? QString::fromLatin1("DUMMY_ADDRESS") : addressIn;
            QString offsetExpression;
            QTextStream(&offsetExpression) << "(size_t)&(((" << type
                    << " *)" << address << ")->value)-" << address;
            return lookupCdbDummyAddressExpression(offsetExpression, address);
        }
    }
    return QString();
}

/* Cdb cannot do tricks like ( "&(std::pair<int,int>*)(0)->second)",
 * that is, use a null pointer to determine the offset of a member.
 * It tries to dereference the address at some point and fails with
 * "memory access error". As a trick, use the address of the watch item
 * to do this. However, in the expression cache, 0 is still used, so,
 * for cache lookups,  use '0' as address. */
QString QtDumperHelper::lookupCdbDummyAddressExpression(const QString &expr,
                                                        const QString &address) const
{
    QString nullExpr = expr;
    nullExpr.replace(address, QString(QLatin1Char('0')));
    const QString rc = m_expressionCache.value(nullExpr, expr);
    if (debug)
        qDebug() << "lookupCdbDummyAddressExpression" << expr << rc;
    return rc;
}

// GdbMi parsing helpers for parsing dumper value results

static bool gdbMiGetIntValue(int *target,
                             const GdbMi &node,
                             const char *child)
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

struct GdbMiRecursionContext {
    GdbMiRecursionContext(int recursionLevelIn = 0) :
            recursionLevel(recursionLevelIn), childNumChild(-1), childIndex(0) {}

    int recursionLevel;
    int childNumChild;
    int childIndex;
    QString childType;
    QString parentIName;
};

static void gbdMiToWatchData(const GdbMi &root,
                             const GdbMiRecursionContext &ctx,
                             QList<WatchData> *wl)
{
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << '\n' << root.toString(false, 0);
    WatchData w;    
    QString v;
    // Check for name/iname and use as expression default
    if (ctx.recursionLevel == 0) {
        // parents have only iname, from which name is derived
        if (!gdbMiGetStringValue(&w.iname, root, "iname"))
            qWarning("Internal error: iname missing");
        w.name = w.iname;
        const int lastDotPos = w.name.lastIndexOf(QLatin1Char('.'));
        if (lastDotPos != -1)
            w.name.remove(0, lastDotPos + 1);
        w.exp = w.name;
    } else {
        // Children can have a 'name' attribute. If missing, assume array index
        // For display purposes, it can be overridden by "key"
        if (!gdbMiGetStringValue(&w.name, root, "name")) {
            w.name = QString::number(ctx.childIndex);
        }
        // Set iname
        w.iname = ctx.parentIName;
        w.iname += QLatin1Char('.');
        w.iname += w.name;
        // Key?
        QString key;
        if (gdbMiGetStringValue(&key, root, "key", "keyencoded")) {
            w.name = key.size() > 13 ? key.mid(0, 13) + QLatin1String("...") : key;
        }
    }
    if (w.name.isEmpty()) {
        const QString msg = QString::fromLatin1("Internal error: Unable to determine name at level %1/%2 for %3").arg(ctx.recursionLevel).arg(w.iname, QLatin1String(root.toString(true, 2)));
        qWarning("%s\n", qPrintable(msg));
    }
    gdbMiGetStringValue(&w.displayedType, root, "displayedtype");
    if (gdbMiGetStringValue(&v, root, "editvalue"))
        w.editvalue = v.toLatin1();
    if (gdbMiGetStringValue(&v, root, "exp"))
        w.exp = v;
    gdbMiGetStringValue(&w.addr, root, "addr");
    gdbMiGetStringValue(&w.saddr, root, "saddr");
    gdbMiGetBoolValue(&w.valueEnabled, root, "valueenabled");    
    gdbMiGetBoolValue(&w.valueEditable, root, "valueeditable");
    if (gdbMiGetStringValue(&v, root, "valuetooltip", "valuetooltipencoded"))
        w.setValue(v);
    if (gdbMiGetStringValue(&v, root, "value", "valueencoded"))
        w.setValue(v);
    // Type from context or self
    if (ctx.childType.isEmpty()) {
        if (gdbMiGetStringValue(&v, root, "type"))
            w.setType(v);
    } else {
        w.setType(ctx.childType);
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
    GdbMiRecursionContext nextLevelContext(ctx.recursionLevel + 1);
    nextLevelContext.parentIName = w.iname;
    gdbMiGetStringValue(&nextLevelContext.childType, root, "childtype");
    if (!gdbMiGetIntValue(&nextLevelContext.childNumChild, root, "childnumchild"))
        nextLevelContext.childNumChild = -1;
    foreach(const GdbMi &child, children) {
        gbdMiToWatchData(child, nextLevelContext, wl);
        nextLevelContext.childIndex++;
    }
}

bool QtDumperHelper::parseValue(const char *data,
                                QList<WatchData> *l)
{
    l->clear();
    QByteArray fullData = data;
    fullData.insert(0, '{');
    fullData.append(data);
    fullData.append('}');
    GdbMi root(fullData);
    if (!root.isValid())
        return false;
    gbdMiToWatchData(root, GdbMiRecursionContext(), l);
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
