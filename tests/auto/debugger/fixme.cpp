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

// This file contains parts of old test code which should be
// incorporated into tst_dumpers.cpp



/////////////////////////////////////////////////////////////////////////
//
// Dumper Tests
//
/////////////////////////////////////////////////////////////////////////

/*
    Foo:
        int a, b;
        char x[6];
        typedef QMap<QString, QString> Map;
        Map m;
        QHash<QObject *, Map::iterator> h;
*/

void dump_Foo()
{
    /* A */ Foo f;
    /* B */ f.doit();
    /* D */ (void) 0;
}

void tst_Gdb::dump_Foo()
{
    prepare("dump_Foo");
    next();
    check("B","{iname='local.f',name='f',type='Foo',"
            "value='-',numchild='5'}", "", 0);
    check("B","{iname='local.f',name='f',type='Foo',"
            "value='-',numchild='5',children=["
            "{name='a',type='int',value='0',numchild='0'},"
            "{name='b',type='int',value='2',numchild='0'},"
            "{name='x',type='char [6]',"
                "value='{...}',numchild='1'},"
            "{name='m',type='" NS "QMap<" NS "QString, " NS "QString>',"
                "value='{...}',numchild='1'},"
            "{name='h',type='" NS "QHash<" NS "QObject*, "
                "" NS "QMap<" NS "QString, " NS "QString>::iterator>',"
                "value='{...}',numchild='1'}]}",
            "local.f", 0);
}


///////////////////////////// Array ///////////////////////////////////////

void dump_array_char()
{
    /* A */ const char s[] = "XYZ";
    /* B */ (void) &s; }

void dump_array_int()
{
    /* A */ int s[] = {1, 2, 3};
    /* B */ (void) s; }

void tst_Gdb::dump_array()
{
    prepare("dump_array_char");
    next();
    // FIXME: numchild should be '4', not '1'
    check("B","{iname='local.s',name='s',type='char [4]',"
            "value='-',numchild='1'}", "");
    check("B","{iname='local.s',name='s',type='char [4]',"
            "value='-',numchild='1',childtype='char',childnumchild='0',"
            "children=[{value='88 'X''},{value='89 'Y''},{value='90 'Z''},"
            "{value='0 '\\\\000''}]}",
            "local.s");

    prepare("dump_array_int");
    next();
    // FIXME: numchild should be '3', not '1'
    check("B","{iname='local.s',name='s',type='int [3]',"
            "value='-',numchild='1'}", "");
    check("B","{iname='local.s',name='s',type='int [3]',"
            "value='-',numchild='1',childtype='int',childnumchild='0',"
            "children=[{value='1'},{value='2'},{value='3'}]}",
            "local.s");
}


///////////////////////////// Misc stuff /////////////////////////////////

void dump_misc()
{
#if 1
    /* A */ int *s = new int(1);
    /* B */ *s += 1;
    /* D */ (void) 0;
#else
    QVariant v1(QLatin1String("hallo"));
    QVariant v2(QStringList(QLatin1String("hallo")));
    QVector<QString> vec;
    vec.push_back("Hallo");
    vec.push_back("Hallo2");
    std::set<std::string> stdSet;
    stdSet.insert("s1");
#    ifdef QT_GUI_LIB
    QWidget *ww = 0; //this;
    QWidget &wwr = *ww;
    Q_UNUSED(wwr);
#    endif

    QSharedPointer<QString> sps(new QString("hallo"));
    QList<QSharedPointer<QString> > spsl;
    spsl.push_back(sps);
    QMap<QString,QString> stringmap;
    QMap<int,int> intmap;
    std::map<std::string, std::string> stdstringmap;
    stdstringmap[std::string("A")]  = std::string("B");
    int xxx = 45;

    if (1 == 1) {
        int xxx = 7;
        qDebug() << xxx;
    }

    QLinkedList<QString> lls;
    lls << "link1" << "link2";
#    ifdef QT_GUI_LIB
    QStandardItemModel *model = new QStandardItemModel;
    model->appendRow(new QStandardItem("i1"));
#    endif

    QList <QList<int> > nestedIntList;
    nestedIntList << QList<int>();
    nestedIntList.front() << 1 << 2;

    QVariantList vList;
    vList.push_back(QVariant(42));
    vList.push_back(QVariant("HALLO"));


    stringmap.insert("A", "B");
    intmap.insert(3,4);
    QSet<QString> stringSet;
    stringSet.insert("S1");
    stringSet.insert("S2");
    qDebug() << *(spsl.front()) << xxx;
#endif
}


void tst_Gdb::dump_misc()
{
    prepare("dump_misc");
    next();
    check("B","{iname='local.s',name='s',type='int *',"
            "value='-',numchild='1'}", "", 0);
    //check("B","{iname='local.s',name='s',type='int *',"
    //        "value='1',numchild='0'}", "local.s,local.model", 0);
    check("B","{iname='local.s',name='s',type='int *',"
            "value='-',numchild='1',children=["
            "{name='*',type='int',value='1',numchild='0'}]}",
            "local.s,local.model", 0);
}


///////////////////////////// typedef  ////////////////////////////////////

void dump_typedef()
{
    /* A */ typedef QMap<uint, double> T;
    /* B */ T t;
    /* C */ t[11] = 13.0;
    /* D */ (void) 0;
}

void tst_Gdb::dump_typedef()
{
    prepare("dump_typedef");
    next(2);
    check("D","{iname='local.t',name='t',type='T',"
            //"basetype='" NS "QMap<unsigned int, double>',"
            "value='-',numchild='1',"
            "childtype='double',childnumchild='0',"
            "children=[{name='11',value='13'}]}", "local.t");
}

#if 0
void tst_Gdb::dump_QAbstractItemHelper(QModelIndex &index)
{
    const QAbstractItemModel *model = index.model();
    const QString &rowStr = N(index.row());
    const QString &colStr = N(index.column());
    const QByteArray &internalPtrStrSymbolic = ptrToBa(index.internalPointer());
    const QByteArray &internalPtrStrValue = ptrToBa(index.internalPointer(), false);
    const QByteArray &modelPtrStr = ptrToBa(model);
    QByteArray indexSpecSymbolic = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrSymbolic + "," + modelPtrStr);
    QByteArray indexSpecValue = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrValue + "," + modelPtrStr);
    QByteArray expected = QByteArray("tiname='iname',addr='").append(ptrToBa(&index)).
        append("',type='" NS "QAbstractItem',addr='$").append(indexSpecSymbolic).
        append("',value='").append(valToString(model->data(index).toString())).
        append("',numchild='1',children=[");
    int rowCount = model->rowCount(index);
    int columnCount = model->columnCount(index);
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            const QModelIndex &childIndex = model->index(row, col, index);
            expected.append("{name='[").append(valToString(row)).append(",").
                append(N(col)).append("]',numchild='1',addr='$").
                append(N(childIndex.row())).append(",").
                append(N(childIndex.column())).append(",").
                append(ptrToBa(childIndex.internalPointer())).append(",").
                append(modelPtrStr).append("',type='" NS "QAbstractItem',value='").
                append(valToString(model->data(childIndex).toString())).append("'}");
            if (col < columnCount - 1 || row < rowCount - 1)
                expected.append(",");
        }
    }
    expected.append("]");
    testDumper(expected, &index, NS"QAbstractItem", true, indexSpecValue);
}
#endif

class PseudoTreeItemModel : public QAbstractItemModel
{
public:
    PseudoTreeItemModel() : QAbstractItemModel(), parent1(0),
        parent1Child(1), parent2(10), parent2Child1(11), parent2Child2(12)
    {}

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        Q_UNUSED(parent);
        return 1;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        return !index.isValid() || role != Qt::DisplayRole ?
                QVariant() : *static_cast<int *>(index.internalPointer());
    }

    QModelIndex index(int row, int column,
                      const QModelIndex & parent = QModelIndex()) const
    {
        QModelIndex index;
        if (column == 0) {
            if (!parent.isValid()) {
                if (row == 0)
                    index = createIndex(row, column, &parent1);
                else if (row == 1)
                    index = createIndex(row, column, &parent2);
            } else if (parent.internalPointer() == &parent1 && row == 0) {
                index = createIndex(row, column, &parent1Child);
            } else if (parent.internalPointer() == &parent2) {
                index = createIndex(row, column,
                            row == 0 ? &parent2Child1 : &parent2Child2);
            }
        }
        return index;
    }

    QModelIndex parent(const QModelIndex & index) const
    {
        QModelIndex parent;
        if (index.isValid()) {
            if (index.internalPointer() == &parent1Child)
                parent = createIndex(0, 0, &parent1);
            else if (index.internalPointer() == &parent2Child1 ||
                     index.internalPointer() == &parent2Child2)
                parent = createIndex(1, 0, &parent2);
        }
        return parent;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        int rowCount;
        if (!parent.isValid() || parent.internalPointer() == &parent2)
            rowCount = 2;
        else if (parent.internalPointer() == &parent1)
            rowCount = 1;
        else
            rowCount = 0;
        return rowCount;
    }

private:
    mutable int parent1;
    mutable int parent1Child;
    mutable int parent2;
    mutable int parent2Child1;
    mutable int parent2Child2;
};



///////////////////////////// QAbstractItemAndModelIndex //////////////////////////

//    /* A */ QStringListModel m(QStringList() << "item1" << "item2" << "item3");
//    /* B */ index = m.index(2, 0);
void dump_QAbstractItemAndModelIndex()
{
    /* A */ PseudoTreeItemModel m2; QModelIndex index;
    /* C */ index = m2.index(0, 0);
    /* D */ index = m2.index(1, 0);
    /* E */ index = m2.index(0, 0, index);
    /* F */ (void) index.row();
}

void tst_Gdb::dump_QAbstractItemAndModelIndex()
{
    prepare("dump_QAbstractItemAndModelIndex");
    if (checkUninitialized)
        check("A", "");
    next();
    check("C", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
            "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
            "value='(invalid)',numchild='0'}",
        "local.index");
    next();
    check("D", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
           "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
          "value='(0, 0)',numchild='5',children=["
            "{name='row',value='0',type='int',numchild='0'},"
            "{name='column',value='0',type='int',numchild='0'},"
            "{name='parent',type='" NS "QModelIndex',value='(invalid)',numchild='0'},"
            "{name='model',value='-',type='" NS "QAbstractItemModel*',numchild='1'}]}",
        "local.index");
    next();
    check("E", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
           "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
          "value='(1, 0)',numchild='5',children=["
            "{name='row',value='1',type='int',numchild='0'},"
            "{name='column',value='0',type='int',numchild='0'},"
            "{name='parent',type='" NS "QModelIndex',value='(invalid)',numchild='0'},"
            "{name='model',value='-',type='" NS "QAbstractItemModel*',numchild='1'}]}",
        "local.index");
    next();
    check("F", "{iname='local.m2',name='m2',type='PseudoTreeItemModel',"
           "value='{...}',numchild='6'},"
        "{iname='local.index',name='index',type='" NS "QModelIndex',"
          "value='(0, 0)',numchild='5',children=["
            "{name='row',value='0',type='int',numchild='0'},"
            "{name='column',value='0',type='int',numchild='0'},"
            "{name='parent',type='" NS "QModelIndex',value='(1, 0)',numchild='5'},"
            "{name='model',value='-',type='" NS "QAbstractItemModel*',numchild='1'}]}",
        "local.index");
}

/*
QByteArray dump_QAbstractItemModelHelper(QAbstractItemModel &m)
{
    QByteArray address = ptrToBa(&m);
    QByteArray expected = QByteArray("tiname='iname',"
        "type='" NS "QAbstractItemModel',value='(%,%)',numchild='1',children=["
        "{numchild='1',name='" NS "QObject',value='%',"
        "valueencoded='2',type='" NS "QObject',displayedtype='%'}")
            << address
            << N(m.rowCount())
            << N(m.columnCount())
            << address
            << utfToHex(m.objectName())
            << m.metaObject()->className();

    for (int row = 0; row < m.rowCount(); ++row) {
        for (int column = 0; column < m.columnCount(); ++column) {
            QModelIndex mi = m.index(row, column);
            expected.append(QByteArray(",{name='[%,%]',value='%',"
                "valueencoded='2',numchild='1',%,%,%',"
                "type='" NS "QAbstractItem'}")
                << N(row)
                << N(column)
                << utfToHex(m.data(mi).toString())
                << N(mi.row())
                << N(mi.column())
                << ptrToBa(mi.internalPointer())
                << ptrToBa(mi.model()));
        }
    }
    expected.append("]");
    return expected;
}
*/

void dump_QAbstractItemModel()
{
#    ifdef QT_GUI_LIB
    /* A */ QStringList strList;
            strList << "String 1";
            QStringListModel model1(strList);
            QStandardItemModel model2(0, 2);
    /* B */ model1.setStringList(strList);
    /* C */ strList << "String 2";
    /* D */ model1.setStringList(strList);
    /* E */ model2.appendRow(QList<QStandardItem *>() << (new QStandardItem("Item (0,0)")) << (new QStandardItem("Item (0,1)")));
    /* F */ model2.appendRow(QList<QStandardItem *>() << (new QStandardItem("Item (1,0)")) << (new QStandardItem("Item (1,1)")));
    /* G */ (void) (model1.rowCount() + model2.rowCount() + strList.size());
#    endif
}

void tst_Gdb::dump_QAbstractItemModel()
{
#    ifdef QT_GUI_LIB
    /* A */ QStringList strList;
    QString template_ =
        "{iname='local.strList',name='strList',type='" NS "QStringList',"
            "value='<%1 items>',numchild='%1'},"
        "{iname='local.model1',name='model1',type='" NS "QStringListModel',"
            "value='{...}',numchild='3'},"
        "{iname='local.model2',name='model2',type='" NS "QStandardItemModel',"
            "value='{...}',numchild='2'}";

    prepare("dump_QAbstractItemModel");
    if (checkUninitialized)
        check("A", template_.arg("1").toAscii());
    next(4);
    check("B", template_.arg("1").toAscii());
#    endif
}


///////////////////////////// QByteArray /////////////////////////////////

void dump_QByteArray()
{
    /* A */ QByteArray ba;                       // Empty object.
    /* B */ ba.append('a');                      // One element.
    /* C */ ba.append('b');                      // Two elements.
    /* D */ ba = QByteArray(101, 'a');           // > 100 elements.
    /* E */ ba = QByteArray("abc\a\n\r\033\'\"?"); // Mixed.
    /* F */ (void) 0;
}

void tst_Gdb::dump_QByteArray()
{
    prepare("dump_QByteArray");
    if (checkUninitialized)
        check("A","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='',numchild='0'}");
    next();
    check("C","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='61',numchild='1'}");
    next();
    check("D","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='6162',numchild='2'}");
    next();
    check("E","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='616161616161616161616161616161616161"
            "616161616161616161616161616161616161616161616161616161616161"
            "616161616161616161616161616161616161616161616161616161616161"
            "6161616161616161616161616161616161616161616161',numchild='101'}");
    next();
    check("F","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='616263070a0d1b27223f',numchild='10'}");
    check("F","{iname='local.ba',name='ba',type='" NS "QByteArray',"
            "valueencoded='6',value='616263070a0d1b27223f',numchild='10',"
            "childtype='char',childnumchild='0',"
            "children=[{value='97 'a''},{value='98 'b''},"
            "{value='99 'c''},{value='7 '\\\\a''},"
            "{value='10 '\\\\n''},{value='13 '\\\\r''},"
            "{value='27 '\\\\033''},{value='39 '\\\\'''},"
            "{value='34 ''''},{value='63 '?''}]}",
            "local.ba");
}


///////////////////////////// QChar /////////////////////////////////

void dump_QChar()
{
    /* A */ QChar c('X');               // Printable ASCII character.
    /* B */ c = QChar(0x600);           // Printable non-ASCII character.
    /* C */ c = QChar::fromAscii('\a'); // Non-printable ASCII character.
    /* D */ c = QChar(0x9f);            // Non-printable non-ASCII character.
    /* E */ c = QChar::fromAscii('?');  // The replacement character.
    /* F */ (void) 0; }

void tst_Gdb::dump_QChar()
{
    prepare("dump_QChar");
    next();

    // Case 1: Printable ASCII character.
    check("B","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''X' (88)',numchild='0'}");
    next();

    // Case 2: Printable non-ASCII character.
    check("C","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (1536)',numchild='0'}");
    next();

    // Case 3: Non-printable ASCII character.
    check("D","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (7)',numchild='0'}");
    next();

    // Case 4: Non-printable non-ASCII character.
    check("E","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (159)',numchild='0'}");
    next();

    // Case 5: Printable ASCII Character that looks like the replacement character.
    check("F","{iname='local.c',name='c',type='" NS "QChar',"
        "value=''?' (63)',numchild='0'}");
}


///////////////////////////// QDateTime /////////////////////////////////

void dump_QDateTime()
{
#    ifndef QT_NO_DATESTRING
    /* A */ QDateTime d;
    /* B */ d = QDateTime::fromString("M5d21y7110:31:02", "'M'M'd'd'y'yyhh:mm:ss");
    /* C */ (void) d.isNull();
#    endif
}

void tst_Gdb::dump_QDateTime()
{
#    ifndef QT_NO_DATESTRING
    prepare("dump_QDateTime");
    if (checkUninitialized)
        check("A","{iname='local.d',name='d',"
            "type='" NS "QDateTime',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B", "{iname='local.d',name='d',type='" NS "QDateTime',"
          "valueencoded='7',value='-',numchild='3',children=["
            "{name='isNull',type='bool',value='true',numchild='0'},"
            "{name='toTime_t',type='unsigned int',value='4294967295',numchild='0'},"
            "{name='toString',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='(ISO)',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='(SystemLocale)',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='(Locale)',type='myns::QString',valueencoded='7',"
                "value='',numchild='0'},"
            "{name='toUTC',type='myns::QDateTime',valueencoded='7',"
                "value='',numchild='3'},"
            "{name='toLocalTime',type='myns::QDateTime',valueencoded='7',"
                "value='',numchild='3'}"
            "]}",
          "local.d");
    next();
    check("C", "{iname='local.d',name='d',type='" NS "QDateTime',"
          "valueencoded='7',value='-',"
                "numchild='3',children=["
            "{name='isNull',type='bool',value='false',numchild='0'},"
            "{name='toTime_t',type='unsigned int',value='43666262',numchild='0'},"
            "{name='toString',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='(ISO)',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='(SystemLocale)',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='(Locale)',type='myns::QString',valueencoded='7',"
                "value='-',numchild='0'},"
            "{name='toUTC',type='myns::QDateTime',valueencoded='7',"
                "value='-',numchild='3'},"
            "{name='toLocalTime',type='myns::QDateTime',valueencoded='7',"
                "value='-',numchild='3'}"
            "]}",
          "local.d");
#    endif
}


///////////////////////////// QDir /////////////////////////////////

void dump_QDir()
{
    /* A */ QDir dir = QDir::current(); // Case 1: Current working directory.
    /* B */ dir = QDir::root();         // Case 2: Root directory.
    /* C */ (void) dir.absolutePath(); }

void tst_Gdb::dump_QDir()
{
    prepare("dump_QDir");
    if (checkUninitialized)
        check("A","{iname='local.dir',name='dir',"
            "type='" NS "QDir',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B", "{iname='local.dir',name='dir',type='" NS "QDir',"
               "valueencoded='7',value='-',numchild='2',children=["
                  "{name='absolutePath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                  "{name='canonicalPath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'}]}",
                "local.dir");
    next();
    check("C", "{iname='local.dir',name='dir',type='" NS "QDir',"
               "valueencoded='7',value='-',numchild='2',children=["
                  "{name='absolutePath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                  "{name='canonicalPath',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'}]}",
                "local.dir");
}


///////////////////////////// QFile /////////////////////////////////

void dump_QFile()
{
    /* A */ QFile file1(""); // Case 1: Empty file name => Does not exist.
    /* B */ QTemporaryFile file2; // Case 2: File that is known to exist.
            file2.open();
    /* C */ QFile file3("jfjfdskjdflsdfjfdls");
    /* D */ (void) (file1.fileName() + file2.fileName() + file3.fileName()); }

void tst_Gdb::dump_QFile()
{
    prepare("dump_QFile");
    next(4);
    check("D", "{iname='local.file1',name='file1',type='" NS "QFile',"
            "valueencoded='7',value='',numchild='2',children=["
                "{name='fileName',type='" NS "QString',"
                    "valueencoded='7',value='',numchild='0'},"
                "{name='exists',type='bool',value='false',numchild='0'}"
            "]},"
        "{iname='local.file2',name='file2',type='" NS "QTemporaryFile',"
            "valueencoded='7',value='-',numchild='2',children=["
                "{name='fileName',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                "{name='exists',type='bool',value='true',numchild='0'}"
            "]},"
        "{iname='local.file3',name='file3',type='" NS "QFile',"
            "valueencoded='7',value='-',numchild='2',children=["
                "{name='fileName',type='" NS "QString',"
                    "valueencoded='7',value='-',numchild='0'},"
                "{name='exists',type='bool',value='false',numchild='0'}"
            "]}",
        "local.file1,local.file2,local.file3");
}


///////////////////////////// QFileInfo /////////////////////////////////

