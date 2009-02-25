/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include <qglobal.h>

// this relies on contents copied from qobject_p.h
#define PRIVATE_OBJECT_ALLOWED 1

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QLinkedList>
#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QModelIndex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <QtCore/QTextCodec>
#include <QtCore/QVector>

int qtGhVersion = QT_VERSION;

#ifdef QT_GUI_LIB
#   include <QtGui/QPixmap>
#   include <QtGui/QImage>
#endif

#include <list>
#include <map>
#include <string>
#include <vector>

#include <ctype.h>
#include <stdio.h>
#include <unistd.h>

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
  \c{handleProtocolVersion2and3()} and needs an entry in the bis switch there.

  Next step is to create a suitable \c{static void qDumpFoo(QDumper &d)}
  function. At the bare minimum it should contain something like:


  \c{
    const Foo &foo = *reinterpret_cast<const Foo *>(d.data);

    P(d, "value", ...);
    P(d, "type", "Foo");
    P(d, "numchild", "0");
  }


  'P(d, name, value)' roughly expands to:
        d << (name) << "=\"" << value << "\"";

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
  about thes children. In this case the dumper should use something like

  \c{
    if (d.dumpChildren) {
        d << ",children=[";
   }

  */

#undef NS
#ifdef QT_NAMESPACE
#   define STRINGIFY0(s) #s
#   define STRINGIFY1(s) STRINGIFY0(s)
#   define NS STRINGIFY1(QT_NAMESPACE) "::"
#   define NSX "'" STRINGIFY1(QT_NAMESPACE) "::"
#   define NSY "'"
#else
#   define NS ""
#   define NSX ""
#   define NSY ""
#endif


#if PRIVATE_OBJECT_ALLOWED

#if defined(QT_BEGIN_NAMESPACE)
QT_BEGIN_NAMESPACE
#endif

class QVariant;
class QThreadData;
class QObjectConnectionListVector;

class QObjectPrivate : public QObjectData
{
    Q_DECLARE_PUBLIC(QObject)

public:
    QObjectPrivate() {}
    virtual ~QObjectPrivate() {}

    // preserve binary compatibility with code compiled without Qt 3 support
    QList<QObject *> pendingChildInsertedEvents; // unused

    // id of the thread that owns the object
    QThreadData *threadData;

    struct Sender
    {
        QObject *sender;
        int signal;
        int ref;
    };

    Sender *currentSender; // object currently activating the object
    QObject *currentChildBeingDeleted;

    QList<QPointer<QObject> > eventFilters;

    struct ExtraData;
    ExtraData *extraData;
    mutable quint32 connectedSignals;

    QString objectName;

    struct Connection
    {
        QObject *receiver;
        int method;
        uint connectionType : 3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
        QBasicAtomicPointer<int> argumentTypes;
    };
    typedef QList<Connection> ConnectionList;

    QObjectConnectionListVector *connectionLists;
    QList<Sender> senders;
    int *deleteWatch;
};

#if defined(QT_BEGIN_NAMESPACE)
QT_END_NAMESPACE
#endif

#endif // PRIVATE_OBJECT_ALLOWED


// this can be mangled typenames of nested templates, each char-by-char
// comma-separated integer list
static char qDumpInBuffer[10000];
static char qDumpOutBuffer[100000];
//static char qDumpSize[20];

namespace {

static bool isPointerType(const QByteArray &type)
{
    return type.endsWith("*") || type.endsWith("* const");
}

static QByteArray stripPointerType(QByteArray type)
{
    if (type.endsWith("*"))
        type.chop(1);
    if (type.endsWith("* const"))
        type.chop(7);
    if (type.endsWith(' '))
        type.chop(1);
    return type;
}

// This is used to abort evaluation of custom data dumpers in a "coordinated"
// way. Abortion will happen anyway when we try to access a non-initialized
// non-trivial object, so there is no way to prevent this from occuring at all
// conceptionally.  Gdb will catch SIGSEGV and return to the calling frame.
// This is just fine provided we only _read_ memory in the custom handlers
// below.

volatile int qProvokeSegFaultHelper;

static const void *addOffset(const void *p, int offset)
{
    return offset + reinterpret_cast<const char *>(p);
}

static const void *skipvtable(const void *p)
{
    return sizeof(void*) + reinterpret_cast<const char *>(p);
}

static const void *deref(const void *p)
{
    return *reinterpret_cast<const char* const*>(p);
}

static const void *dfunc(const void *p)
{
    return deref(skipvtable(p));
}

static bool isEqual(const char *s, const char *t)
{
    return qstrcmp(s, t) == 0;
}

static bool startsWith(const char *s, const char *t)
{
    return qstrncmp(s, t, strlen(t)) == 0;
}

// provoke segfault when address is not readable
#define qCheckAccess(d) do { qProvokeSegFaultHelper = *(char*)d; } while (0)
#define qCheckPointer(d) do { if (d) qProvokeSegFaultHelper = *(char*)d; } while (0)
// provoke segfault unconditionally
#define qCheck(b) do { if (!(b)) qProvokeSegFaultHelper = *(char*)0; } while (0)

const char *stripNamespace(const char *type)
{
    static const size_t nslen = strlen(NS);
    return startsWith(type, NS) ? type + nslen : type;
}

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
            return isEqual(type, "short") || isEqual(type, "signed")
                || startsWith(type, "signed ");
        case 'u':
            return isEqual(type, "unsigned") || startsWith(type, "unsigned ");
    }
    return false;
}

#if 0
static bool isStringType(const char *type)
{
    return isEqual(type, NS"QString")
        || isEqual(type, NS"QByteArray")
        || isEqual(type, "std::string")
        || isEqual(type, "std::wstring")
        || isEqual(type, "wstring");
}
#endif

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
            return isEqual(type, "QCustomTypeInfo");
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

struct QDumper
{
    explicit QDumper();
    ~QDumper();
    void checkFill();
    QDumper &operator<<(long c);
    QDumper &operator<<(int i);
    QDumper &operator<<(double d);
    QDumper &operator<<(float d);
    QDumper &operator<<(unsigned long c);
    QDumper &operator<<(unsigned int i);
    QDumper &operator<<(const void *p);
    QDumper &operator<<(qulonglong c);
    QDumper &operator<<(const char *str);
    QDumper &operator<<(const QByteArray &ba);
    QDumper &operator<<(const QString &str);
    void put(char c);
    void addCommaIfNeeded();
    void putBase64Encoded(const char *buf, int n);
    void putEllipsis();
    void disarm();

    void beginHash(); // start of data hash output
    void endHash(); // start of data hash output

    // the dumper arguments
    int protocolVersion;   // dumper protocol version
    int token;             // some token to show on success
    const char *outertype; // object type
    const char *iname;     // object name used for display
    const char *exp;       // object expression
    const char *innertype; // 'inner type' for class templates
    const void *data;      // pointer to raw data
    bool dumpChildren;     // do we want to see children?

    // handling of nested templates
    void setupTemplateParameters();
    enum { maxTemplateParameters = 10 };
    const char *templateParameters[maxTemplateParameters + 1];
    int templateParametersCount;

    // internal state
    bool success;          // are we finished?
    bool full;
    int pos;

    int extraInt[4];
};


QDumper::QDumper()
{
    success = false;
    full = false;
    qDumpOutBuffer[0] = 'f'; // marks output as 'wrong' 
    pos = 1;
}

QDumper::~QDumper()
{
    qDumpOutBuffer[pos++] = '\0';
    if (success)
        qDumpOutBuffer[0] = (full ? '+' : 't');
}

void QDumper::setupTemplateParameters()
{
    char *s = const_cast<char *>(innertype);

    templateParametersCount = 1;
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
}

QDumper &QDumper::operator<<(unsigned long long c)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%llu", c);
    return *this;
}

QDumper &QDumper::operator<<(unsigned long c)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%lu", c);
    return *this;
}

QDumper &QDumper::operator<<(float d)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%f", d);
    return *this;
}

QDumper &QDumper::operator<<(double d)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%f", d);
    return *this;
}

QDumper &QDumper::operator<<(unsigned int i)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%u", i);
    return *this;
}

QDumper &QDumper::operator<<(long c)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%ld", c);
    return *this;
}

QDumper &QDumper::operator<<(int i)
{
    checkFill();
    pos += sprintf(qDumpOutBuffer + pos, "%d", i);
    return *this;
}

