/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <qglobal.h>

#if USE_QT_CORE
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QLinkedList>
#include <QList>
#include <QQueue>
#include <QLocale>
#include <QMap>
#include <QMetaEnum>
#include <QMetaObject>
#include <QMetaProperty>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QStack>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTextCodec>
#include <QTextStream>
#include <QVector>

#ifndef QT_BOOTSTRAPPED

#include <QModelIndex>

#if QT_VERSION >= 0x040500
#include <QSharedPointer>
#include <QSharedDataPointer>
#include <QSharedData>
#include <QWeakPointer>
#endif

#ifndef USE_QT_GUI
#   ifdef QT_GUI_LIB
#        define USE_QT_GUI 1
#   endif
#endif

#ifndef USE_QT_WIDGETS
#   if defined(QT_WIDGETS_LIB) || ((QT_VERSION < 0x050000) && defined(USE_QT_GUI))
#       define USE_QT_WIDGETS 1
#   endif
#endif

#ifdef USE_QT_GUI
#   include <QImage>
#   include <QRegion>
#   include <QPixmap>
#   include <QFont>
#   include <QColor>
#   include <QKeySequence>
#endif

#ifdef USE_QT_WIDGETS
#   include <QSizePolicy>
#   include <QWidget>
#   include <QApplication>
#endif

#endif // QT_BOOTSTRAPPED

#endif // USE_QT_CORE


#ifdef Q_OS_WIN
#    include <windows.h>
#endif

#include <list>
#include <map>
#include <string>
#include <set>
#include <vector>

#include <stdio.h>

#ifdef QT_BOOTSTRAPPED

#   define NS ""
#   define NSX "'"
#   define NSY "'"

#else

#   include "dumper_p.h"

#endif // QT_BOOTSTRAPPED


#if QT_VERSION >= 0x050000
#       define MAP_WORKS 0
#else
#       define MAP_WORKS 1
#endif

int qtGhVersion = QT_VERSION;

/*!
  \class QDumper
  \brief Helper class for producing "nice" output in Qt Creator's debugger.

  \internal

  The whole "custom dumper" implementation is currently far less modular
  than it could be. But as the code is still in a flux, making it nicer
  from a pure archtectural point of view seems still be a waste of resources.

  Some hints:

  New dumpers for non-templated classes should be mentioned in
  \c{qDumpObjectData440()} in the  \c{protocolVersion == 1} branch.

  Templated classes need extra support on the IDE level
  (see plugins/debugger/gdbengine.cpp) and should not be mentiond in
  \c{qDumpObjectData440()}.

  In any case, dumper processesing should end up in
  \c{handleProtocolVersion2and3()} and needs an entry in the big switch there.

  Next step is to create a suitable \c{static void qDumpFoo(QDumper &d)}
  function. At the bare minimum it should contain something like this:


  \c{
    const Foo &foo = *reinterpret_cast<const Foo *>(d.data);

    d.putItem("value", ...);
    d.putItem("type", "Foo");
    d.putItem("numchild", "0");
  }


  'd.putItem(name, value)' roughly expands to:
        d.put((name)).put("=\"").put(value).put("\"";

  Useful (i.e. understood by the IDE) names include:

  \list
    \o "name" shows up in the first column in the Locals&Watchers view.
    \o "value" shows up in the second column.
    \o "valueencoded" should be set to "1" if the value is base64 encoded.
        Always base64-encode values that might use unprintable or otherwise
        "confuse" the protocol (like spaces and quotes). [A-Za-z0-9] is "safe".
        A value of "3" is used for base64-encoded UCS4, "2" denotes
        base64-encoded UTF16.
    \o "numchild" return the number of children in the view. Effectively, only
        0 and != 0 will be used, so don't try too hard to get the number right.
  \endlist

  If the current item has children, it might be queried to produce information
  about these children. In this case the dumper should use something like this:

  \c{
    if (d.dumpChildren) {
        d.beginChildren();
            [...]
        d.endChildren();
   }

  */

#if defined(QT_BEGIN_NAMESPACE)
QT_BEGIN_NAMESPACE
#endif

const char *stdStringTypeC = "std::basic_string<char,std::char_traits<char>,std::allocator<char> >";
const char *stdWideStringTypeUShortC = "std::basic_string<unsigned short,std::char_traits<unsigned short>,std::allocator<unsigned short> >";

#if defined(QT_BEGIN_NAMESPACE)
QT_END_NAMESPACE
#endif


// This can be mangled typenames of nested templates, each char-by-char
// comma-separated integer list...
// The output buffer.
#ifdef MACROSDEBUG
    Q_DECL_EXPORT char xDumpInBuffer[10000];
    Q_DECL_EXPORT char xDumpOutBuffer[1000000];
#    define inBuffer xDumpInBuffer
#    define outBuffer xDumpOutBuffer
#else
    Q_DECL_EXPORT char qDumpInBuffer[10000];
    Q_DECL_EXPORT char qDumpOutBuffer[1000000];
#    define inBuffer qDumpInBuffer
#    define outBuffer qDumpOutBuffer
#endif

namespace {

static QByteArray strPtrConst = "* const";

static bool isPointerType(const QByteArray &type)
{
    return type.endsWith('*') || type.endsWith(strPtrConst);
}

static QByteArray stripPointerType(const QByteArray &_type)
{
    QByteArray type = _type;
    if (type.endsWith('*'))
        type.chop(1);
    if (type.endsWith(strPtrConst))
        type.chop(7);
    if (type.endsWith(' '))
        type.chop(1);
    return type;
}

// This is used to abort evaluation of custom data dumpers in a "coordinated"
// way. Abortion will happen at the latest when we try to access a non-initialized
// non-trivial object, so there is no way to prevent this from occurring at all
// conceptually. Gdb will catch SIGSEGV and return to the calling frame.
// This is just fine provided we only _read_ memory in the custom handlers below.
// We don't use this code for MSVC/CDB anymore.

volatile int qProvokeSegFaultHelper;

static const void *addOffset(const void *p, int offset)
{
    return offset + reinterpret_cast<const char *>(p);
}

static const void *deref(const void *p)
{
    return *reinterpret_cast<const char* const*>(p);
}

#if USE_QT_CORE
static const void *skipvtable(const void *p)
{
    return sizeof(void *) + reinterpret_cast<const char *>(p);
}

static const void *dfunc(const void *p)
{
    return deref(skipvtable(p));
}
#endif

static bool isEqual(const char *s, const char *t)
{
    return qstrcmp(s, t) == 0;
}

static bool startsWith(const char *s, const char *t)
{
    while (char c = *t++)
        if (c != *s++)
            return false;
    return true;
}

// Check memory for read access and provoke segfault if nothing else helps.
// On Windows, try to be less crash-prone by checking memory using WinAPI

#ifdef Q_OS_WIN

#    define qCheckAccess(d) do { \
        if (IsBadReadPtr(d, 1)) \
            return; \
         qProvokeSegFaultHelper = *(char*)d; \
    } while (0)
#    define qCheckPointer(d) do { \
        if (d && IsBadReadPtr(d, 1)) \
            return; \
        if (d) qProvokeSegFaultHelper = *(char*)d; \
    } while (0)

#else

#    define qCheckAccess(d) do { \
        if (!couldBePointer(d) && d != 0) \
            return; \
        qProvokeSegFaultHelper = *(char*)d; \
     } while (0)
#    define qCheckPointer(d) do { \
        if (!couldBePointer(d)) \
            return; \
        if (d) \
            qProvokeSegFaultHelper = *(char*)d; \
     } while (0)

static bool couldBePointer(const void *p)
{
    // we assume valid pointer to be 4-aligned at least.
    // So use this check only when this is guaranteed.
    // FIXME: this breaks e.g. in the QString dumper...
    const quintptr d = quintptr(p);
    //qDebug() << "CHECKING : " << p << ((d & 3) == 0 && (d > 1000 || d == 0));
    //return (d & 3) == 0 && (d > 1000 || d == 0);
    return d > 1000 || d == 0;
}

#endif

#ifdef QT_NAMESPACE
const char *stripNamespace(const char *type)
{
    static const size_t nslen = strlen(NS);
    return startsWith(type, NS) ? type + nslen : type;
}
#else
inline const char *stripNamespace(const char *type)
{
    return type;
}
#endif

static bool isSimpleType(const char *type)
{
    switch (type[0]) {
        case 'c':
            return isEqual(type, "char");
        case 'd':
            return isEqual(type, "double");
        case 'f':
            return isEqual(type, "float");
        case 'i':
            return isEqual(type, "int");
        case 'l':
            return isEqual(type, "long") || startsWith(type, "long ");
        case 's':
            return isEqual(type, "short") || startsWith(type, "short ")
                || isEqual(type, "signed") || startsWith(type, "signed ");
        case 'u':
            return isEqual(type, "unsigned") || startsWith(type, "unsigned ");
    }
    return false;
}

static bool isStringType(const char *type)
{
    return isEqual(type, NS "QString")
        || isEqual(type, NS "QByteArray")
        || isEqual(type, "std::string")
        || isEqual(type, "std::wstring")
        || isEqual(type, "wstring");
}

#if USE_QT_CORE
static bool isMovableType(const char *type)
{
    if (isPointerType(type))
        return true;

    if (isSimpleType(type))
        return true;

    type = stripNamespace(type);

    switch (type[1]) {
        case 'B':
            return isEqual(type, "QBrush")
                || isEqual(type, "QBitArray")
                || isEqual(type, "QByteArray") ;
        case 'C':
            return isEqual(type, "QCustomTypeInfo")
                || isEqual(type, "QChar");
        case 'D':
            return isEqual(type, "QDate")
                || isEqual(type, "QDateTime");
        case 'F':
            return isEqual(type, "QFileInfo")
                || isEqual(type, "QFixed")
                || isEqual(type, "QFixedPoint")
                || isEqual(type, "QFixedSize");
        case 'H':
            return isEqual(type, "QHashDummyValue");
        case 'I':
            return isEqual(type, "QIcon")
                || isEqual(type, "QImage");
        case 'L':
            return isEqual(type, "QLine")
                || isEqual(type, "QLineF")
                || isEqual(type, "QLatin1Char")
                || isEqual(type, "QLocal");
        case 'M':
            return isEqual(type, "QMatrix")
                || isEqual(type, "QModelIndex");
        case 'P':
            return isEqual(type, "QPoint")
                || isEqual(type, "QPointF")
                || isEqual(type, "QPen")
                || isEqual(type, "QPersistentModelIndex");
        case 'R':
            return isEqual(type, "QResourceRoot")
                || isEqual(type, "QRect")
                || isEqual(type, "QRectF")
                || isEqual(type, "QRegExp");
        case 'S':
            return isEqual(type, "QSize")
                || isEqual(type, "QSizeF")
                || isEqual(type, "QString");
        case 'T':
            return isEqual(type, "QTime")
                || isEqual(type, "QTextBlock");
        case 'U':
            return isEqual(type, "QUrl");
        case 'V':
            return isEqual(type, "QVariant");
        case 'X':
            return isEqual(type, "QXmlStreamAttribute")
                || isEqual(type, "QXmlStreamNamespaceDeclaration")
                || isEqual(type, "QXmlStreamNotationDeclaration")
                || isEqual(type, "QXmlStreamEntityDeclaration");
    }
    return false;
}
#endif

struct QDumper
{
    explicit QDumper();
    ~QDumper();

    // direct write to the output
    QDumper &put(long c);
    QDumper &put(int i);
    QDumper &put(double d);
    QDumper &put(float d);
    QDumper &put(unsigned long c);
    QDumper &put(unsigned int i);
    QDumper &put(const void *p);
    QDumper &put(qulonglong c);
    QDumper &put(long long c);
    QDumper &put(const char *str);
    QDumper &put(const QByteArray &ba);
    QDumper &put(const QString &str);
    QDumper &put(char c);

    // convienience functions for writing key="value" pairs:
    template <class Value>
    void putItem(const char *name, const Value &value)
    {
        putCommaIfNeeded();
        put(name).put('=').put('"').put(value).put('"');
    }

    void putItem(const char *name, const char *value, const char *setvalue)
    {
        if (!isEqual(value, setvalue))
            putItem(name, value);
    }
    // convienience functions for writing typical properties.
    // roughly equivalent to
    //   beginHash();
    //      putItem("name", name);
    //      putItem("value", value);
    //      putItem("type", NS "QString");
    //      putItem("numchild", "0");
    //      putItem("valueencoded", "2");
    //   endHash();
    void putHash(const char *name, const QString &value);
    void putHash(const char *name, const QByteArray &value);
    void putHash(const char *name, int value);
    void putHash(const char *name, long value);
    void putHash(const char *name, bool value);
    void putHash(const char *name, QChar value);
    void putHash(const char *name, float value);
    void putHash(const char *name, double value);
    void putStringValue(const QString &value);

    void beginHash(); // start of data hash output
    void endHash(); // start of data hash output

    void beginChildren(const char *mainInnerType = 0); // start of children list
    void endChildren(); // end of children list

    void beginItem(const char *name); // start of named item, ready to accept value
    void endItem(); // end of named item, used after value output is complete

    // convenience for putting "<n items>"
    void putItemCount(const char *name, int count);
    // convenience for putting "<>n items>" (more than X items)
    void putTruncatedItemCount(const char *name, int count);
    void putCommaIfNeeded();
    // convienience function for writing the last item of an abbreviated list
    void putEllipsis();
    void disarm();

    void putBase64Encoded(const char *buf, int n);
    void checkFill();

    // the dumper arguments
    int protocolVersion;   // dumper protocol version
    int token;             // some token to show on success
    const char *outerType; // object type
    const char *iname;     // object name used for display
    const char *exp;       // object expression
    const char *innerType; // 'inner type' for class templates
    const void *data;      // pointer to raw data
    bool dumpChildren;     // do we want to see children?