void dump_QFileInfo()
{
    /* A */ QFileInfo fi(".");
    /* B */ (void) fi.baseName().size();
}

void tst_Gdb::dump_QFileInfo()
{
    QFileInfo fi(".");
    prepare("dump_QFileInfo");
    next();
    check("B", "{iname='local.fi',name='fi',type='" NS "QFileInfo',"
        "valueencoded='7',value='" + utfToHex(fi.filePath()) + "',numchild='3',"
        "childtype='" NS "QString',childnumchild='0',children=["
        "{name='absolutePath'," + specQString(fi.absolutePath()) + "},"
        "{name='absoluteFilePath'," + specQString(fi.absoluteFilePath()) + "},"
        "{name='canonicalPath'," + specQString(fi.canonicalPath()) + "},"
        "{name='canonicalFilePath'," + specQString(fi.canonicalFilePath()) + "},"
        "{name='completeBaseName'," + specQString(fi.completeBaseName()) + "},"
        "{name='completeSuffix'," + specQString(fi.completeSuffix()) + "},"
        "{name='baseName'," + specQString(fi.baseName()) + "},"
#if 0
        "{name='isBundle'," + specBool(fi.isBundle()) + "},"
        "{name='bundleName'," + specQString(fi.bundleName()) + "},"
#endif
        "{name='fileName'," + specQString(fi.fileName()) + "},"
        "{name='filePath'," + specQString(fi.filePath()) + "},"
        //"{name='group'," + specQString(fi.group()) + "},"
        //"{name='owner'," + specQString(fi.owner()) + "},"
        "{name='path'," + specQString(fi.path()) + "},"
        "{name='groupid',type='unsigned int',value='" + N(fi.groupId()) + "'},"
        "{name='ownerid',type='unsigned int',value='" + N(fi.ownerId()) + "'},"
        "{name='permissions',value=' ',type='" NS "QFile::Permissions',numchild='10'},"
        "{name='caching',type='bool',value='true'},"
        "{name='exists',type='bool',value='true'},"
        "{name='isAbsolute',type='bool',value='false'},"
        "{name='isDir',type='bool',value='true'},"
        "{name='isExecutable',type='bool',value='true'},"
        "{name='isFile',type='bool',value='false'},"
        "{name='isHidden',type='bool',value='false'},"
        "{name='isReadable',type='bool',value='true'},"
        "{name='isRelative',type='bool',value='true'},"
        "{name='isRoot',type='bool',value='false'},"
        "{name='isSymLink',type='bool',value='false'},"
        "{name='isWritable',type='bool',value='true'},"
        "{name='created',type='" NS "QDateTime',"
            + specQString(fi.created().toString()) + ",numchild='3'},"
        "{name='lastModified',type='" NS "QDateTime',"
            + specQString(fi.lastModified().toString()) + ",numchild='3'},"
        "{name='lastRead',type='" NS "QDateTime',"
            + specQString(fi.lastRead().toString()) + ",numchild='3'}]}",
        "local.fi");
}


#if 0
void tst_Gdb::dump_QImageDataHelper(QImage &img)
{
    const QByteArray ba(QByteArray::fromRawData((const char*) img.bits(), img.numBytes()));
    QByteArray expected = QByteArray("tiname='$I',addr='$A',type='" NS "QImageData',").
        append("numchild='0',value='<hover here>',valuetooltipencoded='1',").
        append("valuetooltipsize='").append(N(ba.size())).append("',").
        append("valuetooltip='").append(ba.toBase64()).append("'");
    testDumper(expected, &img, NS"QImageData", false);
}

#endif // 0


///////////////////////////// QImage /////////////////////////////////

void dump_QImage()
{
#    ifdef QT_GUI_LIB
    /* A */ QImage image; // Null image.
    /* B */ image = QImage(30, 700, QImage::Format_RGB555); // Normal image.
    /* C */ image = QImage(100, 0, QImage::Format_Invalid); // Invalid image.
    /* D */ (void) image.size();
    /* E */ (void) image.size();
#    endif
}

void tst_Gdb::dump_QImage()
{
#    ifdef QT_GUI_LIB
    prepare("dump_QImage");
    next();
    check("B", "{iname='local.image',name='image',type='" NS "QImage',"
        "value='(null)',numchild='0'}");
    next();
    check("C", "{iname='local.image',name='image',type='" NS "QImage',"
        "value='(30x700)',numchild='0'}");
    next(2);
    // FIXME:
    //check("E", "{iname='local.image',name='image',type='" NS "QImage',"
    //    "value='(100x0)',numchild='0'}");
#    endif
}

#if 0
void tst_Gdb::dump_QImageData()
{
    // Case 1: Null image.
    QImage img;
    dump_QImageDataHelper(img);

    // Case 2: Normal image.
    img = QImage(3, 700, QImage::Format_RGB555);
    dump_QImageDataHelper(img);

    // Case 3: Invalid image.
    img = QImage(100, 0, QImage::Format_Invalid);
    dump_QImageDataHelper(img);
}
#endif

void dump_QLocale()
{
    /* A */ QLocale german(QLocale::German);
            QLocale chinese(QLocale::Chinese);
            QLocale swahili(QLocale::Swahili);
    /* C */ (void) (german.name() + chinese.name() + swahili.name());
}

QByteArray dump_QLocaleHelper(const QLocale &loc)
{
    return "type='" NS "QLocale',valueencoded='7',value='" + utfToHex(loc.name()) +
            "',numchild='8',childtype='" NS "QChar',childnumchild='0',children=["
         "{name='country',type='" NS "QLocale::Country',value='-'},"
         "{name='language',type='" NS "QLocale::Language',value='-'},"
         "{name='measurementSystem',type='" NS "QLocale::MeasurementSystem',"
            "value='-'},"
         "{name='numberOptions',type='" NS "QFlags<myns::QLocale::NumberOption>',"
            "value='-'},"
         "{name='timeFormat_(short)',type='" NS "QString',"
            + specQString(loc.timeFormat(QLocale::ShortFormat)) + "},"
         "{name='timeFormat_(long)',type='" NS "QString',"
            + specQString(loc.timeFormat()) + "},"
         "{name='decimalPoint'," + specQChar(loc.decimalPoint()) + "},"
         "{name='exponential'," + specQChar(loc.exponential()) + "},"
         "{name='percent'," + specQChar(loc.percent()) + "},"
         "{name='zeroDigit'," + specQChar(loc.zeroDigit()) + "},"
         "{name='groupSeparator'," + specQChar(loc.groupSeparator()) + "},"
         "{name='negativeSign'," + specQChar(loc.negativeSign()) + "}]";
}

void tst_Gdb::dump_QLocale()
{
    QLocale german(QLocale::German);
    QLocale chinese(QLocale::Chinese);
    QLocale swahili(QLocale::Swahili);
    prepare("dump_QLocale");
    if (checkUninitialized)
        check("A","{iname='local.english',name='english',"
            "type='" NS "QLocale',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("C", "{iname='local.german',name='german',"
            + dump_QLocaleHelper(german) + "},"
        "{iname='local.chinese',name='chinese',"
            + dump_QLocaleHelper(chinese) + "},"
        "{iname='local.swahili',name='swahili',"
            + dump_QLocaleHelper(swahili) + "}",
        "local.german,local.chinese,local.swahili");
}

///////////////////////////// QHash<int, int> //////////////////////////////

void dump_QHash_int_int()
{
    /* A */ QHash<int, int> h;
    /* B */ h[43] = 44;
    /* C */ h[45] = 46;
    /* D */ (void) 0;
}

void tst_Gdb::dump_QHash_int_int()
{
    // Need to check the following combinations:
    // int-key optimization, small value
    //struct NodeOS { void *next; uint k; uint  v; } nodeOS
    // int-key optimiatzion, large value
    //struct NodeOL { void *next; uint k; void *v; } nodeOL
    // no optimization, small value
    //struct NodeNS +  { void *next; uint h; uint  k; uint  v; } nodeNS
    // no optimization, large value
    //struct NodeNL { void *next; uint h; uint  k; void *v; } nodeNL
    // complex key
    //struct NodeL  { void *next; uint h; void *k; void *v; } nodeL

    prepare("dump_QHash_int_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QHash<int, int>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QHash<int, int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{name='43',value='44'},"
            "{name='45',value='46'}]}",
            "local.h");
}

///////////////////////////// QHash<QString, QString> //////////////////////////////

void dump_QHash_QString_QString()
{
    /* A */ QHash<QString, QString> h;
    /* B */ h["hello"] = "world";
    /* C */ h["foo"] = "bar";
    /* D */ (void) 0;
}

void tst_Gdb::dump_QHash_QString_QString()
{
    prepare("dump_QHash_QString_QString");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.h',name='h',"
            "type='" NS "QHash<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2',childtype='" NS "QHashNode<" NS "QString, " NS "QString>',"
            "children=["
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='66006f006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',"
                    "valueencoded='7',value='620061007200',numchild='0'}]},"
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='680065006c006c006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',valueencoded='7',"
                     "value='77006f0072006c006400',numchild='0'}]}"
            "]}",
            "local.h,local.h.0,local.h.1");
}


///////////////////////////// QLinkedList<int> ///////////////////////////////////

void dump_QLinkedList_int()
{
    /* A */ QLinkedList<int> h;
    /* B */ h.append(42);
    /* C */ h.append(44);
    /* D */ (void) 0; }

void tst_Gdb::dump_QLinkedList_int()
{
    prepare("dump_QLinkedList_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QLinkedList<int>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QLinkedList<int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='42'},{value='44'}]}", "local.h");
}


///////////////////////////// QList<int> /////////////////////////////////

void dump_QList_int()
{
    /* A */ QList<int> list;
    /* B */ list.append(1);
    /* C */ list.append(2);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QList_int()
{
    prepare("dump_QList_int");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
                "type='" NS "QList<int>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<1 items>',numchild='1',"
            "childtype='int',childnumchild='0',children=["
            "{value='1'}]}", "local.list");
    next();
    check("D","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<2 items>',numchild='2'}");
    check("D","{iname='local.list',name='list',"
            "type='" NS "QList<int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='1'},{value='2'}]}", "local.list");
}


///////////////////////////// QList<int *> /////////////////////////////////

void dump_QList_int_star()
{
    /* A */ QList<int *> list;
    /* B */ list.append(new int(1));
    /* C */ list.append(0);
    /* D */ list.append(new int(2));
    /* E */ (void) 0;
}

void tst_Gdb::dump_QList_int_star()
{
    prepare("dump_QList_int_star");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
                "type='" NS "QList<int*>',value='<invalid>',numchild='0'}");
    next();
    next();
    next();
    next();
    check("E","{iname='local.list',name='list',"
            "type='" NS "QList<int*>',value='<3 items>',numchild='3',"
            "childtype='int',childnumchild='0',children=["
            "{value='1'},{value='0x0',type='int *'},{value='2'}]}", "local.list");
}


///////////////////////////// QList<char> /////////////////////////////////

void dump_QList_char()
{
    /* A */ QList<char> list;
    /* B */ list.append('a');
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_char()
{
    prepare("dump_QList_char");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char>',value='<1 items>',numchild='1',"
            "childtype='char',childnumchild='0',children=["
            "{value='97 'a''}]}", "local.list");
}


///////////////////////////// QList<const char *> /////////////////////////////////

void dump_QList_char_star()
{
    /* A */ QList<const char *> list;
    /* B */ list.append("a");
    /* C */ list.append(0);
    /* D */ list.append("bc");
    /* E */ (void) 0;
}

void tst_Gdb::dump_QList_char_star()
{
    prepare("dump_QList_char_star");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<1 items>',numchild='1',"
            "childtype='const char *',childnumchild='1',children=["
            "{valueencoded='6',value='61',numchild='0'}]}", "local.list");
    next();
    next();
    check("E","{iname='local.list',name='list',"
            "type='" NS "QList<char const*>',value='<3 items>',numchild='3',"
            "childtype='const char *',childnumchild='1',children=["
            "{valueencoded='6',value='61',numchild='0'},"
            "{value='0x0',numchild='0'},"
            "{valueencoded='6',value='6263',numchild='0'}]}", "local.list");
}


///////////////////////////// QList<QString> /////////////////////////////////////

void dump_QList_QString()
{
    /* A */ QList<QString> list;
    /* B */ list.append("Hallo");
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_QString()
{
    prepare("dump_QList_QString");
    if (0 && checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<" NS "QString>',value='<1 items>',numchild='1',"
            "childtype='" NS "QString',childnumchild='0',children=["
            "{valueencoded='7',value='480061006c006c006f00'}]}", "local.list");
}


///////////////////////////// QList<QString3> ///////////////////////////////////

void dump_QList_QString3()
{
    /* A */ QList<QString3> list;
    /* B */ list.append(QString3());
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_QString3()
{
    prepare("dump_QList_QString3");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<1 items>',numchild='1',"
            "childtype='QString3',children=["
            "{value='{...}',numchild='3'}]}", "local.list");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<QString3>',value='<1 items>',numchild='1',"
            "childtype='QString3',children=[{value='{...}',numchild='3',children=["
                 "{name='s1',type='" NS "QString',"
                    "valueencoded='7',value='6100',numchild='0'},"
                 "{name='s2',type='" NS "QString',"
                    "valueencoded='7',value='6200',numchild='0'},"
                 "{name='s3',type='" NS "QString',"
                    "valueencoded='7',value='6300',numchild='0'}]}]}",
            "local.list,local.list.0");
}


///////////////////////////// QList<Int3> /////////////////////////////////////

void dump_QList_Int3()
{
    /* A */ QList<Int3> list;
    /* B */ list.append(Int3());
    /* C */ (void) 0;
}

void tst_Gdb::dump_QList_Int3()
{
    prepare("dump_QList_Int3");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
                "type='" NS "QList<Int3>',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<0 items>',numchild='0'}");
    next();
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<1 items>',numchild='1'}");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<1 items>',numchild='1',"
            "childtype='Int3',children=[{value='{...}',numchild='3'}]}",
            "local.list");
    check("C","{iname='local.list',name='list',"
            "type='" NS "QList<Int3>',value='<1 items>',numchild='1',"
            "childtype='Int3',children=[{value='{...}',numchild='3',children=["
             "{name='i1',type='int',value='42',numchild='0'},"
             "{name='i2',type='int',value='43',numchild='0'},"
             "{name='i3',type='int',value='44',numchild='0'}]}]}",
            "local.list,local.list.0");
}


///////////////////////////// QMap<int, int> //////////////////////////////

void dump_QMap_int_int()
{
    /* A */ QMap<int, int> h;
    /* B */ h[12] = 34;
    /* C */ h[14] = 54;
    /* D */ (void) 0;
}

void tst_Gdb::dump_QMap_int_int()
{
    prepare("dump_QMap_int_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.h',name='h',"
            "type='" NS "QMap<int, int>',value='<2 items>',"
            "numchild='2',childtype='int',childnumchild='0',"
            "children=[{name='12',value='34'},{name='14',value='54'}]}",
            "local.h,local.h.0,local.h.1");
}


///////////////////////////// QMap<QString, QString> //////////////////////////////

void dump_QMap_QString_QString()
{
    /* A */ QMap<QString, QString> m;
    /* B */ m["hello"] = "world";
    /* C */ m["foo"] = "bar";
    /* D */ (void) 0;
}

void tst_Gdb::dump_QMap_QString_QString()
{
    prepare("dump_QMap_QString_QString");
    if (checkUninitialized)
        check("A","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.m',name='m',"
            "type='" NS "QMap<" NS "QString, " NS "QString>',value='<2 items>',"
            "numchild='2',childtype='" NS "QMapNode<" NS "QString, " NS "QString>',"
            "children=["
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='66006f006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',"
                    "valueencoded='7',value='620061007200',numchild='0'}]},"
              "{value=' ',numchild='2',children=[{name='key',type='" NS "QString',"
                    "valueencoded='7',value='680065006c006c006f00',numchild='0'},"
                 "{name='value',type='" NS "QString',valueencoded='7',"
                     "value='77006f0072006c006400',numchild='0'}]}"
            "]}",
            "local.m,local.m.0,local.m.1");
}


///////////////////////////// QObject /////////////////////////////////

void dump_QObject()
{
    /* B */ QObject ob;
    /* D */ ob.setObjectName("An Object");
    /* E */ QObject::connect(&ob, SIGNAL(destroyed()), qApp, SLOT(quit()));
    /* F */ QObject::disconnect(&ob, SIGNAL(destroyed()), qApp, SLOT(quit()));
    /* G */ ob.setObjectName("A renamed Object");
    /* H */ (void) 0; }

void dump_QObject1()
{
    /* A */ QObject parent;
    /* B */ QObject child(&parent);
    /* C */ parent.setObjectName("A Parent");
    /* D */ child.setObjectName("A Child");
    /* H */ (void) 0; }

void tst_Gdb::dump_QObject()
{
    prepare("dump_QObject");
    if (checkUninitialized)
        check("A","{iname='local.ob',name='ob',"
            "type='" NS "QObject',value='<invalid>',"
            "numchild='0'}");
    next(4);

    check("G","{iname='local.ob',name='ob',type='" NS "QObject',valueencoded='7',"
      "value='41006e0020004f0062006a00650063007400',numchild='4',children=["
        "{name='parent',value='0x0',type='" NS "QObject *',"
            "numchild='0'},"
        "{name='children',type='-'," // NS"QObject{Data,}::QObjectList',"
            "value='<0 items>',numchild='0',children=[]},"
        "{name='properties',value='<1 items>',type=' ',numchild='1',children=["
            "{name='objectName',type='" NS "QString',valueencoded='7',"
                "value='41006e0020004f0062006a00650063007400',numchild='0'}]},"
        "{name='connections',value='<0 items>',type=' ',numchild='0',"
            "children=[]},"
        "{name='signals',value='<2 items>',type=' ',numchild='2',"
            "childnumchild='0',children=["
            "{name='signal 0',type=' ',value='destroyed(QObject*)'},"
            "{name='signal 1',type=' ',value='destroyed()'}]},"
        "{name='slots',value='<2 items>',type=' ',numchild='2',"
            "childnumchild='0',children=["
            "{name='slot 0',type=' ',value='deleteLater()'},"
            "{name='slot 1',type=' ',value='_q_reregisterTimers(void*)'}]}]}",
        "local.ob"
        ",local.ob.children"
        ",local.ob.properties"
        ",local.ob.connections"
        ",local.ob.signals"
        ",local.ob.slots"
    );
#if 0
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4'",
        &parent, NS"QObject", false);
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4',children=["
        "{name='properties',addr='$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',addr='$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',addr='$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',value='0x0',type='$T *',numchild='0'},"
        "{name='className',value='QObject',type='',numchild='0'}]",
        &parent, NS"QObject", true);

#if 0
    testDumper("numchild='2',value='<2 items>',type='QObjectSlotList',"
            "children=[{name='2',value='deleteLater()',"
            "numchild='0',addr='$A',type='QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',"
            "numchild='0',addr='$A',type='QObjectSlot'}]",
        &parent, NS"QObjectSlotList", true);