QDumper &QDumper::operator<<(const void *p)
{
    static char buf[100];
    if (p) {
        sprintf(buf, "%p", p);
        // we get a '0x' prefix only on some implementations.
        // if it isn't there, write it out manually.
        if (buf[1] != 'x') {
            put('0');
            put('x');
        }
        *this << buf;
    } else {
        *this << "<null>";
    }
    return *this;
}

void QDumper::checkFill()
{
    if (pos >= int(sizeof(qDumpOutBuffer)) - 100)
        full = true;
}

void QDumper::put(char c)
{
    checkFill();
    if (!full)
        qDumpOutBuffer[pos++] = c;
}

void QDumper::addCommaIfNeeded()
{
    if (pos == 0)
        return;
    char c = qDumpOutBuffer[pos - 1];
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

QDumper &QDumper::operator<<(const char *str)
{
    if (!str)
        return *this << "<null>";
    while (*str)
        put(*(str++));
    return *this;
}

QDumper &QDumper::operator<<(const QByteArray &ba)
{
    putBase64Encoded(ba.constData(), ba.size());
    return *this;
}

QDumper &QDumper::operator<<(const QString &str)
{
    QByteArray ba = str.toUtf8();
    putBase64Encoded(ba.constData(), ba.size());
    return *this;
}

void QDumper::disarm()
{
    success = true;
}

void QDumper::beginHash()
{
    addCommaIfNeeded();
    put('{');
}

void QDumper::endHash()
{
    put('}');
}

void QDumper::putEllipsis()
{
    addCommaIfNeeded();
    *this << "{name=\"<incomplete>\",value=\"\",type=\"" << innertype << "\"}";
}

//
// Some helpers to keep the dumper code short
//

// dump property=value pair
#undef P
#define P(dumper,name,value) \
    do { \
        dumper.addCommaIfNeeded(); \
        dumper << (name) << "=\"" << value << "\""; \
    } while (0)

// simple string property
#undef S
#define S(dumper, name, value) \
    dumper.beginHash(); \
    P(dumper, "name", name); \
    P(dumper, "value", value); \
    P(dumper, "type", NS"QString"); \
    P(dumper, "numchild", "0"); \
    P(dumper, "valueencoded", "1"); \
    dumper.endHash();

// simple integer property
#undef I
#define I(dumper, name, value) \
    dumper.beginHash(); \
    P(dumper, "name", name); \
    P(dumper, "value", value); \
    P(dumper, "type", "int"); \
    P(dumper, "numchild", "0"); \
    dumper.endHash();

// simple boolean property
#undef BL
#define BL(dumper, name, value) \
    dumper.beginHash(); \
    P(dumper, "name", name); \
    P(dumper, "value", (value ? "true" : "false")); \
    P(dumper, "type", "bool"); \
    P(dumper, "numchild", "0"); \
    dumper.endHash();


// a single QChar
#undef QC
#define QC(dumper, name, value) \
    dumper.beginHash(); \
    P(dumper, "name", name); \
    P(dumper, "value", QString(QLatin1String("'%1' (%2, 0x%3)")) \
        .arg(value).arg(value.unicode()).arg(value.unicode(), 0, 16)); \
    P(dumper, "valueencoded", "1"); \
    P(dumper, "type", NS"QChar"); \
    P(dumper, "numchild", "0"); \
    dumper.endHash();

#undef TT
#define TT(type, value) \
    "<tr><td>" << type << "</td><td> : </td><td>" << value << "</td></tr>"

static void qDumpUnknown(QDumper &d)
{
    P(d, "iname", d.iname);
    P(d, "addr", d.data);
    P(d, "value", "<internal error>");
    P(d, "type", d.outertype);
    P(d, "numchild", "0");
    d.disarm();
}

static void qDumpInnerValueHelper(QDumper &d, const char *type, const void *addr,
    const char *field = "value")
{
    type = stripNamespace(type);
    switch (type[1]) {
        case 'l':
            if (isEqual(type, "float"))
                P(d, field, *(float*)addr);
            return;
        case 'n':
            if (isEqual(type, "int"))
                P(d, field, *(int*)addr);
            else if (isEqual(type, "unsigned"))
                P(d, field, *(unsigned int*)addr);
            else if (isEqual(type, "unsigned int"))
                P(d, field, *(unsigned int*)addr);
            else if (isEqual(type, "unsigned long"))
                P(d, field, *(unsigned long*)addr);
            else if (isEqual(type, "unsigned long long"))
                P(d, field, *(qulonglong*)addr);
            return;
        case 'o':
            if (isEqual(type, "bool"))
                switch (*(bool*)addr) {
                    case 0: P(d, field, "false"); break;
                    case 1: P(d, field, "true"); break;
                    default: P(d, field, *(bool*)addr); break;
                }
            else if (isEqual(type, "double"))
                P(d, field, *(double*)addr);
            else if (isEqual(type, "long"))
                P(d, field, *(long*)addr);
            else if (isEqual(type, "long long"))
                P(d, field, *(qulonglong*)addr);
            return;
        case 'B':
            if (isEqual(type, "QByteArray")) {
                d.addCommaIfNeeded();
                d << field << "encoded=\"1\",";
                P(d, field, *(QByteArray*)addr);
            }
            return;
        case 'L':
            if (startsWith(type, "QList<")) {
                const QListData *ldata = reinterpret_cast<const QListData*>(addr);
                P(d, "value", "<" << ldata->size() << " items>");
                P(d, "valuedisabled", "true");
                P(d, "numchild", ldata->size());
            }
            return;
        case 'O':
            if (isEqual(type, "QObject *")) {
                if (addr) {
                    const QObject *ob = reinterpret_cast<const QObject *>(addr);
                    P(d, "addr", ob);
                    P(d, "value", ob->objectName());
                    P(d, "valueencoded", "1");
                    P(d, "type", NS"QObject");
                    P(d, "displayedtype", ob->metaObject()->className());
                } else {
                    P(d, "value", "0x0");
                    P(d, "type", NS"QObject *");
                }
            }
            return;
        case 'S':
            if (isEqual(type, "QString")) {
                d.addCommaIfNeeded();
                d << field << "encoded=\"1\",";
                P(d, field, *(QString*)addr);
            }
            return;
        default:
            return;
    }
}

static void qDumpInnerValue(QDumper &d, const char *type, const void *addr)
{
    P(d, "addr", addr);
    P(d, "type", type);

    if (!type[0])
        return;

    qDumpInnerValueHelper(d, type, addr);
}


static void qDumpInnerValueOrPointer(QDumper &d,
    const char *type, const char *strippedtype, const void *addr)
{
    if (strippedtype) {
        if (deref(addr)) {
            P(d, "addr", deref(addr));
            P(d, "type", strippedtype);
            qDumpInnerValueHelper(d, strippedtype, deref(addr));
        } else {
            P(d, "addr", addr);
            P(d, "type", strippedtype);
            P(d, "value", "<null>");
            P(d, "numchild", "0");
        }
    } else {
        P(d, "addr", addr);
        P(d, "type", type);
        qDumpInnerValueHelper(d, type, addr);
    }
}

//////////////////////////////////////////////////////////////////////////////

static void qDumpQByteArray(QDumper &d)
{
    const QByteArray &ba = *reinterpret_cast<const QByteArray *>(d.data);

    if (!ba.isEmpty()) {
        qCheckAccess(ba.constData());
        qCheckAccess(ba.constData() + ba.size());
    }

    if (ba.size() <= 100)
        P(d, "value", ba);
    else
        P(d, "value", ba.left(100) << " <size: " << ba.size() << ", cut...>");
    P(d, "valueencoded", "1");
    P(d, "type", NS"QByteArray");
    P(d, "numchild", ba.size());
    P(d, "childtype", "char");
    P(d, "childnumchild", "0");
    if (d.dumpChildren) {
        d << ",children=[";
        char buf[20];
        for (int i = 0; i != ba.size(); ++i) {
            unsigned char c = ba.at(i);
            unsigned char u = (isprint(c) && c != '\'' && c != '"') ? c : '?';
            sprintf(buf, "%02x  (%u '%c')", c, c, u);
            d.beginHash();
            P(d, "name", i);
            P(d, "value", buf);
            d.endHash();
        }
        d << "]";
    }
    d.disarm();
}

static void qDumpQDateTime(QDumper &d)
{
#ifdef QT_NO_DATESTRING
    qDumpUnknown(d);
#else
    const QDateTime &date = *reinterpret_cast<const QDateTime *>(d.data);
    if (date.isNull()) {
        P(d, "value", "(null)");
    } else {
        P(d, "value", date.toString());
        P(d, "valueencoded", "1");
    }
    P(d, "type", NS"QDateTime");
    P(d, "numchild", "3");
    if (d.dumpChildren) {
        d << ",children=[";
        BL(d, "isNull", date.isNull());
        I(d, "toTime_t", (long)date.toTime_t());
        S(d, "toString", date.toString());
        S(d, "toString_(ISO)", date.toString(Qt::ISODate));
        S(d, "toString_(SystemLocale)", date.toString(Qt::SystemLocaleDate));
        S(d, "toString_(Locale)", date.toString(Qt::LocaleDate));
        S(d, "toString", date.toString());

        #if 0
        d.beginHash();
        P(d, "name", "toUTC");
        P(d, "exp", "(("NSX"QDateTime"NSY"*)" << d.data << ")"
                    "->toTimeSpec('"NS"Qt::UTC')");
        P(d, "type", NS"QDateTime");
        P(d, "numchild", "1");
        d.endHash();
        #endif

        #if 0
        d.beginHash();
        P(d, "name", "toLocalTime");
        P(d, "exp", "(("NSX"QDateTime"NSY"*)" << d.data << ")"
                    "->toTimeSpec('"NS"Qt::LocalTime')");
        P(d, "type", NS"QDateTime");
        P(d, "numchild", "1");
        d.endHash();
        #endif

        d << "]";
    }
    d.disarm();
#endif // ifdef QT_NO_DATESTRING
}

static void qDumpQDir(QDumper &d)
{
    const QDir &dir = *reinterpret_cast<const QDir *>(d.data);
    P(d, "value", dir.path());
    P(d, "valueencoded", "1");
    P(d, "type", NS"QDir");
    P(d, "numchild", "3");
    if (d.dumpChildren) {
        d << ",children=[";
        S(d, "absolutePath", dir.absolutePath());
        S(d, "canonicalPath", dir.canonicalPath());
        d << "]";
    }
    d.disarm();
}

static void qDumpQFile(QDumper &d)
{
    const QFile &file = *reinterpret_cast<const QFile *>(d.data);
    P(d, "value", file.fileName());
    P(d, "valueencoded", "1");
    P(d, "type", NS"QFile");
    P(d, "numchild", "2");
    if (d.dumpChildren) {
        d << ",children=[";
        S(d, "fileName", file.fileName());
        BL(d, "exists", file.exists());
        d << "]";
    }
    d.disarm();
}

static void qDumpQFileInfo(QDumper &d)
{
    const QFileInfo &info = *reinterpret_cast<const QFileInfo *>(d.data);
    P(d, "value", info.filePath());
    P(d, "valueencoded", "1");
    P(d, "type", NS"QFileInfo");
    P(d, "numchild", "3");
    if (d.dumpChildren) {
        d << ",children=[";
        S(d, "absolutePath", info.absolutePath());
        S(d, "absoluteFilePath", info.absoluteFilePath());
        S(d, "canonicalPath", info.canonicalPath());
        S(d, "canonicalFilePath", info.canonicalFilePath());
        S(d, "completeBaseName", info.completeBaseName());
        S(d, "completeSuffix", info.completeSuffix());
        S(d, "baseName", info.baseName());
#ifdef Q_OS_MACX
        BL(d, "isBundle", info.isBundle());
        S(d, "bundleName", info.bundleName());
#endif
        S(d, "completeSuffix", info.completeSuffix());
        S(d, "fileName", info.fileName());
        S(d, "filePath", info.filePath());
        S(d, "group", info.group());
        S(d, "owner", info.owner());
        S(d, "path", info.path());

        I(d, "groupid", (long)info.groupId());
        I(d, "ownerid", (long)info.ownerId());
        //QFile::Permissions permissions () const
        I(d, "permissions", info.permissions());

        //QDir absoluteDir () const
        //QDir dir () const

        BL(d, "caching", info.caching());
        BL(d, "exists", info.exists());
        BL(d, "isAbsolute", info.isAbsolute());
        BL(d, "isDir", info.isDir());
        BL(d, "isExecutable", info.isExecutable());
        BL(d, "isFile", info.isFile());
        BL(d, "isHidden", info.isHidden());
        BL(d, "isReadable", info.isReadable());
        BL(d, "isRelative", info.isRelative());
        BL(d, "isRoot", info.isRoot());
        BL(d, "isSymLink", info.isSymLink());
        BL(d, "isWritable", info.isWritable());

        d.beginHash();
        P(d, "name", "created");
        P(d, "value", info.created().toString());
        P(d, "valueencoded", "1");
        P(d, "exp", "(("NSX"QFileInfo"NSY"*)" << d.data << ")->created()");
        P(d, "type", NS"QDateTime");
        P(d, "numchild", "1");
        d.endHash();

        d.beginHash();
        P(d, "name", "lastModified");
        P(d, "value", info.lastModified().toString());
        P(d, "valueencoded", "1");
        P(d, "exp", "(("NSX"QFileInfo"NSY"*)" << d.data << ")->lastModified()");
        P(d, "type", NS"QDateTime");
        P(d, "numchild", "1");
        d.endHash();

        d.beginHash();
        P(d, "name", "lastRead");
        P(d, "value", info.lastRead().toString());
        P(d, "valueencoded", "1");
        P(d, "exp", "(("NSX"QFileInfo"NSY"*)" << d.data << ")->lastRead()");
        P(d, "type", NS"QDateTime");
        P(d, "numchild", "1");
        d.endHash();

        d << "]";
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
    QHashData *h = *reinterpret_cast<QHashData *const*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    qCheckPointer(h->fakeNext);
    qCheckPointer(h->buckets);

    unsigned keySize = d.extraInt[0];
    unsigned valueSize = d.extraInt[1];

    int n = h->size;

    if (n < 0)
        qCheck(false);
    if (n > 0) {
        qCheckPointer(h->fakeNext);
        qCheckPointer(*h->buckets);
    }

    P(d, "value", "<" << n << " items>");
    P(d, "numchild", n);
    if (d.dumpChildren) {
        if (n > 1000)
            n = 1000;
        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        bool opt = isOptimizedIntKey(keyType);
        int keyOffset = hashOffset(opt, true, keySize, valueSize);
        int valueOffset = hashOffset(opt, false, keySize, valueSize);

        P(d, "extra", "isSimpleKey: " << isSimpleKey
            << " isSimpleValue: " << isSimpleValue
            << " valueType: '" << isSimpleValue
            << " keySize: " << keyOffset << " valueOffset: " << valueOffset
            << " opt: " << opt);

        QHashData::Node *node = h->firstNode();
        QHashData::Node *end = reinterpret_cast<QHashData::Node *>(h);
        int i = 0;

        d << ",children=[";
        while (node != end) {
            d.beginHash();
                P(d, "name", i);
                qDumpInnerValueHelper(d, keyType, addOffset(node, keyOffset), "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    P(d, "type", valueType);
                    P(d, "addr", addOffset(node, valueOffset));
                } else {
                    P(d, "exp", "*('"NS"QHashNode<" << keyType << ","
                        << valueType << " >'*)" << node);
                    P(d, "type", "'"NS"QHashNode<" << keyType << ","
                        << valueType << " >'");
                }
            d.endHash();
            ++i;
            node = QHashData::nextNode(node);
        }
        d << "]";
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
        P(d, "value", "");

    P(d, "numchild", 2);
    if (d.dumpChildren) {
        // there is a hash specialization in cast the key are integers or shorts
        d << ",children=[";
        d.beginHash();
            P(d, "name", "key");
            P(d, "type", keyType);
            P(d, "addr", addOffset(h, keyOffset));
        d.endHash();
        d.beginHash();
            P(d, "name", "value");
            P(d, "type", valueType);
            P(d, "addr", addOffset(h, valueOffset));
        d.endHash();
        d << "]";
    }
    d.disarm();
}

static void qDumpQImage(QDumper &d)
{
#ifdef QT_GUI_LIB
    const QImage &im = *reinterpret_cast<const QImage *>(d.data);
    P(d, "value", "(" << im.width() << "x" << im.height() << ")");
    P(d, "type", NS"QImage");
    P(d, "numchild", "0");
    d.disarm();
#else
    Q_UNUSED(d);
#endif
}

static void qDumpQList(QDumper &d)
{
    // This uses the knowledge that QList<T> has only a single member
    // of type  union { QListData p; QListData::Data *d; };
    const QListData &ldata = *reinterpret_cast<const QListData*>(d.data);
    const QListData::Data *pdata =
        *reinterpret_cast<const QListData::Data* const*>(d.data);
    int nn = ldata.size();
    if (nn < 0)
        qCheck(false);
    if (nn > 0) {
        qCheckAccess(ldata.d->array);
        //qCheckAccess(ldata.d->array[0]);
        //qCheckAccess(ldata.d->array[nn - 1]);
#if QT_VERSION >= 0x040400
        if (ldata.d->ref._q_value <= 0)
            qCheck(false);
#endif
    }

    int n = nn;
    P(d, "value", "<" << n << " items>");
    P(d, "valuedisabled", "true");
    P(d, "numchild", n);
    P(d, "childtype", d.innertype);
    if (d.dumpChildren) {
        unsigned innerSize = d.extraInt[0];
        bool innerTypeIsPointer = isPointerType(d.innertype);
        QByteArray strippedInnerType = stripPointerType(d.innertype);

        // The exact condition here is:
        //  QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic
        // but this data is available neither in the compiled binary nor
        // in the frontend.
        // So as first approximation only do the 'isLarge' check:
        bool isInternal = innerSize <= int(sizeof(void*))
            && isMovableType(d.innertype);

        P(d, "internal", (int)isInternal);
        P(d, "childtype", d.innertype);
        if (n > 1000)
            n = 1000;
        d << ",children=[";
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            P(d, "name", i);
            if (innerTypeIsPointer) {
                void *p = ldata.d->array + i + pdata->begin;
                if (p) {
                    //P(d, "value","@" << p);
                    qDumpInnerValue(d, strippedInnerType.data(), deref(p));
                } else {
                    P(d, "value", "<null>");
                    P(d, "numchild", "0");
                }
            } else {
                void *p = ldata.d->array + i + pdata->begin;
                if (isInternal) {
                    //qDumpInnerValue(d, d.innertype, p);
                    P(d, "addr", p);
                    qDumpInnerValueHelper(d, d.innertype, p);
                } else {
                    //qDumpInnerValue(d, d.innertype, deref(p));
                    P(d, "addr", deref(p));
                    qDumpInnerValueHelper(d, d.innertype, deref(p));
                }
            }
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

static void qDumpQLinkedList(QDumper &d)
{
    // This uses the knowledge that QLinkedList<T> has only a single member
    // of type  union { QLinkedListData *d; QLinkedListNode<T> *e; };
    const QLinkedListData *ldata =
        reinterpret_cast<const QLinkedListData*>(deref(d.data));
    int nn = ldata->size;
    if (nn < 0)
        qCheck(false);

    int n = nn;
    P(d, "value", "<" << n << " items>");
    P(d, "valuedisabled", "true");
    P(d, "numchild", n);
    P(d, "childtype", d.innertype);
    if (d.dumpChildren) {
        //unsigned innerSize = d.extraInt[0];
        //bool innerTypeIsPointer = isPointerType(d.innertype);
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;

        P(d, "childtype", d.innertype);
        if (n > 1000)
            n = 1000;
        d << ",children=[";
        const void *p = deref(ldata);
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            P(d, "name", i);
            const void *addr = addOffset(p, 2 * sizeof(void*));
            qDumpInnerValueOrPointer(d, d.innertype, stripped, addr);
            p = deref(p);
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

static void qDumpQLocale(QDumper &d)
{
    const QLocale &locale = *reinterpret_cast<const QLocale *>(d.data);
    P(d, "value", locale.name());
    P(d, "valueencoded", "1");
    P(d, "type", NS"QLocale");
    P(d, "numchild", "8");
    if (d.dumpChildren) {
        d << ",children=[";

        d.beginHash();
        P(d, "name", "country");
        P(d, "exp",  "(("NSX"QLocale"NSY"*)" << d.data << ")->country()");
        d.endHash();

        d.beginHash();
        P(d, "name", "language");
        P(d, "exp",  "(("NSX"QLocale"NSY"*)" << d.data << ")->language()");
        d.endHash();

        d.beginHash();
        P(d, "name", "measurementSystem");
        P(d, "exp",  "(("NSX"QLocale"NSY"*)" << d.data << ")->measurementSystem()");
        d.endHash();

        d.beginHash();
        P(d, "name", "numberOptions");
        P(d, "exp",  "(("NSX"QLocale"NSY"*)" << d.data << ")->numberOptions()");
        d.endHash();

        S(d, "timeFormat_(short)", locale.timeFormat(QLocale::ShortFormat));
        S(d, "timeFormat_(long)", locale.timeFormat(QLocale::LongFormat));

        QC(d, "decimalPoint", locale.decimalPoint());
        QC(d, "exponential", locale.exponential());
        QC(d, "percent", locale.percent());
        QC(d, "zeroDigit", locale.zeroDigit());
        QC(d, "groupSeparator", locale.groupSeparator());
        QC(d, "negativeSign", locale.negativeSign());

        d << "]";
    }
    d.disarm();
}

static void qDumpQMapNode(QDumper &d)
{
    const QMapData *h = reinterpret_cast<const QMapData *>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    qCheckAccess(h->backward);
    qCheckAccess(h->forward[0]);

    P(d, "value", "");
    P(d, "numchild", 2);
    if (d.dumpChildren) {
        //unsigned keySize = d.extraInt[0];
        //unsigned valueSize = d.extraInt[1];
        unsigned mapnodesize = d.extraInt[2];
        unsigned valueOff = d.extraInt[3];

        unsigned keyOffset = 2 * sizeof(void*) - mapnodesize;
        unsigned valueOffset = 2 * sizeof(void*) - mapnodesize + valueOff;

        d << ",children=[";
        d.beginHash();
        P(d, "name", "key");
        qDumpInnerValue(d, keyType, addOffset(h, keyOffset));

        d.endHash();
        d.beginHash();
        P(d, "name", "value");
        qDumpInnerValue(d, valueType, addOffset(h, valueOffset));
        d.endHash();
        d << "]";
    }

    d.disarm();
}

static void qDumpQMap(QDumper &d)
{
    QMapData *h = *reinterpret_cast<QMapData *const*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];

    int n = h->size;

    if (n < 0)
        qCheck(false);
    if (n > 0) {
        qCheckAccess(h->backward);
        qCheckAccess(h->forward[0]);
        qCheckPointer(h->backward->backward);
        qCheckPointer(h->forward[0]->backward);
    }

    P(d, "value", "<" << n << " items>");
    P(d, "numchild", n);
    if (d.dumpChildren) {
        if (n > 1000)
            n = 1000;

        //unsigned keySize = d.extraInt[0];
        //unsigned valueSize = d.extraInt[1];
        unsigned mapnodesize = d.extraInt[2];
        unsigned valueOff = d.extraInt[3];

        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        // both negative:
        int keyOffset = 2 * sizeof(void*) - int(mapnodesize);
        int valueOffset = 2 * sizeof(void*) - int(mapnodesize) + valueOff;

        P(d, "extra", "simplekey: " << isSimpleKey << " isSimpleValue: " << isSimpleValue
            << " keyOffset: " << keyOffset << " valueOffset: " << valueOffset
            << " mapnodesize: " << mapnodesize);
        d << ",children=[";

        QMapData::Node *node = reinterpret_cast<QMapData::Node *>(h->forward[0]);
        QMapData::Node *end = reinterpret_cast<QMapData::Node *>(h);
        int i = 0;

        while (node != end) {
            d.beginHash();
                P(d, "name", i);
                qDumpInnerValueHelper(d, keyType, addOffset(node, keyOffset), "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    P(d, "type", valueType);
                    P(d, "addr", addOffset(node, valueOffset));
                } else {
#if QT_VERSION >= 0x040500
                    // actually, any type (even 'char') will do...
                    P(d, "type", NS"QMapNode<"
                        << keyType << "," << valueType << " >");
                    P(d, "exp", "*('"NS"QMapNode<"
                        << keyType << "," << valueType << " >'*)" << node);

                    //P(d, "exp", "*('"NS"QMapData'*)" << (void*)node);
                    //P(d, "exp", "*(char*)" << (void*)node);
                    // P(d, "addr", node);  does not work as gdb fails to parse
#else 
                    P(d, "type", NS"QMapData::Node<"
                        << keyType << "," << valueType << " >");
                    P(d, "exp", "*('"NS"QMapData::Node<"
                        << keyType << "," << valueType << " >'*)" << node);
#endif
                }
            d.endHash();

            ++i;
            node = node->forward[0];
        }
        d << "]";
    }

    d.disarm();
}

static void qDumpQMultiMap(QDumper &d)
{
    qDumpQMap(d);
}

static void qDumpQModelIndex(QDumper &d)
{
    const QModelIndex *mi = reinterpret_cast<const QModelIndex *>(d.data);

    P(d, "type", NS"QModelIndex");
    if (mi->isValid()) {
        P(d, "value", "(" << mi->row() << ", " << mi->column() << ")");
        P(d, "numchild", 5);
        if (d.dumpChildren) {
            d << ",children=[";
            I(d, "row", mi->row());
            I(d, "column", mi->column());

            d.beginHash();
            P(d, "name", "parent");
            const QModelIndex parent = mi->parent();
            if (parent.isValid())
                P(d, "value", "(" << mi->row() << ", " << mi->column() << ")");
            else
                P(d, "value", "<invalid>");
            P(d, "exp", "(("NSX"QModelIndex"NSY"*)" << d.data << ")->parent()");
            P(d, "type", NS"QModelIndex");
            P(d, "numchild", "1");
            d.endHash();

            S(d, "internalId", QString::number(mi->internalId(), 10));

            d.beginHash();
            P(d, "name", "model");
            P(d, "value", static_cast<const void *>(mi->model()));
            P(d, "type", NS"QAbstractItemModel*");
            P(d, "numchild", "1");
            d.endHash();

            d << "]";
        }
    } else {
        P(d, "value", "<invalid>");
        P(d, "numchild", 0);
    }

    d.disarm();
}

static void qDumpQObject(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QMetaObject *mo = ob->metaObject();
    unsigned childrenOffset = d.extraInt[0];
    P(d, "value", ob->objectName());
    P(d, "valueencoded", "1");
    P(d, "type", NS"QObject");
    P(d, "displayedtype", mo->className());
    P(d, "numchild", 4);
    if (d.dumpChildren) {
        const QObjectList &children = ob->children();
        int slotCount = 0;
        int signalCount = 0;
        for (int i = mo->methodCount(); --i >= 0; ) {
            QMetaMethod::MethodType mt = mo->method(i).methodType();
            signalCount += (mt == QMetaMethod::Signal);
            slotCount += (mt == QMetaMethod::Slot);
        }
        d << ",children=[";
        d.beginHash();
            P(d, "name", "properties");
            // FIXME: Note that when simply using '(QObject*)'
            // in the cast below, Gdb/MI _sometimes_ misparses
            // expressions further down in the tree.
            P(d, "exp", "*(class '"NS"QObject'*)" << d.data);
            P(d, "type", NS"QObjectPropertyList");
            P(d, "value", "<" << mo->propertyCount() << " items>");
            P(d, "numchild", mo->propertyCount());
        d.endHash();
#if 0
        d.beginHash();
            P(d, "name", "methods");
            P(d, "exp", "*(class '"NS"QObject'*)" << d.data);
            P(d, "value", "<" << mo->methodCount() << " items>");
            P(d, "numchild", mo->methodCount());
        d.endHash();
#endif
#if 0
        d.beginHash();
            P(d, "name", "senders");
            P(d, "exp", "(*(class '"NS"QObjectPrivate'*)" << dfunc(ob) << ")->senders");
            P(d, "type", NS"QList<"NS"QObjectPrivateSender>");
        d.endHash();
#endif
#if PRIVATE_OBJECT_ALLOWED
        d.beginHash();
            P(d, "name", "signals");
            P(d, "exp", "*(class '"NS"QObject'*)" << d.data);
            P(d, "type", NS"QObjectSignalList");
            P(d, "value", "<" << signalCount << " items>");
            P(d, "numchild", signalCount);
        d.endHash();
        d.beginHash();
            P(d, "name", "slots");
            P(d, "exp", "*(class '"NS"QObject'*)" << d.data);
            P(d, "type", NS"QObjectSlotList");
            P(d, "value", "<" << slotCount << " items>");
            P(d, "numchild", slotCount);
        d.endHash();
#endif
        d.beginHash();
            P(d, "name", "children");
            // works always, but causes additional traffic on the list
            //P(d, "exp", "((class '"NS"QObject'*)" << d.data << ")->children()");
            //
            //P(d, "addr", addOffset(dfunc(ob), childrenOffset));
            //P(d, "type", NS"QList<QObject *>");
            //P(d, "value", "<" << children.size() << " items>");
            qDumpInnerValue(d, NS"QList<"NS"QObject *>",
                addOffset(dfunc(ob), childrenOffset));
            P(d, "numchild", children.size());
        d.endHash();
#if 0
        // Unneeded (and not working): Connections are listes as childen
        // of the signal or slot they are connected to.
        // d.beginHash();
        //     P(d, "name", "connections");
        //     P(d, "exp", "*(*(class "NS"QObjectPrivate*)" << dfunc(ob) << ")->connectionLists");
        //     P(d, "type", NS"QVector<"NS"QList<"NS"QObjectPrivate::Connection> >");
        // d.endHash();
#endif
#if 0
        d.beginHash();
            P(d, "name", "objectprivate");
            P(d, "type", NS"QObjectPrivate");
            P(d, "addr", dfunc(ob));
            P(d, "value", "");
            P(d, "numchild", "1");
        d.endHash();
#endif
        d.beginHash();
            P(d, "name", "parent");
            qDumpInnerValueHelper(d, NS"QObject *", ob->parent());
        d.endHash();
#if 1
        d.beginHash();
            P(d, "name", "className");
            P(d, "value",ob->metaObject()->className());
            P(d, "type", "");
            P(d, "numchild", "0");
        d.endHash();
#endif
        d << "]";
    }
    d.disarm();
}

static void qDumpQObjectPropertyList(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mo = ob->metaObject();
    P(d, "addr", "<synthetic>");
    P(d, "type", NS"QObjectPropertyList");
    P(d, "numchild", mo->propertyCount());
    if (d.dumpChildren) {
        d << ",children=[";
        for (int i = mo->propertyCount(); --i >= 0; ) {
            const QMetaProperty & prop = mo->property(i);
            d.beginHash();
            P(d, "name", prop.name());
            P(d, "exp", "((" << mo->className() << "*)" << ob
                        << ")->" << prop.name() << "()");
            if (isEqual(prop.typeName(), "QString")) {
                P(d, "value", prop.read(ob).toString());
                P(d, "valueencoded", "1");
                P(d, "type", NS"QString");
                P(d, "numchild", "0");
            } else if (isEqual(prop.typeName(), "bool")) {
                P(d, "value", (prop.read(ob).toBool() ? "true" : "false"));
                P(d, "numchild", "0");
            } else if (isEqual(prop.typeName(), "int")) {
                P(d, "value", prop.read(ob).toInt());
                P(d, "numchild", "0");
            }
            P(d, "type", prop.typeName());
            P(d, "numchild", "1");
            d.endHash();
        }
        d << "]";
    }
    d.disarm();
}

static void qDumpQObjectMethodList(QDumper &d)
{
    const QObject *ob = (const QObject *)d.data;
    const QMetaObject *mo = ob->metaObject();
    P(d, "addr", "<synthetic>");
    P(d, "type", NS"QObjectMethodList");
    P(d, "numchild", mo->methodCount());
    P(d, "childtype", "QMetaMethod::Method");
    P(d, "childnumchild", "0");
    if (d.dumpChildren) {
        d << ",children=[";
        for (int i = 0; i != mo->methodCount(); ++i) {
            const QMetaMethod & method = mo->method(i);
            int mt = method.methodType();
            d.beginHash();
            P(d, "name", i << " " << mo->indexOfMethod(method.signature())
                << " " << method.signature());
            P(d, "value", (mt == QMetaMethod::Signal ? "<Signal>" : "<Slot>") << " (" << mt << ")");
            d.endHash();
        }
        d << "]";
    }
    d.disarm();
}

#if PRIVATE_OBJECT_ALLOWED
const char * qConnectionTypes[] ={
    "auto",
    "direct",
    "queued",
    "autocompat",
    "blockingqueued"
};

#if QT_VERSION >= 0x040400
static const QObjectPrivate::ConnectionList &qConnectionList(const QObject *ob, int signalNumber)
{
    static const QObjectPrivate::ConnectionList emptyList;
    const QObjectPrivate *p = reinterpret_cast<const QObjectPrivate *>(dfunc(ob));
    if (!p->connectionLists)
        return emptyList;
    typedef QVector<QObjectPrivate::ConnectionList> ConnLists;
    const ConnLists *lists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    // there's an optimization making the lists only large enough to hold the
    // last non-empty item
    if (signalNumber >= lists->size())
        return emptyList;
    return lists->at(signalNumber);
}
#endif

static void qDumpQObjectSignal(QDumper &d)
{
    unsigned signalNumber = d.extraInt[0];

    P(d, "addr", "<synthetic>");
    P(d, "numchild", "1");
    P(d, "type", NS"QObjectSignal");

#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        const QObject *ob = reinterpret_cast<const QObject *>(d.data);
        d << ",children=[";
        const QObjectPrivate::ConnectionList &connList = qConnectionList(ob, signalNumber);
        for (int i = 0; i != connList.size(); ++i) {
            const QObjectPrivate::Connection &conn = connList.at(i);
            d.beginHash();
                P(d, "name", i << " receiver");
                qDumpInnerValueHelper(d, NS"QObject *", conn.receiver);
            d.endHash();
            d.beginHash();
                P(d, "name", i << " slot");
                P(d, "type", "");
                if (conn.receiver) 
                    P(d, "value", conn.receiver->metaObject()->method(conn.method).signature());
                else
                    P(d, "value", "<invalid receiver>");
                P(d, "numchild", "0");
            d.endHash();
            d.beginHash();
                P(d, "name", i << " type");
                P(d, "type", "");
                P(d, "value", "<" << qConnectionTypes[conn.method] << " connection>");
                P(d, "numchild", "0");
            d.endHash();
        }
        d << "]";
        P(d, "numchild", connList.size());
    }
#endif
    d.disarm();
}

static void qDumpQObjectSignalList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
    const QMetaObject *mo = ob->metaObject();
    int count = 0;
    for (int i = mo->methodCount(); --i >= 0; )
        count += (mo->method(i).methodType() == QMetaMethod::Signal);
    P(d, "addr", d.data);
    P(d, "numchild", count);
#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d << ",children=[";
        for (int i = 0; i != mo->methodCount(); ++i) {
            const QMetaMethod & method = mo->method(i);
            if (method.methodType() == QMetaMethod::Signal) {
                int k = mo->indexOfSignal(method.signature());
                const QObjectPrivate::ConnectionList &connList = qConnectionList(ob, k);
                d.beginHash();
                P(d, "name", k);
                P(d, "value", method.signature());
                P(d, "numchild", connList.size());
                //P(d, "numchild", "1");
                P(d, "exp", "*(class '"NS"QObject'*)" << d.data);
                P(d, "type", NS"QObjectSignal");
                d.endHash();
            }
        }
        d << "]";
    }
#endif
    d.disarm();
}

static void qDumpQObjectSlot(QDumper &d)
{
    int slotNumber = d.extraInt[0];

    P(d, "addr", d.data);
    P(d, "numchild", "1");
    P(d, "type", NS"QObjectSlot");

#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d << ",children=[";
        int numchild = 0;
        const QObject *ob = reinterpret_cast<const QObject *>(d.data);
        const QObjectPrivate *p = reinterpret_cast<const QObjectPrivate *>(dfunc(ob));
        for (int s = 0; s != p->senders.size(); ++s) {
            const QObjectPrivate::Sender &sender = p->senders.at(s);
            const QObjectPrivate::ConnectionList &connList
                = qConnectionList(sender.sender, sender.signal);
            for (int i = 0; i != connList.size(); ++i) {
                const QObjectPrivate::Connection &conn = connList.at(i);
                if (conn.receiver == ob && conn.method == slotNumber) {
                    ++numchild;
                    const QMetaMethod & method =
                        sender.sender->metaObject()->method(sender.signal);
                    d.beginHash();
                        P(d, "name", s << " sender");
                        qDumpInnerValueHelper(d, NS"QObject *", sender.sender);
                    d.endHash();
                    d.beginHash();
                        P(d, "name", s << " signal");
                        P(d, "type", "");
                        P(d, "value", method.signature());
                        P(d, "numchild", "0");
                    d.endHash();
                    d.beginHash();
                        P(d, "name", s << " type");
                        P(d, "type", "");
                        P(d, "value", "<" << qConnectionTypes[conn.method] << " connection>");
                        P(d, "numchild", "0");
                    d.endHash();
                }
            }
        }
        d << "]";
        P(d, "numchild", numchild);
    }
#endif
    d.disarm();
}

static void qDumpQObjectSlotList(QDumper &d)
{
    const QObject *ob = reinterpret_cast<const QObject *>(d.data);
#if QT_VERSION >= 0x040400
    const QObjectPrivate *p = reinterpret_cast<const QObjectPrivate *>(dfunc(ob));
#endif
    const QMetaObject *mo = ob->metaObject();

    int count = 0;
    for (int i = mo->methodCount(); --i >= 0; )
        count += (mo->method(i).methodType() == QMetaMethod::Slot);

    P(d, "addr", d.data);
    P(d, "numchild", count);
#if QT_VERSION >= 0x040400
    if (d.dumpChildren) {
        d << ",children=[";
        for (int i = 0; i != mo->methodCount(); ++i) {
            const QMetaMethod & method = mo->method(i);
            if (method.methodType() == QMetaMethod::Slot) {
                d.beginHash();
                int k = mo->indexOfSlot(method.signature());
                P(d, "name", k);
                P(d, "value", method.signature());

                // count senders. expensive...
                int numchild = 0;
                for (int s = 0; s != p->senders.size(); ++s) {
                    const QObjectPrivate::Sender & sender = p->senders.at(s);
                    const QObjectPrivate::ConnectionList &connList
                        = qConnectionList(sender.sender, sender.signal);
                    for (int c = 0; c != connList.size(); ++c) {
                        const QObjectPrivate::Connection &conn = connList.at(c);
                        if (conn.receiver == ob && conn.method == k)
                            ++numchild;
                    }
                }
                P(d, "numchild", numchild);
                P(d, "exp", "*(class '"NS"QObject'*)" << d.data);
                P(d, "type", NS"QObjectSlot");
                d.endHash();
            }
        }
        d << "]";
    }
#endif
    d.disarm();
}
#endif // PRIVATE_OBJECT_ALLOWED


static void qDumpQPixmap(QDumper &d)
{
#ifdef QT_GUI_LIB
    const QPixmap &im = *reinterpret_cast<const QPixmap *>(d.data);
    P(d, "value", "(" << im.width() << "x" << im.height() << ")");
    P(d, "type", NS"QPixmap");
    P(d, "numchild", "0");
    d.disarm();
#else
    Q_UNUSED(d);
#endif
}

static void qDumpQSet(QDumper &d)
{
    // This uses the knowledge that QHash<T> has only a single member
    // of  union { QHashData *d; QHashNode<Key, T> *e; };
    QHashData *hd = *(QHashData**)d.data;
    QHashData::Node *node = hd->firstNode();

    int n = hd->size;
    if (n < 0)
        qCheck(false);
    if (n > 0) {
        qCheckAccess(node);
        qCheckPointer(node->next);
    }

    P(d, "value", "<" << n << " items>");
    P(d, "valuedisabled", "true");
    P(d, "numchild", 2 * n);
    if (d.dumpChildren) {
        if (n > 100)
            n = 100;
        d << ",children=[";
        int i = 0;
        for (int bucket = 0; bucket != hd->numBuckets && i <= 10000; ++bucket) {
            for (node = hd->buckets[bucket]; node->next; node = node->next) {
                d.beginHash();
                P(d, "name", i);
                P(d, "type", d.innertype);
                P(d, "exp", "(('"NS"QHashNode<" << d.innertype
                    << ","NS"QHashDummyValue>'*)"
                    << static_cast<const void*>(node) << ")->key"
                );
                d.endHash();
                ++i;
                if (i > 10000) {
                    d.putEllipsis();
                    break;
                }
            }
        }
        d << "]";
    }
    d.disarm();
}

static void qDumpQString(QDumper &d)
{
    const QString &str = *reinterpret_cast<const QString *>(d.data);

    if (!str.isEmpty()) {
        qCheckAccess(str.unicode());
        qCheckAccess(str.unicode() + str.size());
    }

    P(d, "value", str);
    P(d, "valueencoded", "1");
    P(d, "type", NS"QString");
    //P(d, "editvalue", str);  // handled generically below
    P(d, "numchild", "0");

    d.disarm();
}

static void qDumpQStringList(QDumper &d)
{
    const QStringList &list = *reinterpret_cast<const QStringList *>(d.data);
    int n = list.size();
    if (n < 0)
        qCheck(false);
    if (n > 0) {
        qCheckAccess(&list.front());
        qCheckAccess(&list.back());
    }

    P(d, "value", "<" << n << " items>");
    P(d, "valuedisabled", "true");
    P(d, "numchild", n);
    P(d, "childtype", NS"QString");
    P(d, "childnumchild", "0");
    if (d.dumpChildren) {
        if (n > 1000)
            n = 1000;
        d << ",children=[";
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            P(d, "name", i);
            P(d, "value", list[i]);
            P(d, "valueencoded", "1");
            d.endHash();
        }
        if (n < list.size())
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

static void qDumpQTextCodec(QDumper &d)
{
    const QTextCodec &codec = *reinterpret_cast<const QTextCodec *>(d.data);
    P(d, "value", codec.name());
    P(d, "valueencoded", "1");
    P(d, "type", NS"QTextCodec");
    P(d, "numchild", "2");
    if (d.dumpChildren) {
        d << ",children=[";
        S(d, "name", codec.name());
        I(d, "mibEnum", codec.mibEnum());
        d << "]";
    }
    d.disarm();
}

static void qDumpQVariantHelper(const void *data, QString *value,
    QString *exp, int *numchild)
{
    const QVariant &v = *reinterpret_cast<const QVariant *>(data);
    switch (v.type()) {
    case QVariant::Invalid:
        *value = QLatin1String("<invalid>");
        *numchild = 0;
        break;
    case QVariant::String:
        *value = QLatin1Char('"') + v.toString() + QLatin1Char('"');
        *numchild = 0;
        break;
    case QVariant::StringList:
        *exp = QString(QLatin1String("((QVariant*)%1)->d.data.c"))
                    .arg((quintptr)data);
        *numchild = v.toStringList().size();
        break;
    case QVariant::Int:
        *value = QString::number(v.toInt());
        *numchild= 0;
        break;
    case QVariant::Double:
        *value = QString::number(v.toDouble());
        *numchild = 0;
        break;
    default: {
        char buf[1000];
        const char *format = (v.typeName()[0] == 'Q')
            ?  "'"NS"%s "NS"qVariantValue<"NS"%s >'(*('"NS"QVariant'*)%p)"
            :  "'%s "NS"qVariantValue<%s >'(*('"NS"QVariant'*)%p)";
        qsnprintf(buf, sizeof(buf) - 1, format, v.typeName(), v.typeName(), data);
        *exp = QLatin1String(buf);
        *numchild = 1;
        break;
        }
    }
}

static void qDumpQVariant(QDumper &d)
{
    const QVariant &v = *reinterpret_cast<const QVariant *>(d.data);
    QString value;
    QString exp;
    int numchild = 0;
    qDumpQVariantHelper(d.data, &value, &exp, &numchild);
    bool isInvalid = (v.typeName() == 0);
    if (isInvalid) {
        P(d, "value", "(invalid)");
    } else if (value.isEmpty()) {
        P(d, "value", "(" << v.typeName() << ") " << qPrintable(value));
    } else {
        QByteArray ba;
        ba += '(';
        ba += v.typeName();
        ba += ") ";
        ba += qPrintable(value);
        P(d, "value", ba);
        P(d, "valueencoded", "1");
    }
    P(d, "type", NS"QVariant");
    P(d, "numchild", (isInvalid ? "0" : "1"));
    if (d.dumpChildren) {
        d << ",children=[";
        d.beginHash();
        P(d, "name", "value");
        if (!exp.isEmpty())
            P(d, "exp", qPrintable(exp));
        if (!value.isEmpty()) {
            P(d, "value", value);
            P(d, "valueencoded", "1");
        }
        P(d, "type", v.typeName());
        P(d, "numchild", numchild);
        d.endHash();
        d << "]";
    }
    d.disarm();
}

static void qDumpQVector(QDumper &d)
{
    QVectorData *v = *reinterpret_cast<QVectorData *const*>(d.data);

    // Try to provoke segfaults early to prevent the frontend
    // from asking for unavailable child details
    int nn = v->size;
    if (nn < 0)
        qCheck(false);
    if (nn > 0) {
        //qCheckAccess(&vec.front());
        //qCheckAccess(&vec.back());
    }

    unsigned innersize = d.extraInt[0];
    unsigned typeddatasize = d.extraInt[1];

    int n = nn;
    P(d, "value", "<" << n << " items>");
    P(d, "valuedisabled", "true");
    P(d, "numchild", n);
    if (d.dumpChildren) {
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;
        if (n > 1000)
            n = 1000;
        d << ",children=[";
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            P(d, "name", i);
            qDumpInnerValueOrPointer(d, d.innertype, stripped,
                addOffset(v, i * innersize + typeddatasize));
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

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

    int nn = 0;
    std::list<int>::const_iterator it = list.begin();
    for (; nn < 101 && it != list.end(); ++nn, ++it)
        qCheckAccess(it.operator->());

    if (nn > 100)
        P(d, "value", "<more than 100 items>");
    else
        P(d, "value", "<" << nn << " items>");
    P(d, "numchild", nn);

    P(d, "valuedisabled", "true");
    if (d.dumpChildren) {
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;
        d << ",children=[";
        it = list.begin();
        for (int i = 0; i < 1000 && it != list.end(); ++i, ++it) {
            d.beginHash();
            P(d, "name", i);
            qDumpInnerValueOrPointer(d, d.innertype, stripped, it.operator->());
            d.endHash();
        }
        if (it != list.end())
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

static void qDumpStdMap(QDumper &d)
{
    typedef std::map<int, int> DummyType;
    const DummyType &map = *reinterpret_cast<const DummyType*>(d.data);
    const char *keyType   = d.templateParameters[0];
    const char *valueType = d.templateParameters[1];
    const void *p = d.data;
    qCheckAccess(p);
    p = deref(p);

    int nn = map.size();
    qCheck(nn >= 0);
    DummyType::const_iterator it = map.begin();
    for (int i = 0; i < nn && i < 10 && it != map.end(); ++i, ++it)
        qCheckAccess(it.operator->());

    QByteArray strippedInnerType = stripPointerType(d.innertype);
    P(d, "numchild", nn);
    P(d, "value", "<" << nn << " items>");
    P(d, "valuedisabled", "true");
    P(d, "valueoffset", d.extraInt[2]);

    // HACK: we need a properly const qualified version of the
    // std::pair used. We extract it from the allocator parameter
    // (#4, "std::allocator<std::pair<key, value> >")
    // as it is there, and, equally importantly, in an order that
    // gdb accepts when fed with it.
    char *pairType = (char *)(d.templateParameters[3]) + 15;
    pairType[strlen(pairType) - 2] = 0;
    P(d, "pairtype", pairType);
    
    if (d.dumpChildren) {
        bool isSimpleKey = isSimpleType(keyType);
        bool isSimpleValue = isSimpleType(valueType);
        int valueOffset = d.extraInt[2];

        P(d, "extra", "isSimpleKey: " << isSimpleKey
            << " isSimpleValue: " << isSimpleValue
            << " valueType: '" << valueType
            << " valueOffset: " << valueOffset);

        d << ",children=[";
        it = map.begin();
        for (int i = 0; i < 1000 && it != map.end(); ++i, ++it) {
            d.beginHash();
                const void *node = it.operator->();
                P(d, "name", i);
                qDumpInnerValueHelper(d, keyType, node, "key");
                qDumpInnerValueHelper(d, valueType, addOffset(node, valueOffset));
                if (isSimpleKey && isSimpleValue) {
                    P(d, "type", valueType);
                    P(d, "addr", addOffset(node, valueOffset));
                    P(d, "numchild", 0);
                } else {
                    P(d, "addr", node);
                    P(d, "type", pairType);
                    P(d, "numchild", 2);
                }
            d.endHash();
        }
        if (it != map.end())
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

static void qDumpStdString(QDumper &d)
{
    const std::string &str = *reinterpret_cast<const std::string *>(d.data);

    if (!str.empty()) {
        qCheckAccess(str.c_str());
        qCheckAccess(str.c_str() + str.size() - 1);
    }

    d << ",value=\"";
    d.putBase64Encoded(str.c_str(), str.size());
    d << "\"";
    P(d, "valueencoded", "1");
    P(d, "type", "std::string");
    P(d, "numchild", "0");

    d.disarm();
}

static void qDumpStdWString(QDumper &d)
{
    const std::wstring &str = *reinterpret_cast<const std::wstring *>(d.data);

    if (!str.empty()) {
        qCheckAccess(str.c_str());
        qCheckAccess(str.c_str() + str.size() - 1);
    }

    d << "value='";
    d.putBase64Encoded((const char *)str.c_str(), str.size() * sizeof(wchar_t));
    d << "'";
    P(d, "valueencoded", (sizeof(wchar_t) == 2 ? "2" : "3"));
    P(d, "type", "std::wstring");
    P(d, "numchild", "0");

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
        qCheck(false);
    if (nn > 0) {
        qCheckAccess(v->start);
        qCheckAccess(v->finish);
        qCheckAccess(v->end_of_storage);
    }

    int n = nn;
    P(d, "value", "<" << n << " items>");
    P(d, "valuedisabled", "true");
    P(d, "numchild", n);
    if (d.dumpChildren) {
        unsigned innersize = d.extraInt[0];
        QByteArray strippedInnerType = stripPointerType(d.innertype);
        const char *stripped =
            isPointerType(d.innertype) ? strippedInnerType.data() : 0;
        if (n > 1000)
            n = 1000;
        d << ",children=[";
        for (int i = 0; i != n; ++i) {
            d.beginHash();
            P(d, "name", i);
            qDumpInnerValueOrPointer(d, d.innertype, stripped,
                addOffset(v->start, i * innersize));
            d.endHash();
        }
        if (n < nn)
            d.putEllipsis();
        d << "]";
    }
    d.disarm();
}

static void qDumpStdVectorBool(QDumper &d)
{
    // FIXME
    return qDumpStdVector(d);
}

static void handleProtocolVersion2and3(QDumper & d)
{
    if (!d.outertype[0]) {
        qDumpUnknown(d);
        return;
    }

    d.setupTemplateParameters();
    P(d, "iname", d.iname);
    P(d, "addr", d.data);

#ifdef QT_NO_QDATASTREAM
    if (d.protocolVersion == 3) {
        QVariant::Type type = QVariant::nameToType(d.outertype);
        if (type != QVariant::Invalid) {
            QVariant v(type, d.data);
            QByteArray ba;
            QDataStream ds(&ba, QIODevice::WriteOnly);
            ds << v;
            P(d, "editvalue", ba);
        }
    }
#endif

    const char *type = stripNamespace(d.outertype);
    // type[0] is usally 'Q', so don't use it
    switch (type[1]) {
        case 'B':
            if (isEqual(type, "QByteArray"))
                qDumpQByteArray(d);
            break;
        case 'D':
            if (isEqual(type, "QDateTime"))
                qDumpQDateTime(d);
            else if (isEqual(type, "QDir"))
                qDumpQDir(d);
            break;
        case 'F':
            if (isEqual(type, "QFile"))
                qDumpQFile(d);
            else if (isEqual(type, "QFileInfo"))
                qDumpQFileInfo(d);
            break;
        case 'H':
            if (isEqual(type, "QHash"))
                qDumpQHash(d);
            else if (isEqual(type, "QHashNode"))
                qDumpQHashNode(d);
            break;
        case 'I':
            if (isEqual(type, "QImage"))
                qDumpQImage(d);
            break;
        case 'L':
            if (isEqual(type, "QList"))
                qDumpQList(d);
            else if (isEqual(type, "QLinkedList"))
                qDumpQLinkedList(d);
            else if (isEqual(type, "QLocale"))
                qDumpQLocale(d);
            break;
        case 'M':
            if (isEqual(type, "QMap"))
                qDumpQMap(d);
            else if (isEqual(type, "QMapNode"))
                qDumpQMapNode(d);
            else if (isEqual(type, "QModelIndex"))
                qDumpQModelIndex(d);
            else if (isEqual(type, "QMultiMap"))
                qDumpQMultiMap(d);
            break;
        case 'O':
            if (isEqual(type, "QObject"))
                qDumpQObject(d);
            else if (isEqual(type, "QObjectPropertyList"))
                qDumpQObjectPropertyList(d);
            else if (isEqual(type, "QObjectMethodList"))
                qDumpQObjectMethodList(d);
            #if PRIVATE_OBJECT_ALLOWED
            else if (isEqual(type, "QObjectSignal"))
                qDumpQObjectSignal(d);
            else if (isEqual(type, "QObjectSignalList"))
                qDumpQObjectSignalList(d);
            else if (isEqual(type, "QObjectSlot"))
                qDumpQObjectSlot(d);
            else if (isEqual(type, "QObjectSlotList"))
                qDumpQObjectSlotList(d);
            #endif
            break;
        case 'P':
            if (isEqual(type, "QPixmap"))
                qDumpQPixmap(d);
            break;
        case 'S':
            if (isEqual(type, "QSet"))
                qDumpQSet(d);
            else if (isEqual(type, "QString"))
                qDumpQString(d);
            else if (isEqual(type, "QStringList"))
                qDumpQStringList(d);
            break;
        case 'T':
            if (isEqual(type, "QTextCodec"))
                qDumpQTextCodec(d);
            break;
        case 'V':
            if (isEqual(type, "QVariant"))
                qDumpQVariant(d);
            else if (isEqual(type, "QVector"))
                qDumpQVector(d);
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
            else if (isEqual(type, "std::string") || isEqual(type, "string"))
                qDumpStdString(d);
            else if (isEqual(type, "std::wstring"))
                qDumpStdWString(d);
            break;
    }

    if (!d.success)
        qDumpUnknown(d);
}

} // anonymous namespace


extern "C" Q_DECL_EXPORT
void qDumpObjectData440(
    int protocolVersion,
    int token,
    void *data,
    bool dumpChildren,
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
        // They are mentioned here nevertheless. For types that not listed
        // here, dumpers won't be used.
        d << "dumpers=["
            "\""NS"QByteArray\","
            "\""NS"QDateTime\","
            "\""NS"QDir\","
            "\""NS"QFile\","
            "\""NS"QFileInfo\","
            "\""NS"QHash\","
            "\""NS"QHashNode\","
            "\""NS"QImage\","
            "\""NS"QLinkedList\","
            "\""NS"QList\","
            "\""NS"QLocale\","
            "\""NS"QMap\","
            "\""NS"QMapNode\","
            "\""NS"QModelIndex\","
            #if QT_VERSION >= 0x040500
            "\""NS"QMultiMap\","
            #endif
            "\""NS"QObject\","
            "\""NS"QObjectMethodList\","   // hack to get nested properties display
            "\""NS"QObjectPropertyList\","
            #if PRIVATE_OBJECT_ALLOWED
            "\""NS"QObjectSignal\","
            "\""NS"QObjectSignalList\","
            "\""NS"QObjectSlot\","
            "\""NS"QObjectSlotList\","
            #endif // PRIVATE_OBJECT_ALLOWED
            // << "\""NS"QRegion\","
            "\""NS"QSet\","
            "\""NS"QString\","
            "\""NS"QStringList\","
            "\""NS"QTextCodec\","
            "\""NS"QVariant\","
            "\""NS"QVector\","
            "\""NS"QWidget\","
            "\"string\","
            "\"wstring\","
            "\"std::basic_string\","
            "\"std::list\","
            "\"std::map\","
            "\"std::string\","
            "\"std::vector\","
            "\"std::wstring\","
            "]";
        d << ",qtversion=["
            "\"" << ((QT_VERSION >> 16) & 255) << "\","
            "\"" << ((QT_VERSION >> 8)  & 255) << "\","
            "\"" << ((QT_VERSION)       & 255) << "\"]";
        d << ",namespace=\""NS"\"";
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

        const char *inbuffer = qDumpInBuffer;
        d.outertype = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.iname     = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.exp       = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.innertype = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;
        d.iname     = inbuffer; while (*inbuffer) ++inbuffer; ++inbuffer;

        handleProtocolVersion2and3(d);
    }

    else {
        qDebug() << "Unsupported protocol version" << protocolVersion;
    }
}
