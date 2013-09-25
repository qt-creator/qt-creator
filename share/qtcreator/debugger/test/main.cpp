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

#include <QStringList>
#include <QLinkedList>
#include <QVector>
#include <QSharedPointer>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QVariant>
#include <QFileInfo>
#include <QCoreApplication>
#include <QAction>

#include <string>
#include <list>
#include <vector>
#include <set>
#include <map>

#include <stdio.h>
#include <string.h>

// Test uninitialized variables allocing memory
bool optTestUninitialized = false;
bool optTestAll = false;
bool optEmptyContainers = false;
unsigned optVerbose = 0;
const char *appPath = 0;

// Provide address of type of be tested.
// When testing uninitialized memory, allocate at random.
template <class T>
        inline T* testAddress(T* in)
{
    unsigned char *mem = 0;
    if (optTestUninitialized) {
        mem = new unsigned char[sizeof(T)];
        for (unsigned int i = 0; i < sizeof(T); i++) {
            mem[i] = char(rand() % 255u);
        }
    } else {
        mem = reinterpret_cast<unsigned char*>(in);
    }
    if (optVerbose) {
        for (unsigned int i = 0; i < sizeof(T); i++) {
            unsigned int b = mem[i];
            printf("%2d %2x %3d\n", i, b, b);
        }
        fflush(stdout);
    }
    return reinterpret_cast<T*>(mem);
}

/* Test program for Dumper development/porting.
 * Takes the type as first argument. */

// --------------- Dumper symbols
extern char qDumpInBuffer[10000];
extern char qDumpOutBuffer[100000];

extern "C" void *qDumpObjectData440(
    int protocolVersion,
    int token,
    void *data,
#ifdef Q_CC_MSVC // CDB cannot handle boolean parameters
    int dumpChildren,
#else
    bool dumpChildren,
#endif
    int extraInt0, int extraInt1, int extraInt2, int extraInt3);

static void prepareInBuffer(const char *outerType,
                            const char *iname,
                            const char *expr,
                            const char *innerType)
{
    // Leave trailing '\0'
    char *ptr = qDumpInBuffer;
    strcpy(ptr, outerType);
    ptr += strlen(outerType);
    ptr++;
    strcpy(ptr, iname);
    ptr += strlen(iname);
    ptr++;
    strcpy(ptr, expr);
    ptr += strlen(expr);
    ptr++;
    strcpy(ptr, innerType);
    ptr += strlen(innerType);
    ptr++;
    strcpy(ptr, iname);
}