#endif

    parent.setObjectName("A Parent");
    testDumper("value='QQAgAFAAYQByAGUAbgB0AA==',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4'",
        &parent, NS"QObject", false);
    QObject child(&parent);
    testDumper("value='',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
    child.setObjectName("A Child");
    QByteArray ba ="value='QQAgAEMAaABpAGwAZAA=',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4',children=["
        "{name='properties',addr='$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',addr='$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',addr='$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',addr='" + str(&parent) + "',"
            "value='QQAgAFAAYQByAGUAbgB0AA==',valueencoded='2',type='$T',"
            "displayedtype='QObject',numchild='1'},"
        "{name='className',value='QObject',type='',numchild='0'}]";
    testDumper(ba, &child, NS"QObject", true);
    connect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    disconnect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    child.setObjectName("A renamed Child");
    testDumper("value='QQAgAHIAZQBuAGEAbQBlAGQAIABDAGgAaQBsAGQA',valueencoded='2',"
        "type='$T',displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
#endif
}

#if 0
void tst_Gdb::dump_QObjectChildListHelper(QObject &o)
{
    const QObjectList children = o.children();
    const int size = children.size();
    const QString sizeStr = N(size);
    QByteArray expected = QByteArray("numchild='").append(sizeStr).append("',value='<").
        append(sizeStr).append(" items>',type='" NS "QObjectChildList',children=[");
    for (int i = 0; i < size; ++i) {
        const QObject *child = children.at(i);
        expected.append("{addr='").append(ptrToBa(child)).append("',value='").
            append(utfToHex(child->objectName())).
            append("',valueencoded='2',type='" NS "QObject',displayedtype='").
            append(child->metaObject()->className()).append("',numchild='1'}");
        if (i < size - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &o, NS"QObjectChildList", true);
}

void tst_Gdb::dump_QObjectChildList()
{
    // Case 1: Object with no children.
    QObject o;
    dump_QObjectChildListHelper(o);

    // Case 2: Object with one child.
    QObject o2(&o);
    dump_QObjectChildListHelper(o);

    // Case 3: Object with two children.
    QObject o3(&o);
    dump_QObjectChildListHelper(o);
}

void tst_Gdb::dump_QObjectMethodList()
{
    QStringListModel m;
    testDumper("addr='<synthetic>',type='$T',numchild='20',"
        "childtype='" NS "QMetaMethod::Method',childnumchild='0',children=["
        "{name='0 0 destroyed(QObject*)',value='<Signal> (1)'},"
        "{name='1 1 destroyed()',value='<Signal> (1)'},"
        "{name='2 2 deleteLater()',value='<Slot> (2)'},"
        "{name='3 3 _q_reregisterTimers(void*)',value='<Slot> (2)'},"
        "{name='4 4 dataChanged(QModelIndex,QModelIndex)',value='<Signal> (1)'},"
        "{name='5 5 headerDataChanged(Qt::Orientation,int,int)',value='<Signal> (1)'},"
        "{name='6 6 layoutChanged()',value='<Signal> (1)'},"
        "{name='7 7 layoutAboutToBeChanged()',value='<Signal> (1)'},"
        "{name='8 8 rowsAboutToBeInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='9 9 rowsInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='10 10 rowsAboutToBeRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='11 11 rowsRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='12 12 columnsAboutToBeInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='13 13 columnsInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='14 14 columnsAboutToBeRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='15 15 columnsRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='16 16 modelAboutToBeReset()',value='<Signal> (1)'},"
        "{name='17 17 modelReset()',value='<Signal> (1)'},"
        "{name='18 18 submit()',value='<Slot> (2)'},"
        "{name='19 19 revert()',value='<Slot> (2)'}]",
    &m, NS"QObjectMethodList", true);
}

void tst_Gdb::dump_QObjectPropertyList()
{
    // Case 1: Model without a parent.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    testDumper("addr='<synthetic>',type='$T',numchild='1',value='<1 items>',"
        "children=[{name='objectName',type='QString',value='',"
            "valueencoded='2',numchild='0'}]",
         &m, NS"QObjectPropertyList", true);

    // Case 2: Model with a parent.
    QStringListModel m2(&m);
    testDumper("addr='<synthetic>',type='$T',numchild='1',value='<1 items>',"
        "children=[{name='objectName',type='QString',value='',"
            "valueencoded='2',numchild='0'}]",
         &m2, NS"QObjectPropertyList", true);
}

static const char *connectionType(uint type)
{
    Qt::ConnectionType connType = static_cast<Qt::ConnectionType>(type);
    const char *output = "unknown";
    switch (connType) {
        case Qt::AutoConnection: output = "auto"; break;
        case Qt::DirectConnection: output = "direct"; break;
        case Qt::QueuedConnection: output = "queued"; break;
        case Qt::BlockingQueuedConnection: output = "blockingqueued"; break;
        case 3: output = "autocompat"; break;
#if QT_VERSION >= 0x040600
        case Qt::UniqueConnection: break; // Can't happen.
#endif
        default:
            qWarning() << "Unknown connection type: " << type;
            break;
        };
    return output;
};

class Cheater : public QObject
{
public:
    static const QObjectPrivate *getPrivate(const QObject &o)
    {
        const Cheater &cheater = static_cast<const Cheater&>(o);
#if QT_VERSION >= 0x040600
        return dynamic_cast<const QObjectPrivate *>(cheater.d_ptr.data());
#else
        return dynamic_cast<const QObjectPrivate *>(cheater.d_ptr);
#endif
    }
};

typedef QVector<QObjectPrivate::ConnectionList> ConnLists;

void tst_Gdb::dump_QObjectSignalHelper(QObject &o, int sigNum)
{
    //qDebug() << o.objectName() << sigNum;
    QByteArray expected("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal'");
#if QT_VERSION >= 0x040400
    expected.append(",children=[");
    const QObjectPrivate *p = Cheater::getPrivate(o);
    Q_ASSERT(p != 0);
    const ConnLists *connLists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    QObjectPrivate::ConnectionList connList =
        connLists != 0 && connLists->size() > sigNum ?
        connLists->at(sigNum) : QObjectPrivate::ConnectionList();
    int i = 0;
    for (QObjectPrivate::Connection *conn = connList.first; conn != 0;
         ++i, conn = conn->nextConnectionList) {
        const QString iStr = N(i);
        expected.append("{name='").append(iStr).append(" receiver',");
        if (conn->receiver == &o)
            expected.append("value='<this>',type='").
                append(o.metaObject()->className()).
                append("',numchild='0',addr='").append(ptrToBa(&o)).append("'");
        else if (conn->receiver == 0)
            expected.append("value='0x0',type='" NS "QObject *',numchild='0'");
        else
            expected.append("addr='").append(ptrToBa(conn->receiver)).append("',value='").
                append(utfToHex(conn->receiver->objectName())).append("',valueencoded='2',").
                append("type='" NS "QObject',displayedtype='").
                append(conn->receiver->metaObject()->className()).append("',numchild='1'");
        expected.append("},{name='").append(iStr).append(" slot',type='',value='");
        if (conn->receiver != 0)
            expected.append(conn->receiver->metaObject()->method(conn->method).signature());
        else
            expected.append("<invalid receiver>");
        expected.append("',numchild='0'},{name='").append(iStr).append(" type',type='',value='<").
            append(connectionType(conn->connectionType)).append(" connection>',").
            append("numchild='0'}");
        if (conn != connList.last)
            expected.append(",");
    }
    expected.append("],numchild='").append(N(i)).append("'");
#endif
    testDumper(expected, &o, NS"QObjectSignal", true, "", "", sigNum);
}

void tst_Gdb::dump_QObjectSignal()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    testDumper("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal',"
            "children=[],numchild='0'",
        &o, NS"QObjectSignal", true, "", "", 0);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    for (int signalIndex = 0; signalIndex < 17; ++signalIndex)
        dump_QObjectSignalHelper(m, signalIndex);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    for (int signalIndex = 0; signalIndex < 17; ++signalIndex)
        dump_QObjectSignalHelper(m, signalIndex);
}

void tst_Gdb::dump_QObjectSignalList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");

    testDumper("type='" NS "QObjectSignalList',value='<2 items>',"
                "addr='$A',numchild='2',children=["
            "{name='0',value='destroyed(QObject*)',numchild='0',"
                "addr='$A',type='" NS "QObjectSignal'},"
            "{name='1',value='destroyed()',numchild='0',"
                "addr='$A',type='" NS "QObjectSignal'}]",
        &o, NS"QObjectSignalList", true);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    QByteArray expected = "type='" NS "QObjectSignalList',value='<16 items>',"
        "addr='$A',numchild='16',children=["
        "{name='0',value='destroyed(QObject*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='1',value='destroyed()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='4',value='dataChanged(QModelIndex,QModelIndex)',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='5',value='headerDataChanged(Qt::Orientation,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='6',value='layoutChanged()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='7',value='layoutAboutToBeChanged()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='8',value='rowsAboutToBeInserted(QModelIndex,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='9',value='rowsInserted(QModelIndex,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='10',value='rowsAboutToBeRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='11',value='rowsRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='12',value='columnsAboutToBeInserted(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='13',value='columnsInserted(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='14',value='columnsAboutToBeRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='15',value='columnsRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='16',value='modelAboutToBeReset()',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='17',value='modelReset()',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'}]";

    testDumper(expected << "0" << "0" << "0" << "0" << "0" << "0",
        &m, NS"QObjectSignalList", true);


    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);

    testDumper(expected << "1" << "1" << "2" << "1" << "0" << "0",
        &m, NS"QObjectSignalList", true);
}

QByteArray slotIndexList(const QObject *ob)
{
    QByteArray slotIndices;
    const QMetaObject *mo = ob->metaObject();
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &mm = mo->method(i);
        if (mm.methodType() == QMetaMethod::Slot) {
            int slotIndex = mo->indexOfSlot(mm.signature());
            Q_ASSERT(slotIndex != -1);
            slotIndices.append(N(slotIndex));
            slotIndices.append(',');
        }
    }
    slotIndices.chop(1);
    return slotIndices;
}

void tst_Gdb::dump_QObjectSlot()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");

    QByteArray slotIndices = slotIndexList(&o);
    QCOMPARE(slotIndices, QByteArray("2,3"));
    QCOMPARE(o.metaObject()->indexOfSlot("deleteLater()"), 2);

    QByteArray expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);


    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    slotIndices = slotIndexList(&o);

    QCOMPARE(slotIndices, QByteArray("2,3"));
    QCOMPARE(o.metaObject()->indexOfSlot("deleteLater()"), 2);

    expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);


    // Case 3: QAbstractItemModel with connections to itself and to another
    //         o, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&o, SIGNAL(destroyed(QObject*)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);

}

void tst_Gdb::dump_QObjectSlotList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    testDumper("numchild='2',value='<2 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &o, NS"QObjectSlotList", true);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    testDumper("numchild='4',value='<4 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='18',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='19',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
    connect(&o, SIGNAL(destroyed(QObject*)), &m, SLOT(submit()));
    testDumper("numchild='4',value='<4 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='18',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='19',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);
}


#endif // #if 0

///////////////////////////// QPixmap /////////////////////////////////

const char * const pixmap[] = {
    "2 24 3 1", "       c None", ".      c #DBD3CB", "+      c #FCFBFA",
    "  ", "  ", "  ", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+",
    ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", "  ", "  ", "  "
};

void dump_QPixmap()
{
#    ifdef QT_GUI_LIB
    /* A */ QPixmap p; // Case 1: Null Pixmap.
    /* B */ p = QPixmap(20, 100); // Case 2: Uninitialized non-null pixmap.
    /* C */ p = QPixmap(pixmap); // Case 3: Initialized non-null pixmap.
    /* D */ (void) p.size();
#    endif
}

void tst_Gdb::dump_QPixmap()
{
#    ifdef QT_GUI_LIB
    prepare("dump_QPixmap");
    next();
    check("B", "{iname='local.p',name='p',type='" NS "QPixmap',"
        "value='(null)',numchild='0'}");
    next();
    check("C", "{iname='local.p',name='p',type='" NS "QPixmap',"
        "value='(20x100)',numchild='0'}");
    next();
    check("D", "{iname='local.p',name='p',type='" NS "QPixmap',"
        "value='(2x24)',numchild='0'}");
#    endif
}


///////////////////////////// QPoint /////////////////////////////////

void dump_QPoint()
{
    /* A */ QPoint p(43, 44);
    /* B */ QPointF f(45, 46);
    /* C */ (void) (p.x() + f.x()); }

void tst_Gdb::dump_QPoint()
{
    prepare("dump_QPoint");
    next();
    next();
    check("C","{iname='local.p',name='p',type='" NS "QPoint',"
        "value='(43, 44)',numchild='2',childtype='int',childnumchild='0',"
            "children=[{name='x',value='43'},{name='y',value='44'}]},"
        "{iname='local.f',name='f',type='" NS "QPointF',"
        "value='(45, 46)',numchild='2',childtype='double',childnumchild='0',"
            "children=[{name='x',value='45'},{name='y',value='46'}]}",
        "local.p,local.f");
}


///////////////////////////// QRect /////////////////////////////////

void dump_QRect()
{
    /* A */ QRect p(43, 44, 100, 200);
    /* B */ QRectF f(45, 46, 100, 200);
    /* C */ (void) (p.x() + f.x()); }

void tst_Gdb::dump_QRect()
{
    prepare("dump_QRect");
    next();
    next();

    check("C","{iname='local.p',name='p',type='" NS "QRect',"
        "value='100x200+43+44',numchild='4',childtype='int',childnumchild='0',"
            "children=[{name='x1',value='43'},{name='y1',value='44'},"
            "{name='x2',value='142'},{name='y2',value='243'}]},"
        "{iname='local.f',name='f',type='" NS "QRectF',"
        "value='100x200+45+46',numchild='4',childtype='double',childnumchild='0',"
            "children=[{name='x',value='45'},{name='y',value='46'},"
            "{name='w',value='100'},{name='h',value='200'}]}",
        "local.p,local.f");
}


///////////////////////////// QSet<int> ///////////////////////////////////

void dump_QSet_int()
{
    /* A */ QSet<int> h;
    /* B */ h.insert(42);
    /* C */ h.insert(44);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QSet_int()
{
    prepare("dump_QSet_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QSet<int>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QSet<int>',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='42'},{value='44'}]}", "local.h");
}


///////////////////////////// QSet<Int3> ///////////////////////////////////

void dump_QSet_Int3()
{
    /* A */ QSet<Int3> h;
    /* B */ h.insert(Int3(42));
    /* C */ h.insert(Int3(44));
    /* D */ (void) 0;
}

void tst_Gdb::dump_QSet_Int3()
{
    prepare("dump_QSet_Int3");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" NS "QSet<Int3>',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" NS "QSet<Int3>',value='<2 items>',numchild='2',"
            "childtype='Int3',children=["
            "{value='{...}',numchild='3'},{value='{...}',numchild='3'}]}", "local.h");
}


///////////////////////////// QSize /////////////////////////////////

#if QT_VERSION >= 0x040500
void dump_QSharedPointer()
{
    /* A */ // Case 1: Simple type.
            // Case 1.1: Null pointer.
            QSharedPointer<int> simplePtr;
            // Case 1.2: Non-null pointer,
            QSharedPointer<int> simplePtr2(new int(99));
            // Case 1.3: Shared pointer.
            QSharedPointer<int> simplePtr3 = simplePtr2;
            // Case 1.4: Weak pointer.
            QWeakPointer<int> simplePtr4(simplePtr2);

            // Case 2: Composite type.
            // Case 2.1: Null pointer.
            QSharedPointer<QString> compositePtr;
            // Case 2.2: Non-null pointer,
            QSharedPointer<QString> compositePtr2(new QString("Test"));
            // Case 2.3: Shared pointer.
            QSharedPointer<QString> compositePtr3 = compositePtr2;
            // Case 2.4: Weak pointer.
            QWeakPointer<QString> compositePtr4(compositePtr2);
    /* C */ (void) simplePtr.data();
}

#endif

void tst_Gdb::dump_QSharedPointer()
{
#if QT_VERSION >= 0x040500
    prepare("dump_QSharedPointer");
    if (checkUninitialized)
        check("A","{iname='local.simplePtr',name='simplePtr',"
            "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.simplePtr2',name='simplePtr2',"
            "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.simplePtr3',name='simplePtr3',"
            "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.simplePtr4',name='simplePtr4',"
            "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr',name='compositePtr',"
            "type='" NS "QSharedPointer<" NS "QString>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr2',name='compositePtr2',"
            "type='" NS "QSharedPointer<" NS "QString>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr3',name='compositePtr3',"
            "type='" NS "QSharedPointer<" NS "QString>',value='<invalid>',numchild='0'},"
        "{iname='local.compositePtr4',name='compositePtr4',"
            "type='" NS "QWeakPointer<" NS "QString>',value='<invalid>',numchild='0'}");

    next(8);
    check("C","{iname='local.simplePtr',name='simplePtr',"
            "type='" NS "QSharedPointer<int>',value='(null)',numchild='0'},"
        "{iname='local.simplePtr2',name='simplePtr2',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
        "{iname='local.simplePtr3',name='simplePtr3',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
        "{iname='local.simplePtr4',name='simplePtr4',"
            "type='" NS "QWeakPointer<int>',value='',numchild='3'},"
        "{iname='local.compositePtr',name='compositePtr',"
            "type='" NS "QSharedPointer<" NS "QString>',value='(null)',numchild='0'},"
        "{iname='local.compositePtr2',name='compositePtr2',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr3',name='compositePtr3',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr4',name='compositePtr4',"
            "type='" NS "QWeakPointer<" NS "QString>',value='',numchild='3'}");

    check("C","{iname='local.simplePtr',name='simplePtr',"
        "type='" NS "QSharedPointer<int>',value='(null)',numchild='0'},"
            "{iname='local.simplePtr2',name='simplePtr2',"
        "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='2',type='int',numchild='0'}]},"
        "{iname='local.simplePtr3',name='simplePtr3',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='2',type='int',numchild='0'}]},"
        "{iname='local.simplePtr4',name='simplePtr4',"
            "type='" NS "QWeakPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='2',type='int',numchild='0'}]},"
        "{iname='local.compositePtr',name='compositePtr',"
            "type='" NS "QSharedPointer<" NS "QString>',value='(null)',numchild='0'},"
        "{iname='local.compositePtr2',name='compositePtr2',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr3',name='compositePtr3',"
            "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3'},"
        "{iname='local.compositePtr4',name='compositePtr4',"
            "type='" NS "QWeakPointer<" NS "QString>',value='',numchild='3'}",
        "local.simplePtr,local.simplePtr2,local.simplePtr3,local.simplePtr4,"
        "local.compositePtr,local.compositePtr,local.compositePtr,"
        "local.compositePtr");

#endif
}

///////////////////////////// QSize /////////////////////////////////

void dump_QSize()
{
    /* A */ QSize p(43, 44);
//    /* B */ QSizeF f(45, 46);
    /* C */ (void) 0; }

void tst_Gdb::dump_QSize()
{
    prepare("dump_QSize");
    next(1);
// FIXME: Enable child type as soon as 'double' is not reported
// as 'myns::QVariant::Private::Data::qreal' anymore
    check("C","{iname='local.p',name='p',type='" NS "QSize',"
            "value='(43, 44)',numchild='2',childtype='int',childnumchild='0',"
                "children=[{name='w',value='43'},{name='h',value='44'}]}"
//            ",{iname='local.f',name='f',type='" NS "QSizeF',"
//            "value='(45, 46)',numchild='2',childtype='double',childnumchild='0',"
//                "children=[{name='w',value='45'},{name='h',value='46'}]}"
                "",
            "local.p,local.f");
}


///////////////////////////// QStack /////////////////////////////////

void dump_QStack()
{
    /* A */ QStack<int> v;
    /* B */ v.append(3);
    /* C */ v.append(2);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QStack()
{
    prepare("dump_QStack");
    if (checkUninitialized)
        check("A","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<0 items>',numchild='0'}");
    check("B","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<0 items>',numchild='0',children=[]}", "local.v");
    next();
    check("C","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<1 items>',numchild='1'}");
    check("C","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<1 items>',numchild='1',childtype='int',"
            "childnumchild='0',children=[{value='3'}]}",  // rounding...
            "local.v");
    next();
    check("D","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<2 items>',numchild='2'}");
    check("D","{iname='local.v',name='v',type='" NS "QStack<int>',"
            "value='<2 items>',numchild='2',childtype='int',"
            "childnumchild='0',children=[{value='3'},{value='2'}]}",
            "local.v");
}


///////////////////////////// QString /////////////////////////////////////

void dump_QString()
{
    /* A */ QString s;
    /* B */ s = "hallo";
    /* C */ s += "x";
    /* D */ (void) 0; }

void tst_Gdb::dump_QString()
{
    prepare("dump_QString");
    if (checkUninitialized)
        check("A","{iname='local.s',name='s',type='" NS "QString',"
                "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "valueencoded='7',value='',numchild='0'}", "local.s");
    // Plain C style dumping:
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "value='{...}',numchild='5'}", "", 0);
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "value='{...}',numchild='5',children=["
            "{name='d',type='" NS "QString::Data *',"
            "value='-',numchild='1'}]}", "local.s", 0);
/*
    // FIXME: changed after auto-deref commit
    check("B","{iname='local.s',name='s',type='" NS "QString',"
            "value='{...}',numchild='5',"
            "children=[{name='d',"
              "type='" NS "QString::Data *',value='-',numchild='1',"
                "children=[{iname='local.s.d.*',name='*d',"
                "type='" NS "QString::Data',value='{...}',numchild='11'}]}]}",
            "local.s,local.s.d", 0);
*/
    next();
    check("C","{iname='local.s',name='s',type='" NS "QString',"
            "valueencoded='7',value='680061006c006c006f00',numchild='0'}");
    next();
    check("D","{iname='local.s',name='s',type='" NS "QString',"
            "valueencoded='7',value='680061006c006c006f007800',numchild='0'}");
}


///////////////////////////// QStringList /////////////////////////////////

void dump_QStringList()
{
    /* A */ QStringList s;
    /* B */ s.append("hello");
    /* C */ s.append("world");
    /* D */ (void) 0;
}

void tst_Gdb::dump_QStringList()
{
    prepare("dump_QStringList");
    if (checkUninitialized)
        check("A","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<0 items>',numchild='0'}");
    check("B","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<0 items>',numchild='0',children=[]}", "local.s");
    next();
    check("C","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<1 items>',numchild='1'}");
    check("C","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<1 items>',numchild='1',childtype='" NS "QString',"
            "childnumchild='0',children=[{valueencoded='7',"
            "value='680065006c006c006f00'}]}",
            "local.s");
    next();
    check("D","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<2 items>',numchild='2'}");
    check("D","{iname='local.s',name='s',type='" NS "QStringList',"
            "value='<2 items>',numchild='2',childtype='" NS "QString',"
            "childnumchild='0',children=["
            "{valueencoded='7',value='680065006c006c006f00'},"
            "{valueencoded='7',value='77006f0072006c006400'}]}",
            "local.s");
}


///////////////////////////// QTextCodec /////////////////////////////////

void dump_QTextCodec()
{
    /* A */ QTextCodec *codec = QTextCodec::codecForName("UTF-16");
    /* D */ (void) codec; }