    // handling of nested templates
    void setupTemplateParameters();
    enum { maxTemplateParameters = 10 };
    const char *templateParameters[maxTemplateParameters + 1];

    // internal state
    int extraInt[4];

    bool success;          // are we finished?
    bool full;
    int pos;

    const char *currentChildType;
    const char *currentChildNumChild;
};


QDumper::QDumper()
{
    success = false;
    full = false;
    outBuffer[0] = 'f'; // marks output as 'wrong'
    pos = 1;
    currentChildType = 0;
    currentChildNumChild = 0;
}

QDumper::~QDumper()
{
    outBuffer[pos++] = '\0';
    if (success)
        outBuffer[0] = (full ? '+' : 't');
}

void QDumper::setupTemplateParameters()
{
    char *s = const_cast<char *>(innerType);

    int templateParametersCount = 1;
    templateParameters[0] = s;
    for (int i = 1; i != maxTemplateParameters + 1; ++i)
        templateParameters[i] = 0;

    while (*s) {
        while (*s && *s != '@')
            ++s;
        if (*s) {
            *s = '\0';
            ++s;
            templateParameters[templateParametersCount++] = s;
        }
    }
    while (templateParametersCount < maxTemplateParameters)
        templateParameters[templateParametersCount++] = 0;
}

QDumper &QDumper::put(char c)
{
    checkFill();
    if (!full)
        outBuffer[pos++] = c;
    return *this;
}

QDumper &QDumper::put(unsigned long long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%llu", c);
    return *this;
}

QDumper &QDumper::put(long long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%lld", c);
    return *this;
}

QDumper &QDumper::put(unsigned long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%lu", c);
    return *this;
}

QDumper &QDumper::put(float d)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%f", d);
    return *this;
}

QDumper &QDumper::put(double d)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%f", d);
    return *this;
}

QDumper &QDumper::put(unsigned int i)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%u", i);
    return *this;
}

QDumper &QDumper::put(long c)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%ld", c);
    return *this;
}

QDumper &QDumper::put(int i)
{
    checkFill();
    pos += sprintf(outBuffer + pos, "%d", i);
    return *this;
}

QDumper &QDumper::put(const void *p)
{
    if (p) {
        // Pointer is 'long long' on WIN_64, only
        static const char *printFormat = sizeof(void *) == sizeof(long) ? "0x%lx" : "0x%llx";
        pos += sprintf(outBuffer + pos, printFormat, p);
    } else {
        pos += sprintf(outBuffer + pos, "<null>");
    }
    return *this;
}

QDumper &QDumper::put(const char *str)
{
    if (!str)
        return put("<null>");
    while (*str)
        put(*(str++));
    return *this;
}

QDumper &QDumper::put(const QByteArray &ba)
{
    putBase64Encoded(ba.constData(), ba.size());
    return *this;
}

QDumper &QDumper::put(const QString &str)
{
    putBase64Encoded((const char *)str.constData(), 2 * str.size());
    return *this;
}

void QDumper::checkFill()
{
    if (pos >= int(sizeof(outBuffer)) - 100)
        full = true;
}

void QDumper::putCommaIfNeeded()
{
    if (pos == 0)
        return;
    char c = outBuffer[pos - 1];
    if (c == '}' || c == '"' || c == ']')
        put(',');
}

void QDumper::putBase64Encoded(const char *buf, int n)
{
    const char alphabet[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                            "ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
    const char padchar = '=';
    int padlen = 0;

    //int tmpsize = ((n * 4) / 3) + 3;

    int i = 0;
    while (i < n) {
        int chunk = 0;
        chunk |= int(uchar(buf[i++])) << 16;
        if (i == n) {
            padlen = 2;
        } else {
            chunk |= int(uchar(buf[i++])) << 8;
            if (i == n)
                padlen = 1;
            else
                chunk |= int(uchar(buf[i++]));
        }

        int j = (chunk & 0x00fc0000) >> 18;
        int k = (chunk & 0x0003f000) >> 12;
        int l = (chunk & 0x00000fc0) >> 6;
        int m = (chunk & 0x0000003f);
        put(alphabet[j]);
        put(alphabet[k]);
        put(padlen > 1 ? padchar : alphabet[l]);
        put(padlen > 0 ? padchar : alphabet[m]);
    }
}

void QDumper::putStringValue(const QString &str)
{
    if (str.isNull()) {
        beginItem("value");
        putBase64Encoded("\"\" (null)", 9);
        endItem();
        putItem("valueencoded", "5");
    } else {
        putItem("value", str);
        putItem("valueencoded", "2");
    }
}

void QDumper::disarm()
{
    success = true;
}

void QDumper::beginHash()
{
    putCommaIfNeeded();
    put('{');
}

void QDumper::endHash()
{
    put('}');
}

void QDumper::putEllipsis()
{
    putCommaIfNeeded();
    put("{name=\"<incomplete>\",value=\"\",type=\"").put(innerType).put("\"}");
}

void QDumper::putItemCount(const char *name, int count)
{
    putCommaIfNeeded();
    put(name).put("=\"<").put(count).put(" items>\"");
}

void QDumper::putTruncatedItemCount(const char *name, int count)
{
    putCommaIfNeeded();
    put(name).put("=\"<>").put(count).put(" items>\"");
}

//
// Some helpers to keep the dumper code short
//

void QDumper::beginItem(const char *name)
{
    putCommaIfNeeded();
    put(name).put('=').put('"');
}

void QDumper::endItem()
{
    put('"');
}

void QDumper::beginChildren(const char *mainInnerType)
{
    if (mainInnerType) {
        putItem("childtype", mainInnerType);
        currentChildType = mainInnerType;
        if (isSimpleType(mainInnerType) || isStringType(mainInnerType)) {
            putItem("childnumchild", "0");
            currentChildNumChild = "0";
        } else if (isPointerType(mainInnerType)) {
            putItem("childnumchild", "1");
            currentChildNumChild = "1";
        }
    }

    putCommaIfNeeded();
    put("children=[");
}

void QDumper::endChildren()
{
    put(']');
    currentChildType = 0;
    currentChildNumChild = 0;
}

// simple string property
void QDumper::putHash(const char *name, const QString &value)
{
    beginHash();
    putItem("name", name);
    putStringValue(value);
    putItem("type", NS "QString");
    putItem("numchild", "0");
    endHash();
}

void QDumper::putHash(const char *name, const QByteArray &value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", NS "QByteArray");
    putItem("numchild", "0");
    putItem("valueencoded", "1");
    endHash();
}

// simple integer property
void QDumper::putHash(const char *name, int value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", "int");
    putItem("numchild", "0");
    endHash();
}

void QDumper::putHash(const char *name, long value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", "long");
    putItem("numchild", "0");
    endHash();
}

void QDumper::putHash(const char *name, float value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", "float");
    putItem("numchild", "0");
    endHash();
}

void QDumper::putHash(const char *name, double value)
{
    beginHash();
    putItem("name", name);
    putItem("value", value);
    putItem("type", "double");
    putItem("numchild", "0");
    endHash();
}

// simple boolean property
void QDumper::putHash(const char *name, bool value)
{
    beginHash();
    putItem("name", name);
    putItem("value", (value ? "true" : "false"));
    putItem("type", "bool");
    putItem("numchild", "0");
    endHash();
}

// a single QChar
void QDumper::putHash(const char *name, QChar value)
{
    beginHash();
    putItem("name", name);
    putStringValue(QString(QLatin1String("'%1' (%2, 0x%3)"))
        .arg(value).arg(value.unicode()).arg(value.unicode(), 0, 16));
    putItem("type", NS "QChar");
    putItem("numchild", "0");
    endHash();
}

#define DUMPUNKNOWN_MESSAGE "<not in scope>"
static void qDumpUnknown(QDumper &d, const char *why = 0)
{
    if (!why)
        why = DUMPUNKNOWN_MESSAGE;
    d.putItem("value", why);
    d.putItem("valueeditable", "false");
    d.putItem("valueenabled", "false");
    d.putItem("numchild", "0", d.currentChildNumChild);
    d.disarm();
}

static void qDumpStdStringValue(QDumper &d, const std::string &str)
{
    d.beginItem("value");
    d.putBase64Encoded(str.c_str(), str.size());
    d.endItem();
    d.putItem("valueencoded", "1");
    d.putItem("type", "std::string");
    d.putItem("numchild", "0", d.currentChildNumChild);
}

static void qDumpStdWStringValue(QDumper &d, const std::wstring &str)
{
    d.beginItem("value");
    d.putBase64Encoded((const char *)str.c_str(), str.size() * sizeof(wchar_t));
    d.endItem();
    d.putItem("valueencoded", (sizeof(wchar_t) == 2 ? "2" : "3"));
    d.putItem("type", "std::wstring", d.currentChildType);
    d.putItem("numchild", "0", d.currentChildNumChild);
}

// Called by templates, so, not static.
static void qDumpInnerQCharValue(QDumper &d, QChar c, const char *field)
{
    char buf[30];
    sprintf(buf, "'?', ucs=%d", c.unicode());
    if (c.isPrint() && c.unicode() < 127)
        buf[1] = char(c.unicode());
    d.putCommaIfNeeded();
    d.putItem(field, buf);
    d.putItem("numchild", "0", d.currentChildNumChild);
}

static void qDumpInnerCharValue(QDumper &d, char c, const char *field)
{
    char buf[30];
    sprintf(buf, "'?', ascii=%d", c);
    if (QChar(QLatin1Char(c)).isPrint() && c < 127)
        buf[1] = c;
    d.putCommaIfNeeded();
    d.putItem(field, buf);
    d.putItem("numchild", "0", d.currentChildNumChild);
}

void qDumpInnerValueHelper(QDumper &d, const char *type, const void *addr,
    const char *field = "value")
{
    type = stripNamespace(type);
    switch (type[1]) {
        case 'h':
            if (isEqual(type, "char"))
                qDumpInnerCharValue(d, *(char *)addr, field);
            break;
        case 'l':
            if (isEqual(type, "float"))
                d.putItem(field, *(float*)addr);
            break;
        case 'n':
            if (isEqual(type, "int"))
                d.putItem(field, *(int*)addr);
            else if (isEqual(type, "unsigned") || isEqual(type, "unsigned int"))
                d.putItem(field, *(unsigned int*)addr);
            else if (isEqual(type, "unsigned char"))
                qDumpInnerCharValue(d, *(char *)addr, field);
            else if (isEqual(type, "unsigned long"))
                d.putItem(field, *(unsigned long*)addr);
            else if (isEqual(type, "unsigned long long"))
                d.putItem(field, *(qulonglong*)addr);
            break;
        case 'o':
            if (isEqual(type, "bool")) {
                switch (*(unsigned char*)addr) {
                case 0: d.putItem(field, "false"); break;
                case 1: d.putItem(field, "true"); break;
                default: d.putItem(field, *(unsigned char*)addr); break;
                }
            } else if (isEqual(type, "double"))
                d.putItem(field, *(double*)addr);
            else if (isEqual(type, "long"))
                d.putItem(field, *(long*)addr);
            else if (isEqual(type, "long long"))
                d.putItem(field, *(qulonglong*)addr);
            break;
        case 'B':
            if (isEqual(type, "QByteArray")) {
                d.putCommaIfNeeded();
                d.put(field).put("encoded=\"1\",");
                d.putItem(field, *(QByteArray*)addr);
            }
            break;
        case 'C':
            if (isEqual(type, "QChar"))
                qDumpInnerQCharValue(d, *(QChar*)addr, field);
            break;
        case 'L':
            if (startsWith(type, "QList<")) {
                const QListData *ldata = reinterpret_cast<const QListData*>(addr);
                d.putItemCount("value", ldata->size());
                d.putItem("valueeditable", "false");
                d.putItem("numchild", ldata->size());
            }
            break;
        case 'O':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QObject *")) {
                if (addr) {
                    const QObject *ob = reinterpret_cast<const QObject *>(addr);
                    d.putItem("addr", ob);
                    d.putItem("value", ob->objectName());
                    d.putItem("valueencoded", "2");
                    d.putItem("type", NS "QObject");
                    d.putItem("displayedtype", ob->metaObject()->className());
                    d.putItem("numchild", 1);
                } else {
                    d.putItem("value", "0x0");
                    d.putItem("type", NS "QObject *");
                    d.putItem("numchild", 0);
                }
            }
#            endif
            break;
        case 'S':
            if (isEqual(type, "QString")) {
                d.putCommaIfNeeded();
                d.putItem(field, *(QString*)addr);
                d.put(',').put(field).put("encoded=\"2\"");
            }
            break;
        case 't':
            if (isEqual(type, "std::string")
                || isEqual(type, stdStringTypeC)) {
                d.putCommaIfNeeded();
                qDumpStdStringValue(d, *reinterpret_cast<const std::string*>(addr));
            } else if (isEqual(type, "std::wstring")
                || isEqual(type, stdWideStringTypeUShortC)) {
                qDumpStdWStringValue(d, *reinterpret_cast<const std::wstring*>(addr));
            }
            break;
        default:
            break;
    }
}

#if USE_QT_CORE
static void qDumpInnerValue(QDumper &d, const char *type, const void *addr)
{
    d.putItem("addr", addr);
    d.putItem("type", type, d.currentChildType);

    if (!type[0])
        return;

    return qDumpInnerValueHelper(d, type, addr);
}
#endif

static void qDumpInnerValueOrPointer(QDumper &d,
    const char *type, const char *strippedtype, const void *addr)
{
    if (strippedtype) {
        if (deref(addr)) {
            d.putItem("addr", deref(addr));
            d.putItem("type", strippedtype, d.currentChildType);
            qDumpInnerValueHelper(d, strippedtype, deref(addr));
        } else {
            d.putItem("addr", addr);
            d.putItem("type", strippedtype);
            d.putItem("value", "<null>");
            d.putItem("numchild", "0");
        }
    } else {
        d.putItem("addr", addr);
        d.putItem("type", type, d.currentChildType);
        qDumpInnerValueHelper(d, type, addr);
    }
}