// ---------------  Qt types
static int dumpQString()
{
    QString test = QLatin1String("hallo");
    prepareInBuffer("QString", "local.qstring", "local.qstring", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    QString uninitialized;
    return 0;
}

static int dumpQSharedPointerQString()
{
    QSharedPointer<QString> test(new QString(QLatin1String("hallo")));
    prepareInBuffer("QSharedPointer", "local.sharedpointerqstring", "local.local.sharedpointerqstring", "QString");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(QString), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    QString uninitialized;
    return 0;
}

static int dumpQStringList()
{
    QStringList test = QStringList() << QLatin1String("item1") << QLatin1String("item2");
    prepareInBuffer("QList", "local.qstringlist", "local.qstringlist", "QString");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(QString), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQIntList()
{
    QList<int> test = QList<int>() << 1 << 2;
    prepareInBuffer("QList", "local.qintlist", "local.qintlist", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQIntLinkedList()
{
    QLinkedList<int> test = QLinkedList<int>() << 1 << 2;
    prepareInBuffer("QLinkedList", "local.qintlinkedlist", "local.qlinkedintlist", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQIntVector()
{
    QVector<int> test = QVector<int>() << 42 << 43;
    prepareInBuffer("QVector", "local.qintvector", "local.qintvector", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQQStringVector()
{
    QVector<QString> test = QVector<QString>() << "42s" << "43s";
    prepareInBuffer("QVector", "local.qstringvector", "local.qstringvector", "QString");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(QString), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQMapIntInt()
{
    QMap<int,int> test;
    QMapNode<int,int> mapNode;
    const int valueOffset = (char*)&(mapNode.value) - (char*)&mapNode;
    if (!optEmptyContainers) {
        test.insert(42, 43);
        test.insert(43, 44);
    }
    prepareInBuffer("QMap", "local.qmapintint", "local.qmapintint", "int@int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), sizeof(int), sizeof(mapNode), valueOffset);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQMapIntString()
{
    QMap<int,QString> test;
    QMapNode<int,QString> mapNode;
    const int valueOffset = (char*)&(mapNode.value) - (char*)&mapNode;
    if (!optEmptyContainers) {
        test.insert(42, QLatin1String("fortytwo"));
        test.insert(43, QLatin1String("fortytree"));
    }
    prepareInBuffer("QMap", "local.qmapintqstring", "local.qmapintqstring", "int@QString");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), sizeof(QString), sizeof(mapNode), valueOffset);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQSetInt()
{
    QSet<int> test;
    if (!optEmptyContainers) {
        test.insert(42);
        test.insert(43);
    }
    prepareInBuffer("QSet", "local.qsetint", "local.qsetint", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}


static int dumpQMapQStringString()
{
    QMap<QString,QString> test;
    QMapNode<QString,QString> mapNode;
    const int valueOffset = (char*)&(mapNode.value) - (char*)&mapNode;
    if (!optEmptyContainers) {
        test.insert(QLatin1String("42s"), QLatin1String("fortytwo"));
        test.insert(QLatin1String("423"), QLatin1String("fortytree"));
    }
    prepareInBuffer("QMap", "local.qmapqstringqstring", "local.qmapqstringqstring", "QString@QString");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(QString), sizeof(QString), sizeof(mapNode), valueOffset);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQVariant()
{
    QVariant test = QLatin1String("item");
    prepareInBuffer("QVariant", "local.qvariant", "local.qvariant", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    fputs("\n\n", stdout);
    test = QVariant(int(42));
    prepareInBuffer("QVariant", "local.qvariant", "local.qvariant", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    fputs("\n\n", stdout);
    test = QVariant(double(3.141));
    prepareInBuffer("QVariant", "local.qvariant", "local.qvariant", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    fputs("\n\n", stdout);
    test = QVariant(QStringList(QLatin1String("item1")));
    prepareInBuffer("QVariant", "local.qvariant", "local.qvariant", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    test = QVariant(QRect(1,2, 3, 4));
    prepareInBuffer("QVariant", "local.qvariant", "local.qvariant", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    return 0;
}

static int dumpQVariantList()
{
    QVariantList test;
    if (!optEmptyContainers) {
        test.push_back(QVariant(QLatin1String("hallo")));
        test.push_back(QVariant(42));
        test.push_back(QVariant(3.141));
    }
    // As a list
    prepareInBuffer("QList", "local.qvariantlist", "local.qvariantlist", "QVariant");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(QVariant), 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    // As typedef
    fputs("\n\n", stdout);
    prepareInBuffer("QVariantList", "local.qvariantlist", "local.qvariantlist", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0,0 ,0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

// ---------------  std types

static int dumpStdString()
{
    std::string test = "hallo";
    prepareInBuffer("std::string", "local.string", "local.string", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdWString()
{
    std::wstring test = L"hallo";
    prepareInBuffer("std::wstring", "local.wstring", "local.wstring", "");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdStringList()
{
    std::list<std::string> test;
    if (!optEmptyContainers) {
        test.push_back("item1");
        test.push_back("item2");
    }
    prepareInBuffer("std::list", "local.stringlist", "local.stringlist", "std::string");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(std::string), sizeof(std::list<std::string>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdStringQList()
{
    QList<std::string> test;
    if (!optEmptyContainers) {
        test.push_back("item1");
        test.push_back("item2");
    }
    prepareInBuffer("QList", "local.stringqlist", "local.stringqlist", "std::string");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(std::string), 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdIntList()
{
    std::list<int> test;
    if (!optEmptyContainers) {
        test.push_back(1);
        test.push_back(2);
    }
    prepareInBuffer("std::list", "local.intlist", "local.intlist", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdIntVector()
{
    std::vector<int> test;
    if (!optEmptyContainers) {
        test.push_back(1);
        test.push_back(2);
    }
    prepareInBuffer("std::vector", "local.intvector", "local.intvector", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdStringVector()
{
    std::vector<std::string> test;
    if (!optEmptyContainers) {
        test.push_back("item1");
        test.push_back("item2");
    }
    prepareInBuffer("std::vector", "local.stringvector", "local.stringvector", "std::string");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(std::string), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdWStringVector()
{
    std::vector<std::wstring> test;
    if (!optEmptyContainers) {
        test.push_back(L"item1");
        test.push_back(L"item2");
    }
    prepareInBuffer("std::vector", "local.wstringvector", "local.wstringvector", "std::wstring");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(std::wstring), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdIntSet()
{
    std::set<int> test;
    if (!optEmptyContainers) {
        test.insert(1);
        test.insert(2);
    }
    prepareInBuffer("std::set", "local.intset", "local.intset", "int");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdStringSet()
{
    std::set<std::string> test;
    if (!optEmptyContainers) {
        test.insert("item1");
        test.insert("item2");
    }
    prepareInBuffer("std::set", "local.stringset", "local.stringset", "std::string");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(std::string), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdQStringSet()
{
    std::set<QString> test;
    if (!optEmptyContainers) {
        test.insert(QLatin1String("item1"));
        test.insert(QLatin1String("item2"));
    }
    prepareInBuffer("std::set", "local.stringset", "local.stringset", "QString");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(QString), sizeof(std::list<int>::allocator_type), 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdMapIntString()
{
    std::map<int,std::string> test;
    std::map<int,std::string>::value_type entry(42, std::string("fortytwo"));
    if (!optEmptyContainers) {
        test.insert(entry);
    }
    const int valueOffset = (char*)&(entry.second) - (char*)&entry;
    prepareInBuffer("std::map", "local.stdmapintstring", "local.stdmapintstring",
                    "int@std::basic_string<char,std::char_traits<char>,std::allocator<char> >@std::less<int>@std::allocator<std::pair<const int,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(int), sizeof(std::string), valueOffset, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpStdMapStringString()
{
    typedef std::map<std::string,std::string> TestType;
    TestType test;
    const TestType::value_type entry("K", "V");
    if (!optEmptyContainers) {
        test.insert(entry);
    }
    const int valueOffset = (char*)&(entry.second) - (char*)&entry;
    prepareInBuffer("std::map", "local.stdmapstringstring", "local.stdmapstringstring",
                    "std::basic_string<char,std::char_traits<char>,std::allocator<char> >@std::basic_string<char,std::char_traits<char>,std::allocator<char> >@std::less<int>@std::allocator<std::pair<const std::basic_string<char,std::char_traits<char>,std::allocator<char> >,std::basic_string<char,std::char_traits<char>,std::allocator<char> > > >");
    qDumpObjectData440(2, 42, testAddress(&test), 1, sizeof(std::string), sizeof(std::string), valueOffset, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQObject()
{
    // Requires the childOffset to be know, but that is not critical
    QAction action(0);
    QObject x;
    QAction *a2= new QAction(&action);
    a2->setObjectName(QLatin1String("a2"));
    action.setObjectName(QLatin1String("action"));
    QObject::connect(&action, SIGNAL(triggered()), &x, SLOT(deleteLater()));
    prepareInBuffer("QObject", "local.qobject", "local.qobject", "");
    qDumpObjectData440(2, 42, testAddress(&action), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputs("\n\n", stdout);
    // Property list
    prepareInBuffer("QObjectPropertyList", "local.qobjectpropertylist", "local.qobjectpropertylist", "");
    qDumpObjectData440(2, 42, testAddress(&action), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputs("\n\n", stdout);
    // Signal list
    prepareInBuffer("QObjectSignalList", "local.qobjectsignallist", "local.qobjectsignallist", "");
    qDumpObjectData440(2, 42, testAddress(&action), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    // Slot list
    prepareInBuffer("QObjectSlotList", "local.qobjectslotlist", "local.qobjectslotlist", "");
    qDumpObjectData440(2, 42, testAddress(&action), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputs("\n\n", stdout);
    // Signal list
    prepareInBuffer("QObjectChildList", "local.qobjectchildlist", "local.qobjectchildlist", "");
    qDumpObjectData440(2, 42, testAddress(&action), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    return 0;
}

static int dumpQFileInfo()
{
    QFileInfo test(QString::fromLatin1(appPath));
    prepareInBuffer("QFileInfo", "local.qfileinfo", "local.qfileinfo","");
    qDumpObjectData440(2, 42, testAddress(&test), 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    return 0;
}

static int dumpQObjectList()
{
    // Requires the childOffset to be know, but that is not critical
    QObject *root = new QObject;
    root ->setObjectName("root");
    QTimer *t1 = new QTimer;
    t1 ->setObjectName("t1");
    QTimer *t2 = new QTimer;
    t2 ->setObjectName("t2");
    QObjectList test;
    test << root << t1 << t2;
    prepareInBuffer("QList", "local.qobjectlist", "local.qobjectlist", "QObject *");
    qDumpObjectData440(2, 42, testAddress(&test), sizeof(QObject*), 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    delete root;
    return 0;
}

typedef int (*DumpFunction)();
typedef QMap<QString, DumpFunction> TypeDumpFunctionMap;

static TypeDumpFunctionMap registerTypes()
{
    TypeDumpFunctionMap rc;
    rc.insert("QString", dumpQString);
    rc.insert("QSharedPointer<QString>", dumpQSharedPointerQString);
    rc.insert("QStringList", dumpQStringList);
    rc.insert("QList<int>", dumpQIntList);
    rc.insert("QLinkedList<int>", dumpQIntLinkedList);
    rc.insert("QList<std::string>", dumpStdStringQList);
    rc.insert("QVector<int>", dumpQIntVector);
    rc.insert("QVector<QString>", dumpQQStringVector);
    rc.insert("QMap<int,QString>", dumpQMapIntString);
    rc.insert("QMap<QString,QString>", dumpQMapQStringString);
    rc.insert("QMap<int,int>", dumpQMapIntInt);
    rc.insert("QSet<int>", dumpQSetInt);
    rc.insert("string", dumpStdString);
    rc.insert("wstring", dumpStdWString);
    rc.insert("list<int>", dumpStdIntList);
    rc.insert("list<string>", dumpStdStringList);
    rc.insert("vector<int>", dumpStdIntVector);
    rc.insert("vector<string>", dumpStdStringVector);
    rc.insert("vector<wstring>", dumpStdWStringVector);
    rc.insert("set<int>", dumpStdIntSet);
    rc.insert("set<string>", dumpStdStringSet);
    rc.insert("set<QString>", dumpStdQStringSet);
    rc.insert("map<int,string>", dumpStdMapIntString);
    rc.insert("map<string,string>", dumpStdMapStringString);
    rc.insert("QFileInfo", dumpQFileInfo);
    rc.insert("QObject", dumpQObject);
    rc.insert("QObjectList", dumpQObjectList);
    rc.insert("QVariant", dumpQVariant);
    rc.insert("QVariantList", dumpQVariantList);
    return rc;
}

static void usage(const char *b, const TypeDumpFunctionMap &tdm)
{
    printf("Usage: %s [-v][-u][-e] <type1> <type2..>\n", b);
    printf("Usage: %s [-v][-u][-e] -a excluded_type1 <excluded_type2...>\n", b);
    printf("Options:  -u  Test uninitialized memory\n");
    printf("          -e  Empty containers\n");
    printf("          -v  Verbose\n");
    printf("          -a  Test all available types\n");
    printf("Supported types: ");
    const TypeDumpFunctionMap::const_iterator cend = tdm.constEnd();
    for (TypeDumpFunctionMap::const_iterator it = tdm.constBegin(); it != cend; ++it) {
        fputs(qPrintable(it.key()), stdout);
        fputc(' ', stdout);
    }
    fputc('\n', stdout);
}

int main(int argc, char *argv[])
{
    appPath = argv[0];
    printf("\nQt Creator Debugging Helper testing tool\n\n");
    printf("Running query protocol\n");
    qDumpObjectData440(1, 42, 0, 1, 0, 0, 0, 0);
    fputs(qDumpOutBuffer, stdout);
    fputc('\n', stdout);
    fputc('\n', stdout);

    const TypeDumpFunctionMap tdm = registerTypes();
    const TypeDumpFunctionMap::const_iterator cend = tdm.constEnd();

    if (argc < 2) {
        usage(argv[0], tdm);
        return 0;
    }
    // Parse args
    QStringList tests;
    for (int a = 1; a < argc; a++) {
        const char *arg = argv[a];
        if (arg[0] == '-') {
            switch (arg[1]) {
            case 'a':
                optTestAll = true;
                break;
            case 'u':
                optTestUninitialized = true;
                break;
            case 'v':
                optVerbose++;
                break;
            case 'e':
                optEmptyContainers = true;
                break;
            default:
                fprintf(stderr, "Invalid option %s\n", arg);
                usage(argv[0], tdm);
                return -1;
            }
        } else {
            tests.push_back(QLatin1String(arg));
        }
    }
    // Go
    int rc = 0;
    if (optTestAll) {
        for (TypeDumpFunctionMap::const_iterator it = tdm.constBegin(); it != cend; ++it) {
            const QString test = it.key();
            if (tests.contains(test)) {
                printf("\nSkipping: %s\n", qPrintable(test));
            } else {
                printf("\nTesting: %s\n", qPrintable(test));
                rc += (*it.value())();
                if (optTestUninitialized)
                    printf("Survived: %s\n", qPrintable(test));
            }
        }
    } else {
        foreach(const QString &test, tests) {
            printf("\nTesting: %s\n", qPrintable(test));
            const TypeDumpFunctionMap::const_iterator it = tdm.constFind(test);
            if (it == cend) {
                rc = -1;
                fprintf(stderr, "\nUnhandled type: %s\n", qPrintable(test));
            } else {
                rc = (*it.value())();
            }
        }
    }
    return rc;
}