void tst_Gdb::dump_QTextCodec()
{
    // FIXME
    prepare("dump_QTextCodec");
    next();
    check("D", "{iname='local.codec',name='codec',type='" NS "QTextCodec',"
           "valueencoded='6',value='5554462d3136',numchild='2',children=["
            "{name='name',type='" NS "QByteArray',valueencoded='6',"
             "value='5554462d3136',numchild='6'},"
            "{name='mibEnum',type='int',value='1015',numchild='0'}]}",
        "local.codec,local.codec.*");
}


///////////////////////////// QVector /////////////////////////////////

void dump_QVector()
{
    /* A */ QVector<double> v;
    /* B */ v.append(3.14);
    /* C */ v.append(2.81);
    /* D */ (void) 0;
}

void tst_Gdb::dump_QVector()
{
    prepare("dump_QVector");
    if (checkUninitialized)
        check("A","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<0 items>',numchild='0'}");
    check("B","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<0 items>',numchild='0',children=[]}", "local.v");
    next();
    check("C","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<1 items>',numchild='1'}");
    check("C","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<1 items>',numchild='1',childtype='double',"
            "childnumchild='0',children=[{value='-'}]}",  // rounding...
            "local.v");
    next();
    check("D","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<2 items>',numchild='2'}");
    check("D","{iname='local.v',name='v',type='" NS "QVector<double>',"
            "value='<2 items>',numchild='2',childtype='double',"
            "childnumchild='0',children=[{value='-'},{value='-'}]}",
            "local.v");
}


///////////////////////////// QVariant /////////////////////////////////

void dump_QVariant1()
{
    QVariant v(QLatin1String("hallo"));
    (void) v.toInt();
}

#ifdef QT_GUI_LIB
#define GUI(s) s
#else
#define GUI(s) 0
#endif

void dump_QVariant()
{
    /*<invalid>*/ QVariant v;
    /* <invalid>    */ v = QBitArray();
    /* QBitArray    */ v = GUI(QBitmap());
    /* QBitMap      */ v = bool(true);
    /* bool         */ v = GUI(QBrush());
    /* QBrush       */ v = QByteArray("abc");
    /* QByteArray   */ v = QChar(QLatin1Char('x'));
    /* QChar        */ v = GUI(QColor());
    /* QColor       */ v = GUI(QCursor());
    /* QCursor      */ v = QDate();
    /* QDate        */ v = QDateTime();
    /* QDateTime    */ v = double(46);
    /* double       */ v = GUI(QFont());
    /* QFont        */ v = QVariantHash();
    /* QVariantHash */ v = GUI(QIcon());
    /* QIcon        */ v = GUI(QImage(10, 10, QImage::Format_RGB32));
    /* QImage       */ v = int(42);
    /* int          */ v = GUI(QKeySequence());
    /* QKeySequence */ v = QLine();
    /* QLine        */ v = QLineF();
    /* QLineF       */ v = QVariantList();
    /* QVariantList */ v = QLocale();
    /* QLocale      */ v = qlonglong(44);
    /* qlonglong    */ v = QVariantMap();
    /* QVariantMap  */ v = GUI(QTransform());
    /* QTransform   */ v = GUI(QMatrix4x4());
    /* QMatrix4x4   */ v = GUI(QPalette());
    /* QPalette     */ v = GUI(QPen());
    /* QPen         */ v = GUI(QPixmap());
    /* QPixmap      */ v = QPoint(45, 46);
    /* QPoint       */ v = QPointF(41, 42);
    /* QPointF      */ v = GUI(QPolygon());
    /* QPolygon     */ v = GUI(QQuaternion());
    /* QQuaternion  */ v = QRect(1, 2, 3, 4);
    /* QRect        */ v = QRectF(1, 2, 3, 4);
    /* QRectF       */ v = QRegExp("abc");
    /* QRegExp      */ v = GUI(QRegion());
    /* QRegion      */ v = QSize(0, 0);
    /* QSize        */ v = QSizeF(0, 0);
    /* QSizeF       */ v = GUI(QSizePolicy());
    /* QSizePolicy  */ v = QString("abc");
    /* QString      */ v = QStringList() << "abc";
    /* QStringList  */ v = GUI(QTextFormat());
    /* QTextFormat  */ v = GUI(QTextLength());
    /* QTextLength  */ v = QTime();
    /* QTime        */ v = uint(43);
    /* uint         */ v = qulonglong(45);
    /* qulonglong   */ v = QUrl("http://foo");
    /* QUrl         */ v = GUI(QVector2D());
    /* QVector2D    */ v = GUI(QVector3D());
    /* QVector3D    */ v = GUI(QVector4D());
    /* QVector4D    */ (void) 0;
}