//////////////////////////////////////////////////////////////////////////////

#if USE_QT_CORE

#ifndef QT_BOOTSTRAPPED
struct ModelIndex { int r; int c; void *p; void *m; };

static void qDumpQAbstractItem(QDumper &d)
{
    QModelIndex mi;
    {
       ModelIndex *mm = reinterpret_cast<ModelIndex *>(&mi);
       memset(&mi, 0, sizeof(mi));
       static const char *printFormat = sizeof(void *) == sizeof(long) ?
                                        "%d,%d,0x%lx,0x%lx" : "%d,%d,0x%llx,0x%llx";
       sscanf(d.templateParameters[0], printFormat, &mm->r, &mm->c, &mm->p, &mm->m);
    }
    const QAbstractItemModel *m = mi.model();
    const int rowCount = m->rowCount(mi);
    if (rowCount < 0)
        return;
    const int columnCount = m->columnCount(mi);
    if (columnCount < 0)
        return;
    d.putItem("type", NS "QAbstractItem");
    d.beginItem("addr");
        d.put('$').put(mi.row()).put(',').put(mi.column()).put(',')
            .put(mi.internalPointer()).put(',').put(mi.model());
    d.endItem();
    //d.putItem("value", "(").put(rowCount).put(",").put(columnCount).put(")");
    d.putItem("value", m->data(mi, Qt::DisplayRole).toString());
    d.putItem("valueencoded", "2");
    d.putItem("numchild", rowCount * columnCount);
    if (d.dumpChildren) {
        d.beginChildren();
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QModelIndex child = m->index(row, column, mi);
                d.beginHash();
                d.beginItem("name");
                    d.put("[").put(row).put(",").put(column).put("]");
                d.endItem();
                //d.putItem("numchild", (m->hasChildren(child) ? "1" : "0"));
                d.putItem("numchild", m->rowCount(child) * m->columnCount(child));
                d.beginItem("addr");
                    d.put("$").put(child.row()).put(",").put(child.column()).put(",")
                        .put(child.internalPointer()).put(",").put(child.model());
                d.endItem();
                d.putItem("type", NS "QAbstractItem");
                d.putItem("value", m->data(child, Qt::DisplayRole).toString());
                d.putItem("valueencoded", "2");
                d.endHash();
            }
        }
/*
        d.beginHash();
        d.putItem("name", "DisplayRole");
        d.putItem("numchild", 0);
        d.putItem("value", m->data(mi, Qt::DisplayRole).toString());
        d.putItem("valueencoded", 2);
        d.putItem("type", NS "QString");
        d.endHash();
*/
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQAbstractItemModel(QDumper &d)
{
    const QAbstractItemModel &m = *reinterpret_cast<const QAbstractItemModel *>(d.data);

    const int rowCount = m.rowCount();
    if (rowCount < 0)
        return;
    const int columnCount = m.columnCount();
    if (columnCount < 0)
        return;

    d.putItem("type", NS "QAbstractItemModel");
    d.beginItem("value");
        d.put("(").put(rowCount).put(",").put(columnCount).put(")");
    d.endItem();
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("numchild", "1");
            d.putItem("name", NS "QObject");
            d.putItem("addr", d.data);
            d.putItem("value", m.objectName());
            d.putItem("valueencoded", "2");
            d.putItem("type", NS "QObject");
            d.putItem("displayedtype", m.metaObject()->className());
        d.endHash();
        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                QModelIndex mi = m.index(row, column);
                d.beginHash();
                d.beginItem("name");
                    d.put("[").put(row).put(",").put(column).put("]");
                d.endItem();
                d.putItem("value", m.data(mi, Qt::DisplayRole).toString());
                d.putItem("valueencoded", "2");
                //d.putItem("numchild", (m.hasChildren(mi) ? "1" : "0"));
                d.putItem("numchild", m.rowCount(mi) * m.columnCount(mi));
                d.beginItem("addr");
                    d.put("$").put(mi.row()).put(",").put(mi.column()).put(",");
                    d.put(mi.internalPointer()).put(",").put(mi.model());
                d.endItem();
                d.putItem("type", NS "QAbstractItem");
                d.endHash();
            }
        }
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_BOOTSTRAPPED

static void qDumpQByteArray(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    const QByteArray &ba = *reinterpret_cast<const QByteArray *>(d.data);

    const int size = ba.size();
    if (size < 0)
        return;

    if (!ba.isEmpty()) {
        qCheckAccess(ba.constData());
        qCheckAccess(ba.constData() + size);
    }

    d.beginItem("value");
    if (size <= 100)
        d.put(ba);
    else
        d.put(ba.left(100)).put(" <size: ").put(size).put(", cut...>");
    d.endItem();
    d.putItem("valueencoded", "1");
    d.putItem("type", NS "QByteArray");
    d.putItem("numchild", size);
    if (d.dumpChildren) {
        d.putItem("childtype", "char");
        d.putItem("childnumchild", "0");
        d.beginChildren();
        char buf[20];
        for (int i = 0; i != size; ++i) {
            unsigned char c = ba.at(i);
            unsigned char u = (isprint(c) && c != '\'' && c != '"') ? c : '?';
            sprintf(buf, "%02x  (%u '%c')", c, c, u);
            d.beginHash();
            d.putItem("value", buf);
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQChar(QDumper &d)
{
    qDumpInnerQCharValue(d, *reinterpret_cast<const QChar *>(d.data), "value");
    d.disarm();
}

static void qDumpQDate(QDumper &d)
{
#ifdef QT_NO_DATESTRING
    qDumpUnknown(d);
#else
    const QDate &date = *reinterpret_cast<const QDate *>(d.data);
    if (date.isNull()) {
        d.putItem("value", "(null)");
        d.putItem("type", NS "QDate");
        d.putItem("numchild", "0");
        return;
    }
    d.putItem("value", date.toString());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QDate");
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("isNull", date.isNull());
        d.putHash("toString", date.toString());
#        if QT_VERSION >= 0x040500
        d.putHash("toString_(ISO)", date.toString(Qt::ISODate));
        d.putHash("toString_(SystemLocale)", date.toString(Qt::SystemLocaleDate));
        d.putHash("toString_(Locale)", date.toString(Qt::LocaleDate));
#        endif
        d.endChildren();
    }
    d.disarm();
#endif // ifdef QT_NO_DATESTRING
}

static void qDumpQTime(QDumper &d)
{
#ifdef QT_NO_DATESTRING
    qDumpUnknown(d);
#else
    const QTime &date = *reinterpret_cast<const QTime *>(d.data);
    if (date.isNull()) {
        d.putItem("value", "(null)");
        d.putItem("type", NS "QTime");
        d.putItem("numchild", "0");
        return;
    }
    d.putItem("value", date.toString());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QTime");
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("isNull", date.isNull());
        d.putHash("toString", date.toString());
#        if QT_VERSION >= 0x040500
        d.putHash("toString_(ISO)", date.toString(Qt::ISODate));
        d.putHash("toString_(SystemLocale)", date.toString(Qt::SystemLocaleDate));
        d.putHash("toString_(Locale)", date.toString(Qt::LocaleDate));
#        endif
        d.endChildren();
    }
    d.disarm();
#endif // ifdef QT_NO_DATESTRING
}

static void qDumpQDateTime(QDumper &d)
{
#ifdef QT_NO_DATESTRING
    qDumpUnknown(d);
#else
    const QDateTime &date = *reinterpret_cast<const QDateTime *>(d.data);
    if (date.isNull()) {
        d.putItem("value", "(null)");
        d.putItem("type", NS "QDateTime");
        d.putItem("numchild", "0");
        d.disarm();
        return;
    }
    d.putItem("value", date.toString());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QDateTime");
    d.putItem("numchild", "1");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("toTime_t", (long)date.toTime_t());
        d.putHash("toString", date.toString());
#        if QT_VERSION >= 0x040500
        d.putHash("toString_(ISO)", date.toString(Qt::ISODate));
        d.putHash("toString_(SystemLocale)", date.toString(Qt::SystemLocaleDate));
        d.putHash("toString_(Locale)", date.toString(Qt::LocaleDate));
#        endif

#        if 0
        d.beginHash();
        d.putItem("name", "toUTC");
        d.putItem("exp", "((" NSX "QDateTime" NSY "*)").put(d.data).put(")"
                    "->toTimeSpec('" NS "Qt::UTC')");
        d.putItem("type", NS "QDateTime");
        d.putItem("numchild", "1");
        d.endHash();
#        endif

#        if 0
        d.beginHash();
        d.putItem("name", "toLocalTime");
        d.putItem("exp", "((" NSX "QDateTime" NSY "*)").put(d.data).put(")"
                    "->toTimeSpec('" NS "Qt::LocalTime')");
        d.putItem("type", NS "QDateTime");
        d.putItem("numchild", "1");
        d.endHash();
#        endif

        d.endChildren();
    }
    d.disarm();
#endif // ifdef QT_NO_DATESTRING
}