void tst_Gdb::dump_QVariant()
{
#    define PRE "iname='local.v',name='v',type='" NS "QVariant',"
    prepare("dump_QVariant");
    if (checkUninitialized) /*<invalid>*/
        check("A","{" PRE "'value=<invalid>',numchild='0'}");
    next();
    check("<invalid>", "{" PRE "value='(invalid)',numchild='0'}");
    next();
    check("QBitArray", "{" PRE "value='(" NS "QBitArray)',numchild='1',children=["
        "{name='data',type='" NS "QBitArray',value='{...}',numchild='1'}]}",
        "local.v");
    next();
    (void)GUI(check("QBitMap", "{" PRE "value='(" NS "QBitmap)',numchild='1',children=["
        "{name='data',type='" NS "QBitmap',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("bool", "{" PRE "value='true',numchild='0'}", "local.v");
    next();
    (void)GUI(check("QBrush", "{" PRE "value='(" NS "QBrush)',numchild='1',children=["
        "{name='data',type='" NS "QBrush',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("QByteArray", "{" PRE "value='(" NS "QByteArray)',numchild='1',"
        "children=[{name='data',type='" NS "QByteArray',valueencoded='6',"
            "value='616263',numchild='3'}]}", "local.v");
    next();
    check("QChar", "{" PRE "value='(" NS "QChar)',numchild='1',"
        "children=[{name='data',type='" NS "QChar',value=''x' (120)',numchild='0'}]}", "local.v");
    next();
    (void)GUI(check("QColor", "{" PRE "value='(" NS "QColor)',numchild='1',children=["
        "{name='data',type='" NS "QColor',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    (void)GUI(check("QCursor", "{" PRE "value='(" NS "QCursor)',numchild='1',children=["
        "{name='data',type='" NS "QCursor',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("QDate", "{" PRE "value='(" NS "QDate)',numchild='1',children=["
        "{name='data',type='" NS "QDate',value='{...}',numchild='1'}]}", "local.v");
    next();
    check("QDateTime", "{" PRE "value='(" NS "QDateTime)',numchild='1',children=["
        "{name='data',type='" NS "QDateTime',valueencoded='7',value='',numchild='3'}]}", "local.v");
    next();
    check("double", "{" PRE "value='46',numchild='0'}", "local.v");
    next();
    (void)GUI(check("QFont", "{" PRE "value='(" NS "QFont)',numchild='1',children=["
        "{name='data',type='" NS "QFont',value='{...}',numchild='3'}]}",
        "local.v"));
    next();
    check("QVariantHash", "{" PRE "value='(" NS "QVariantHash)',numchild='1',children=["
        "{name='data',type='" NS "QHash<" NS "QString, " NS "QVariant>',"
            "value='<0 items>',numchild='0'}]}", "local.v");
    next();
    (void)GUI(check("QIcon", "{" PRE "value='(" NS "QIcon)',numchild='1',children=["
        "{name='data',type='" NS "QIcon',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
// FIXME:
//    (void)GUI(check("QImage", "{" PRE "value='(" NS "QImage)',numchild='1',children=["
//        "{name='data',type='" NS "QImage',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
    check("int", "{" PRE "value='42',numchild='0'}", "local.v");
    next();
    (void)GUI(check("QKeySequence","{" PRE "value='(" NS "QKeySequence)',numchild='1',children=["
        "{name='data',type='" NS "QKeySequence',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
    check("QLine", "{" PRE "value='(" NS "QLine)',numchild='1',children=["
        "{name='data',type='" NS "QLine',value='{...}',numchild='2'}]}", "local.v");
    next();
    check("QLineF", "{" PRE "value='(" NS "QLineF)',numchild='1',children=["
        "{name='data',type='" NS "QLineF',value='{...}',numchild='2'}]}", "local.v");
    next();
    check("QVariantList", "{" PRE "value='(" NS "QVariantList)',numchild='1',children=["
        "{name='data',type='" NS "QList<" NS "QVariant>',"
            "value='<0 items>',numchild='0'}]}", "local.v");
    next();
    check("QLocale", "{" PRE "value='(" NS "QLocale)',numchild='1',children=["
        "{name='data',type='" NS "QLocale',valueencoded='7',value='-',numchild='8'}]}", "local.v");
    next();
    check("qlonglong", "{" PRE "value='44',numchild='0'}", "local.v");
    next();
    check("QVariantMap", "{" PRE "value='(" NS "QVariantMap)',numchild='1',children=["
        "{name='data',type='" NS "QMap<" NS "QString, " NS "QVariant>',"
            "value='<0 items>',numchild='0'}]}", "local.v");
    next();
    (void)GUI(check("QTransform", "{" PRE "value='(" NS "QTransform)',numchild='1',children=["
        "{name='data',type='" NS "QTransform',value='{...}',numchild='7'}]}",
        "local.v"));
    next();
    (void)GUI(check("QMatrix4x4", "{" PRE "value='(" NS "QMatrix4x4)',numchild='1',children=["
        "{name='data',type='" NS "QMatrix4x4',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    (void)GUI(check("QPalette", "{" PRE "value='(" NS "QPalette)',numchild='1',children=["
        "{name='data',type='" NS "QPalette',value='{...}',numchild='4'}]}",
        "local.v"));
    next();
    (void)GUI(check("QPen", "{" PRE "value='(" NS "QPen)',numchild='1',children=["
        "{name='data',type='" NS "QPen',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
// FIXME:
//    (void)GUI(check("QPixmap", "{" PRE "value='(" NS "QPixmap)',numchild='1',children=["
//        "{name='data',type='" NS "QPixmap',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
    check("QPoint", "{" PRE "value='(" NS "QPoint)',numchild='1',children=["
        "{name='data',type='" NS "QPoint',value='(45, 46)',numchild='2'}]}",
            "local.v");
    next();
// FIXME
//    check("QPointF", "{" PRE "value='(" NS "QPointF)',numchild='1',children=["
//        "{name='data',type='" NS "QBrush',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
    (void)GUI(check("QPolygon", "{" PRE "value='(" NS "QPolygon)',numchild='1',children=["
        "{name='data',type='" NS "QPolygon',value='{...}',numchild='1'}]}",
        "local.v"));
    next();
// FIXME:
//    (void)GUI(check("QQuaternion", "{" PRE "value='(" NS "QQuaternion)',numchild='1',children=["
//        "{name='data',type='" NS "QQuadernion',value='{...}',numchild='1'}]}",
//        "local.v"));
    next();
// FIXME: Fix value
    check("QRect", "{" PRE "value='(" NS "QRect)',numchild='1',children=["
        "{name='data',type='" NS "QRect',value='-',numchild='4'}]}", "local.v");
    next();
// FIXME: Fix value
    check("QRectF", "{" PRE "value='(" NS "QRectF)',numchild='1',children=["
        "{name='data',type='" NS "QRectF',value='-',numchild='4'}]}", "local.v");
    next();
    check("QRegExp", "{" PRE "value='(" NS "QRegExp)',numchild='1',children=["
        "{name='data',type='" NS "QRegExp',value='{...}',numchild='1'}]}", "local.v");
    next();
    (void)GUI(check("QRegion", "{" PRE "value='(" NS "QRegion)',numchild='1',children=["
        "{name='data',type='" NS "QRegion',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    check("QSize", "{" PRE "value='(" NS "QSize)',numchild='1',children=["
        "{name='data',type='" NS "QSize',value='(0, 0)',numchild='2'}]}", "local.v");
    next();

// FIXME:
//  check("QSizeF", "{" PRE "value='(" NS "QSizeF)',numchild='1',children=["
//        "{name='data',type='" NS "QBrush',value='{...}',numchild='1'}]}",
//        "local.v");
    next();
    (void)GUI(check("QSizePolicy", "{" PRE "value='(" NS "QSizePolicy)',numchild='1',children=["
        "{name='data',type='" NS "QSizePolicy',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    check("QString", "{" PRE "value='(" NS "QString)',numchild='1',children=["
        "{name='data',type='" NS "QString',valueencoded='7',value='610062006300',numchild='0'}]}",
        "local.v");
    next();
    check("QStringList", "{" PRE "value='(" NS "QStringList)',numchild='1',children=["
        "{name='data',type='" NS "QStringList',value='<1 items>',numchild='1'}]}", "local.v");
    next();
    (void)GUI(check("QTextFormat", "{" PRE "value='(" NS "QTextFormat)',numchild='1',children=["
        "{name='data',type='" NS "QTextFormat',value='{...}',numchild='3'}]}",
        "local.v"));
    next();
    (void)GUI(check("QTextLength", "{" PRE "value='(" NS "QTextLength)',numchild='1',children=["
        "{name='data',type='" NS "QTextLength',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    check("QTime", "{" PRE "value='(" NS "QTime)',numchild='1',children=["
        "{name='data',type='" NS "QTime',value='{...}',numchild='1'}]}", "local.v");
    next();
    check("uint", "{" PRE "value='43',numchild='0'}", "local.v");
    next();
    check("qulonglong", "{" PRE "value='45',numchild='0'}", "local.v");
    next();
    check("QUrl", "{" PRE "value='(" NS "QUrl)',numchild='1',children=["
        "{name='data',type='" NS "QUrl',value='{...}',numchild='1'}]}", "local.v");
    next();
    (void)GUI(check("QVector2D", "{" PRE "value='(" NS "QVector2D)',numchild='1',children=["
        "{name='data',type='" NS "QVector2D',value='{...}',numchild='2'}]}",
        "local.v"));
    next();
    (void)GUI(check("QVector3D", "{" PRE "value='(" NS "QVector3D)',numchild='1',children=["
        "{name='data',type='" NS "QVector3D',value='{...}',numchild='3'}]}",
        "local.v"));
    next();
    (void)GUI(check("QVector4D", "{" PRE "value='(" NS "QVector4D)',numchild='1',children=["
        "{name='data',type='" NS "QVector4D',value='{...}',numchild='4'}]}",
        "local.v"));
}


///////////////////////////// QWeakPointer /////////////////////////////////

#if QT_VERSION >= 0x040500

void dump_QWeakPointer_11()
{
    /* A */ QSharedPointer<int> sp;
    /*   */ QWeakPointer<int> wp = sp.toWeakRef();
    /* B */ (void) 0; }

void tst_Gdb::dump_QWeakPointer_11()
{
    // Case 1.1: Null pointer.
    prepare("dump_QWeakPointer_11");
    if (checkUninitialized)
        check("A","{iname='local.sp',name='sp',"
               "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp',name='wp',"
               "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'}");
    next(2);
    check("B","{iname='local.sp',name='sp',"
            "type='" NS "QSharedPointer<int>',value='(null)',numchild='0'},"
            "{iname='local.wp',name='wp',"
            "type='" NS "QWeakPointer<int>',value='(null)',numchild='0'}");
}


void dump_QWeakPointer_12()
{
    /* A */ QSharedPointer<int> sp(new int(99));
    /*   */ QWeakPointer<int> wp = sp.toWeakRef();
    /* B */ (void) 0; }

void tst_Gdb::dump_QWeakPointer_12()
{
return;
    // Case 1.2: Weak pointer is unique.
    prepare("dump_QWeakPointer_12");
    if (checkUninitialized)
        check("A","{iname='local.sp',name='sp',"
               "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp',name='wp',"
                "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'}");
    next();
    next();
    check("B","{iname='local.sp',name='sp',"
                "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
            "{iname='local.wp',name='wp',"
                "type='" NS "QWeakPointer<int>',value='',numchild='3'}");
    check("B","{iname='local.sp',name='sp',"
            "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='2',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]},"
            "{iname='local.wp',name='wp',"
            "type='" NS "QWeakPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='2',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]}",
            "local.sp,local.wp");
}


void dump_QWeakPointer_13()
{
    /* A */ QSharedPointer<int> sp(new int(99));
    /*   */ QWeakPointer<int> wp = sp.toWeakRef();
    /*   */ QWeakPointer<int> wp2 = sp.toWeakRef();
    /* B */ (void) 0; }

void tst_Gdb::dump_QWeakPointer_13()
{
    // Case 1.3: There are other weak pointers.
    prepare("dump_QWeakPointer_13");
    if (checkUninitialized)
       check("A","{iname='local.sp',name='sp',"
              "type='" NS "QSharedPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp',name='wp',"
              "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'},"
            "{iname='local.wp2',name='wp2',"
              "type='" NS "QWeakPointer<int>',value='<invalid>',numchild='0'}");
    next(3);
    check("B","{iname='local.sp',name='sp',"
              "type='" NS "QSharedPointer<int>',value='',numchild='3'},"
            "{iname='local.wp',name='wp',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3'},"
            "{iname='local.wp2',name='wp2',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3'}");
    check("B","{iname='local.sp',name='sp',"
              "type='" NS "QSharedPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]},"
            "{iname='local.wp',name='wp',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3',children=["
                "{name='data',type='int',value='99',numchild='0'},"
                "{name='weakref',value='3',type='int',numchild='0'},"
                "{name='strongref',value='1',type='int',numchild='0'}]},"
            "{iname='local.wp2',name='wp2',"
              "type='" NS "QWeakPointer<int>',value='',numchild='3'}",
            "local.sp,local.wp");
}


void dump_QWeakPointer_2()
{
    /* A */ QSharedPointer<QString> sp(new QString("Test"));
    /*   */ QWeakPointer<QString> wp = sp.toWeakRef();
    /* B */ (void *) wp.data(); }

void tst_Gdb::dump_QWeakPointer_2()
{
    // Case 2: Composite type.
    prepare("dump_QWeakPointer_2");
    if (checkUninitialized)
        check("A","{iname='local.sp',name='sp',"
                    "type='" NS "QSharedPointer<" NS "QString>',"
                    "value='<invalid>',numchild='0'},"
                "{iname='local.wp',name='wp',"
                    "type='" NS "QWeakPointer<" NS "QString>',"
                    "value='<invalid>',numchild='0'}");
    next(2);
    check("B","{iname='local.sp',name='sp',"
          "type='" NS "QSharedPointer<" NS "QString>',value='',numchild='3',children=["
            "{name='data',type='" NS "QString',"
                "valueencoded='7',value='5400650073007400',numchild='0'},"
            "{name='weakref',value='2',type='int',numchild='0'},"
            "{name='strongref',value='1',type='int',numchild='0'}]},"
        "{iname='local.wp',name='wp',"
          "type='" NS "QWeakPointer<" NS "QString>',value='',numchild='3',children=["
            "{name='data',type='" NS "QString',"
                "valueencoded='7',value='5400650073007400',numchild='0'},"
            "{name='weakref',value='2',type='int',numchild='0'},"
            "{name='strongref',value='1',type='int',numchild='0'}]}",
        "local.sp,local.wp");
}

#else // before Qt 4.5

void tst_Gdb::dump_QWeakPointer_11() {}
void tst_Gdb::dump_QWeakPointer_12() {}
void tst_Gdb::dump_QWeakPointer_13() {}
void tst_Gdb::dump_QWeakPointer_2() {}

#endif


///////////////////////////// QWidget //////////////////////////////

void dump_QWidget()
{
#    ifdef QT_GUI_LIB
    /* A */ QWidget w;
    /* B */ (void) w.size();
#    endif
}

void tst_Gdb::dump_QWidget()
{
#    ifdef QT_GUI_LIB
    prepare("dump_QWidget");
    if (checkUninitialized)
        check("A","{iname='local.w',name='w',"
                "type='" NS "QWidget',value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.w',name='w',type='" NS "QWidget',"
        "value='{...}',numchild='4',children=["
      "{name='" NS "QObject',type='" NS "QObject',"
        "valueencoded='7',value='',numchild='4',children=["
          "{name='parent',type='" NS "QObject *',value='0x0',numchild='0'},"
          "{name='children',type='" NS "QObject::QObjectList',"
            "value='<0 items>',numchild='0'},"
          "{name='properties',value='<1 items>',type='',numchild='1'},"
          "{name='connections',value='<0 items>',type='',numchild='0'},"
          "{name='signals',value='<2 items>',type='',"
            "numchild='2',childnumchild='0'},"
          "{name='slots',value='<2 items>',type='',"
            "numchild='2',childnumchild='0'}"
        "]},"
      "{name='" NS "QPaintDevice',type='" NS "QPaintDevice',"
        "value='{...}',numchild='2'},"
      "{name='data',type='" NS "QWidgetData *',"
        "value='-',numchild='1'}]}",
      "local.w,local.w." NS "QObject");
#    endif
}


///////////////////////////// std::deque<int> //////////////////////////////

void dump_std_deque()
{
    /* A */ std::deque<int> deque;
    /* B */ deque.push_back(45);
    /* C */ deque.push_back(46);
    /* D */ deque.push_back(47);
    /* E */ (void) 0;
}

void tst_Gdb::dump_std_deque()
{
    prepare("dump_std_deque");
    if (checkUninitialized)
        check("A","{iname='local.deque',name='deque',"
            "type='std::deque<int, std::allocator<int> >',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B", "{iname='local.deque',name='deque',"
            "type='std::deque<int, std::allocator<int> >',"
            "value='<0 items>',numchild='0',children=[]}",
            "local.deque");
    next(3);
    check("E", "{iname='local.deque',name='deque',"
            "type='std::deque<int, std::allocator<int> >',"
            "value='<3 items>',numchild='3',"
            "childtype='int',childnumchild='0',children=["
                "{value='45'},{value='46'},{value='47'}]}",
            "local.deque");
    // FIXME: Try large container
}


///////////////////////////// std::list<int> //////////////////////////////

void dump_std_list()
{
    /* A */ std::list<int> list;
    /* B */ list.push_back(45);
    /* C */ list.push_back(46);
    /* D */ list.push_back(47);
    /* E */ (void) 0;
}

void tst_Gdb::dump_std_list()
{
    prepare("dump_std_list");
    if (checkUninitialized)
        check("A","{iname='local.list',name='list',"
            "numchild='0'}");
    next();
    check("B", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<0 items>',numchild='0',children=[]}",
            "local.list");
    next();
    check("C", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<1 items>',numchild='1',"
            "childtype='int',childnumchild='0',children=[{value='45'}]}",
            "local.list");
    next();
    check("D", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
                "{value='45'},{value='46'}]}",
            "local.list");
    next();
    check("E", "{iname='local.list',name='list',"
            "type='std::list<int, std::allocator<int> >',"
            "value='<3 items>',numchild='3',"
            "childtype='int',childnumchild='0',children=["
                "{value='45'},{value='46'},{value='47'}]}",
            "local.list");
}


///////////////////////////// std::map<int, int> //////////////////////////////

void dump_std_map_int_int()
{
    /* A */ std::map<int, int> h;
    /* B */ h[12] = 34;
    /* C */ h[14] = 54;
    /* D */ (void) 0;
}

void tst_Gdb::dump_std_map_int_int()
{
    QByteArray type = "std::map<int, int, std::less<int>, "
        "std::allocator<std::pair<int const, int> > >";

    prepare("dump_std_map_int_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" + type + "',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.h',name='h',"
            "type='" + type + "',value='<0 items>',"
            "numchild='0'}");
    next(2);
    check("D","{iname='local.h',name='h',"
            "type='" + type + "',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.h',name='h',"
            "type='" + type + "',value='<2 items>',"
            "numchild='2',childtype='int',childnumchild='0',"
            "children=[{name='12',value='34'},{name='14',value='54'}]}",
            "local.h,local.h.0,local.h.1");
}


//////////////////////// std::map<std::string, std::string> ////////////////////////

void dump_std_map_string_string()
{
    /* A */ std::map<std::string, std::string> m;
    /* B */ m["hello"] = "world";
    /* C */ m["foo"] = "bar";
    /* D */ (void) 0; }

void tst_Gdb::dump_std_map_string_string()
{
    QByteArray strType =
        "std::basic_string<char, std::char_traits<char>, std::allocator<char> >";
    QByteArray pairType =
        + "std::pair<" + strType + " const, " + strType + " >";
    QByteArray type = "std::map<" + strType + ", " + strType + ", "
        + "std::less<" + strType + " >, "
        + "std::allocator<" + pairType + " > >";

    prepare("dump_std_map_string_string");
    if (checkUninitialized)
        check("A","{iname='local.m',name='m',"
            "type='" + type + "',value='<invalid>',"
            "numchild='0'}");
    next();
    check("B","{iname='local.m',name='m',"
            "type='" + type + "',value='<0 items>',"
            "numchild='0'}");
    next();
    next();
    check("D","{iname='local.m',name='m',"
            "type='" + type + "',value='<2 items>',"
            "numchild='2'}");
    check("D","{iname='local.m',name='m',type='" + type + "',"
            "value='<2 items>',numchild='2',childtype='" + pairType + "',"
            "childnumchild='2',children=["
              "{value=' ',children=["
                 "{name='first',type='const " + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='666f6f',numchild='0'},"
                 "{name='second',type='" + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='626172',numchild='0'}]},"
              "{value=' ',children=["
                 "{name='first',type='const " + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='68656c6c6f',numchild='0'},"
                 "{name='second',type='" + strType + "',"
                    "type='std::string',"
                    "valueencoded='6',value='776f726c64',numchild='0'}]}"
            "]}",
            "local.m,local.m.0,local.m.1");
}


///////////////////////////// std::set<int> ///////////////////////////////////

void dump_std_set_int()
{
    /* A */ std::set<int> h;
    /* B */ h.insert(42);
    /* C */ h.insert(44);
    /* D */ (void) 0;
}

void tst_Gdb::dump_std_set_int()
{
    QByteArray setType = "std::set<int, std::less<int>, std::allocator<int> >";
    prepare("dump_std_set_int");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" + setType + "',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" + setType + "',value='<2 items>',numchild='2',"
            "childtype='int',childnumchild='0',children=["
            "{value='42'},{value='44'}]}", "local.h");
}


///////////////////////////// QSet<Int3> ///////////////////////////////////

void dump_std_set_Int3()
{
    /* A */ std::set<Int3> h;
    /* B */ h.insert(Int3(42));
    /* C */ h.insert(Int3(44));
    /* D */ (void) 0; }

void tst_Gdb::dump_std_set_Int3()
{
    QByteArray setType = "std::set<Int3, std::less<Int3>, std::allocator<Int3> >";
    prepare("dump_std_set_Int3");
    if (checkUninitialized)
        check("A","{iname='local.h',name='h',"
            "type='" + setType + "',value='<invalid>',"
            "numchild='0'}");
    next(3);
    check("D","{iname='local.h',name='h',"
            "type='" + setType + "',value='<2 items>',numchild='2',"
            "childtype='Int3',children=["
            "{value='{...}',numchild='3'},{value='{...}',numchild='3'}]}", "local.h");
}



///////////////////////////// std::string //////////////////////////////////

void dump_std_string()
{
    /* A */ std::string str;
    /* B */ str = "Hallo";
    /* C */ (void) 0; }

void tst_Gdb::dump_std_string()
{
    prepare("dump_std_string");
    if (checkUninitialized)
        check("A","{iname='local.str',name='str',type='-',"
            "value='<invalid>',numchild='0'}");
    next();
    check("B","{iname='local.str',name='str',type='std::string',"
            "valueencoded='6',value='',numchild='0'}");
    next();
    check("C","{iname='local.str',name='str',type='std::string',"
            "valueencoded='6',value='48616c6c6f',numchild='0'}");
}


///////////////////////////// std::wstring //////////////////////////////////

void dump_std_wstring()
{
    /* A */ std::wstring str;
    /* B */ str = L"Hallo";
    /* C */ (void) 0;
}

void tst_Gdb::dump_std_wstring()
{
    prepare("dump_std_wstring");
    if (checkUninitialized)
        check("A","{iname='local.str',name='str',"
            "numchild='0'}");
    next();
    check("B","{iname='local.str',name='str',type='std::string',valueencoded='6',"
            "value='',numchild='0'}");
    next();
    if (sizeof(wchar_t) == 2)
        check("C","{iname='local.str',name='str',type='std::string',valueencoded='6',"
            "value='00480061006c006c006f',numchild='0'}");
    else
        check("C","{iname='local.str',name='str',type='std::string',valueencoded='6',"
            "value='00000048000000610000006c0000006c0000006f',numchild='0'}");
}


///////////////////////////// std::vector<int> //////////////////////////////

void dump_std_vector()
{
    /* A */ std::vector<std::list<int> *> vector;
            std::list<int> list;
    /* B */ list.push_back(45);
    /* C */ vector.push_back(new std::list<int>(list));
    /* D */ vector.push_back(0);
    /* E */ (void) list.size();
    /* F */ (void) list.size(); }

void tst_Gdb::dump_std_vector()
{
    QByteArray listType = "std::list<int, std::allocator<int> >";
    QByteArray vectorType = "std::vector<" + listType + "*, "
        "std::allocator<" + listType + "*> >";

    prepare("dump_std_vector");
    if (checkUninitialized)
        check("A","{iname='local.vector',name='vector',type='" + vectorType + "',"
            "value='<invalid>',numchild='0'},"
        "{iname='local.list',name='list',type='" + listType + "',"
            "value='<invalid>',numchild='0'}");
    next(2);
    check("B","{iname='local.vector',name='vector',type='" + vectorType + "',"
            "value='<0 items>',numchild='0'},"
        "{iname='local.list',name='list',type='" + listType + "',"
            "value='<0 items>',numchild='0'}");
    next(3);
    check("E","{iname='local.vector',name='vector',type='" + vectorType + "',"
            "value='<2 items>',numchild='2',childtype='" + listType + " *',"
            "childnumchild='1',children=["
                "{type='" + listType + "',value='<1 items>',"
                    "childtype='int',"
                    "childnumchild='0',children=[{value='45'}]},"
                "{value='0x0',numchild='0'}]},"
            "{iname='local.list',name='list',type='" + listType + "',"
                "value='<1 items>',numchild='1'}",
        "local.vector,local.vector.0");
}



//////////////////////////////////////////////////////////////

static const char gdbmi1[] =
    "[frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]";

static const char gdbmi2[] =
    "[frame={level=\"0\",addr=\"0x00002ac058675840\","
    "func=\"QApplication\",file=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\","
    "fullname=\"/home/apoenitz/dev/qt/src/gui/kernel/qapplication.cpp\",line=\"592\"},"
    "frame={level=\"1\",addr=\"0x00000000004061e0\",func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}]";

static const char gdbmi3[] =
    "[stack={frame={level=\"0\",addr=\"0x00000000004061ca\","
    "func=\"main\",file=\"test1.cpp\","
    "fullname=\"/home/apoenitz/work/test1/test1.cpp\",line=\"209\"}}]";

static const char gdbmi4[] =
    "&\"source /home/apoenitz/dev/ide/main/bin/gdb/qt4macros\\n\""
    "4^done\n";

static const char gdbmi5[] =
    "[reason=\"breakpoint-hit\",bkptno=\"1\",thread-id=\"1\","
    "frame={addr=\"0x0000000000405738\",func=\"main\","
    "args=[{name=\"argc\",value=\"1\"},{name=\"argv\",value=\"0x7fff1ac78f28\"}],"
    "file=\"test1.cpp\",fullname=\"/home/apoenitz/work/test1/test1.cpp\","
    "line=\"209\"}]";

static const char gdbmi8[] =
    "[data={locals={{name=\"a\"},{name=\"w\"}}}]";

static const char gdbmi9[] =
    "[data={locals=[name=\"baz\",name=\"urgs\",name=\"purgs\"]}]";

static const char gdbmi10[] =
    "[name=\"urgs\",numchild=\"1\",type=\"Urgs\"]";

static const char gdbmi11[] =
    "[{name=\"size\",value=\"1\",type=\"size_t\",readonly=\"true\"},"
     "{name=\"0\",value=\"one\",type=\"QByteArray\"}]";

static const char gdbmi12[] =
    "[{iname=\"local.hallo\",value=\"\\\"\\\\\\00382\\t\\377\",type=\"QByteArray\","
     "numchild=\"0\"}]";


static const char jsont1[] =
    "{\"Size\":100564,\"UID\":0,\"GID\":0,\"Permissions\":33261,"
     "\"ATime\":1242370878000,\"MTime\":1239154689000}";

struct Int3 {
    Int3() { i1 = 42; i2 = 43; i3 = 44; }
    int i1, i2, i3;
};

struct QString3 {
    QString3() { s1 = "a"; s2 = "b"; s3 = "c"; }
    QString s1, s2, s3;
};

class tst_Dumpers : public QObject
{
    Q_OBJECT

public:
    tst_Dumpers() {}

    void testMi(const char* input)
    {
        GdbMi gdbmi;
        gdbmi.fromString(QByteArray(input));
        QCOMPARE('\n' + QString::fromLatin1(gdbmi.toString(false)),
            '\n' + QString(input));
    }

    void testJson(const char* input)
    {
        QCOMPARE('\n' + Utils::JsonStringValue(QLatin1String(input)).value(),
            '\n' + QString(input));
    }

private slots:
    void mi1()  { testMi(gdbmi1);  }
    void mi2()  { testMi(gdbmi2);  }
    void mi3()  { testMi(gdbmi3);  }
    //void mi4()  { testMi(gdbmi4);  }
    void mi5()  { testMi(gdbmi5);  }
    void mi8()  { testMi(gdbmi8);  }
    void mi9()  { testMi(gdbmi9);  }
    void mi10() { testMi(gdbmi10); }
    void mi11() { testMi(gdbmi11); }
    //void mi12() { testMi(gdbmi12); }

    void json1() { testJson(jsont1); }

    void infoBreak();
    void niceType();
    void niceType_data();

    void dumperCompatibility();
    void dumpQAbstractItemAndModelIndex();
    void dumpQAbstractItemModel();
    void dumpQByteArray();
    void dumpQChar();
    void dumpQDateTime();
    void dumpQDir();
    void dumpQFile();
    void dumpQFileInfo();
    void dumpQHash();
    void dumpQHashNode();
    void dumpQImage();
    void dumpQImageData();
    void dumpQLinkedList();
    void dumpQList_int();
    void dumpQList_int_star();
    void dumpQList_char();
    void dumpQList_QString();
    void dumpQList_QString3();
    void dumpQList_Int3();
    void dumpQLocale();
    void dumpQMap();
    void dumpQMapNode();
#ifdef USE_PRIVATE
    void dumpQObject();
    void dumpQObjectChildList();
    void dumpQObjectMethodList();
    void dumpQObjectPropertyList();
    void dumpQObjectSignal();
    void dumpQObjectSignalList();
    void dumpQObjectSlot();
    void dumpQObjectSlotList();
#endif
    void dumpQPixmap();
    void dumpQSharedPointer();
    void dumpQString();
    void dumpQTextCodec();
    void dumpQVariant_invalid();
    void dumpQVariant_QString();
    void dumpQVariant_QStringList();
#ifndef Q_CC_MSVC
    void dumpStdVector();
#endif // !Q_CC_MSVC
    void dumpQWeakPointer();
    void initTestCase();

private:
    void dumpQAbstractItemHelper(QModelIndex &index);
    void dumpQAbstractItemModelHelper(QAbstractItemModel &m);
    template <typename K, typename V> void dumpQHashNodeHelper(QHash<K, V> &hash);
    void dumpQImageHelper(const QImage &img);
    void dumpQImageDataHelper(QImage &img);
    template <typename T> void dumpQLinkedListHelper(QLinkedList<T> &l);
    void dumpQLocaleHelper(QLocale &loc);
    template <typename K, typename V> void dumpQMapHelper(QMap<K, V> &m);
    template <typename K, typename V> void dumpQMapNodeHelper(QMap<K, V> &m);
    void dumpQObjectChildListHelper(QObject &o);
    void dumpQObjectSignalHelper(QObject &o, int sigNum);
#if QT_VERSION >= 0x040500
    template <typename T>
    void dumpQSharedPointerHelper(QSharedPointer<T> &ptr);
    template <typename T>
    void dumpQWeakPointerHelper(QWeakPointer<T> &ptr);
#endif
    void dumpQTextCodecHelper(QTextCodec *codec);
};

void tst_Dumpers::infoBreak()
{
    // This tests the regular expression used in GdbEngine::extractDataFromInfoBreak
    // to discover breakpoints in constructors.

    // Copied from gdbengine.cpp:

    QRegExp re("MULTIPLE.*(0x[0-9a-f]+) in (.*)\\s+at (.*):([\\d]+)([^\\d]|$)");
    re.setMinimal(true);

    QCOMPARE(re.indexIn(
        "2       breakpoint     keep y   <MULTIPLE> 0x0040168e\n"
        "2.1                         y     0x0040168e "
            "in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7\n"
        "2.2                         y     0x00401792 "
            "in MainWindow::MainWindow(QWidget*) at mainwindow.cpp:7\n"), 33);
    QCOMPARE(re.cap(1), QString("0x0040168e"));
    QCOMPARE(re.cap(2).trimmed(), QString("MainWindow::MainWindow(QWidget*)"));
    QCOMPARE(re.cap(3), QString("mainwindow.cpp"));
    QCOMPARE(re.cap(4), QString("7"));


    QCOMPARE(re.indexIn(
        "Num     Type           Disp Enb Address            What"
        "4       breakpoint     keep y   <MULTIPLE>         0x00000000004066ad"
        "4.1                         y     0x00000000004066ad in CTorTester"
        " at /main/tests/manual/gdbdebugger/simple/app.cpp:124"), 88);

    QCOMPARE(re.cap(1), QString("0x00000000004066ad"));
    QCOMPARE(re.cap(2).trimmed(), QString("CTorTester"));
    QCOMPARE(re.cap(3), QString("/main/tests/manual/gdbdebugger/simple/app.cpp"));
    QCOMPARE(re.cap(4), QString("124"));
}


void tst_Dumpers::dumperCompatibility()
{
    // Ensure that no arbitrary padding is introduced by QVectorTypedData.
    const size_t qVectorDataSize = 16;
    QCOMPARE(sizeof(QVectorData), qVectorDataSize);
    QVectorTypedData<int> *v = 0;
    QCOMPARE(size_t(&v->array), qVectorDataSize);
}


template <typename K, typename V>
void getMapNodeParams(size_t &nodeSize, size_t &valOffset)
{
#if QT_VERSION >= 0x040500
    typedef QMapNode<K, V> NodeType;
    NodeType *node = 0;
    nodeSize = sizeof(NodeType);
    valOffset = size_t(&node->value);
#else
    nodeSize = sizeof(K) + sizeof(V) + 2*sizeof(void *);
    valOffset = sizeof(K);
#endif
}

void tst_Dumpers::dumpQAbstractItemHelper(QModelIndex &index)
{
    const QAbstractItemModel *model = index.model();
    const QString &rowStr = N(index.row());
    const QString &colStr = N(index.column());
    const QByteArray &internalPtrStrSymbolic = ptrToBa(index.internalPointer());
    const QByteArray &internalPtrStrValue = ptrToBa(index.internalPointer(), false);
    const QByteArray &modelPtrStr = ptrToBa(model);
    QByteArray indexSpecSymbolic = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrSymbolic + "," + modelPtrStr);
    QByteArray indexSpecValue = QByteArray().append(rowStr + "," + colStr + ",").
        append(internalPtrStrValue + "," + modelPtrStr);
    QByteArray expected = QByteArray("tiname='iname',addr='").append(ptrToBa(&index)).
        append("',type='" NS "QAbstractItem',addr='$").append(indexSpecSymbolic).
        append("',value='").append(valToString(model->data(index).toString())).
        append("',numchild='%',children=[");
    int rowCount = model->rowCount(index);
    int columnCount = model->columnCount(index);
    expected <<= N(rowCount * columnCount);
    for (int row = 0; row < rowCount; ++row) {
        for (int col = 0; col < columnCount; ++col) {
            const QModelIndex &childIndex = model->index(row, col, index);
            expected.append("{name='[").append(valToString(row)).append(",").
                append(N(col)).append("]',numchild='%',addr='$").
                append(N(childIndex.row())).append(",").
                append(N(childIndex.column())).append(",").
                append(ptrToBa(childIndex.internalPointer())).append(",").
                append(modelPtrStr).append("',type='" NS "QAbstractItem',value='").
                append(valToString(model->data(childIndex).toString())).append("'}");
            expected <<= N(model->rowCount(childIndex)
                           * model->columnCount(childIndex));
            if (col < columnCount - 1 || row < rowCount - 1)
                expected.append(",");
        }
    }
    expected.append("]");
    testDumper(expected, &index, NS"QAbstractItem", true, indexSpecValue);
}

void tst_Dumpers::dumpQAbstractItemAndModelIndex()
{
    class PseudoTreeItemModel : public QAbstractItemModel
    {
    public:
        PseudoTreeItemModel() : QAbstractItemModel(), parent1(0),
            parent1Child(1), parent2(10), parent2Child1(11), parent2Child2(12)
        {}

        int columnCount(const QModelIndex &parent = QModelIndex()) const
        {
            Q_UNUSED(parent);
            return 1;
        }

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
        {
            return !index.isValid() || role != Qt::DisplayRole ?
                    QVariant() : *static_cast<int *>(index.internalPointer());
        }

        QModelIndex index(int row, int column,
                          const QModelIndex & parent = QModelIndex()) const
        {
            QModelIndex index;
            if (column == 0) {
                if (!parent.isValid()) {
                    if (row == 0)
                        index = createIndex(row, column, &parent1);
                    else if (row == 1)
                        index = createIndex(row, column, &parent2);
                } else if (parent.internalPointer() == &parent1 && row == 0) {
                    index = createIndex(row, column, &parent1Child);
                } else if (parent.internalPointer() == &parent2) {
                    index = createIndex(row, column,
                                row == 0 ? &parent2Child1 : &parent2Child2);
                }
            }
            return index;
        }

        QModelIndex parent(const QModelIndex & index) const
        {
            QModelIndex parent;
            if (index.isValid()) {
                if (index.internalPointer() == &parent1Child)
                    parent = createIndex(0, 0, &parent1);
                else if (index.internalPointer() == &parent2Child1 ||
                         index.internalPointer() == &parent2Child2)
                    parent = createIndex(1, 0, &parent2);
            }
            return parent;
        }

        int rowCount(const QModelIndex &parent = QModelIndex()) const
        {
            int rowCount;
            if (!parent.isValid() || parent.internalPointer() == &parent2)
                rowCount = 2;
            else if (parent.internalPointer() == &parent1)
                rowCount = 1;
            else
                rowCount = 0;
            return rowCount;
        }

    private:
        mutable int parent1;
        mutable int parent1Child;
        mutable int parent2;
        mutable int parent2Child1;
        mutable int parent2Child2;
    };

    PseudoTreeItemModel m2;

    // Case 1: ModelIndex with no children.
    QStringListModel m(QStringList() << "item1" << "item2" << "item3");
    QModelIndex index = m.index(2, 0);

    testDumper(QByteArray("type='$T',value='(2, 0)',numchild='5',children=["
        "{name='row',value='2',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='<invalid>',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index.internalId()))
         << ptrToBa(&m),
        &index, NS"QModelIndex", true);

    // Case 2: ModelIndex with one child.
    QModelIndex index2 = m2.index(0, 0);
    dumpQAbstractItemHelper(index2);

    qDebug() << "FIXME: invalid indices should not have children";
    testDumper(QByteArray("type='$T',value='(0, 0)',numchild='5',children=["
        "{name='row',value='0',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='<invalid>',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index2.internalId()))
         << ptrToBa(&m2),
        &index2, NS"QModelIndex", true);


    // Case 3: ModelIndex with two children.
    QModelIndex index3 = m2.index(1, 0);
    dumpQAbstractItemHelper(index3);

    testDumper(QByteArray("type='$T',value='(1, 0)',numchild='5',children=["
        "{name='row',value='1',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='<invalid>',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index3.internalId()))
         << ptrToBa(&m2),
        &index3, NS"QModelIndex", true);


    // Case 4: ModelIndex with a parent.
    index = m2.index(0, 0, index3);
    testDumper(QByteArray("type='$T',value='(0, 0)',numchild='5',children=["
        "{name='row',value='0',type='int',numchild='0'},"
        "{name='column',value='0',type='int',numchild='0'},"
        "{name='parent',value='(1, 0)',exp='(('$T'*)$A)->parent()',"
            "type='$T',numchild='1'},"
        "{name='internalId',%},"
        "{name='model',value='%',type='" NS "QAbstractItemModel*',"
            "numchild='1'}]")
         << generateQStringSpec(N(index.internalId()))
         << ptrToBa(&m2),
        &index, NS"QModelIndex", true);


    // Case 5: Empty ModelIndex
    QModelIndex index4;
    testDumper("type='$T',value='<invalid>',numchild='0'",
        &index4, NS"QModelIndex", true);
}

void tst_Dumpers::dumpQAbstractItemModelHelper(QAbstractItemModel &m)
{
    QByteArray address = ptrToBa(&m);
    QByteArray expected = QByteArray("tiname='iname',addr='%',"
        "type='" NS "QAbstractItemModel',value='(%,%)',numchild='1',children=["
        "{numchild='1',name='" NS "QObject',addr='%',value='%',"
        "valueencoded='2',type='" NS "QObject',displayedtype='%'}")
            << address
            << N(m.rowCount())
            << N(m.columnCount())
            << address
            << utfToBase64(m.objectName())
            << m.metaObject()->className();

    for (int row = 0; row < m.rowCount(); ++row) {
        for (int column = 0; column < m.columnCount(); ++column) {
            QModelIndex mi = m.index(row, column);
            expected.append(QByteArray(",{name='[%,%]',value='%',"
                "valueencoded='2',numchild='%',addr='$%,%,%,%',"
                "type='" NS "QAbstractItem'}")
                << N(row)
                << N(column)
                << utfToBase64(m.data(mi).toString())
                << N(row * column)
                << N(mi.row())
                << N(mi.column())
                << ptrToBa(mi.internalPointer())
                << ptrToBa(mi.model()));
        }
    }
    expected.append("]");
    testDumper(expected, &m, NS"QAbstractItemModel", true);
}

void tst_Dumpers::dumpQAbstractItemModel()
{
    // Case 1: No rows, one column.
    QStringList strList;
    QStringListModel model(strList);
    dumpQAbstractItemModelHelper(model);

    // Case 2: One row, one column.
    strList << "String 1";
    model.setStringList(strList);
    dumpQAbstractItemModelHelper(model);

    // Case 3: Two rows, one column.
    strList << "String 2";
    model.setStringList(strList);
    dumpQAbstractItemModelHelper(model);

    // Case 4: No rows, two columns.
    QStandardItemModel model2(0, 2);
    dumpQAbstractItemModelHelper(model2);

    // Case 5: One row, two columns.
    QStandardItem item1("Item (0,0)");
    QStandardItem item2("(Item (0,1)");
    model2.appendRow(QList<QStandardItem *>() << &item1 << &item2);
    dumpQAbstractItemModelHelper(model2);

    // Case 6: Two rows, two columns
    QStandardItem item3("Item (1,0");
    QStandardItem item4("Item (1,1)");
    model2.appendRow(QList<QStandardItem *>() << &item3 << &item4);
    dumpQAbstractItemModelHelper(model);
}

void tst_Dumpers::dumpQByteArray()
{
    // Case 1: Empty object.
    QByteArray ba;
    testDumper("value='',valueencoded='1',type='" NS "QByteArray',numchild='0',"
            "childtype='char',childnumchild='0',children=[]",
        &ba, NS"QByteArray", true);

    // Case 2: One element.
    ba.append('a');
    testDumper("value='YQ==',valueencoded='1',type='" NS "QByteArray',numchild='1',"
            "childtype='char',childnumchild='0',children=[{value='61  (97 'a')'}]",
        &ba, NS"QByteArray", true);

    // Case 3: Two elements.
    ba.append('b');
    testDumper("value='YWI=',valueencoded='1',type='" NS "QByteArray',numchild='2',"
            "childtype='char',childnumchild='0',children=["
            "{value='61  (97 'a')'},{value='62  (98 'b')'}]",
        &ba, NS"QByteArray", true);

    // Case 4: > 100 elements.
    ba = QByteArray(101, 'a');
    QByteArray children;
    for (int i = 0; i < 101; i++)
        children.append("{value='61  (97 'a')'},");
    children.chop(1);
    testDumper(QByteArray("value='YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh"
            "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFhYWFh"
            "YWFhYWFhYWFhYWFhYWFhYWFhYWFhYQ== <size: 101, cut...>',"
            "valueencoded='1',type='" NS "QByteArray',numchild='101',"
            "childtype='char',childnumchild='0',children=[%]") << children,
        &ba, NS"QByteArray", true);

    // Case 5: Regular and special characters and the replacement character.
    ba = QByteArray("abc\a\n\r\033\'\"?");
    testDumper("value='YWJjBwoNGyciPw==',valueencoded='1',type='" NS "QByteArray',"
            "numchild='10',childtype='char',childnumchild='0',children=["
            "{value='61  (97 'a')'},{value='62  (98 'b')'},"
            "{value='63  (99 'c')'},{value='07  (7 '?')'},"
            "{value='0a  (10 '?')'},{value='0d  (13 '?')'},"
            "{value='1b  (27 '?')'},{value='27  (39 '?')'},"
            "{value='22  (34 '?')'},{value='3f  (63 '?')'}]",
        &ba, NS"QByteArray", true);
}

void tst_Dumpers::dumpQChar()
{
    // Case 1: Printable ASCII character.
    QChar c('X');
    testDumper("value=''X', ucs=88',numchild='0'",
        &c, NS"QChar", false);

    // Case 2: Printable non-ASCII character.
    c = QChar(0x600);
    testDumper("value=''?', ucs=1536',numchild='0'",
        &c, NS"QChar", false);

    // Case 3: Non-printable ASCII character.
    c = QChar::fromAscii('\a');
    testDumper("value=''?', ucs=7',numchild='0'",
        &c, NS"QChar", false);

    // Case 4: Non-printable non-ASCII character.
    c = QChar(0x9f);
    testDumper("value=''?', ucs=159',numchild='0'",
        &c, NS"QChar", false);

    // Case 5: Printable ASCII Character that looks like the replacement character.
    c = QChar::fromAscii('?');
    testDumper("value=''?', ucs=63',numchild='0'",
        &c, NS"QChar", false);
}

void tst_Dumpers::dumpQDateTime()
{
    // Case 1: Null object.
    QDateTime d;
    testDumper("value='(null)',type='$T',numchild='0'",
        &d, NS"QDateTime", true);

    // Case 2: Non-null object.
    d = QDateTime::currentDateTime();
    testDumper(QByteArray("value='%',valueencoded='2',"
        "type='$T',numchild='1',children=["
        "{name='toTime_t',%},"
        "{name='toString',%},"
        "{name='toString_(ISO)',%},"
        "{name='toString_(SystemLocale)',%},"
        "{name='toString_(Locale)',%}]")
            << utfToBase64(d.toString())
            << generateLongSpec((d.toTime_t()))
            << generateQStringSpec(d.toString())
            << generateQStringSpec(d.toString(Qt::ISODate))
            << generateQStringSpec(d.toString(Qt::SystemLocaleDate))
            << generateQStringSpec(d.toString(Qt::LocaleDate)),
        &d, NS"QDateTime", true);

}

void tst_Dumpers::dumpQDir()
{
    // Case 1: Current working directory.
    QDir dir = QDir::current();
    testDumper(QByteArray("value='%',valueencoded='2',type='" NS "QDir',numchild='3',"
        "children=[{name='absolutePath',%},{name='canonicalPath',%}]")
            << utfToBase64(dir.absolutePath())
            << generateQStringSpec(dir.absolutePath())
            << generateQStringSpec(dir.canonicalPath()),
        &dir, NS"QDir", true);

    // Case 2: Root directory.
    dir = QDir::root();
    testDumper(QByteArray("value='%',valueencoded='2',type='" NS "QDir',numchild='3',"
        "children=[{name='absolutePath',%},{name='canonicalPath',%}]")
            << utfToBase64(dir.absolutePath())
            << generateQStringSpec(dir.absolutePath())
            << generateQStringSpec(dir.canonicalPath()),
        &dir, NS"QDir", true);
}


void tst_Dumpers::dumpQFile()
{
    // Case 1: Empty file name => Does not exist.
    QFile file1("");
    testDumper(QByteArray("value='',valueencoded='2',type='$T',numchild='2',"
        "children=[{name='fileName',value='',valueencoded='2',type='" NS "QString',"
        "numchild='0'},"
        "{name='exists',value='false',type='bool',numchild='0'}]"),
        &file1, NS"QFile", true);

    // Case 2: File that is known to exist.
    QTemporaryFile file2;
    file2.open();
    testDumper(QByteArray("value='%',valueencoded='2',type='$T',numchild='2',"
        "children=[{name='fileName',value='%',valueencoded='2',type='" NS "QString',"
        "numchild='0'},"
        "{name='exists',value='true',type='bool',numchild='0'}]")
            << utfToBase64(file2.fileName()) << utfToBase64(file2.fileName()),
        &file2, NS"QFile", true);

    // Case 3: File with a name that most likely does not exist.
    QFile file3("jfjfdskjdflsdfjfdls");
    testDumper(QByteArray("value='%',valueencoded='2',type='$T',numchild='2',"
        "children=[{name='fileName',value='%',valueencoded='2',type='" NS "QString',"
        "numchild='0'},"
        "{name='exists',value='false',type='bool',numchild='0'}]")
            << utfToBase64(file3.fileName()) << utfToBase64(file3.fileName()),
        &file3, NS"QFile", true);
}

void tst_Dumpers::dumpQFileInfo()
{
    QFileInfo fi(".");
    QByteArray expected("value='%',valueencoded='2',type='$T',numchild='3',"
        "children=["
        "{name='absolutePath',%},"
        "{name='absoluteFilePath',%},"
        "{name='canonicalPath',%},"
        "{name='canonicalFilePath',%},"
        "{name='completeBaseName',%},"
        "{name='completeSuffix',%},"
        "{name='baseName',%},"
#ifdef QX
        "{name='isBundle',%},"
        "{name='bundleName',%},"
#endif
        "{name='fileName',%},"
        "{name='filePath',%},"
        "{name='group',%},"
        "{name='owner',%},"
        "{name='path',%},"
        "{name='groupid',%},"
        "{name='ownerid',%},"
        "{name='permissions',value=' ',type='%',numchild='10',"
        "children=[{name='ReadOwner',%},{name='WriteOwner',%},"
        "{name='ExeOwner',%},{name='ReadUser',%},{name='WriteUser',%},"
        "{name='ExeUser',%},{name='ReadGroup',%},{name='WriteGroup',%},"
        "{name='ExeGroup',%},{name='ReadOther',%},{name='WriteOther',%},"
        "{name='ExeOther',%}]},"
        "{name='caching',%},"
        "{name='exists',%},"
        "{name='isAbsolute',%},"
        "{name='isDir',%},"
        "{name='isExecutable',%},"
        "{name='isFile',%},"
        "{name='isHidden',%},"
        "{name='isReadable',%},"
        "{name='isRelative',%},"
        "{name='isRoot',%},"
        "{name='isSymLink',%},"
        "{name='isWritable',%},"
        "{name='created',value='%',valueencoded='2',%,"
            "type='" NS "QDateTime',numchild='1'},"
        "{name='lastModified',value='%',valueencoded='2',%,"
            "type='" NS "QDateTime',numchild='1'},"
        "{name='lastRead',value='%',valueencoded='2',%,"
            "type='" NS "QDateTime',numchild='1'}]");
    expected <<= utfToBase64(fi.filePath());
    expected <<= generateQStringSpec(fi.absolutePath());
    expected <<= generateQStringSpec(fi.absoluteFilePath());
    expected <<= generateQStringSpec(fi.canonicalPath());
    expected <<= generateQStringSpec(fi.canonicalFilePath());
    expected <<= generateQStringSpec(fi.completeBaseName());
    expected <<= generateQStringSpec(fi.completeSuffix(), true);
    expected <<= generateQStringSpec(fi.baseName());
#ifdef Q_OS_MACX
    expected <<= generateBoolSpec(fi.isBundle());
    expected <<= generateQStringSpec(fi.bundleName());
#endif
    expected <<= generateQStringSpec(fi.fileName());
    expected <<= generateQStringSpec(fi.filePath());
    expected <<= generateQStringSpec(fi.group());
    expected <<= generateQStringSpec(fi.owner());
    expected <<= generateQStringSpec(fi.path());
    expected <<= generateLongSpec(fi.groupId());
    expected <<= generateLongSpec(fi.ownerId());
    QFile::Permissions perms = fi.permissions();
    expected <<= QByteArray(NS"QFile::Permissions");
    expected <<= generateBoolSpec((perms & QFile::ReadOwner) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteOwner) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeOwner) != 0);
    expected <<= generateBoolSpec((perms & QFile::ReadUser) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteUser) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeUser) != 0);
    expected <<= generateBoolSpec((perms & QFile::ReadGroup) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteGroup) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeGroup) != 0);
    expected <<= generateBoolSpec((perms & QFile::ReadOther) != 0);
    expected <<= generateBoolSpec((perms & QFile::WriteOther) != 0);
    expected <<= generateBoolSpec((perms & QFile::ExeOther) != 0);
    expected <<= generateBoolSpec(fi.caching());
    expected <<= generateBoolSpec(fi.exists());
    expected <<= generateBoolSpec(fi.isAbsolute());
    expected <<= generateBoolSpec(fi.isDir());
    expected <<= generateBoolSpec(fi.isExecutable());
    expected <<= generateBoolSpec(fi.isFile());
    expected <<= generateBoolSpec(fi.isHidden());
    expected <<= generateBoolSpec(fi.isReadable());
    expected <<= generateBoolSpec(fi.isRelative());
    expected <<= generateBoolSpec(fi.isRoot());
    expected <<= generateBoolSpec(fi.isSymLink());
    expected <<= generateBoolSpec(fi.isWritable());
    expected <<= utfToBase64(fi.created().toString());
    expected <<= createExp(&fi, "QFileInfo", "created()");
    expected <<= utfToBase64(fi.lastModified().toString());
    expected <<= createExp(&fi, "QFileInfo", "lastModified()");
    expected <<= utfToBase64(fi.lastRead().toString());
    expected <<= createExp(&fi, "QFileInfo", "lastRead()");

    testDumper(expected, &fi, NS"QFileInfo", true);
}

void tst_Dumpers::dumpQHash()
{
    QHash<QString, QList<int> > hash;
    hash.insert("Hallo", QList<int>());
    hash.insert("Welt", QList<int>() << 1);
    hash.insert("!", QList<int>() << 1 << 2);
    hash.insert("!", QList<int>() << 1 << 2);
}

template <typename K, typename V>
void tst_Dumpers::dumpQHashNodeHelper(QHash<K, V> &hash)
{
    typename QHash<K, V>::iterator it = hash.begin();
    typedef QHashNode<K, V> HashNode;
    HashNode *dummy = 0;
    HashNode *node =
        reinterpret_cast<HashNode *>(reinterpret_cast<char *>(const_cast<K *>(&it.key())) -
            size_t(&dummy->key));
    const K &key = it.key();
    const V &val = it.value();
    QByteArray expected("value='");
    if (isSimpleType<V>())
        expected.append(valToString(val));
    expected.append("',numchild='2',children=[{name='key',type='").
        append(typeToString<K>()).append("',addr='").append(ptrToBa(&key)).
        append("'},{name='value',type='").append(typeToString<V>()).
        append("',addr='").append(ptrToBa(&val)).append("'}]");
    testDumper(expected, node, NS"QHashNode", true,
        getMapType<K, V>(), "", sizeof(it.key()), sizeof(it.value()));
}

void tst_Dumpers::dumpQHashNode()
{
    // Case 1: simple type -> simple type.
    QHash<int, int> hash1;
    hash1[2] = 3;
    dumpQHashNodeHelper(hash1);

    // Case 2: simple type -> composite type.
    QHash<int, QString> hash2;
    hash2[5] = "String 7";
    dumpQHashNodeHelper(hash2);

    // Case 3: composite type -> simple type
    QHash<QString, int> hash3;
    hash3["String 11"] = 13;
    dumpQHashNodeHelper(hash3);

    // Case 4: composite type -> composite type
    QHash<QString, QString> hash4;
    hash4["String 17"] = "String 19";
    dumpQHashNodeHelper(hash4);
}

void tst_Dumpers::dumpQImageHelper(const QImage &img)
{
    QByteArray expected = "value='(%x%)',type='" NS "QImage',numchild='0'"
            << N(img.width())
            << N(img.height());
    testDumper(expected, &img, NS"QImage", true);
}

void tst_Dumpers::dumpQImage()
{
    // Case 1: Null image.
    QImage img;
    dumpQImageHelper(img);

    // Case 2: Normal image.
    img = QImage(3, 700, QImage::Format_RGB555);
    dumpQImageHelper(img);

    // Case 3: Invalid image.
    img = QImage(100, 0, QImage::Format_Invalid);
    dumpQImageHelper(img);
}

void tst_Dumpers::dumpQImageDataHelper(QImage &img)
{
#if QT_VERSION >= 0x050000
    QSKIP("QImage::numBytes deprecated");
#else
    const QByteArray ba(QByteArray::fromRawData((const char*) img.bits(), img.numBytes()));
    QByteArray expected = QByteArray("tiname='$I',addr='$A',type='" NS "QImageData',").
        append("numchild='0',value='<hover here>',valuetooltipencoded='1',").
        append("valuetooltipsize='").append(N(ba.size())).append("',").
        append("valuetooltip='").append(ba.toBase64()).append("'");
    testDumper(expected, &img, NS"QImageData", false);
#endif
}

void tst_Dumpers::dumpQImageData()
{
    // Case 1: Null image.
    QImage img;
    dumpQImageDataHelper(img);

    // Case 2: Normal image.
    img = QImage(3, 700, QImage::Format_RGB555);
    dumpQImageDataHelper(img);

    // Case 3: Invalid image.
    img = QImage(100, 0, QImage::Format_Invalid);
    dumpQImageDataHelper(img);
}

template <typename T>
        void tst_Dumpers::dumpQLinkedListHelper(QLinkedList<T> &l)
{
    const int size = qMin(l.size(), 1000);
    const QString &sizeStr = N(size);
    const QByteArray elemTypeStr = typeToString<T>();
    QByteArray expected = QByteArray("value='<").append(sizeStr).
        append(" items>',valueeditable='false',numchild='").append(sizeStr).
        append("',childtype='").append(elemTypeStr).append("',childnumchild='").
        append(typeToNumchild<T>()).append("',children=[");
    typename QLinkedList<T>::const_iterator iter = l.constBegin();
    for (int i = 0; i < size; ++i, ++iter) {
        expected.append("{");
        const T &curElem = *iter;
        if (isPointer<T>()) {
            const QString typeStr = stripPtrType(typeToString<T>());
            const QByteArray addrStr = valToString(curElem);
            if (curElem != 0) {
                expected.append("addr='").append(addrStr).append("',type='").
                        append(typeStr).append("',value='").
                        append(derefValToString(curElem)).append("'");
            } else {
                expected.append("addr='").append(ptrToBa(&curElem)).append("',type='").
                    append(typeStr).append("',value='<null>',numchild='0'");
            }
        } else {
            expected.append("addr='").append(ptrToBa(&curElem)).
                append("',value='").append(valToString(curElem)).append("'");
        }
        expected.append("}");
        if (i < size - 1)
            expected.append(",");
    }
    if (size < l.size())
        expected.append(",...");
    expected.append("]");
    testDumper(expected, &l, NS"QLinkedList", true, elemTypeStr);
}

void tst_Dumpers::dumpQLinkedList()
{
    // Case 1: Simple element type.
    QLinkedList<int> l;

    // Case 1.1: Empty list.
    dumpQLinkedListHelper(l);

    // Case 1.2: One element.
    l.append(2);
    dumpQLinkedListHelper(l);

    // Case 1.3: Two elements
    l.append(3);
    dumpQLinkedListHelper(l);

    // Case 2: Composite element type.
    QLinkedList<QString> l2;

    // Case 2.1: Empty list.
    dumpQLinkedListHelper(l2);

    // Case 2.2: One element.
    l2.append("Teststring 1");
    dumpQLinkedListHelper(l2);

    // Case 2.3: Two elements.
    l2.append("Teststring 2");
    dumpQLinkedListHelper(l2);

    // Case 2.4: > 1000 elements.
    for (int i = 3; i <= 1002; ++i)
        l2.append("Test " + N(i));

    // Case 3: Pointer type.
    QLinkedList<int *> l3;
    l3.append(new int(5));
    l3.append(new int(7));
    l3.append(0);
    dumpQLinkedListHelper(l3);
}

#    if 0
    void tst_Debugger::dumpQLinkedList()
    {
        // Case 1: Simple element type.
        QLinkedList<int> l;

        // Case 1.1: Empty list.
        testDumper("value='<0 items>',valueeditable='false',numchild='0',"
            "childtype='int',childnumchild='0',children=[]",
            &l, NS"QLinkedList", true, "int");

        // Case 1.2: One element.
        l.append(2);
        testDumper("value='<1 items>',valueeditable='false',numchild='1',"
            "childtype='int',childnumchild='0',children=[{addr='%',value='2'}]"
                << ptrToBa(l.constBegin().operator->()),
            &l, NS"QLinkedList", true, "int");

        // Case 1.3: Two elements
        l.append(3);
        QByteArray it0 = ptrToBa(l.constBegin().operator->());
        QByteArray it1 = ptrToBa(l.constBegin().operator++().operator->());
        testDumper("value='<2 items>',valueeditable='false',numchild='2',"
            "childtype='int',childnumchild='0',children=[{addr='%',value='2'},"
            "{addr='%',value='3'}]" << it0 << it1,
            &l, NS"QLinkedList", true, "int");

        // Case 2: Composite element type.
        QLinkedList<QString> l2;
        QLinkedList<QString>::const_iterator iter;

        // Case 2.1: Empty list.
        testDumper("value='<0 items>',valueeditable='false',numchild='0',"
            "childtype='" NS "QString',childnumchild='0',children=[]",
            &l2, NS"QLinkedList", true, NS"QString");

        // Case 2.2: One element.
        l2.append("Teststring 1");
        iter = l2.constBegin();
        qDebug() << *iter;
        testDumper("value='<1 items>',valueeditable='false',numchild='1',"
            "childtype='" NS "QString',childnumchild='0',children=[{addr='%',value='%',}]"
                << ptrToBa(iter.operator->()) << utfToBase64(*iter),
            &l2, NS"QLinkedList", true, NS"QString");

        // Case 2.3: Two elements.
        QByteArray expected = "value='<2 items>',valueeditable='false',numchild='2',"
            "childtype='int',childnumchild='0',children=[";
        iter = l2.constBegin();
        expected.append("{addr='%',%},"
            << ptrToBa(iter.operator->()) << utfToBase64(*iter));
        ++iter;
        expected.append("{addr='%',%}]"
            << ptrToBa(iter.operator->()) << utfToBase64(*iter));
        testDumper(expected,
            &l, NS"QLinkedList", true, NS"QString");

        // Case 2.4: > 1000 elements.
        for (int i = 3; i <= 1002; ++i)
            l2.append("Test " + N(i));

        expected = "value='<1002 items>',valueeditable='false',"
            "numchild='1002',childtype='" NS "QString',childnumchild='0',children=['";
        iter = l2.constBegin();
        for (int i = 0; i < 1002; ++i, ++iter)
            expected.append("{addr='%',value='%'},"
                << ptrToBa(iter.operator->()) << utfToBase64(*iter));
        expected.append(",...]");
        testDumper(expected, &l, NS"QLinkedList", true, NS"QString");


        // Case 3: Pointer type.
        QLinkedList<int *> l3;
        l3.append(new int(5));
        l3.append(new int(7));
        l3.append(0);
        //dumpQLinkedListHelper(l3);
        testDumper("", &l, NS"QLinkedList", true, NS"QString");
    }
#    endif

void tst_Dumpers::dumpQList_int()
{
    QList<int> ilist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &ilist, NS"QList", true, "int");
    ilist.append(1);
    ilist.append(2);
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='int',childnumchild='0',children=["
        "{addr='" + str(&ilist.at(0)) + "',value='1'},"
        "{addr='" + str(&ilist.at(1)) + "',value='2'}]",
        &ilist, NS"QList", true, "int");
}

void tst_Dumpers::dumpQList_int_star()
{
    QList<int *> ilist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &ilist, NS"QList", true, "int*");
    ilist.append(new int(1));
    ilist.append(0);
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='int*',childnumchild='1',children=["
        "{addr='" + str(deref(&ilist.at(0))) +
            "',type='int',value='1'},"
        "{value='<null>',numchild='0'}]",
        &ilist, NS"QList", true, "int*");
}

void tst_Dumpers::dumpQList_char()
{
    QList<char> clist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &clist, NS"QList", true, "char");
    clist.append('a');
    clist.append('b');
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='char',childnumchild='0',children=["
        "{addr='" + str(&clist.at(0)) + "',value=''a', ascii=97'},"
        "{addr='" + str(&clist.at(1)) + "',value=''b', ascii=98'}]",
        &clist, NS"QList", true, "char");
}

void tst_Dumpers::dumpQList_QString()
{
    QList<QString> slist;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='1',children=[]",
        &slist, NS"QList", true, NS"QString");
    slist.append("a");
    slist.append("b");
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='1',childtype='" NS "QString',childnumchild='0',children=["
        "{addr='" + str(&slist.at(0)) + "',value='YQA=',valueencoded='2'},"
        "{addr='" + str(&slist.at(1)) + "',value='YgA=',valueencoded='2'}]",
        &slist, NS"QList", true, NS"QString");
}

void tst_Dumpers::dumpQList_Int3()
{
    QList<Int3> i3list;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='0',children=[]",
        &i3list, NS"QList", true, "Int3");
    i3list.append(Int3());
    i3list.append(Int3());
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='0',childtype='Int3',children=["
        "{addr='" + str(&i3list.at(0)) + "'},"
        "{addr='" + str(&i3list.at(1)) + "'}]",
        &i3list, NS"QList", true, "Int3");
}

void tst_Dumpers::dumpQList_QString3()
{
    QList<QString3> s3list;
    testDumper("value='<0 items>',valueeditable='false',numchild='0',"
        "internal='0',children=[]",
        &s3list, NS"QList", true, "QString3");
    s3list.append(QString3());
    s3list.append(QString3());
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "internal='0',childtype='QString3',children=["
        "{addr='" + str(&s3list.at(0)) + "'},"
        "{addr='" + str(&s3list.at(1)) + "'}]",
        &s3list, NS"QList", true, "QString3");
}

void tst_Dumpers::dumpQLocaleHelper(QLocale &loc)
{
    QByteArray expected = QByteArray("value='%',type='$T',numchild='8',"
            "children=[{name='country',%},"
            "{name='language',%},"
            "{name='measurementSystem',%},"
            "{name='numberOptions',%},"
            "{name='timeFormat_(short)',%},"
            "{name='timeFormat_(long)',%},"
            "{name='decimalPoint',%},"
            "{name='exponential',%},"
            "{name='percent',%},"
            "{name='zeroDigit',%},"
            "{name='groupSeparator',%},"
            "{name='negativeSign',%}]")
        << valToString(loc.name())
        << createExp(&loc, "QLocale", "country()")
        << createExp(&loc, "QLocale", "language()")
        << createExp(&loc, "QLocale", "measurementSystem()")
        << createExp(&loc, "QLocale", "numberOptions()")
        << generateQStringSpec(loc.timeFormat(QLocale::ShortFormat))
        << generateQStringSpec(loc.timeFormat())
        << generateQCharSpec(loc.decimalPoint())
        << generateQCharSpec(loc.exponential())
        << generateQCharSpec(loc.percent())
        << generateQCharSpec(loc.zeroDigit())
        << generateQCharSpec(loc.groupSeparator())
        << generateQCharSpec(loc.negativeSign());
    testDumper(expected, &loc, NS"QLocale", true);
}

void tst_Dumpers::dumpQLocale()
{
    QLocale english(QLocale::English);
    dumpQLocaleHelper(english);

    QLocale german(QLocale::German);
    dumpQLocaleHelper(german);

    QLocale chinese(QLocale::Chinese);
    dumpQLocaleHelper(chinese);

    QLocale swahili(QLocale::Swahili);
    dumpQLocaleHelper(swahili);
}

template <typename K, typename V>
        void tst_Dumpers::dumpQMapHelper(QMap<K, V> &map)
{
    QByteArray sizeStr(valToString(map.size()));
    size_t nodeSize;
    size_t valOff;
    getMapNodeParams<K, V>(nodeSize, valOff);
    int transKeyOffset = static_cast<int>(2*sizeof(void *)) - static_cast<int>(nodeSize);
    int transValOffset = transKeyOffset + valOff;
    bool simpleKey = isSimpleType<K>();
    bool simpleVal = isSimpleType<V>();
    QByteArray expected = QByteArray("value='<").append(sizeStr).append(" items>',numchild='").
        append(sizeStr).append("',extra='simplekey: ").append(N(simpleKey)).
        append(" isSimpleValue: ").append(N(simpleVal)).
        append(" keyOffset: ").append(N(transKeyOffset)).append(" valueOffset: ").
        append(N(transValOffset)).append(" mapnodesize: ").
        append(N(qulonglong(nodeSize))).append("',children=["); // 64bit Linux hack
    typedef typename QMap<K, V>::iterator mapIter;
    for (mapIter it = map.begin(); it != map.end(); ++it) {
        if (it != map.begin())
            expected.append(",");
        const QByteArray keyString =
            QByteArray(valToString(it.key())).replace("valueencoded","keyencoded");
        expected.append("{key='").append(keyString).append("',value='").
            append(valToString(it.value())).append("',");
        if (simpleKey && simpleVal) {
            expected.append("type='").append(typeToString<V>()).
                append("',addr='").append(ptrToBa(&it.value())).append("'");
        } else {
            QByteArray keyTypeStr = typeToString<K>();
            QByteArray valTypeStr = typeToString<V>();
#if QT_VERSION >= 0x040500
            QMapNode<K, V> *node = 0;
            size_t backwardOffset = size_t(&node->backward) - valOff;
            char *addr = reinterpret_cast<char *>(&(*it)) + backwardOffset;
            expected.append("addr='").append(ptrToBa(addr)).
                append("',type='" NS "QMapNode<").append(keyTypeStr).append(",").
                    append(valTypeStr).append(" >").append("'");
#else
            expected.append("type='" NS "QMapData::Node<").append(keyTypeStr).
                append(",").append(valTypeStr).append(" >").
                append("',exp='*('" NS "QMapData::Node<").append(keyTypeStr).
                append(",").append(valTypeStr).append(" >").
                append(" >'*)").append(ptrToBa(&(*it))).append("'");
#endif
        }
        expected.append("}");
    }
    expected.append("]");
    mapIter it = map.begin();
    testDumper(expected, *reinterpret_cast<QMapData **>(&it), NS"QMap",
               true, getMapType<K,V>(), "", 0, 0, nodeSize, valOff);
}

void tst_Dumpers::dumpQMap()
{
    qDebug() << "QMap<int, int>";
    // Case 1: Simple type -> simple type.
    QMap<int, int> map1;

    // Case 1.1: Empty map.
    dumpQMapHelper(map1);

    // Case 1.2: One element.
    map1[2] = 3;
    dumpQMapHelper(map1);

    // Case 1.3: Two elements.
    map1[3] = 5;
    dumpQMapHelper(map1);

    // Case 2: Simple type -> composite type.
    qDebug() << "QMap<int, QString>";
    QMap<int, QString> map2;

    // Case 2.1: Empty Map.
    dumpQMapHelper(map2);

    // Case 2.2: One element.
    map2[5] = "String 7";
    dumpQMapHelper(map2);

    // Case 2.3: Two elements.
    map2[7] = "String 11";
    dumpQMapHelper(map2);

    // Case 3: Composite type -> simple type.
    qDebug() << "QMap<QString, int>";
    QMap<QString, int> map3;

    // Case 3.1: Empty map.
    dumpQMapHelper(map3);

    // Case 3.2: One element.
    map3["String 13"] = 11;
    dumpQMapHelper(map3);

    // Case 3.3: Two elements.
    map3["String 17"] = 13;
    dumpQMapHelper(map3);

    // Case 4: Composite type -> composite type.
    QMap<QString, QString> map4;

    // Case 4.1: Empty map.
    dumpQMapHelper(map4);

    // Case 4.2: One element.
    map4["String 19"] = "String 23";
    dumpQMapHelper(map4);

    // Case 4.3: Two elements.
    map4["String 29"] = "String 31";
    dumpQMapHelper(map4);

    // Case 4.4: Different value, same key (multimap functionality).
    map4["String 29"] = "String 37";
    dumpQMapHelper(map4);
}

void tst_Dumpers::dumpQMapNode()
{
    // Case 1: simple type -> simple type.
    QMap<int, int> map;
    map[2] = 3;
    dumpQMapNodeHelper(map);

    // Case 2: simple type -> composite type.
    QMap<int, QString> map2;
    map2[3] = "String 5";
    dumpQMapNodeHelper(map2);

    // Case 3: composite type -> simple type.
    QMap<QString, int> map3;
    map3["String 7"] = 11;
    dumpQMapNodeHelper(map3);

    // Case 4: composite type -> composite type.
    QMap<QString, QString> map4;
    map4["String 13"] = "String 17";
    dumpQMapNodeHelper(map4);
}

#ifdef USE_PRIVATE
void tst_Dumpers::dumpQObject()
{
    QObject parent;
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4'",
        &parent, NS"QObject", false);
    testDumper("value='',valueencoded='2',type='$T',displayedtype='QObject',"
        "numchild='4',children=["
        "{name='properties',addr='$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',addr='$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',addr='$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',value='0x0',type='$T *',numchild='0'},"
        "{name='className',value='QObject',type='',numchild='0'}]",
        &parent, NS"QObject", true);

#if 0
    testDumper("numchild='2',value='<2 items>',type='QObjectSlotList',"
            "children=[{name='2',value='deleteLater()',"
            "numchild='0',addr='$A',type='QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',"
            "numchild='0',addr='$A',type='QObjectSlot'}]",
        &parent, NS"QObjectSlotList", true);
#endif

    parent.setObjectName("A Parent");
    testDumper("value='QQAgAFAAYQByAGUAbgB0AA==',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4'",
        &parent, NS"QObject", false);
    QObject child(&parent);
    testDumper("value='',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
    child.setObjectName("A Child");
    QByteArray ba ="value='QQAgAEMAaABpAGwAZAA=',valueencoded='2',type='$T',"
        "displayedtype='QObject',numchild='4',children=["
        "{name='properties',addr='$A',type='$TPropertyList',"
            "value='<1 items>',numchild='1'},"
        "{name='signals',addr='$A',type='$TSignalList',"
            "value='<2 items>',numchild='2'},"
        "{name='slots',addr='$A',type='$TSlotList',"
            "value='<2 items>',numchild='2'},"
        "{name='parent',addr='" + str(&parent) + "',"
            "value='QQAgAFAAYQByAGUAbgB0AA==',valueencoded='2',type='$T',"
            "displayedtype='QObject',numchild='1'},"
        "{name='className',value='QObject',type='',numchild='0'}]";
    testDumper(ba, &child, NS"QObject", true);
    connect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    disconnect(&child, SIGNAL(destroyed()), qApp, SLOT(quit()));
    testDumper(ba, &child, NS"QObject", true);
    child.setObjectName("A renamed Child");
    testDumper("value='QQAgAHIAZQBuAGEAbQBlAGQAIABDAGgAaQBsAGQA',valueencoded='2',"
        "type='$T',displayedtype='QObject',numchild='4'",
        &child, NS"QObject", false);
}

void tst_Dumpers::dumpQObjectChildListHelper(QObject &o)
{
    const QObjectList children = o.children();
    const int size = children.size();
    const QString sizeStr = N(size);
    QByteArray expected = QByteArray("numchild='").append(sizeStr).append("',value='<").
        append(sizeStr).append(" items>',type='" NS "QObjectChildList',children=[");
    for (int i = 0; i < size; ++i) {
        const QObject *child = children.at(i);
        expected.append("{addr='").append(ptrToBa(child)).append("',value='").
            append(utfToBase64(child->objectName())).
            append("',valueencoded='2',type='" NS "QObject',displayedtype='").
            append(child->metaObject()->className()).append("',numchild='1'}");
        if (i < size - 1)
            expected.append(",");
    }
    expected.append("]");
    testDumper(expected, &o, NS"QObjectChildList", true);
}

void tst_Dumpers::dumpQObjectChildList()
{
    // Case 1: Object with no children.
    QObject o;
    dumpQObjectChildListHelper(o);

    // Case 2: Object with one child.
    QObject o2(&o);
    dumpQObjectChildListHelper(o);

    // Case 3: Object with two children.
    QObject o3(&o);
    dumpQObjectChildListHelper(o);
}

void tst_Dumpers::dumpQObjectMethodList()
{
    QStringListModel m;
    testDumper("addr='<synthetic>',type='$T',numchild='24',"
        "childtype='" NS "QMetaMethod::Method',childnumchild='0',children=["
        "{name='0 0 destroyed(QObject*)',value='<Signal> (1)'},"
        "{name='1 1 destroyed()',value='<Signal> (1)'},"
        "{name='2 2 deleteLater()',value='<Slot> (2)'},"
        "{name='3 3 _q_reregisterTimers(void*)',value='<Slot> (2)'},"
        "{name='4 4 dataChanged(QModelIndex,QModelIndex)',value='<Signal> (1)'},"
        "{name='5 5 headerDataChanged(Qt::Orientation,int,int)',value='<Signal> (1)'},"
        "{name='6 6 layoutChanged()',value='<Signal> (1)'},"
        "{name='7 7 layoutAboutToBeChanged()',value='<Signal> (1)'},"
        "{name='8 8 rowsAboutToBeInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='9 9 rowsInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='10 10 rowsAboutToBeRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='11 11 rowsRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='12 12 columnsAboutToBeInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='13 13 columnsInserted(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='14 14 columnsAboutToBeRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='15 15 columnsRemoved(QModelIndex,int,int)',value='<Signal> (1)'},"
        "{name='16 16 modelAboutToBeReset()',value='<Signal> (1)'},"
        "{name='17 17 modelReset()',value='<Signal> (1)'},"
        "{name='18 18 rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='19 19 rowsMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='20 20 columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='21 21 columnsMoved(QModelIndex,int,int,QModelIndex,int)',value='<Signal> (1)'},"
        "{name='22 22 submit()',value='<Slot> (2)'},"
        "{name='23 23 revert()',value='<Slot> (2)'}]",
    &m, NS"QObjectMethodList", true);
}

void tst_Dumpers::dumpQObjectPropertyList()
{
    // Case 1: Model without a parent.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    testDumper("addr='<synthetic>',type='$T',numchild='1',value='<1 items>',"
        "children=[{name='objectName',type='QString',value='',"
            "valueencoded='2',numchild='0'}]",
         &m, NS"QObjectPropertyList", true);

    // Case 2: Model with a parent.
    QStringListModel m2(&m);
    testDumper("addr='<synthetic>',type='$T',numchild='1',value='<1 items>',"
        "children=[{name='objectName',type='QString',value='',"
            "valueencoded='2',numchild='0'}]",
         &m2, NS"QObjectPropertyList", true);
}

static const char *connectionType(uint type)
{
    Qt::ConnectionType connType = static_cast<Qt::ConnectionType>(type);
    const char *output = "unknown";
    switch (connType) {
        case Qt::AutoConnection: output = "auto"; break;
        case Qt::DirectConnection: output = "direct"; break;
        case Qt::QueuedConnection: output = "queued"; break;
        case Qt::BlockingQueuedConnection: output = "blockingqueued"; break;
        case 3: output = "autocompat"; break;
#if QT_VERSION >= 0x040600
        case Qt::UniqueConnection: break; // Can't happen.
#endif
        default:
            qWarning() << "Unknown connection type: " << type;
            break;
        };
    return output;
};

class Cheater : public QObject
{
public:
    static const QObjectPrivate *getPrivate(const QObject &o)
    {
        const Cheater &cheater = static_cast<const Cheater&>(o);
#if QT_VERSION >= 0x040600
        return dynamic_cast<const QObjectPrivate *>(cheater.d_ptr.data());
#else
        return dynamic_cast<const QObjectPrivate *>(cheater.d_ptr);
#endif
    }
};

typedef QVector<QObjectPrivate::ConnectionList> ConnLists;

void tst_Dumpers::dumpQObjectSignalHelper(QObject &o, int sigNum)
{
    //qDebug() << o.objectName() << sigNum;
    QByteArray expected("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal'");
#if QT_VERSION >= 0x040400 && QT_VERSION <= 0x040700
    expected.append(",children=[");
    const QObjectPrivate *p = Cheater::getPrivate(o);
    Q_ASSERT(p != 0);
    const ConnLists *connLists = reinterpret_cast<const ConnLists *>(p->connectionLists);
    QObjectPrivate::ConnectionList connList =
        connLists != 0 && connLists->size() > sigNum ?
        connLists->at(sigNum) : QObjectPrivate::ConnectionList();
    int i = 0;
    // FIXME: 4.6 only
    for (QObjectPrivate::Connection *conn = connList.first; conn != 0;
         ++i, conn = conn->nextConnectionList) {
        const QString iStr = N(i);
        expected.append("{name='").append(iStr).append(" receiver',");
        if (conn->receiver == &o)
            expected.append("value='<this>',type='").
                append(o.metaObject()->className()).
                append("',numchild='0',addr='").append(ptrToBa(&o)).append("'");
        else if (conn->receiver == 0)
            expected.append("value='0x0',type='" NS "QObject *',numchild='0'");
        else
            expected.append("addr='").append(ptrToBa(conn->receiver)).append("',value='").
                append(utfToBase64(conn->receiver->objectName())).append("',valueencoded='2',").
                append("type='" NS "QObject',displayedtype='").
                append(conn->receiver->metaObject()->className()).append("',numchild='1'");
        expected.append("},{name='").append(iStr).append(" slot',type='',value='");
        if (conn->receiver != 0)
            expected.append(conn->receiver->metaObject()->method(conn->method).signature());
        else
            expected.append("<invalid receiver>");
        expected.append("',numchild='0'},{name='").append(iStr).append(" type',type='',value='<").
            append(connectionType(conn->connectionType)).append(" connection>',").
            append("numchild='0'}");
        if (conn != connList.last)
            expected.append(",");
    }
    expected.append("],numchild='").append(N(i)).append("'");
#endif
    testDumper(expected, &o, NS"QObjectSignal", true, "", "", sigNum);
}

void tst_Dumpers::dumpQObjectSignal()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    testDumper("addr='<synthetic>',numchild='1',type='" NS "QObjectSignal',"
            "children=[],numchild='0'",
        &o, NS"QObjectSignal", true, "", "", 0);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    for (int signalIndex = 0; signalIndex < 17; ++signalIndex)
        dumpQObjectSignalHelper(m, signalIndex);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    for (int signalIndex = 0; signalIndex < 17; ++signalIndex)
        dumpQObjectSignalHelper(m, signalIndex);
}

void tst_Dumpers::dumpQObjectSignalList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");

    testDumper("type='" NS "QObjectSignalList',value='<2 items>',"
                "addr='$A',numchild='2',children=["
            "{name='0',value='destroyed(QObject*)',numchild='0',"
                "addr='$A',type='" NS "QObjectSignal'},"
            "{name='1',value='destroyed()',numchild='0',"
                "addr='$A',type='" NS "QObjectSignal'}]",
        &o, NS"QObjectSignalList", true);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    QByteArray expected = "type='" NS "QObjectSignalList',value='<20 items>',"
        "addr='$A',numchild='20',children=["
        "{name='0',value='destroyed(QObject*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='1',value='destroyed()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='4',value='dataChanged(QModelIndex,QModelIndex)',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='5',value='headerDataChanged(Qt::Orientation,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='6',value='layoutChanged()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='7',value='layoutAboutToBeChanged()',numchild='0',"
            "addr='$A',type='" NS "QObjectSignal'},"
        "{name='8',value='rowsAboutToBeInserted(QModelIndex,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='9',value='rowsInserted(QModelIndex,int,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='10',value='rowsAboutToBeRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='11',value='rowsRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='12',value='columnsAboutToBeInserted(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='13',value='columnsInserted(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='14',value='columnsAboutToBeRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='15',value='columnsRemoved(QModelIndex,int,int)',"
            "numchild='%',addr='$A',type='" NS "QObjectSignal'},"
        "{name='16',value='modelAboutToBeReset()',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='17',value='modelReset()',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='18',value='rowsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',"
             "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='19',value='rowsMoved(QModelIndex,int,int,QModelIndex,int)',"
             "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='20',value='columnsAboutToBeMoved(QModelIndex,int,int,QModelIndex,int)',"
             "numchild='0',addr='$A',type='" NS "QObjectSignal'},"
        "{name='21',value='columnsMoved(QModelIndex,int,int,QModelIndex,int)',"
            "numchild='0',addr='$A',type='" NS "QObjectSignal'}]";


    testDumper(expected << "0" << "0" << "0" << "0" << "0" << "0",
        &m, NS"QObjectSignalList", true);


    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);

    testDumper(expected << "1" << "1" << "2" << "1" << "0" << "0",
        &m, NS"QObjectSignalList", true);
}

QByteArray slotIndexList(const QObject *ob)
{
    QByteArray slotIndices;
    const QMetaObject *mo = ob->metaObject();
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QMetaMethod &mm = mo->method(i);
        if (mm.methodType() == QMetaMethod::Slot) {
            int slotIndex = mo->indexOfSlot(mm.signature());
            Q_ASSERT(slotIndex != -1);
            slotIndices.append(N(slotIndex));
            slotIndices.append(',');
        }
    }
    slotIndices.chop(1);
    return slotIndices;
}

void tst_Dumpers::dumpQObjectSlot()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");

    QByteArray slotIndices = slotIndexList(&o);
    QCOMPARE(slotIndices, QByteArray("2,3"));
    QCOMPARE(o.metaObject()->indexOfSlot("deleteLater()"), 2);

    QByteArray expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);


    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    slotIndices = slotIndexList(&o);

    QCOMPARE(slotIndices, QByteArray("2,3"));
    QCOMPARE(o.metaObject()->indexOfSlot("deleteLater()"), 2);

    expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);


    // Case 3: QAbstractItemModel with connections to itself and to another
    //         o, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&o, SIGNAL(destroyed(QObject*)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
#if QT_VERSION >= 0x040600
    connect(&m, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            &m, SLOT(revert()), Qt::UniqueConnection);
#endif
    expected = QByteArray("addr='$A',numchild='1',type='$T',"
        //"children=[{name='1 sender'}],numchild='1'");
        "children=[],numchild='0'");
    qDebug() << "FIXME!";
    testDumper(expected, &o, NS"QObjectSlot", true, "", "", 2);

}

void tst_Dumpers::dumpQObjectSlotList()
{
    // Case 1: Simple QObject.
    QObject o;
    o.setObjectName("Test");
    testDumper("numchild='2',value='<2 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &o, NS"QObjectSlotList", true);

    // Case 2: QAbstractItemModel with no connections.
    QStringListModel m(QStringList() << "Test1" << "Test2");
    testDumper("numchild='4',value='<4 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='22',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='23',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);

    // Case 3: QAbstractItemModel with connections to itself and to another
    //         object, using different connection types.
    qRegisterMetaType<QModelIndex>("QModelIndex");
    connect(&m, SIGNAL(columnsAboutToBeInserted(QModelIndex,int,int)),
            &o, SLOT(deleteLater()), Qt::DirectConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(revert()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsAboutToBeRemoved(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::QueuedConnection);
    connect(&m, SIGNAL(columnsInserted(QModelIndex,int,int)),
            &m, SLOT(submit()), Qt::BlockingQueuedConnection);
    connect(&m, SIGNAL(columnsRemoved(QModelIndex,int,int)),
            &m, SLOT(deleteLater()), Qt::AutoConnection);
    connect(&o, SIGNAL(destroyed(QObject*)), &m, SLOT(submit()));
    testDumper("numchild='4',value='<4 items>',type='$T',"
        "children=[{name='2',value='deleteLater()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='3',value='_q_reregisterTimers(void*)',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='22',value='submit()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'},"
        "{name='23',value='revert()',numchild='0',"
            "addr='$A',type='" NS "QObjectSlot'}]",
        &m, NS"QObjectSlotList", true);
}
#endif

void tst_Dumpers::dumpQPixmap()
{
    // Case 1: Null Pixmap.
    QPixmap p;

    testDumper("value='(0x0)',type='$T',numchild='0'",
        &p, NS"QPixmap", true);


    // Case 2: Uninitialized non-null pixmap.
    p = QPixmap(20, 100);
    testDumper("value='(20x100)',type='$T',numchild='0'",
        &p, NS"QPixmap", true);


    // Case 3: Initialized non-null pixmap.
    const char * const pixmap[] = {
        "2 24 3 1", "       c None", ".      c #DBD3CB", "+      c #FCFBFA",
        "  ", "  ", "  ", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+",
        ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", ".+", "  ", "  ", "  "
    };
    p = QPixmap(pixmap);
    testDumper("value='(2x24)',type='$T',numchild='0'",
        &p, NS"QPixmap", true);
}

#if QT_VERSION >= 0x040500
template<typename T>
void tst_Dumpers::dumpQSharedPointerHelper(QSharedPointer<T> &ptr)
{
    struct Cheater : public QSharedPointer<T>
    {
        static const typename QSharedPointer<T>::Data *getData(const QSharedPointer<T> &p)
        {
            return static_cast<const Cheater &>(p).d;
        }
    };

    QByteArray expected("value='");
    QString val1 = ptr.isNull() ? "<null>" : valToString(*ptr.data());
    QString val2 = isSimpleType<T>() ? val1 : "";
/*
    const int *weakAddr;
    const int *strongAddr;
    int weakValue;
    int strongValue;
    if (!ptr.isNull()) {
        weakAddr = reinterpret_cast<const int *>(&Cheater::getData(ptr)->weakref);
        strongAddr = reinterpret_cast<const int *>(&Cheater::getData(ptr)->strongref);
        weakValue = *weakAddr;
        strongValue = *strongAddr;
    } else {
        weakAddr = strongAddr = 0;
        weakValue = strongValue = 0;
    }
    expected.append(val2).append("',valueeditable='false',numchild='1',children=[").
        append("{name='data',addr='").append(ptrToBa(ptr.data())).
        append("',type='").append(typeToString<T>()).append("',value='").append(val1).
        append("'},{name='weakref',value='").append(N(weakValue)).
        append("',type='int',addr='").append(ptrToBa(weakAddr)).append("',numchild='0'},").
        append("{name='strongref',value='").append(N(strongValue)).
        append("',type='int',addr='").append(ptrToBa(strongAddr)).append("',numchild='0'}]");
    testDumper(expected, &ptr, NS"QSharedPointer", true, typeToString<T>());
*/
}
#endif

void tst_Dumpers::dumpQSharedPointer()
{
#if QT_VERSION >= 0x040500
    // Case 1: Simple type.
    // Case 1.1: Null pointer.
    QSharedPointer<int> simplePtr;
    dumpQSharedPointerHelper(simplePtr);

    // Case 1.2: Non-null pointer,
    QSharedPointer<int> simplePtr2(new int(99));
    dumpQSharedPointerHelper(simplePtr2);

    // Case 1.3: Shared pointer.
    QSharedPointer<int> simplePtr3 = simplePtr2;
    dumpQSharedPointerHelper(simplePtr2);

    // Case 1.4: Weak pointer.
    QWeakPointer<int> simplePtr4(simplePtr2);
    dumpQSharedPointerHelper(simplePtr2);

    // Case 2: Composite type.
    // Case 1.1: Null pointer.
    QSharedPointer<QString> compositePtr;
    // TODO: This case is not handled in dumper.cpp (segfault!)
    //dumpQSharedPointerHelper(compoistePtr);

    // Case 1.2: Non-null pointer,
    QSharedPointer<QString> compositePtr2(new QString("Test"));
    dumpQSharedPointerHelper(compositePtr2);

    // Case 1.3: Shared pointer.
    QSharedPointer<QString> compositePtr3 = compositePtr2;
    dumpQSharedPointerHelper(compositePtr2);

    // Case 1.4: Weak pointer.
    QWeakPointer<QString> compositePtr4(compositePtr2);
    dumpQSharedPointerHelper(compositePtr2);
#endif
}

void tst_Dumpers::dumpQString()
{
    QString s;
    testDumper("value='IiIgKG51bGwp',valueencoded='5',type='$T',numchild='0'",
        &s, NS"QString", false);
    s = "abc";
    testDumper("value='YQBiAGMA',valueencoded='2',type='$T',numchild='0'",
        &s, NS"QString", false);
}

void tst_Dumpers::dumpQVariant_invalid()
{
    QVariant v;
    testDumper("value='(invalid)',type='$T',numchild='0'",
        &v, NS"QVariant", false);
}

void tst_Dumpers::dumpQVariant_QString()
{
    QVariant v = "abc";
    testDumper("value='KFFTdHJpbmcpICJhYmMi',valueencoded='5',type='$T',"
        "numchild='0'",
        &v, NS"QVariant", true);
/*
    FIXME: the QString version should have a child:
    testDumper("value='KFFTdHJpbmcpICJhYmMi',valueencoded='5',type='$T',"
        "numchild='1',children=[{name='value',value='IgBhAGIAYwAiAA==',"
        "valueencoded='4',type='QString',numchild='0'}]",
        &v, NS"QVariant", true);
*/
}

void tst_Dumpers::dumpQVariant_QStringList()
{
    QVariant v = QStringList() << "Hi";
    testDumper("value='(QStringList) ',type='$T',numchild='1',"
        "children=[{name='value',exp='(*('" NS "QStringList'*)%)',"
        "type='QStringList',numchild='1'}]"
            << QByteArray::number(quintptr(&v)),
        &v, NS"QVariant", true);
}

#ifndef Q_CC_MSVC

void tst_Dumpers::dumpStdVector()
{
    std::vector<std::list<int> *> vector;
    QByteArray inner = "std::list<int> *";
    QByteArray innerp = "std::list<int>";
    testDumper("value='<0 items>',valueeditable='false',numchild='0'",
        &vector, "std::vector", false, inner, "", sizeof(std::list<int> *));
    std::list<int> list;
    vector.push_back(new std::list<int>(list));
    testDumper("value='<1 items>',valueeditable='false',numchild='1',"
        "childtype='" + inner + "',childnumchild='1',"
        "children=[{addr='" + str(deref(&vector[0])) + "',type='" + innerp + "'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(0);
    list.push_back(45);
    testDumper("value='<2 items>',valueeditable='false',numchild='2',"
        "childtype='" + inner + "',childnumchild='1',"
        "children=[{addr='" + str(deref(&vector[0])) + "',type='" + innerp + "'},"
          "{addr='" + str(&vector[1]) + "',"
            "type='" + innerp + "',value='<null>',numchild='0'}]",
        &vector, "std::vector", true, inner, "", sizeof(std::list<int> *));
    vector.push_back(new std::list<int>(list));
    vector.push_back(0);
}

#endif // !Q_CC_MSVC

void tst_Dumpers::dumpQTextCodecHelper(QTextCodec *codec)
{
    const QByteArray name = codec->name().toBase64();
    QByteArray expected = QByteArray("value='%',valueencoded='1',type='$T',"
        "numchild='2',children=[{name='name',value='%',type='" NS "QByteArray',"
        "numchild='0',valueencoded='1'},{name='mibEnum',%}]")
        << name << name << generateIntSpec(codec->mibEnum());
    testDumper(expected, codec, NS"QTextCodec", true);
}

void tst_Dumpers::dumpQTextCodec()
{
    const QList<QByteArray> &codecNames = QTextCodec::availableCodecs();
    foreach (const QByteArray &codecName, codecNames)
        dumpQTextCodecHelper(QTextCodec::codecForName(codecName));
}

#if QT_VERSION >= 0x040500
template <typename T1, typename T2>
        size_t offsetOf(const T1 *klass, const T2 *member)
{
    return static_cast<size_t>(reinterpret_cast<const char *>(member) -
                               reinterpret_cast<const char *>(klass));
}

template <typename T>
void tst_Dumpers::dumpQWeakPointerHelper(QWeakPointer<T> &ptr)
{
    typedef QtSharedPointer::ExternalRefCountData Data;
    const size_t dataOffset = 0;
    const Data *d = *reinterpret_cast<const Data **>(
            reinterpret_cast<const char **>(&ptr) + dataOffset);
    const int *weakRefPtr = reinterpret_cast<const int *>(&d->weakref);
    const int *strongRefPtr = reinterpret_cast<const int *>(&d->strongref);
    T *data = ptr.toStrongRef().data();
    const QString dataStr = valToString(*data);
    QByteArray expected("value='");
    if (isSimpleType<T>())
        expected.append(dataStr);
    expected.append("',valueeditable='false',numchild='1',children=[{name='data',addr='").
        append(ptrToBa(data)).append("',type='").append(typeToString<T>()).
        append("',value='").append(dataStr).append("'},{name='weakref',value='").
        append(valToString(*weakRefPtr)).append("',type='int',addr='").
        append(ptrToBa(weakRefPtr)).append("',numchild='0'},{name='strongref',value='").
        append(valToString(*strongRefPtr)).append("',type='int',addr='").
        append(ptrToBa(strongRefPtr)).append("',numchild='0'}]");
    testDumper(expected, &ptr, NS"QWeakPointer", true, typeToString<T>());
}
#endif

void tst_Dumpers::dumpQWeakPointer()
{
#if QT_VERSION >= 0x040500
    // Case 1: Simple type.

    // Case 1.1: Null pointer.
    QSharedPointer<int> spNull;
    QWeakPointer<int> wp = spNull.toWeakRef();
    testDumper("value='<null>',valueeditable='false',numchild='0'",
        &wp, NS"QWeakPointer", true, "int");

    // Case 1.2: Weak pointer is unique.
    QSharedPointer<int> sp(new int(99));
    wp = sp.toWeakRef();
    dumpQWeakPointerHelper(wp);

    // Case 1.3: There are other weak pointers.
    QWeakPointer<int> wp2 = sp.toWeakRef();
    dumpQWeakPointerHelper(wp);

    // Case 1.4: There are other strong shared pointers as well.
    QSharedPointer<int> sp2(sp);
    dumpQWeakPointerHelper(wp);

    // Case 2: Composite type.
    QSharedPointer<QString> spS(new QString("Test"));
    QWeakPointer<QString> wpS = spS.toWeakRef();
    dumpQWeakPointerHelper(wpS);
#endif
}

#define VERIFY_OFFSETOF(member)                           \
do {                                                      \
    QObjectPrivate *qob = 0;                              \
    ObjectPrivate *ob = 0;                                \
    QVERIFY(size_t(&qob->member) == size_t(&ob->member)); \
} while (0)


void tst_Dumpers::initTestCase()
{
    QVERIFY(sizeof(QWeakPointer<int>) == 2*sizeof(void *));
    QVERIFY(sizeof(QSharedPointer<int>) == 2*sizeof(void *));
#if QT_VERSION < 0x050000
    QtSharedPointer::ExternalRefCountData d;
    d.weakref = d.strongref = 0; // That's what the destructor expects.
    QVERIFY(sizeof(int) == sizeof(d.weakref));
    QVERIFY(sizeof(int) == sizeof(d.strongref));
#endif
#ifdef USE_PRIVATE
    const size_t qObjectPrivateSize = sizeof(QObjectPrivate);
    const size_t objectPrivateSize = sizeof(ObjectPrivate);
    QVERIFY2(qObjectPrivateSize == objectPrivateSize, QString::fromLatin1("QObjectPrivate=%1 ObjectPrivate=%2").arg(qObjectPrivateSize).arg(objectPrivateSize).toLatin1().constData());
    VERIFY_OFFSETOF(threadData);
    VERIFY_OFFSETOF(extraData);
    VERIFY_OFFSETOF(objectName);
    VERIFY_OFFSETOF(connectionLists);
    VERIFY_OFFSETOF(senders);
    VERIFY_OFFSETOF(currentSender);
    VERIFY_OFFSETOF(eventFilters);
    VERIFY_OFFSETOF(currentChildBeingDeleted);
    VERIFY_OFFSETOF(connectedSignals);
    //VERIFY_OFFSETOF(deleteWatch);
#ifdef QT3_SUPPORT
#if QT_VERSION < 0x040600
    VERIFY_OFFSETOF(pendingChildInsertedEvents);
#endif
#endif
#if QT_VERSION >= 0x040600
    VERIFY_OFFSETOF(sharedRefcount);
#endif
#endif
}