static void qDumpQDir(QDumper &d)
{
    const QDir &dir = *reinterpret_cast<const QDir *>(d.data);
    d.putItem("value", dir.path());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QDir");
    d.putItem("numchild", "3");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("absolutePath", dir.absolutePath());
        d.putHash("canonicalPath", dir.canonicalPath());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQFile(QDumper &d)
{
    const QFile &file = *reinterpret_cast<const QFile *>(d.data);
    d.putItem("value", file.fileName());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QFile");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("fileName", file.fileName());
        d.putHash("exists", file.exists());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQFileInfo(QDumper &d)
{
    const QFileInfo &info = *reinterpret_cast<const QFileInfo *>(d.data);
    d.putItem("value", info.filePath());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QFileInfo");
    d.putItem("numchild", "3");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("absolutePath", info.absolutePath());
        d.putHash("absoluteFilePath", info.absoluteFilePath());
        d.putHash("canonicalPath", info.canonicalPath());
        d.putHash("canonicalFilePath", info.canonicalFilePath());
        d.putHash("completeBaseName", info.completeBaseName());
        d.putHash("completeSuffix", info.completeSuffix());
        d.putHash("baseName", info.baseName());
#ifdef Q_OS_MACX
        d.putHash("isBundle", info.isBundle());
        d.putHash("bundleName", info.bundleName());
#endif
        d.putHash("fileName", info.fileName());
        d.putHash("filePath", info.filePath());
        d.putHash("group", info.group());
        d.putHash("owner", info.owner());
        d.putHash("path", info.path());

        d.putHash("groupid", (long)info.groupId());
        d.putHash("ownerid", (long)info.ownerId());
        //QFile::Permissions permissions () const
        long perms = info.permissions();
        d.beginHash();
            d.putItem("name", "permissions");
            d.putItem("value", " ");
            d.putItem("type", NS "QFile::Permissions");
            d.putItem("numchild", 10);
            d.beginChildren();
                d.putHash("ReadOwner",  bool(perms & QFile::ReadOwner));
                d.putHash("WriteOwner", bool(perms & QFile::WriteOwner));
                d.putHash("ExeOwner",   bool(perms & QFile::ExeOwner));
                d.putHash("ReadUser",   bool(perms & QFile::ReadUser));
                d.putHash("WriteUser",  bool(perms & QFile::WriteUser));
                d.putHash("ExeUser",    bool(perms & QFile::ExeUser));
                d.putHash("ReadGroup",  bool(perms & QFile::ReadGroup));
                d.putHash("WriteGroup", bool(perms & QFile::WriteGroup));
                d.putHash("ExeGroup",   bool(perms & QFile::ExeGroup));
                d.putHash("ReadOther",  bool(perms & QFile::ReadOther));
                d.putHash("WriteOther", bool(perms & QFile::WriteOther));
                d.putHash("ExeOther",   bool(perms & QFile::ExeOther));
            d.endChildren();
        d.endHash();
        //QDir absoluteDir () const
        //QDir dir () const
        d.putHash("caching", info.caching());
        d.putHash("exists", info.exists());
        d.putHash("isAbsolute", info.isAbsolute());
        d.putHash("isDir", info.isDir());
        d.putHash("isExecutable", info.isExecutable());
        d.putHash("isFile", info.isFile());
        d.putHash("isHidden", info.isHidden());
        d.putHash("isReadable", info.isReadable());
        d.putHash("isRelative", info.isRelative());
        d.putHash("isRoot", info.isRoot());
        d.putHash("isSymLink", info.isSymLink());
        d.putHash("isWritable", info.isWritable());

        d.beginHash();
        d.putItem("name", "created");
        d.putItem("value", info.created().toString());
        d.putItem("valueencoded", "2");
        d.beginItem("exp");
            d.put("((" NSX "QFileInfo" NSY "*)").put(d.data).put(")->created()");
        d.endItem();
        d.putItem("type", NS "QDateTime");
        d.putItem("numchild", "1");
        d.endHash();

        d.beginHash();
        d.putItem("name", "lastModified");
        d.putItem("value", info.lastModified().toString());
        d.putItem("valueencoded", "2");
        d.beginItem("exp");
            d.put("((" NSX "QFileInfo" NSY "*)").put(d.data).put(")->lastModified()");
        d.endItem();
        d.putItem("type", NS "QDateTime");
        d.putItem("numchild", "1");
        d.endHash();

        d.beginHash();
        d.putItem("name", "lastRead");
        d.putItem("value", info.lastRead().toString());
        d.putItem("valueencoded", "2");
        d.beginItem("exp");
            d.put("((" NSX "QFileInfo" NSY "*)").put(d.data).put(")->lastRead()");
        d.endItem();
        d.putItem("type", NS "QDateTime");
        d.putItem("numchild", "1");
        d.endHash();

        d.endChildren();
    }
    d.disarm();
}

bool isOptimizedIntKey(const char *keyType)
{
    return isEqual(keyType, "int")
#if defined(Q_BYTE_ORDER) && Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        || isEqual(keyType, "short")
        || isEqual(keyType, "ushort")
#endif
        || isEqual(keyType, "uint");
}

int hashOffset(bool optimizedIntKey, bool forKey, unsigned keySize, unsigned valueSize)
{
    // int-key optimization, small value
    struct NodeOS { void *next; uint k; uint  v; } nodeOS;
    // int-key optimiatzion, large value
    struct NodeOL { void *next; uint k; void *v; } nodeOL;
    // no optimization, small value
    struct NodeNS { void *next; uint h; uint  k; uint  v; } nodeNS;
    // no optimization, large value
    struct NodeNL { void *next; uint h; uint  k; void *v; } nodeNL;
    // complex key
    struct NodeL  { void *next; uint h; void *k; void *v; } nodeL;

    if (forKey) {
        // offsetof(...,...) not yet in Standard C++
        const ulong nodeOSk ( (char *)&nodeOS.k - (char *)&nodeOS );
        const ulong nodeOLk ( (char *)&nodeOL.k - (char *)&nodeOL );
        const ulong nodeNSk ( (char *)&nodeNS.k - (char *)&nodeNS );
        const ulong nodeNLk ( (char *)&nodeNL.k - (char *)&nodeNL );
        const ulong nodeLk  ( (char *)&nodeL.k  - (char *)&nodeL );
        if (optimizedIntKey)
            return valueSize > sizeof(int) ? nodeOLk : nodeOSk;
        if (keySize > sizeof(int))
            return nodeLk;
        return valueSize > sizeof(int) ? nodeNLk : nodeNSk;
    } else {
        const ulong nodeOSv ( (char *)&nodeOS.v - (char *)&nodeOS );
        const ulong nodeOLv ( (char *)&nodeOL.v - (char *)&nodeOL );
        const ulong nodeNSv ( (char *)&nodeNS.v - (char *)&nodeNS );
        const ulong nodeNLv ( (char *)&nodeNL.v - (char *)&nodeNL );
        const ulong nodeLv  ( (char *)&nodeL.v  - (char *)&nodeL );
        if (optimizedIntKey)
            return valueSize > sizeof(int) ? nodeOLv : nodeOSv;
        if (keySize > sizeof(int))
            return nodeLv;
        return valueSize > sizeof(int) ? nodeNLv : nodeNSv;
    }
}

static void qDumpQHash(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    QHashData *h = *reinterpret_cast<QHashData *const*>(d.data);
    qCheckPointer(h->fakeNext);
    qCheckPointer(h->buckets);

    unsigned keySize = d.extraInt[0];
    unsigned valueSize = d.extraInt[1];

    int n = h->size;

    if (n < 0)
        return;
    if (n > 0) {
        qCheckPointer(h->fakeNext);
        qCheckPointer(*h->buckets);
    }

    d.putItemCount("value", n);
    d.putItem("numchild", n);

    if (d.dumpChildren) {
        const bool isSimpleKey = isSimpleType(keyType);
        const bool isSimpleValue = isSimpleType(valueType);
        const bool opt = isOptimizedIntKey(keyType);
        const int keyOffset = hashOffset(opt, true, keySize, valueSize);
        const int valueOffset = hashOffset(opt, false, keySize, valueSize);

#if 0
        d.beginItem("extra");
        d.put("isSimpleKey: ").put(isSimpleKey);
        d.put(" isSimpleValue: ").put(isSimpleValue);
        d.put(" valueType: '").put(isSimpleValue);
        d.put(" keySize: ").put(keyOffset);
        d.put(" valueOffset: ").put(valueOffset);
        d.put(" opt: ").put(opt);
        d.endItem();
#endif
        QHashData::Node *node = h->firstNode();
        QHashData::Node *end = reinterpret_cast<QHashData::Node *>(h);
        int i = 0;

        d.beginChildren();
        while (node != end) {
            d.beginHash();
                qDumpInnerValueHelper(d, keyType, addOffset(node, keyOffset), "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    d.putItem("type", valueType);
                    d.putItem("addr", addOffset(node, valueOffset));
                } else {
                    d.putItem("addr", node);
                    d.beginItem("type");
                        d.put(NS "QHashNode<").put(keyType).put(",")
                            .put(valueType).put(" >");
                    d.endItem();
                }
            d.endHash();
            ++i;
            node = QHashData::nextNode(node);
        }
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQHashNode(QDumper &d)
{
    const QHashData *h = reinterpret_cast<const QHashData *>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    unsigned keySize = d.extraInt[0];
    unsigned valueSize = d.extraInt[1];
    bool opt = isOptimizedIntKey(keyType);
    int keyOffset = hashOffset(opt, true, keySize, valueSize);
    int valueOffset = hashOffset(opt, false, keySize, valueSize);
    if (isSimpleType(valueType))
        qDumpInnerValueHelper(d, valueType, addOffset(h, valueOffset));
    else
        d.putItem("value", "");

    d.putItem("numchild", 2);
    if (d.dumpChildren) {
        // there is a hash specialization in case the keys are integers or shorts
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "key");
            d.putItem("type", keyType);
            d.putItem("addr", addOffset(h, keyOffset));
        d.endHash();
        d.beginHash();
            d.putItem("name", "value");
            d.putItem("type", valueType);
            d.putItem("addr", addOffset(h, valueOffset));
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}

#if USE_QT_GUI
static void qDumpQImage(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    const QImage &im = *reinterpret_cast<const QImage *>(d.data);
    d.beginItem("value");
        d.put("(").put(im.width()).put("x").put(im.height()).put(")");
    d.endItem();
    d.putItem("type", NS "QImage");
    d.putItem("numchild", "0");
#if 0
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "data");
            d.putItem("type", NS "QImageData");
            d.putItem("addr", d.data);
        d.endHash();
        d.endChildren();
    }
#endif
    d.disarm();
}
#endif

#if USE_QT_GUI
static void qDumpQImageData(QDumper &d)
{
    const QImage &im = *reinterpret_cast<const QImage *>(d.data);
    const QByteArray ba(QByteArray::fromRawData((const char*)im.bits(), im.byteCount()));
    d.putItem("type", NS "QImageData");
    d.putItem("numchild", "0");
#if 1
    d.putItem("value", "<hover here>");
    d.putItem("valuetooltipencoded", "1");
    d.putItem("valuetooltipsize", ba.size());
    d.putItem("valuetooltip", ba);
#else
    d.putItem("valueencoded", "1");
    d.putItem("value", ba);
#endif
    d.disarm();
}
#endif

static void qDumpQList(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    // This uses the knowledge that QList<T> has only a single member
    // of type  union { QListData p; QListData::Data *d; };

    const QListData &pdata = *reinterpret_cast<const QListData*>(d.data);
    const int nn = pdata.size();
    if (nn < 0)
        return;
    const bool innerTypeIsPointer = isPointerType(d.innerType);
    const int n = qMin(nn, 1000);
    if (nn > 0) {
        if (pdata.d->begin < 0)
            return;
        if (pdata.d->begin > pdata.d->end)
            return;
#if QT_VERSION >= 0x050000
        if (pdata.d->ref.atomic._q_value <= 0)
            return;
#elif QT_VERSION >= 0x040400
        if (pdata.d->ref._q_value <= 0)
            return;
#endif
        qCheckAccess(pdata.d->array);
        // Additional checks on pointer arrays
        if (innerTypeIsPointer)
            for (int i = 0; i != n; ++i)
                if (const void *p = pdata.d->array + i + pdata.d->begin)
                    qCheckPointer(deref(p));
    }

    d.putItemCount("value", nn);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        const unsigned innerSize = d.extraInt[0];
        QByteArray strippedInnerType = stripPointerType(d.innerType);

        // The exact condition here is:
        //  QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic
        // but this data is available neither in the compiled binary nor
        // in the frontend.
        // So as first approximation only do the 'isLarge' check:
        bool isInternal = innerSize <= int(sizeof(void*))
            && isMovableType(d.innerType);
        d.putItem("internal", (int)isInternal);
        d.beginChildren(n ? d.innerType : 0);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            if (innerTypeIsPointer) {
                void *p = pdata.d->array + i + pdata.d->begin;
                if (*(void**)p) {
                    //d.putItem("value","@").put(p);
                    qDumpInnerValue(d, strippedInnerType.data(), deref(p));
                } else {
                    d.putItem("value", "<null>");
                    d.putItem("numchild", "0");
                }
            } else {
                void *p = pdata.d->array + i + pdata.d->begin;
                if (isInternal) {
                    //qDumpInnerValue(d, d.innerType, p);
                    d.putItem("addr", p);
                    qDumpInnerValueHelper(d, d.innerType, p);
                } else {
                    //qDumpInnerValue(d, d.innerType, deref(p));
                    d.putItem("addr", deref(p));
                    qDumpInnerValueHelper(d, d.innerType, deref(p));
                }
            }
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQLinkedList(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    // This uses the knowledge that QLinkedList<T> has only a single member
    // of type  union { QLinkedListData *d; QLinkedListNode<T> *e; };
    const QLinkedListData *ldata =
        reinterpret_cast<const QLinkedListData*>(deref(d.data));
    int nn = ldata->size;
    if (nn < 0)
        return;

    int n = nn;
    d.putItemCount("value", n);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        //unsigned innerSize = d.extraInt[0];
        //bool innerTypeIsPointer = isPointerType(d.innerType);
        QByteArray strippedInnerType = stripPointerType(d.innerType);
        const char *stripped =
            isPointerType(d.innerType) ? strippedInnerType.data() : 0;

        if (n > 1000)
            n = 1000;
        d.beginChildren(d.innerType);
        const void *p = deref(ldata);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            const void *addr = addOffset(p, 2 * sizeof(void*));
            qDumpInnerValueOrPointer(d, d.innerType, stripped, addr);
            p = deref(p);
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQLocale(QDumper &d)
{
    const QLocale &locale = *reinterpret_cast<const QLocale *>(d.data);
    d.putItem("value", locale.name());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QLocale");
    d.putItem("numchild", "8");
    if (d.dumpChildren) {
        d.beginChildren();

        d.beginHash();
        d.putItem("name", "country");
        d.beginItem("exp");
        d.put("((" NSX "QLocale" NSY "*)").put(d.data).put(")->country()");
        d.endItem();
        d.endHash();

        d.beginHash();
        d.putItem("name", "language");
        d.beginItem("exp");
        d.put("((" NSX "QLocale" NSY "*)").put(d.data).put(")->language()");
        d.endItem();
        d.endHash();

        d.beginHash();
        d.putItem("name", "measurementSystem");
        d.beginItem("exp");
        d.put("((" NSX "QLocale" NSY "*)").put(d.data).put(")->measurementSystem()");
        d.endItem();
        d.endHash();

        d.beginHash();
        d.putItem("name", "numberOptions");
        d.beginItem("exp");
        d.put("((" NSX "QLocale" NSY "*)").put(d.data).put(")->numberOptions()");
        d.endItem();
        d.endHash();

        d.putHash("timeFormat_(short)", locale.timeFormat(QLocale::ShortFormat));
        d.putHash("timeFormat_(long)", locale.timeFormat(QLocale::LongFormat));

        d.putHash("decimalPoint", locale.decimalPoint());
        d.putHash("exponential", locale.exponential());
        d.putHash("percent", locale.percent());
        d.putHash("zeroDigit", locale.zeroDigit());
        d.putHash("groupSeparator", locale.groupSeparator());
        d.putHash("negativeSign", locale.negativeSign());

        d.endChildren();
    }
    d.disarm();
}

#if MAP_WORKS
static void qDumpQMapNode(QDumper &d)
{
    const QMapData *h = reinterpret_cast<const QMapData *>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    qCheckAccess(h->backward);
    qCheckAccess(h->forward[0]);

    d.putItem("value", "");
    d.putItem("numchild", 2);
    if (d.dumpChildren) {
        unsigned mapnodesize = d.extraInt[2];
        unsigned valueOff = d.extraInt[3];

        unsigned keyOffset = 2 * sizeof(void*) - mapnodesize;
        unsigned valueOffset = 2 * sizeof(void*) - mapnodesize + valueOff;

        d.beginChildren();
        d.beginHash();
        d.putItem("name", "key");
        qDumpInnerValue(d, keyType, addOffset(h, keyOffset));

        d.endHash();
        d.beginHash();
        d.putItem("name", "value");
        qDumpInnerValue(d, valueType, addOffset(h, valueOffset));
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQMap(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    QMapData *h = *reinterpret_cast<QMapData *const*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    int n = h->size;

    if (n < 0)
        return;
    if (n > 0) {
        qCheckAccess(h->backward);
        qCheckAccess(h->forward[0]);
        qCheckPointer(h->backward->backward);
        qCheckPointer(h->forward[0]->backward);
    }

    d.putItemCount("value", n);
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        //unsigned keySize = d.extraInt[0];
        //unsigned valueSize = d.extraInt[1];
        unsigned mapnodesize = d.extraInt[2];
        unsigned valueOff = d.extraInt[3];

        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        // both negative:
        int keyOffset = 2 * sizeof(void*) - int(mapnodesize);
        int valueOffset = 2 * sizeof(void*) - int(mapnodesize) + valueOff;

        d.beginItem("extra");
        d.put("simplekey: ").put(isSimpleKey).put(" isSimpleValue: ").put(isSimpleValue);
        d.put(" keyOffset: ").put(keyOffset).put(" valueOffset: ").put(valueOffset);
        d.put(" mapnodesize: ").put(mapnodesize);
        d.endItem();
        d.beginChildren();

        QMapData::Node *node = reinterpret_cast<QMapData::Node *>(h->forward[0]);
        QMapData::Node *end = reinterpret_cast<QMapData::Node *>(h);
        int i = 0;

        while (node != end) {
            d.beginHash();
                qDumpInnerValueHelper(d, keyType, addOffset(node, keyOffset), "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    d.putItem("type", valueType);
                    d.putItem("addr", addOffset(node, valueOffset));
                } else {
#if QT_VERSION >= 0x040500
                    d.putItem("addr", node);
                    // actually, any type (even 'char') will do...
                    d.beginItem("type");
                        d.put(NS "QMapNode<").put(keyType).put(",");
                        d.put(valueType).put(" >");
                    d.endItem();
#else
                    d.beginItem("type");
                        d.put(NS "QMapData::Node<").put(keyType).put(",");
                        d.put(valueType).put(" >");
                    d.endItem();
                    d.beginItem("exp");
                        d.put("*('" NS "QMapData::Node<").put(keyType).put(",");
                        d.put(valueType).put(" >'*)").put(node);
                    d.endItem();
#endif
                }
            d.endHash();

            ++i;
            node = node->forward[0];
        }
        d.endChildren();
    }

    d.disarm();
}

static void qDumpQMultiMap(QDumper &d)
{
    qDumpQMap(d);
}
#endif // MAP_WORKS

#ifndef QT_BOOTSTRAPPED
static void qDumpQModelIndex(QDumper &d)
{
    const QModelIndex *mi = reinterpret_cast<const QModelIndex *>(d.data);

    d.putItem("type", NS "QModelIndex");
    if (mi->isValid()) {
        d.beginItem("value");
            d.put("(").put(mi->row()).put(", ").put(mi->column()).put(")");
        d.endItem();
        d.putItem("numchild", 5);
        if (d.dumpChildren) {
            d.beginChildren();
            d.putHash("row", mi->row());
            d.putHash("column", mi->column());

            d.beginHash();
            d.putItem("name", "parent");
            const QModelIndex parent = mi->parent();
            d.beginItem("value");
            if (parent.isValid())
                d.put("(").put(parent.row()).put(", ").put(parent.column()).put(")");
            else
                d.put("<invalid>");
            d.endItem();
            d.beginItem("exp");
                d.put("((" NSX "QModelIndex" NSY "*)").put(d.data).put(")->parent()");
            d.endItem();
            d.putItem("type", NS "QModelIndex");
            d.putItem("numchild", "1");
            d.endHash();

            d.putHash("internalId", QString::number(mi->internalId(), 10));

            d.beginHash();
            d.putItem("name", "model");
            d.putItem("value", static_cast<const void *>(mi->model()));
            d.putItem("type", NS "QAbstractItemModel*");
            d.putItem("numchild", "1");
            d.endHash();

            d.endChildren();
        }
    } else {
        d.putItem("value", "<invalid>");
        d.putItem("numchild", 0);
    }

    d.disarm();
}

static void qDumpQObject(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QMetaObject *mo = ob->metaObject();
    d.putItem("value", ob->objectName());
    d.putItem("valueencoded", "2");
    d.putItem("type", NS "QObject");
    d.putItem("displayedtype", mo->className());
    d.putItem("numchild", 4);
    if (d.dumpChildren) {
        int slotCount = 0;
        int signalCount = 0;
        for (int i = mo->methodCount(); --i >= 0; ) {
            QMetaMethod::MethodType mt = mo->method(i).methodType();
            signalCount += (mt == QMetaMethod::Signal);
            slotCount += (mt == QMetaMethod::Slot);
        }
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "properties");
            // using 'addr' does not work in gdb as 'exp' is recreated as
            // (type *)addr, and here we have different 'types':
            // QObject vs QObjectPropertyList!
            d.putItem("addr", d.data);
            d.putItem("type", NS "QObjectPropertyList");
            d.putItemCount("value", mo->propertyCount());
            d.putItem("numchild", mo->propertyCount());
        d.endHash();
        d.beginHash();
            d.putItem("name", "signals");
            d.putItem("addr", d.data);
            d.putItem("type", NS "QObjectSignalList");
            d.putItemCount("value", signalCount);
            d.putItem("numchild", signalCount);
        d.endHash();
        d.beginHash();
            d.putItem("name", "slots");
            d.putItem("addr", d.data);
            d.putItem("type", NS "QObjectSlotList");
            d.putItemCount("value", slotCount);
            d.putItem("numchild", slotCount);
        d.endHash();
        const QObjectList objectChildren = ob->children();
        if (!objectChildren.empty()) {
            d.beginHash();
            d.putItem("name", "children");
            d.putItem("addr", d.data);
            d.putItem("type", NS "QObjectChildList");
            d.putItemCount("value", objectChildren.size());
            d.putItem("numchild", objectChildren.size());
            d.endHash();
        }
        d.beginHash();
            d.putItem("name", "parent");
            qDumpInnerValueHelper(d, NS "QObject *", ob->parent());
        d.endHash();
#if 1
        d.beginHash();
            d.putItem("name", "className");
            d.putItem("value", ob->metaObject()->className());
            d.putItem("type", "");
            d.putItem("numchild", "0");
        d.endHash();
#endif
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_BOOTSTRAPPED

#if USE_QT_GUI
static const char *sizePolicyEnumValue(QSizePolicy::Policy p)
{
    switch (p) {
    case QSizePolicy::Fixed:
        return "Fixed";
    case QSizePolicy::Minimum:
        return "Minimum";
    case QSizePolicy::Maximum:
        return "Maximum";
    case QSizePolicy::Preferred:
        return "Preferred";
    case QSizePolicy::Expanding:
        return "Expanding";
    case QSizePolicy::MinimumExpanding:
        return "MinimumExpanding";
    case QSizePolicy::Ignored:
        break;
    }
    return "Ignored";
}

static QString sizePolicyValue(const QSizePolicy &sp)
{
    QString rc;
    QTextStream str(&rc);
    // Display as in Designer
    str << '[' << sizePolicyEnumValue(sp.horizontalPolicy())
        << ", " << sizePolicyEnumValue(sp.verticalPolicy())
        << ", " << sp.horizontalStretch() << ", " << sp.verticalStretch() << ']';
    return rc;
}
#endif

static void qDumpQVariantHelper(const QVariant *v, QString *value,
    QString *exp, int *numchild)
{
    switch (v->type()) {
    case QVariant::Invalid:
        *value = QLatin1String("<invalid>");
        *numchild = 0;
        break;
    case QVariant::String:
        *value = QLatin1Char('"') + v->toString() + QLatin1Char('"');
        *numchild = 0;
        break;
#    if QT_VERSION >= 0x040500
    case QVariant::StringList:
        *exp = QString(QLatin1String("(*('" NS "QStringList'*)%1)"))
                    .arg((quintptr)v);
        *numchild = v->toStringList().size();
        break;
#    endif
    case QVariant::Int:
        *value = QString::number(v->toInt());
        *numchild= 0;
        break;
    case QVariant::Double:
        *value = QString::number(v->toDouble());
        *numchild = 0;
        break;
#    ifndef QT_BOOTSTRAPPED
    case QVariant::Point: {
            const QPoint p = v->toPoint();
            *value = QString::fromLatin1("%1, %2").arg(p.x()).arg(p.y());
        }
        *numchild = 0;
        break;
    case QVariant::Size: {
            const QSize size = v->toSize();
            *value = QString::fromLatin1("%1x%2")
                .arg(size.width()).arg(size.height());
        }
        *numchild = 0;
        break;
    case QVariant::Rect: {
            const QRect rect = v->toRect();
            *value = QString::fromLatin1("%1x%2+%3+%4")
                .arg(rect.width()).arg(rect.height())
                .arg(rect.x()).arg(rect.y());
        }
        *numchild = 0;
        break;
    case QVariant::PointF: {
            const QPointF p = v->toPointF();
            *value = QString::fromLatin1("%1, %2").arg(p.x()).arg(p.y());
        }
        *numchild = 0;
        break;

    case QVariant::SizeF: {
            const QSizeF size = v->toSizeF();
            *value = QString::fromLatin1("%1x%2")
                .arg(size.width()).arg(size.height());
        }
        *numchild = 0;
        break;
    case QVariant::RectF: {
            const QRectF rect = v->toRectF();
            *value = QString::fromLatin1("%1x%2+%3+%4")
                .arg(rect.width()).arg(rect.height())
                .arg(rect.x()).arg(rect.y());
        }
        *numchild = 0;
        break;
#    endif // QT_BOOTSTRAPPED
#    if USE_QT_GUI
    case QVariant::Font:
        *value = qvariant_cast<QFont>(*v).toString();
        break;
    case QVariant::Color:
        *value = qvariant_cast<QColor>(*v).name();
        break;
    case QVariant::KeySequence:
#        ifndef QT_NO_SHORTCUT
        *value = qvariant_cast<QKeySequence>(*v).toString();
#        else
        *value = QString::fromLatin1("Disabled by QT_NO_SHORTCUT");
#        endif
        break;
    case QVariant::SizePolicy:
        *value = sizePolicyValue(qvariant_cast<QSizePolicy>(*v));
        break;
#    endif
    default: {
        static const char *qTypeFormat = sizeof(void *) == sizeof(long)
            ? "'" NS "%s " NS "qvariant_cast<" NS "%s >'(*('" NS "QVariant'*)0x%lx)"
            : "'" NS "%s " NS "qvariant_cast<" NS "%s >'(*('" NS "QVariant'*)0x%llx)";
        static const char *nonQTypeFormat = sizeof(void *) == sizeof(long)
            ? "'%s " NS "qvariant_cast<%s >'(*('" NS "QVariant'*)0x%lx)"
            : "'%s " NS "qvariant_cast<%s >'(*('" NS "QVariant'*)0x%llx)";
        char buf[1000];
        const char *format = (v->typeName()[0] == 'Q') ? qTypeFormat : nonQTypeFormat;
        qsnprintf(buf, sizeof(buf) - 1, format, v->typeName(), v->typeName(), v);
        *exp = QLatin1String(buf);
        *numchild = 1;
        break;
        }
    }
}

static void qDumpQVariant(QDumper &d, const QVariant *v)
{
    QString value;
    QString exp;
    int numchild = 0;
    qDumpQVariantHelper(v, &value, &exp, &numchild);
    bool isInvalid = (v->typeName() == 0);
    if (isInvalid) {
        d.putItem("value", "(invalid)");
    } else if (value.isEmpty()) {
        d.beginItem("value");
            d.put("(").put(v->typeName()).put(") ");
        d.endItem();
    } else {
        QByteArray ba;
        ba += '(';
        ba += v->typeName();
        ba += ") ";
        ba += qPrintable(value);
        d.putItem("value", ba);
        d.putItem("valueencoded", "5");
    }
    d.putItem("type", NS "QVariant");
    if (isInvalid || !numchild) {
        d.putItem("numchild", "0");
    } else {
        d.putItem("numchild", "1");
        if (d.dumpChildren) {
            d.beginChildren();
            d.beginHash();
            d.putItem("name", "value");
            if (!exp.isEmpty())
                d.putItem("exp", qPrintable(exp));
            if (!value.isEmpty()) {
                d.putItem("value", value);
                d.putItem("valueencoded", "4");
            }
            d.putItem("type", v->typeName());
            d.putItem("numchild", numchild);
            d.endHash();
            d.endChildren();
        }
    }
    d.disarm();
}

static inline void qDumpQVariant(QDumper &d)
{
    qCheckAccess(d.data);
    qDumpQVariant(d, reinterpret_cast<const QVariant *>(d.data));
}

// Meta enumeration helpers
static inline void dumpMetaEnumType(QDumper &d, const QMetaEnum &me)
{
    QByteArray type = me.scope();
    if (!type.isEmpty())
        type += "::";
    type += me.name();
    d.putItem("type", type.constData());
}

static inline void dumpMetaEnumValue(QDumper &d, const QMetaProperty &mop,
                                     int value)
{

    const QMetaEnum me = mop.enumerator();
    dumpMetaEnumType(d, me);
    if (const char *enumValue = me.valueToKey(value)) {
        d.putItem("value", enumValue);
    } else {
        d.putItem("value", value);
    }
    d.putItem("numchild", 0);
}

static inline void dumpMetaFlagValue(QDumper &d, const QMetaProperty &mop,
                                     int value)
{
    const QMetaEnum me = mop.enumerator();
    dumpMetaEnumType(d, me);
    const QByteArray flagsValue = me.valueToKeys(value);
    if (flagsValue.isEmpty()) {
        d.putItem("value", value);
    } else {
        d.putItem("value", flagsValue.constData());
    }
    d.putItem("numchild", 0);
}

#ifndef QT_BOOTSTRAPPED
static void qDumpQObjectProperty(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mob = ob->metaObject();
    // extract "local.Object.property"
    QString iname = d.iname;
    const int dotPos = iname.lastIndexOf(QLatin1Char('.'));
    if (dotPos == -1)
        return;
    iname.remove(0, dotPos + 1);
    const int index = mob->indexOfProperty(iname.toLatin1());
    if (index == -1)
        return;
    const QMetaProperty mop = mob->property(index);
    const QVariant value = mop.read(ob);
    const bool isInteger = value.type() == QVariant::Int;
    if (isInteger && mop.isEnumType()) {
        dumpMetaEnumValue(d, mop, value.toInt());
    } else if (isInteger && mop.isFlagType()) {
        dumpMetaFlagValue(d, mop, value.toInt());
    } else {
        qDumpQVariant(d, &value);
    }
    d.disarm();
}

static void qDumpQObjectPropertyList(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mo = ob->metaObject();
    const int propertyCount = mo->propertyCount();
    d.putItem("addr", "<synthetic>");
    d.putItem("type", NS "QObjectPropertyList");
    d.putItem("numchild", propertyCount);
    d.putItemCount("value", propertyCount);
    if (d.dumpChildren) {
        d.beginChildren();
        for (int i = propertyCount; --i >= 0; ) {
            const QMetaProperty & prop = mo->property(i);
            d.beginHash();
            d.putItem("name", prop.name());
            switch (prop.type()) {
            case QVariant::String:
                d.putItem("type", prop.typeName());
                d.putItem("value", prop.read(ob).toString());
                d.putItem("valueencoded", "2");
                d.putItem("numchild", "0");
                break;
            case QVariant::Bool:
                d.putItem("type", prop.typeName());
                d.putItem("value", (prop.read(ob).toBool() ? "true" : "false"));
                d.putItem("numchild", "0");
                break;
            case QVariant::Int:
                if (prop.isEnumType()) {
                    dumpMetaEnumValue(d, prop, prop.read(ob).toInt());
                } else if (prop.isFlagType()) {
                    dumpMetaFlagValue(d, prop, prop.read(ob).toInt());
                } else {
                    d.putItem("value", prop.read(ob).toInt());
                    d.putItem("numchild", "0");
                }
                break;
            default:
                d.putItem("addr", d.data);
                d.putItem("type", NS "QObjectProperty");
                d.putItem("numchild", "1");
                break;
            }
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

static QByteArray methodSignature(const QMetaMethod &method)
{
#if QT_VERSION >= 0x050000
    return method.methodSignature();
#else
    return QByteArray(method.signature());
#endif
}

static void qDumpQObjectMethodList(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mo = ob->metaObject();
    d.putItem("addr", "<synthetic>");
    d.putItem("type", NS "QObjectMethodList");
    d.putItem("numchild", mo->methodCount());
    if (d.dumpChildren) {
        d.putItem("childtype", NS "QMetaMethod::Method");
        d.putItem("childnumchild", "0");
        d.beginChildren();
        for (int i = 0; i != mo->methodCount(); ++i) {
            const QMetaMethod & method = mo->method(i);
            int mt = method.methodType();
            const QByteArray sig = methodSignature(method);
            d.beginHash();
            d.beginItem("name");
                d.put(i).put(" ").put(mo->indexOfMethod(sig));
                d.put(" ").put(sig);
            d.endItem();
            d.beginItem("value");
                d.put((mt == QMetaMethod::Signal ? "<Signal>" : "<Slot>"));
                d.put(" (").put(mt).put(")");
            d.endItem();
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}

static const char *qConnectionType(uint type)
{
    Qt::ConnectionType connType = static_cast<Qt::ConnectionType>(type);
    const char *output = "unknown";
    switch (connType) {
        case Qt::AutoConnection: output = "auto"; break;
        case Qt::DirectConnection: output = "direct"; break;
        case Qt::QueuedConnection: output = "queued"; break;
        case Qt::BlockingQueuedConnection: output = "blockingqueued"; break;
#if QT_VERSION < 0x050000
        case 3: output = "autocompat"; break;
#endif
#if QT_VERSION >= 0x040600
        case Qt::UniqueConnection: break; // Can't happen.
#endif
        };
    return output;
}

#if QT_VERSION < 0x040400

#else
static const ConnectionList &qConnectionList(const QObject *ob, int signalNumber)
{
    static const ConnectionList emptyList;
    const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob));
    if (!p->connectionLists)
        return emptyList;
    typedef QVector<ConnectionList> ConnLists;
    const ConnLists *lists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    // there's an optimization making the lists only large enough to hold the
    // last non-empty item
    if (signalNumber >= lists->size())
        return emptyList;
    return lists->at(signalNumber);
}
#endif

// Write party involved in a slot/signal element,
// avoid to recursion to self.
static inline void qDumpQObjectConnectionPart(QDumper &d,
                                              const QObject *owner,
                                              const QObject *partner,
                                              int number, const char *namePostfix)
{
    d.beginHash();
    d.beginItem("name");
    d.put(number).put(namePostfix);
    d.endItem();
    if (partner == owner) {
        d.putItem("value", "<this>");
        d.putItem("type", owner->metaObject()->className());
        d.putItem("numchild", 0);
        d.putItem("addr", owner);
    } else {
        qDumpInnerValueHelper(d, NS "QObject *", partner);
    }
    d.endHash();
}

static void qDumpQObjectSignal(QDumper &d)
{
    unsigned signalNumber = d.extraInt[0];

    d.putItem("addr", "<synthetic>");
    d.putItem("numchild", "1");
    d.putItem("type", NS "QObjectSignal");

#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        const QObject *ob = reinterpret_cast<const QObject *>(d.data);
        d.beginChildren();
        const ConnectionList &connList = qConnectionList(ob, signalNumber);
        for (int i = 0; i != connList.size(); ++i) {
            const Connection &conn = connectionAt(connList, i);
            qDumpQObjectConnectionPart(d, ob, conn.receiver, i, " receiver");
            d.beginHash();
                d.beginItem("name");
                    d.put(i).put(" slot");
                d.endItem();
                d.putItem("type", "");
                if (conn.receiver)
                    d.putItem("value", methodSignature(conn.receiver->metaObject()
                        ->method(conn.method_())));
                else
                    d.putItem("value", "<invalid receiver>");
                d.putItem("numchild", "0");
            d.endHash();
            d.beginHash();
                d.beginItem("name");
                    d.put(i).put(" type");
                d.endItem();
                d.putItem("type", "");
                d.beginItem("value");
                    d.put("<").put(qConnectionType(conn.connectionType)).put(" connection>");
                d.endItem();
                d.putItem("numchild", "0");
            d.endHash();
        }
        d.endChildren();
        d.putItem("numchild", connList.size());
    }
#endif
    d.disarm();
}

static void qDumpQObjectSignalList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QMetaObject *mo = ob->metaObject();
    int count = 0;
    const int methodCount = mo->methodCount();
    for (int i = methodCount; --i >= 0; )
        count += (mo->method(i).methodType() == QMetaMethod::Signal);
    d.putItem("type", NS "QObjectSignalList");
    d.putItemCount("value", count);
    d.putItem("addr", d.data);
    d.putItem("numchild", count);
#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d.beginChildren();
        for (int i = 0; i != methodCount; ++i) {
            const QMetaMethod & method = mo->method(i);
            if (method.methodType() == QMetaMethod::Signal) {
                int k = mo->indexOfSignal(methodSignature(method));
                const ConnectionList &connList = qConnectionList(ob, k);
                d.beginHash();
                d.putItem("name", k);
                d.putItem("value", methodSignature(method));
                d.putItem("numchild", connList.size());
                d.putItem("addr", d.data);
                d.putItem("type", NS "QObjectSignal");
                d.endHash();
            }
        }
        d.endChildren();
    }
#endif
    d.disarm();
}

static void qDumpQObjectSlot(QDumper &d)
{
    int slotNumber = d.extraInt[0];

    d.putItem("addr", d.data);
    d.putItem("numchild", "1");
    d.putItem("type", NS "QObjectSlot");

#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d.beginChildren();
        int numchild = 0;
        const QObject *ob = reinterpret_cast<const QObject *>(d.data);
        const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob));
#if QT_VERSION >= 0x040600
        int s = 0;
        for (SenderList senderList = p->senders; senderList != 0;
             senderList = senderList->next, ++s) {
            const QObject *sender = senderList->sender;
            int signal = senderList->method_(); // FIXME: 'method' is wrong.
#else
        for (int s = 0; s != p->senders.size(); ++s) {
            const QObject *sender = senderAt(p->senders, s);
            int signal = signalAt(p->senders, s);
#endif
            const ConnectionList &connList = qConnectionList(sender, signal);
            for (int i = 0; i != connList.size(); ++i) {
                const Connection &conn = connectionAt(connList, i);
                if (conn.receiver == ob && conn.method_() == slotNumber) {
                    ++numchild;
                    QMetaMethod method = sender->metaObject()->method(signal);
                    qDumpQObjectConnectionPart(d, ob, sender, s, " sender");
                    d.beginHash();
                        d.beginItem("name");
                            d.put(s).put(" signal");
                        d.endItem();
                        d.putItem("type", "");
                        d.putItem("value", methodSignature(method));
                        d.putItem("numchild", "0");
                    d.endHash();
                    d.beginHash();
                        d.beginItem("name");
                            d.put(s).put(" type");
                        d.endItem();
                        d.putItem("type", "");
                        d.beginItem("value");
                            d.put("<").put(qConnectionType(conn.method_()));
                            d.put(" connection>");
                        d.endItem();
                        d.putItem("numchild", "0");
                    d.endHash();
                }
            }
        }
        d.endChildren();
        d.putItem("numchild", numchild);
    }
#endif
    d.disarm();
}

static void qDumpQObjectSlotList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
#if QT_VERSION >= 0x040400
    const ObjectPrivate *p = reinterpret_cast<const ObjectPrivate *>(dfunc(ob));
#endif
    const QMetaObject *mo = ob->metaObject();

    int count = 0;
    const int methodCount = mo->methodCount();
    for (int i = methodCount; --i >= 0; )
        count += (mo->method(i).methodType() == QMetaMethod::Slot);

    d.putItem("numchild", count);
    d.putItemCount("value", count);
    d.putItem("type", NS "QObjectSlotList");
    if (d.dumpChildren) {
        d.beginChildren();
#if QT_VERSION >= 0x040400
        for (int i = 0; i != methodCount; ++i) {
            QMetaMethod method = mo->method(i);
            if (method.methodType() == QMetaMethod::Slot) {
                d.beginHash();
                QByteArray sig = methodSignature(method);
                int k = mo->indexOfSlot(sig);
                d.putItem("name", k);
                d.putItem("value", sig);

                // count senders. expensive...
                int numchild = 0;
#if QT_VERSION >= 0x040600
                int s = 0;
                for (SenderList senderList = p->senders; senderList != 0;
                     senderList = senderList->next, ++s) {
                    const QObject *sender = senderList->sender;
                    int signal = senderList->method_(); // FIXME: 'method' is wrong.
#else
                for (int s = 0; s != p->senders.size(); ++s) {
                    const QObject *sender = senderAt(p->senders, s);
                    int signal = signalAt(p->senders, s);
#endif
                    const ConnectionList &connList = qConnectionList(sender, signal);
                    for (int c = 0; c != connList.size(); ++c) {
                        const Connection &conn = connectionAt(connList, c);
                        if (conn.receiver == ob && conn.method_() == k)
                            ++numchild;
                    }
                }
                d.putItem("numchild", numchild);
                d.putItem("addr", d.data);
                d.putItem("type", NS "QObjectSlot");
                d.endHash();
            }
        }
#endif
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQObjectChildList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QObjectList children = ob->children();
    const int size = children.size();

    d.putItem("numchild", size);
    d.putItemCount("value", size);
    d.putItem("type", NS "QObjectChildList");
    if (d.dumpChildren) {
        d.beginChildren();
        for (int i = 0; i != size; ++i) {
            d.beginHash();
            qDumpInnerValueHelper(d, NS "QObject *", children.at(i));
            d.endHash();
        }
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_BOOTSTRAPPED

#if USE_QT_GUI
static void qDumpQPixmap(QDumper &d)
{
    const QPixmap &im = *reinterpret_cast<const QPixmap *>(d.data);
    d.beginItem("value");
        d.put("(").put(im.width()).put("x").put(im.height()).put(")");
    d.endItem();
    d.putItem("type", NS "QPixmap");
    d.putItem("numchild", "0");
    d.disarm();
}
#endif

#ifndef QT_BOOTSTRAPPED
static void qDumpQPoint(QDumper &d)
{
    const QPoint &pnt = *reinterpret_cast<const QPoint *>(d.data);
    d.beginItem("value");
        d.put("(").put(pnt.x()).put(", ").put(pnt.y()).put(")");
    d.endItem();
    d.putItem("type", NS "QPoint");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("x", pnt.x());
        d.putHash("y", pnt.y());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQPointF(QDumper &d)
{
    const QPointF &pnt = *reinterpret_cast<const QPointF *>(d.data);
    d.beginItem("value");
        d.put("(").put(pnt.x()).put(", ").put(pnt.y()).put(")");
    d.endItem();
    d.putItem("type", NS "QPointF");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("x", pnt.x());
        d.putHash("y", pnt.y());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQRect(QDumper &d)
{
    const QRect &rc = *reinterpret_cast<const QRect *>(d.data);
    d.beginItem("value");
        d.put("(").put(rc.width()).put("x").put(rc.height());
        if (rc.x() >= 0)
            d.put("+");
        d.put(rc.x());
        if (rc.y() >= 0)
            d.put("+");
        d.put(rc.y());
        d.put(")");
    d.endItem();
    d.putItem("type", NS "QRect");
    d.putItem("numchild", "4");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("x", rc.x());
        d.putHash("y", rc.y());
        d.putHash("width", rc.width());
        d.putHash("height", rc.height());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQRectF(QDumper &d)
{
    const QRectF &rc = *reinterpret_cast<const QRectF *>(d.data);
    d.beginItem("value");
        d.put("(").put(rc.width()).put("x").put(rc.height());
        if (rc.x() >= 0)
            d.put("+");
        d.put(rc.x());
        if (rc.y() >= 0)
            d.put("+");
        d.put(rc.y());
        d.put(")");
    d.endItem();
    d.putItem("type", NS "QRectF");
    d.putItem("numchild", "4");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("x", rc.x());
        d.putHash("y", rc.y());
        d.putHash("width", rc.width());
        d.putHash("height", rc.height());
        d.endChildren();
    }
    d.disarm();
}
#endif

static void qDumpQSet(QDumper &d)
{
    // This uses the knowledge that QHash<T> has only a single member
    // of  union { QHashData *d; QHashNode<Key, T> *e; };
    QHashData *hd = *(QHashData**)d.data;
    QHashData::Node *node = hd->firstNode();

    int n = hd->size;
    if (n < 0)
        return;
    if (n > 0) {
        qCheckAccess(node);
        qCheckPointer(node->next);
    }

    d.putItemCount("value", n);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", 2 * n);
    if (d.dumpChildren) {
        d.beginChildren();
        int i = 0;
        for (int bucket = 0; bucket != hd->numBuckets && i <= 10000; ++bucket) {
            for (node = hd->buckets[bucket]; node->next; node = node->next) {
                d.beginHash();
                d.putItem("type", d.innerType);
                d.beginItem("exp");
                    d.put("(('" NS "QHashNode<").put(d.innerType
                   ).put("," NS "QHashDummyValue>'*)"
                   ).put(static_cast<const void*>(node)).put(")->key");
                d.endItem();
                d.endHash();
                ++i;
                if (i > 10000) {
                    d.putEllipsis();
                    break;
                }
            }
        }
        d.endChildren();
    }
    d.disarm();
}

#ifndef QT_BOOTSTRAPPED
#if QT_VERSION >= 0x040500
static void qDumpQSharedPointer(QDumper &d)
{
    const QSharedPointer<int> &ptr =
        *reinterpret_cast<const QSharedPointer<int> *>(d.data);

    if (ptr.isNull()) {
        d.putItem("value", "<null>");
        d.putItem("valueeditable", "false");
        d.putItem("numchild", 0);
        d.disarm();
        return;
    }

    if (isSimpleType(d.innerType))
        qDumpInnerValueHelper(d, d.innerType, ptr.data());
    else
        d.putItem("value", "");
    d.putItem("valueeditable", "false");
    d.putItem("numchild", 1);
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "data");
            qDumpInnerValue(d, d.innerType, ptr.data());
        d.endHash();
        const int v = sizeof(void *);
        d.beginHash();
            const void *weak = addOffset(deref(addOffset(d.data, v)), v);
            d.putItem("name", "weakref");
            d.putItem("value", *static_cast<const int *>(weak));
            d.putItem("type", "int");
            d.putItem("addr",  weak);
            d.putItem("numchild", "0");
        d.endHash();
        d.beginHash();
            const void *strong = addOffset(weak, sizeof(int));
            d.putItem("name", "strongref");
            d.putItem("value", *static_cast<const int *>(strong));
            d.putItem("type", "int");
            d.putItem("addr",  strong);
            d.putItem("numchild", "0");
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_VERSION >= 0x040500
#endif // QT_BOOTSTRAPPED

static void qDumpQSize(QDumper &d)
{
    const QSize s = *reinterpret_cast<const QSize *>(d.data);
    d.beginItem("value");
        d.put("(").put(s.width()).put("x").put(s.height()).put(")");
    d.endItem();
    d.putItem("type", NS "QSize");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("w", s.width());
        d.putHash("h", s.height());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQSizeF(QDumper &d)
{
    const QSizeF s = *reinterpret_cast<const QSizeF *>(d.data);
    d.beginItem("value");
        d.put("(").put(s.width()).put("x").put(s.height()).put(")");
    d.endItem();
    d.putItem("type", NS "QSizeF");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("w", s.width());
        d.putHash("h", s.height());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQString(QDumper &d)
{
    //qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    const QString &str = *reinterpret_cast<const QString *>(d.data);

    const int size = str.size();
    if (size < 0)
        return;
    if (size) {
        const QChar *unicode = str.unicode();
        qCheckAccess(unicode);
        qCheckAccess(unicode + size);
        if (!unicode[size].isNull()) // must be '\0' terminated
            return;
    }

    d.putStringValue(str);
    d.putItem("type", NS "QString");
    //d.putItem("editvalue", str);  // handled generically below
    d.putItem("numchild", "0");

    d.disarm();
}

static void qDumpQStringList(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid
    const QStringList &list = *reinterpret_cast<const QStringList *>(d.data);
    int n = list.size();
    if (n < 0)
        return;
    if (n > 0) {
        qCheckAccess(&list.front());
        qCheckAccess(&list.back());
    }

    d.putItemCount("value", n);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        if (n > 1000)
            n = 1000;
        d.beginChildren(n ? NS "QString" : 0);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            d.putStringValue(list.at(i));
            d.endHash();
        }
        if (n < list.size())
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQTextCodec(QDumper &d)
{
    qCheckPointer(deref(d.data));
    const QTextCodec &codec = *reinterpret_cast<const QTextCodec *>(d.data);
    d.putItem("value", codec.name());
    d.putItem("valueencoded", "1");
    d.putItem("type", NS "QTextCodec");
    d.putItem("numchild", "2");
    if (d.dumpChildren) {
        d.beginChildren();
        d.putHash("name", codec.name());
        d.putHash("mibEnum", codec.mibEnum());
        d.endChildren();
    }
    d.disarm();
}

static void qDumpQVector(QDumper &d)
{
    qCheckAccess(deref(d.data)); // is the d-ptr de-referenceable and valid

#if QT_VERSION >= 0x050000
    QArrayData *v = *reinterpret_cast<QArrayData *const*>(d.data);
    const unsigned typeddatasize = (char *)(&v->offset) - (char *)v;
#else
    QVectorData *v = *reinterpret_cast<QVectorData *const*>(d.data);
    QVectorTypedData<int> *dummy = 0;
    const unsigned typeddatasize = (char*)(&dummy->array) - (char*)dummy;
#endif

    // Try to provoke segfaults early to prevent the frontend
    // from asking for unavailable child details
    int nn = v->size;
    if (nn < 0)
        return;
    const bool innerIsPointerType = isPointerType(d.innerType);
    const unsigned innersize = d.extraInt[0];
    const int n = qMin(nn, 1000);
    // Check pointers
    if (innerIsPointerType && nn > 0)
        for (int i = 0; i != n; ++i)
            if (const void *p = addOffset(v, i * innersize + typeddatasize))
                qCheckPointer(deref(p));

    d.putItemCount("value", nn);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", nn);
    if (d.dumpChildren) {
        QByteArray strippedInnerType = stripPointerType(d.innerType);
        const char *stripped = innerIsPointerType ? strippedInnerType.data() : 0;
        d.beginChildren(d.innerType);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            qDumpInnerValueOrPointer(d, d.innerType, stripped,
                addOffset(v, i * innersize + typeddatasize));
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

#ifndef QT_BOOTSTRAPPED
#if QT_VERSION >= 0x040500
static void qDumpQWeakPointer(QDumper &d)
{
    const int v = sizeof(void *);
    const void *value = deref(addOffset(d.data, v));
    const void *data = deref(d.data);

    if (value == 0 || data == 0) {
        d.putItem("value", "<null>");
        d.putItem("valueeditable", "false");
        d.putItem("numchild", 0);
        d.disarm();
        return;
    }

    if (isSimpleType(d.innerType))
        qDumpInnerValueHelper(d, d.innerType, value);
    else
        d.putItem("value", "");
    d.putItem("valueeditable", "false");
    d.putItem("numchild", 1);
    if (d.dumpChildren) {
        d.beginChildren();
        d.beginHash();
            d.putItem("name", "data");
            qDumpInnerValue(d, d.innerType, value);
        d.endHash();
        d.beginHash();
            const void *weak = addOffset(deref(d.data), v);
            d.putItem("name", "weakref");
            d.putItem("value", *static_cast<const int *>(weak));
            d.putItem("type", "int");
            d.putItem("addr",  weak);
            d.putItem("numchild", "0");
        d.endHash();
        d.beginHash();
            const void *strong = addOffset(weak, sizeof(int));
            d.putItem("name", "strongref");
            d.putItem("value", *static_cast<const int *>(strong));
            d.putItem("type", "int");
            d.putItem("addr",  strong);
            d.putItem("numchild", "0");
        d.endHash();
        d.endChildren();
    }
    d.disarm();
}
#endif // QT_VERSION >= 0x040500
#endif // QT_BOOTSTRAPPED

#endif // QT_CORE

static void qDumpStdList(QDumper &d)
{
    const std::list<int> &list = *reinterpret_cast<const std::list<int> *>(d.data);
    const void *p = d.data;
    qCheckAccess(p);
    p = deref(p);
    qCheckAccess(p);
    p = deref(p);
    qCheckAccess(p);
    p = deref(addOffset(d.data, sizeof(void*)));
    qCheckAccess(p);
    p = deref(addOffset(p, sizeof(void*)));
    qCheckAccess(p);
    p = deref(addOffset(p, sizeof(void*)));
    qCheckAccess(p);
    std::list<int>::size_type nn = 0;
    const std::list<int>::size_type maxItems = 100;
    std::list<int>::const_iterator it = list.begin();
    const std::list<int>::const_iterator cend = list.end();
    for (; nn < maxItems && it != cend; ++nn, ++it)
        qCheckAccess(it.operator->());

    if (it != cend) {
        d.putTruncatedItemCount("value", nn);
    } else {
        d.putItemCount("value", nn);
    }
    d.putItem("numchild", nn);

    d.putItem("valueeditable", "false");
    if (d.dumpChildren) {
        QByteArray strippedInnerType = stripPointerType(d.innerType);
        const char *stripped =
            isPointerType(d.innerType) ? strippedInnerType.data() : 0;
        d.beginChildren(d.innerType);
        it = list.begin();
        for (std::list<int>::size_type i = 0; i < maxItems && it != cend; ++i, ++it) {
            d.beginHash();
            qDumpInnerValueOrPointer(d, d.innerType, stripped, it.operator->());
            d.endHash();
        }
        if (it != list.end())
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

/* Dump out an arbitrary map. To iterate the map,
 * it is cast to a map of <KeyType,Value>. 'int' can be used for both
 * for all types if the implementation does not depend on the types
 * which is the case for GNU STL. The implementation used by MS VC, however,
 * does depend on the key/value type, so, special cases need to be hardcoded. */

template <class KeyType, class ValueType>
static void qDumpStdMapHelper(QDumper &d)
{
    typedef std::map<KeyType, ValueType> DummyType;
    const DummyType &map = *reinterpret_cast<const DummyType*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];
    const void *p = d.data;
    qCheckAccess(p);

    const int nn = map.size();
    if (nn < 0)
        return;
    typename DummyType::const_iterator it = map.begin();
    const typename DummyType::const_iterator cend = map.end();
    for (int i = 0; i < nn && i < 10 && it != cend; ++i, ++it)
        qCheckAccess(it.operator->());

    d.putItem("numchild", nn);
    d.putItemCount("value", nn);
    d.putItem("valueeditable", "false");
    d.putItem("valueoffset", d.extraInt[2]);

    // HACK: we need a properly const qualified version of the
    // std::pair used. We extract it from the allocator parameter
    // (#4, "std::allocator<std::pair<key, value> >")
    // as it is there, and, equally importantly, in an order that
    // gdb accepts when fed with it.
    char *pairType = (char *)(d.templateParameters[3]) + 15;
    pairType[strlen(pairType) - 2] = 0;
    d.putItem("pairtype", pairType);

    if (d.dumpChildren) {
        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        int valueOffset = d.extraInt[2];

        d.beginItem("extra");
            d.put("isSimpleKey: ").put(isSimpleKey);
            d.put(" isSimpleValue: ").put(isSimpleValue);
            d.put(" valueType: '").put(valueType);
            d.put(" valueOffset: ").put(valueOffset);
        d.endItem();

        d.beginChildren(d.innerType);
        it = map.begin();
        for (int i = 0; i < 1000 && it != cend; ++i, ++it) {
            d.beginHash();
                const void *node = it.operator->();
                qDumpInnerValueHelper(d, keyType, node, "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    d.putItem("type", valueType);
                    d.putItem("addr", addOffset(node, valueOffset));
                    d.putItem("numchild", 0);
                } else {
                    d.putItem("addr", node);
                    d.putItem("type", pairType);
                    d.putItem("numchild", 2);
                }
            d.endHash();
        }
        if (it != map.end())
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpStdMap(QDumper &d)
{
    qDumpStdMapHelper<int,int>(d);
}

/* Dump out an arbitrary set. To iterate the set,
 * it is cast to a set of <KeyType>. 'int' can be used
 * for all types if the implementation does not depend on the key type
 * which is the case for GNU STL. The implementation used by MS VC, however,
 * does depend on the key type, so, special cases need to be hardcoded. */

template <class KeyType>
static void qDumpStdSetHelper(QDumper &d)
{
    typedef std::set<KeyType> DummyType;
    const DummyType &set = *reinterpret_cast<const DummyType*>(d.data);
    const void *p = d.data;
    qCheckAccess(p);

    const int nn = set.size();
    if (nn < 0)
        return;
    typename DummyType::const_iterator it = set.begin();
    const typename DummyType::const_iterator cend = set.end();
    for (int i = 0; i < nn && i < 10 && it != cend; ++i, ++it)
        qCheckAccess(it.operator->());

    d.putItemCount("value", nn);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", nn);
    d.putItem("valueoffset", d.extraInt[0]);

    if (d.dumpChildren) {
        int valueOffset = 0; // d.extraInt[0];
        QByteArray strippedInnerType = stripPointerType(d.innerType);
        const char *stripped =
            isPointerType(d.innerType) ? strippedInnerType.data() : 0;

        d.beginItem("extra");
            d.put("valueOffset: ").put(valueOffset);
        d.endItem();

        d.beginChildren(d.innerType);
        it = set.begin();
        for (int i = 0; i < 1000 && it != cend; ++i, ++it) {
            const void *node = it.operator->();
            d.beginHash();
            qDumpInnerValueOrPointer(d, d.innerType, stripped, node);
            d.endHash();
        }
        if (it != set.end())
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpStdSet(QDumper &d)
{
    qDumpStdSetHelper<int>(d);
}

static void qDumpStdString(QDumper &d)
{
    const std::string &str = *reinterpret_cast<const std::string *>(d.data);

    const std::string::size_type size = str.size();
    if (int(size) < 0)
        return;
    if (size) {
        qCheckAccess(str.c_str());
        qCheckAccess(str.c_str() + size - 1);
    }
    qDumpStdStringValue(d, str);
    d.disarm();
}

static void qDumpStdWString(QDumper &d)
{
    const std::wstring &str = *reinterpret_cast<const std::wstring *>(d.data);
    const std::wstring::size_type size = str.size();
    if (int(size) < 0)
        return;
    if (size) {
        qCheckAccess(str.c_str());
        qCheckAccess(str.c_str() + size - 1);
    }
    qDumpStdWStringValue(d, str);
    d.disarm();
}

static void qDumpStdVector(QDumper &d)
{
    // Correct type would be something like:
    // std::_Vector_base<int,std::allocator<int, std::allocator<int> >>::_Vector_impl
    struct VectorImpl {
        char *start;
        char *finish;
        char *end_of_storage;
    };
    const VectorImpl *v = static_cast<const VectorImpl *>(d.data);
    // Try to provoke segfaults early to prevent the frontend
    // from asking for unavailable child details
    int nn = (v->finish - v->start) / d.extraInt[0];
    if (nn < 0)
        return;
    if (nn > 0) {
        qCheckAccess(v->start);
        qCheckAccess(v->finish);
        qCheckAccess(v->end_of_storage);
    }

    int n = nn;
    d.putItemCount("value", n);
    d.putItem("valueeditable", "false");
    d.putItem("numchild", n);
    if (d.dumpChildren) {
        unsigned innersize = d.extraInt[0];
        QByteArray strippedInnerType = stripPointerType(d.innerType);
        const char *stripped =
            isPointerType(d.innerType) ? strippedInnerType.data() : 0;
        if (n > 1000)
            n = 1000;
        d.beginChildren(n ? d.innerType : 0);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            qDumpInnerValueOrPointer(d, d.innerType, stripped,
                addOffset(v->start, i * innersize));
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d.endChildren();
    }
    d.disarm();
}

static void qDumpStdVectorBool(QDumper &d)
{
    // FIXME
    return qDumpStdVector(d);
}

static void handleProtocolVersion2and3(QDumper &d)
{
    if (!d.outerType[0]) {
        qDumpUnknown(d);
        return;
    }

    d.setupTemplateParameters();
    d.putItem("iname", d.iname);
    if (d.data)
        d.putItem("addr", d.data);

#ifdef QT_NO_QDATASTREAM
    if (d.protocolVersion == 3) {
        QVariant::Type type = QVariant::nameToType(d.outerType);
        if (type != QVariant::Invalid) {
            QVariant v(type, d.data);
            QByteArray ba;
            QDataStream ds(&ba, QIODevice::WriteOnly);
            ds << v;
            d.putItem("editvalue", ba);
        }
    }
#endif

    const char *type = stripNamespace(d.outerType);
    // type[0] is usually 'Q', so don't use it
    switch (type[1]) {
        case 'a':
            if (isEqual(type, "map"))
                qDumpStdMap(d);
            break;
        case 'e':
            if (isEqual(type, "vector"))
                qDumpStdVector(d);
            else if (isEqual(type, "set"))
                qDumpStdSet(d);
            break;
        case 'i':
            if (isEqual(type, "list"))
                qDumpStdList(d);
            break;
        case 's':
            if (isEqual(type, "wstring"))
                qDumpStdWString(d);
            break;
        case 't':
            if (isEqual(type, "std::vector"))
                qDumpStdVector(d);
            else if (isEqual(type, "std::vector::bool"))
                qDumpStdVectorBool(d);
            else if (isEqual(type, "std::list"))
                qDumpStdList(d);
            else if (isEqual(type, "std::map"))
                qDumpStdMap(d);
            else if (isEqual(type, "std::set"))
                qDumpStdSet(d);
            else if (isEqual(type, "std::string") || isEqual(type, "string"))
                qDumpStdString(d);
            else if (isEqual(type, "std::wstring"))
                qDumpStdWString(d);
            break;
#if USE_QT_CORE
        case 'A':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QAbstractItemModel"))
                qDumpQAbstractItemModel(d);
            else if (isEqual(type, "QAbstractItem"))
                qDumpQAbstractItem(d);
#            endif
            break;
        case 'B':
            if (isEqual(type, "QByteArray"))
                qDumpQByteArray(d);
            break;
        case 'C':
            if (isEqual(type, "QChar"))
                qDumpQChar(d);
            break;
        case 'D':
            if (isEqual(type, "QDate"))
                qDumpQDate(d);
            else if (isEqual(type, "QDateTime"))
                qDumpQDateTime(d);
            else if (isEqual(type, "QDir"))
                qDumpQDir(d);
            break;
        case 'F':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QFile"))
                qDumpQFile(d);
            else if (isEqual(type, "QFileInfo"))
                qDumpQFileInfo(d);
#            endif
            break;
        case 'H':
            if (isEqual(type, "QHash"))
                qDumpQHash(d);
            else if (isEqual(type, "QHashNode"))
                qDumpQHashNode(d);
            break;
        case 'I':
#            if USE_QT_GUI
            if (isEqual(type, "QImage"))
                qDumpQImage(d);
            else if (isEqual(type, "QImageData"))
                qDumpQImageData(d);
#            endif
            break;
        case 'L':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QList"))
                qDumpQList(d);
            else if (isEqual(type, "QLinkedList"))
                qDumpQLinkedList(d);
            else if (isEqual(type, "QLocale"))
                qDumpQLocale(d);
#            endif
            break;
        case 'M':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QModelIndex"))
                qDumpQModelIndex(d);
#           if MAP_WORKS
            else if (isEqual(type, "QMap"))
                qDumpQMap(d);
            else if (isEqual(type, "QMapNode"))
                qDumpQMapNode(d);
            else if (isEqual(type, "QMultiMap"))
                qDumpQMultiMap(d);
#           endif
#            endif
            break;
        case 'O':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QObject"))
                qDumpQObject(d);
            else if (isEqual(type, "QObjectPropertyList"))
                qDumpQObjectPropertyList(d);
            else if (isEqual(type, "QObjectProperty"))
                qDumpQObjectProperty(d);
            else if (isEqual(type, "QObjectMethodList"))
                qDumpQObjectMethodList(d);
            else if (isEqual(type, "QObjectSignalList"))
                qDumpQObjectSignalList(d);
            else if (isEqual(type, "QObjectSignal"))
                qDumpQObjectSignal(d);
            else if (isEqual(type, "QObjectSlot"))
                qDumpQObjectSlot(d);
            else if (isEqual(type, "QObjectSlotList"))
                qDumpQObjectSlotList(d);
            else if (isEqual(type, "QObjectChildList"))
                qDumpQObjectChildList(d);
#            endif
            break;
        case 'P':
#            if USE_QT_GUI
            if (isEqual(type, "QPixmap"))
                qDumpQPixmap(d);
#            endif
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QPoint"))
                qDumpQPoint(d);
            else if (isEqual(type, "QPointF"))
                qDumpQPointF(d);
#            endif
            break;
        case 'R':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QRect"))
                qDumpQRect(d);
            else if (isEqual(type, "QRectF"))
                qDumpQRectF(d);
#            endif
            break;
        case 'S':
            if (isEqual(type, "QString"))
                qDumpQString(d);
            else if (isEqual(type, "QStringList"))
                qDumpQStringList(d);
#            ifndef QT_BOOTSTRAPPED
            else if (isEqual(type, "QSet"))
                qDumpQSet(d);
            else if (isEqual(type, "QStack"))
                qDumpQVector(d);
#            if QT_VERSION >= 0x040500
            else if (isEqual(type, "QSharedPointer"))
                qDumpQSharedPointer(d);
#            endif
#            endif // QT_BOOTSTRAPPED
            else if (isEqual(type, "QSize"))
                qDumpQSize(d);
            else if (isEqual(type, "QSizeF"))
                qDumpQSizeF(d);
            break;
        case 'T':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QTextCodec"))
                qDumpQTextCodec(d);
#            endif
            if (isEqual(type, "QTime"))
                qDumpQTime(d);
            break;
        case 'V':
#            ifndef QT_BOOTSTRAPPED
            if (isEqual(type, "QVariantList")) { // resolve typedef
                d.outerType = "QList";
                d.innerType = "QVariant";
                d.extraInt[0] = sizeof(QVariant);
                qDumpQList(d);
            } else if (isEqual(type, "QVariant")) {
                qDumpQVariant(d);
            } else if (isEqual(type, "QVector")) {
                qDumpQVector(d);
            }
#            endif
            break;
        case 'W':
#            ifndef QT_BOOTSTRAPPED
#            if QT_VERSION >= 0x040500
            if (isEqual(type, "QWeakPointer"))
                qDumpQWeakPointer(d);
#            endif
#            endif
            break;
#endif
    }

    if (!d.success)
        qDumpUnknown(d);
}

} // anonymous namespace


#if USE_QT_GUI
extern "C" Q_DECL_EXPORT
void *watchPoint(int x, int y)
{
    return QApplication::widgetAt(x, y);
}
#endif

extern "C" Q_DECL_EXPORT
void *qDumpObjectData440(
    int protocolVersion,
    int token,
    const void *data,
    int dumpChildren,
    int extraInt0,
    int extraInt1,
    int extraInt2,
    int extraInt3)
{
    //sleep(20);
    if (protocolVersion == 1) {
        QDumper d;
        d.protocolVersion = protocolVersion;
        d.token           = token;

        // This is a list of all available dumpers. Note that some templates
        // currently require special hardcoded handling in the debugger plugin.
        // They are mentioned here nevertheless. For types that are not listed
        // here, dumpers won't be used.
        d.put("dumpers=["
            "\"" NS "QAbstractItem\","
            "\"" NS "QAbstractItemModel\","
            "\"" NS "QByteArray\","
            "\"" NS "QChar\","
            "\"" NS "QDate\","
            "\"" NS "QDateTime\","
            "\"" NS "QDir\","
            "\"" NS "QFile\","
            "\"" NS "QFileInfo\","
            "\"" NS "QHash\","
            "\"" NS "QHashNode\","
            "\"" NS "QImage\","
            //"\"" NS "QImageData\","
            "\"" NS "QLinkedList\","
            "\"" NS "QList\","
            "\"" NS "QLocale\","
#if MAP_WORKS
            "\"" NS "QMap\","
            "\"" NS "QMapNode\","
#endif
            "\"" NS "QModelIndex\","
            "\"" NS "QObject\","
            "\"" NS "QObjectMethodList\","   // hack to get nested properties display
            "\"" NS "QObjectProperty\","
            "\"" NS "QObjectPropertyList\","
            "\"" NS "QObjectSignal\","
            "\"" NS "QObjectSignalList\","
            "\"" NS "QObjectSlot\","
            "\"" NS "QObjectSlotList\","
            "\"" NS "QObjectChildList\","
            "\"" NS "QPoint\","
            "\"" NS "QPointF\","
            "\"" NS "QRect\","
            "\"" NS "QRectF\","
            //"\"" NS "QRegion\","
            "\"" NS "QSet\","
            "\"" NS "QSize\","
            "\"" NS "QSizeF\","
            "\"" NS "QStack\","
            "\"" NS "QString\","
            "\"" NS "QStringList\","
            "\"" NS "QTextCodec\","
            "\"" NS "QTime\","
            "\"" NS "QVariant\","
            "\"" NS "QVariantList\","
            "\"" NS "QVector\","
#if QT_VERSION >= 0x040500
#if MAP_WORKS
            "\"" NS "QMultiMap\","
#endif
            "\"" NS "QSharedPointer\","
            "\"" NS "QWeakPointer\","
#endif
#if USE_QT_GUI
            "\"" NS "QPixmap\","
            "\"" NS "QWidget\","
#endif
#ifdef Q_OS_WIN
            "\"basic_string\","
            "\"list\","
            "\"map\","
            "\"set\","
            "\"vector\","
#endif
            "\"string\","
            "\"wstring\","
            "\"std::basic_string\","
            "\"std::list\","
            "\"std::map\","
            "\"std::set\","
            "\"std::string\","
            "\"std::vector\","
            "\"std::wstring\","
            "]");
        d.put(",qtversion=["
            "\"").put(((QT_VERSION >> 16) & 255)).put("\","
            "\"").put(((QT_VERSION >> 8)  & 255)).put("\","
            "\"").put(((QT_VERSION)       & 255)).put("\"]");
        d.put(",namespace=\"" NS "\",");
        d.put("dumperversion=\"1.3\",");
        d.disarm();
    }

    else if (protocolVersion == 2 || protocolVersion == 3) {
        QDumper d;

        d.protocolVersion = protocolVersion;
        d.token           = token;
        d.data            = data;
        d.dumpChildren    = dumpChildren;
        d.extraInt[0]     = extraInt0;
        d.extraInt[1]     = extraInt1;
        d.extraInt[2]     = extraInt2;
        d.extraInt[3]     = extraInt3;

        const char *inbuffer = inBuffer;
        d.outerType = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.iname     = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.exp       = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.innerType = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.iname     = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
#if 0
        qDebug() << "data=" << d.data << "dumpChildren=" << d.dumpChildren
                << " extra=" << d.extraInt[0] << d.extraInt[1]  << d.extraInt[2]  << d.extraInt[3]
                << d.outerType << d.iname << d.exp << d.iname;
#endif
        handleProtocolVersion2and3(d);
    }

    else {
#if USE_QT_CORE
#        ifndef QT_BOOTSTRAPPED
        qDebug() << "Unsupported protocol version" << protocolVersion;
#        endif
#endif
    }
    return outBuffer;
}
