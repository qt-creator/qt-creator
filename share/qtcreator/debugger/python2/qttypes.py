# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import platform
import struct
import re
from dumper import Children, SubItem, UnnamedSubItem, toInteger, DumperBase
from utils import DisplayFormat, TypeCode


def qdump__QAtomicInt(d, value):
    d.putValue(value.integer())


def qdump__QBasicAtomicInt(d, value):
    d.putValue(value.integer())


def qdump__QAtomicPointer(d, value):
    d.putItem(value.cast(value.type[0].pointer()))
    d.putBetterType(value.type)


def qform__QByteArray():
    return [DisplayFormat.Latin1String, DisplayFormat.SeparateLatin1String,
            DisplayFormat.Utf8String, DisplayFormat.SeparateUtf8String]


def qedit__QByteArray(d, value, data):
    d.call('void', value, 'resize', str(len(data)))
    (base, size, alloc) = d.stringData(value)
    d.setValues(base, 'char', [ord(c) for c in data])


def qdump__QByteArray(d, value):
    if d.qtVersion() >= 0x60000:
        dd, data, size = value.split('ppi')
        if dd:
            _, _, alloc = d.split('iii', dd)
        else: # fromRawData
            alloc = size
    else:
        data, size, alloc = d.qArrayData(value)

    d.check(alloc == 0 or (0 <= size and size <= alloc and alloc <= 100000000))
    if size > 0:
        d.putExpandable()

    elided, shown = d.computeLimit(size, d.displayStringLimit)
    p = d.readMemory(data, shown)

    displayFormat = d.currentItemFormat()
    if displayFormat == DisplayFormat.Automatic or displayFormat == DisplayFormat.Latin1String:
        d.putValue(p, 'latin1', elided=elided)
    elif displayFormat == DisplayFormat.SeparateLatin1String:
        d.putValue(p, 'latin1', elided=elided)
        d.putDisplay('latin1:separate', d.encodeByteArray(value, limit=100000))
    elif displayFormat == DisplayFormat.Utf8String:
        d.putValue(p, 'utf8', elided=elided)
    elif displayFormat == DisplayFormat.SeparateUtf8String:
        d.putValue(p, 'utf8', elided=elided)
        d.putDisplay('utf8:separate', d.encodeByteArray(value, limit=100000))
    if d.isExpanded():
        d.putArrayData(data, size, d.charType())


#def qdump__QArrayData(d, value):
#    data, size, alloc = d.qArrayDataHelper(value.address())
#    d.check(alloc == 0 or (0 <= size and size <= alloc and alloc <= 100000000))
#    d.putValue(d.readMemory(data, size), 'latin1')
#    d.putPlainChildren(value)


#def qdump__QByteArrayData(d, value):
#    qdump__QArrayData(d, value)


def qdump__QBitArray(d, value):
    if d.qtVersion() >= 0x60000:
        _, data, basize = value.split('ppi')
    else:
        data, basize, _ = d.qArrayData(value['d'])
    unused = d.extractByte(data) if data else 0
    size = basize * 8 - unused
    d.putItemCount(size)
    if d.isExpanded():
        with Children(d, size, maxNumChild=10000):
            for i in d.childRange():
                q = data + 1 + int(i / 8)
                with SubItem(d, i):
                    d.putValue((int(d.extractPointer(q)) >> (i % 8)) & 1)
                    d.putType('bool')


def qdump__QChar(d, value):
    d.putValue(d.extractUShort(value))


def qform_X_QAbstractItemModel():
    return [DisplayFormat.Simple, DisplayFormat.Enhanced]


def qdump_X_QAbstractItemModel(d, value):
    displayFormat = d.currentItemFormat()
    if displayFormat == DisplayFormat.Simple:
        d.putPlainChildren(value)
        return
    #displayFormat == Enhanced:
    # Create a default-constructed QModelIndex on the stack.
    try:
        ri = d.pokeValue(d.qtNamespace() + 'QModelIndex', '-1, -1, 0, 0')
        this_ = d.makeExpression(value)
        ri_ = d.makeExpression(ri)
        rowCount = int(d.parseAndEvaluate('%s.rowCount(%s)' % (this_, ri_)))
        columnCount = int(d.parseAndEvaluate('%s.columnCount(%s)' % (this_, ri_)))
    except:
        d.putPlainChildren(value)
        return
    d.putValue('%d x %d' % (rowCount, columnCount))
    d.putNumChild(rowCount * columnCount)
    if d.isExpanded():
        with Children(d, numChild=rowCount * columnCount, childType=ri.type):
            i = 0
            for row in range(rowCount):
                for column in range(columnCount):
                    with SubItem(d, i):
                        d.putName('[%s, %s]' % (row, column))
                        mi = d.parseAndEvaluate('%s.index(%d,%d,%s)'
                                                % (this_, row, column, ri_))
                        d.putItem(mi)
                        i = i + 1
    #gdb.execute('call free($ri)')


def qform_X_QModelIndex():
    return [DisplayFormat.Simple, DisplayFormat.Enhanced]


def qdump_X_QModelIndex(d, value):
    displayFormat = d.currentItemFormat()
    if displayFormat == DisplayFormat.Simple:
        d.putPlainChildren(value)
        return
    r = value['r']
    c = value['c']
    try:
        p = value['p']
    except:
        p = value['i']
    m = value['m']
    if m.pointer() == 0 or r < 0 or c < 0:
        d.putValue('(invalid)')
        d.putPlainChildren(value)
        return

    mm = m.dereference()
    mm = mm.cast(mm.type.unqualified())
    ns = d.qtNamespace()
    try:
        mi = d.pokeValue(ns + 'QModelIndex', '%s,%s,%s,%s' % (r, c, p, m))
        mm_ = d.makeExpression(mm)
        mi_ = d.makeExpression(mi)
        rowCount = int(d.parseAndEvaluate('%s.rowCount(%s)' % (mm_, mi_)))
        columnCount = int(d.parseAndEvaluate('%s.columnCount(%s)' % (mm_, mi_)))
    except:
        d.putPlainChildren(value)
        return

    try:
        # Access DisplayRole as value
        val = d.parseAndEvaluate('%s.data(%s, 0)' % (mm_, mi_))
        v = val['d']['data']['ptr']
        d.putStringValue(d.pokeValue(ns + 'QString', v))
    except:
        d.putValue('')

    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putFields(value, False)
            i = 0
            for row in range(rowCount):
                for column in range(columnCount):
                    with UnnamedSubItem(d, i):
                        d.putName('[%s, %s]' % (row, column))
                        mi2 = d.parseAndEvaluate('%s.index(%d,%d,%s)'
                                                 % (mm_, row, column, mi_))
                        d.putItem(mi2)
                        i = i + 1
            d.putCallItem('parent', '@QModelIndex', value, 'parent')
    #gdb.execute('call free($mi)')


def qdump__Qt__ItemDataRole(d, value):
    d.putEnumValue(value.integer(), {
        0:      "Qt::DisplayRole",
        1:      "Qt::DecorationRole",
        2:      "Qt::EditRole",
        3:      "Qt::ToolTipRole",
        4:      "Qt::StatusTipRole",
        5:      "Qt::WhatsThisRole",
        6:      "Qt::FontRole",
        7:      "Qt::TextAlignmentRole",
        # obsolete: 8  : "Qt::BackgroundColorRole",
        8:      "Qt::BackgroundRole",
        # obsolete: 9  : "Qt::TextColorRole",
        9:      "Qt::ForegroundRole",
        10:     "Qt::CheckStateRole",
        11:     "Qt::AccessibleTextRole",
        12:     "Qt::AccessibleDescriptionRole",
        13:     "Qt::SizeHintRole",
        14:     "Qt::InitialSortOrderRole",
        # 27-31 Qt4 ItemDataRoles
        27:     "Qt::DisplayPropertyRole",
        28:     "Qt::DecorationPropertyRole",
        29:     "Qt::ToolTipPropertyRole",
        30:     "Qt::StatusTipPropertyRole",
        31:     "Qt::WhatsThisPropertyRole",
        0x100:  "Qt::UserRole"
    })


def qdump__QStandardItemData(d, value):
    d.createType('@Qt::ItemDataRole') # warm up cache
    role, pad, val = value.split('{@Qt::ItemDataRole}@{@QVariant}')
    d.putPairContents(role.value(), (role, val), 'role', 'value')


def qdump__QStandardItem(d, value):
    d.createType('@QStandardItemData') # warm up cache
    d.createType('@QStandardItem')
    d.createType('@QStandardItem*')

    vtable, dptr = value.split('pp')
    if d.qtVersion() >= 0x060000:
        model, parent, values, children, rows, cols, item = \
            d.split('pp{@QList<@QStandardItemData>}{@QList<@QStandardItem *>}IIp', dptr)
    else:
        # There used to be a virtual destructor that got removed in
        # 88b6abcebf29b455438 on Apr 18 17:01:22 2017
        if d.qtVersion() < 0x050900 and not d.isMsvcTarget():
            dptr += d.ptrSize();
        model, parent, values, children, rows, cols, item = \
            d.split('pp{@QVector<@QStandardItemData>}{@QVector<@QStandardItem *>}IIp', dptr)

    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putSubItem('[model]', d.createValue(model, '@QStandardItemModel'))
            d.putSubItem('[values]', values)
            d.putSubItem('[children]', children)


def qdump__QDate(d, value):
    jd = value.pointer()
    if not jd:
        d.putValue('(invalid)')
        return
    d.putValue(jd, 'juliandate')
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            if d.canCallLocale():
                d.putCallItem('toString', '@QString', value, 'toString',
                              d.enumExpression('DateFormat', 'TextDate'))
                d.putCallItem('(ISO)', '@QString', value, 'toString',
                              d.enumExpression('DateFormat', 'ISODate'))
                if d.qtVersion() < 0x060000:
                    d.putCallItem('(SystemLocale)', '@QString', value, 'toString',
                                  d.enumExpression('DateFormat', 'SystemLocaleDate'))
                    d.putCallItem('(Locale)', '@QString', value, 'toString',
                                  d.enumExpression('DateFormat', 'LocaleDate'))
            d.putFields(value)


def qdump__QTime(d, value):
    mds = value.split('i')[0]
    if mds == -1:
        d.putValue('(invalid)')
        return
    d.putValue(mds, 'millisecondssincemidnight')
    if d.isExpanded():
        with Children(d):
            d.putCallItem('toString', '@QString', value, 'toString',
                          d.enumExpression('DateFormat', 'TextDate'))
            d.putCallItem('(ISO)', '@QString', value, 'toString',
                          d.enumExpression('DateFormat', 'ISODate'))
            if d.canCallLocale() and d.qtVersion() < 0x060000:
                d.putCallItem('(SystemLocale)', '@QString', value, 'toString',
                              d.enumExpression('DateFormat', 'SystemLocaleDate'))
                d.putCallItem('(Locale)', '@QString', value, 'toString',
                              d.enumExpression('DateFormat', 'LocaleDate'))
            d.putFields(value)


def qdump__QTimeZone(d, value):
    base = d.extractPointer(value)
    if base == 0:
        d.putValue('(null)')
        return
    idAddr = base + 2 * d.ptrSize()  # [QSharedData] + [vptr]
    d.putByteArrayValue(idAddr)
    d.putPlainChildren(value['d'])


def qdump__QDateTime(d, value):
    qtVersion = d.qtVersion()
    isValid = False
    # This relies on the Qt4/Qt5 internal structure layout:
    # {sharedref(4), ...
    base = d.extractPointer(value)
    is32bit = d.ptrSize() == 4
    if qtVersion >= 0x050200:
        tiVersion = d.qtTypeInfoVersion()
        #DumperBase.warn('TI VERSION: %s' % tiVersion)
        if tiVersion is None:
            tiVersion = 4
        if tiVersion > 10:
            status = d.extractByte(value)
            #DumperBase.warn('STATUS: %s' % status)
            if status & 0x01:
                # Short data
                msecs = d.extractUInt64(value) >> 8
                spec = (status & 0x30) >> 4
                offsetFromUtc = 0
                timeZone = 0
                isValid = status & 0x08
            else:
                dptr = d.extractPointer(value)
                (_, status, msecs, offsetFromUtc, _, timeZone) = d.split('iIqII{QTimeZone}', dptr)
                spec = (status & 0x30) >> 4
                isValid = True
                tzD = d.extractPointer(timeZone)
                if tzD == 0:
                    timeZone = 'UTC'
                else:
                    idAddr = tzD + 2 * d.ptrSize()
                    tzBa = d.encodeByteArray(idAddr, limit=100)
                    timeZone = tzBa
            d.putValue(
                '%s/%s/%s/%s/%s/%s' %
                (msecs,
                 spec,
                 offsetFromUtc,
                 timeZone,
                 status,
                 tiVersion),
                'datetimeinternal')
        else:
            if d.isWindowsTarget():
                msecsOffset = 8
                specOffset = 16
                offsetFromUtcOffset = 20
                timeZoneOffset = 24
                statusOffset = 28 if is32bit else 32
            else:
                msecsOffset = 4 if is32bit else 8
                specOffset = 12 if is32bit else 16
                offsetFromUtcOffset = 16 if is32bit else 20
                timeZoneOffset = 20 if is32bit else 24
                statusOffset = 24 if is32bit else 32
            status = d.extractInt(base + statusOffset)
            if int(status & 0x0c == 0x0c):  # ValidDate and ValidTime
                isValid = True
                msecs = d.extractInt64(base + msecsOffset)
                spec = d.extractInt(base + specOffset)
                offset = d.extractInt(base + offsetFromUtcOffset)
                tzp = d.extractPointer(base + timeZoneOffset)
                if tzp == 0:
                    tz = ''
                else:
                    idBase = tzp + 2 * d.ptrSize()  # [QSharedData] + [vptr]
                    tz = d.encodeByteArray(idBase, limit=100)
                d.putValue('%s/%s/%s/%s/%s/%s' % (msecs, spec, offset, tz, status, 0),
                           'datetimeinternal')
    else:
        # This relies on the Qt4/Qt5 internal structure layout:
        # {sharedref(4), date(8), time(4+x)}
        # QDateTimePrivate:
        # - QAtomicInt ref;    (padded on 64 bit)
        # -     [QDate date;]
        # -      -  uint jd in Qt 4,  qint64 in Qt 5.0 and Qt 5.1; padded on 64 bit
        # -     [QTime time;]
        # -      -  uint mds;
        # -  Spec spec;
        dateSize = 8 if qtVersion >= 0x050000 else 4  # Qt5: qint64, Qt4 uint
        # 4 byte padding after 4 byte QAtomicInt if we are on 64 bit and QDate is 64 bit
        refPlusPadding = 8 if qtVersion >= 0x050000 and d.ptrSize() == 8 else 4
        dateBase = base + refPlusPadding
        timeBase = dateBase + dateSize
        mds = d.extractInt(timeBase)
        isValid = mds > 0
        if isValid:
            jd = d.extractInt(dateBase)
            d.putValue('%s/%s' % (jd, mds), 'juliandateandmillisecondssincemidnight')

    if not isValid:
        d.putValue('(invalid)')
        return

    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putCallItem('toTime_t', 'unsigned int', value, 'toTime_t')
            if d.canCallLocale():
                d.putCallItem('toString', '@QString', value, 'toString',
                              d.enumExpression('DateFormat', 'TextDate'))
                d.putCallItem('(ISO)', '@QString', value, 'toString',
                              d.enumExpression('DateFormat', 'ISODate'))
                d.putCallItem('toUTC', '@QDateTime', value, 'toTimeSpec',
                              d.enumExpression('TimeSpec', 'UTC'))
                if d.qtVersion() < 0x060000:
                    d.putCallItem('(SystemLocale)', '@QString', value, 'toString',
                                  d.enumExpression('DateFormat', 'SystemLocaleDate'))
                    d.putCallItem('(Locale)', '@QString', value, 'toString',
                                  d.enumExpression('DateFormat', 'LocaleDate'))
                d.putCallItem('toLocalTime', '@QDateTime', value, 'toTimeSpec',
                              d.enumExpression('TimeSpec', 'LocalTime'))
            d.putFields(value)


def qdump__QDir(d, value):
    d.putExpandable()
    privAddress = d.extractPointer(value)
    bit32 = d.ptrSize() == 4

    # change fc3942114da adds FileCache
    # QStringList nameFilters;
    # QDir::SortFlags sort;
    # QDir::Filters filters;
    # std::unique_ptr<QAbstractFileEngine> fileEngine;
    # QFileSystemEntry dirEntry;
    # struct FileCache
    # {
    #     QMutex mutex;
    #     QStringList files;
    #     QFileInfoList fileInfos;
    #     std::atomic<bool> fileListsInitialized = false;
    #     QFileSystemEntry absoluteDirEntry;
    #     QFileSystemMetaData metaData;
    # };
    # mutable FileCache fileCache;


    # Change 9fc0965 reorders members again.
    #   bool fileListsInitialized
    #   QStringList files
    #   QFileInfoList fileInfos
    #   QStringList nameFilters
    #   QDir::SortFlags sort
    #   QDir::Filters filters

    # Before 9fc0965:
    # QDirPrivate:
    #   QAtomicInt ref
    #   QStringList nameFilters;
    #   QDir::SortFlags sort;
    #   QDir::Filters filters;
    #   // qt3support:
    #   QChar filterSepChar;
    #   bool matchAllDirs;
    #   // end qt3support
    #   QScopedPointer<QAbstractFileEngine> fileEngine;
    #   bool fileListsInitialized;
    #   QStringList files;
    #   QFileInfoList fileInfos;
    #   QFileSystemEntry dirEntry;
    #   QFileSystemEntry absoluteDirEntry;

    # QFileSystemEntry:
    #   QString m_filePath
    #   QByteArray m_nativeFilePath
    #   qint16 m_lastSeparator
    #   qint16 m_firstDotInFileName
    #   qint16 m_lastDotInFileName
    #   + 2 byte padding
    fileSystemEntrySize = 2 * d.ptrSize() + 8

    if d.qtVersion() >= 0x060600:
        case = 3
    elif d.qtVersion() >= 0x060000:
        case = 2
    elif d.qtVersion() >= 0x050300:
        case = 1
    elif d.qtVersion() < 0x050200:
        case = 0
    else:
        # Try to distinguish bool vs QStringList at the first item
        # after the (padded) refcount. If it looks like a bool assume
        # this is after 9fc0965. This is not safe.
        firstValue = d.extractInt(privAddress + d.ptrSize())
        case = 1 if firstValue == 0 or firstValue == 1 else 0

    if case == 3:
        if bit32:
            dirEntryOffset = 24
            fileCacheOffset = 52
            filesOffset = fileCacheOffset + 4
            fileInfosOffset = fileCacheOffset + 16
            absoluteDirEntryOffset = fileCacheOffset + 32
        else:
            dirEntryOffset = 48
            fileCacheOffset = 104
            filesOffset = fileCacheOffset + 8
            fileInfosOffset = fileCacheOffset + 32
            absoluteDirEntryOffset = fileCacheOffset + 64
    elif case == 2:
        if bit32:
            filesOffset = 4
            fileInfosOffset = 16
            dirEntryOffset = 40
            absoluteDirEntryOffset = 72
        else:
            filesOffset = 8
            fileInfosOffset = 32
            dirEntryOffset = 96
            absoluteDirEntryOffset = 152
    elif case == 1:
        if bit32:
            filesOffset = 4
            fileInfosOffset = 8
            dirEntryOffset = 32
            absoluteDirEntryOffset = 48
        else:
            filesOffset = 8
            fileInfosOffset = 16
            dirEntryOffset = 48
            absoluteDirEntryOffset = 72
    else:
        # Assume this is before 9fc0965.
        qt3support = d.isQt3Support()
        qt3SupportAddition = d.ptrSize() if qt3support else 0
        filesOffset = (24 if bit32 else 40) + qt3SupportAddition
        fileInfosOffset = filesOffset + d.ptrSize()
        dirEntryOffset = fileInfosOffset + d.ptrSize()
        absoluteDirEntryOffset = dirEntryOffset + fileSystemEntrySize

    d.putStringValue(privAddress + dirEntryOffset)
    if d.isExpanded():
        with Children(d):
            if not d.isMsvcTarget():
                ns = d.qtNamespace()
                try:
                    d.call('int', value, 'count')  # Fill cache.
                except:
                    pass

                #d.putCallItem('absolutePath', '@QString', value, 'absolutePath')
                #d.putCallItem('canonicalPath', '@QString', value, 'canonicalPath')
                with SubItem(d, 'absolutePath'):
                    typ = d.lookupType(ns + 'QString')
                    d.putItem(d.createValue(privAddress + absoluteDirEntryOffset, typ))
                with SubItem(d, 'entryInfoList'):
                    typ = d.lookupType(ns + 'QFileInfo')
                    qdumpHelper_QList(d, privAddress + fileInfosOffset, typ)
                with SubItem(d, 'entryList'):
                    typ = d.lookupType(ns + 'QStringList')
                    d.putItem(d.createValue(privAddress + filesOffset, typ))
            d.putFields(value)


def qdump__QEvent(d, value):
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            # Add a sub-item with the event type.
            with SubItem(d, '[type]'):
                (vtable, privateD, t, flags) = value.split("pp{short}{short}")
                event_type_name = d.qtNamespace() + "QEvent::Type"
                type_value = t.cast(event_type_name)
                d.putValue(type_value.displayEnum('0x%04x', bitsize=16))
                d.putType(event_type_name)

            # Show the rest of the class fields as usual.
            d.putFields(value)


def qdump__QKeyEvent(d, value):
    # QEvent fields
    #   virtual table pointer
    #   QEventPrivate *d;
    #   ushort t;
    #   ushort posted : 1;
    #   ushort spont : 1;
    #   ushort m_accept : 1;
    #   ushort reserved : 13;
    # QInputEvent fields
    #   Qt::KeyboardModifiers modState;
    #   ulong ts;
    # QKeyEvent fields
    #   QString txt;
    #   int k;
    #   quint32 nScanCode;
    #   quint32 nVirtualKey;
    #   quint32 nModifiers; <- nativeModifiers
    #   ushort c;
    #   ushort autor:1;
    #   ushort reserved:15;
    (vtable, privateD, t, flags, modState, ts, txt, k, scanCode,
     virtualKey, modifiers,
     c, autor) = value.split("ppHHiQ{@QString}{int}IIIHH")

    #d.putStringValue(txt)
    #data = d.encodeString(txt)
    key_txt_utf8 = d.encodeStringUtf8(txt)

    k_type_name = d.qtNamespace() + "Qt::Key"
    k_cast_to_enum_value = k.cast(k_type_name)
    k_name = k_cast_to_enum_value.displayEnum(bitsize=32)
    matches = re.search(r'Key_(\w+)', k_name)
    if matches:
        k_name = matches.group(1)

    if t == 6:
        key_event_type = "Pressed"
    elif t == 7:
        key_event_type = "Released"
    else:
        key_event_type = ""

    data = ""

    if key_event_type:
        data += "{} ".format(key_event_type)

    # Try to use the name of the enum value, otherwise the value
    # of txt in QKeyEvent.
    if k_name:
        data += "'{}'".format(k_name)
    elif key_txt_utf8:
        data += "'{}'".format(key_txt_utf8)
    else:
        data += "<non-ascii>"

    k_int = k.integer()
    data += " (key:{} vKey:{}".format(k_int, virtualKey)

    modifier_list = []
    modifier_list.append(("Shift", 0x02000000))
    modifier_list.append(("Control", 0x04000000))
    modifier_list.append(("Alt", 0x08000000))
    modifier_list.append(("Meta", 0x10000000))
    # modifier_map.append(("KeyPad", 0x20000000)) Is this useful?
    modifier_list.append(("Grp", 0x40000000))

    modifiers = []
    for modifier_name, mask in modifier_list:
        if modState & mask:
            modifiers.append(modifier_name)

    if modifiers:
        data += " mods:" + "+".join(modifiers)

    data += ")"

    d.putValue(d.hexencode(data), 'utf8')

    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            # Add a sub-item with the enum name and value.
            with SubItem(d, '[{}]'.format(k_type_name)):
                k_cast_to_enum_value = k.cast(k_type_name)
                d.putValue(k_cast_to_enum_value.displayEnum('0x%04x', bitsize=32))
                d.putType(k_type_name)

            # Show the rest of the class fields as usual.
            d.putFields(value, dumpBase=True)


def qdump__QKeySequence(d, value):
    dd = d.extractPointer(value)
    _, k0, k1, k2, k3 = d.split('iiiii', dd)
    d.putValue("(0x%x, 0x%x, 0x%x, 0x%x)" % (k0, k1, k2, k3));
    d.putPlainChildren(value)


def qdump__QFile(d, value):
    # 9fc0965 and a373ffcd change the layout of the private structure
    qtVersion = d.qtVersion()
    is32bit = d.ptrSize() == 4
    # FIXME:  values 0 are wrong. As the file name is the
    # only direct member of QFilePrivate, the offsets are
    # equal to sizeof(QFileDevicePrivate), the base class.
    if qtVersion >= 0x060300 and d.qtTypeInfoVersion() >= 22:
        if d.isWindowsTarget():
            if d.isMsvcTarget():
                offset = 0 if is32bit else 424
            else:
                offset = 0 if is32bit else 424
        else:
            offset = 300 if is32bit else 424
    elif qtVersion >= 0x060000 and d.qtTypeInfoVersion() >= 20:
        if d.isWindowsTarget():
            if d.isMsvcTarget():
                offset = 0 if is32bit else 304
            else:
                offset = 0 if is32bit else 304
        else:
            offset = 196 if is32bit else 304
    elif qtVersion >= 0x050600 and d.qtTypeInfoVersion() >= 17:
        # Some QRingBuffer member got removed in 8f92baf5c9
        if d.isWindowsTarget():
            if d.isMsvcTarget():
                offset = 164 if is32bit else 224
            else:
                offset = 160 if is32bit else 224
        else:
            offset = 156 if is32bit else 224
    elif qtVersion >= 0x050700:
        if d.isWindowsTarget():
            if d.isMsvcTarget():
                offset = 176 if is32bit else 248
            else:
                offset = 172 if is32bit else 248
        else:
            offset = 168 if is32bit else 248
    elif qtVersion >= 0x050600:
        if d.isWindowsTarget():
            if d.isMsvcTarget():
                offset = 184 if is32bit else 248
            else:
                offset = 180 if is32bit else 248
        else:
            offset = 168 if is32bit else 248
    elif qtVersion >= 0x050500:
        if d.isWindowsTarget():
            offset = 164 if is32bit else 248
        else:
            offset = 164 if is32bit else 248
    elif qtVersion >= 0x050400:
        if d.isWindowsTarget():
            offset = 188 if is32bit else 272
        else:
            offset = 180 if is32bit else 272
    elif qtVersion > 0x050200:
        if d.isWindowsTarget():
            offset = 180 if is32bit else 272
        else:
            offset = 176 if is32bit else 272
    elif qtVersion >= 0x050000:
        offset = 176 if is32bit else 280
    else:
        if d.isWindowsTarget():
            offset = 144 if is32bit else 232
        else:
            offset = 140 if is32bit else 232
    vtable, privAddress = value.split('pp')
    fileNameAddress = privAddress + offset
    d.putStringValue(fileNameAddress)
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putCallItem('exists', 'bool', value, 'exists')
            d.putFields(value)


def qdump__QFileInfo(d, value):
    privAddress = d.extractPointer(value)
    #bit32 = d.ptrSize() == 4
    #qt5 = d.qtVersion() >= 0x050000
    #try:
    #    d.putStringValue(value['d_ptr']['d'].dereference()['fileNames'][3])
    #except:
    #    d.putPlainChildren(value)
    #    return
    filePathAddress = privAddress + d.ptrSize()
    d.putStringValue(filePathAddress)
    d.putExpandable()
    if d.isExpanded():
        ns = d.qtNamespace()
        with Children(d):
            stype = '@QString'
            d.putCallItem('absolutePath', stype, value, 'absolutePath')
            d.putCallItem('absoluteFilePath', stype, value, 'absoluteFilePath')
            d.putCallItem('canonicalPath', stype, value, 'canonicalPath')
            d.putCallItem('canonicalFilePath', stype, value, 'canonicalFilePath')
            d.putCallItem('completeBaseName', stype, value, 'completeBaseName')
            d.putCallItem('completeSuffix', stype, value, 'completeSuffix')
            d.putCallItem('baseName', stype, value, 'baseName')
            if platform.system() == 'Darwin':
                d.putCallItem('isBundle', stype, value, 'isBundle')
                d.putCallItem('bundleName', stype, value, 'bundleName')
            d.putCallItem('fileName', stype, value, 'fileName')
            d.putCallItem('filePath', stype, value, 'filePath')
            # Crashes gdb (archer-tromey-python, at dad6b53fe)
            #d.putCallItem('group', value, 'group')
            #d.putCallItem('owner', value, 'owner')
            d.putCallItem('path', stype, value, 'path')

            d.putCallItem('groupid', 'unsigned int', value, 'groupId')
            d.putCallItem('ownerid', 'unsigned int', value, 'ownerId')

            #QFile::Permissions permissions () const
            try:
                perms = d.call('int', value, 'permissions')
            except:
                perms = None

            if perms is None:
                with SubItem(d, 'permissions'):
                    d.putSpecialValue('notcallable')
                    d.putType(ns + 'QFile::Permissions')
            else:
                with SubItem(d, 'permissions'):
                    d.putEmptyValue()
                    d.putType(ns + 'QFile::Permissions')
                    d.putExpandable()
                    if d.isExpanded():
                        with Children(d, 10):
                            perms = perms['i']
                            d.putBoolItem('ReadOwner',  perms & 0x4000)
                            d.putBoolItem('WriteOwner', perms & 0x2000)
                            d.putBoolItem('ExeOwner',   perms & 0x1000)
                            d.putBoolItem('ReadUser',   perms & 0x0400)
                            d.putBoolItem('WriteUser',  perms & 0x0200)
                            d.putBoolItem('ExeUser',    perms & 0x0100)
                            d.putBoolItem('ReadGroup',  perms & 0x0040)
                            d.putBoolItem('WriteGroup', perms & 0x0020)
                            d.putBoolItem('ExeGroup',   perms & 0x0010)
                            d.putBoolItem('ReadOther',  perms & 0x0004)
                            d.putBoolItem('WriteOther', perms & 0x0002)
                            d.putBoolItem('ExeOther',   perms & 0x0001)

            #QDir absoluteDir () const
            #QDir dir () const
            d.putCallItem('caching', 'bool', value, 'caching')
            d.putCallItem('exists', 'bool', value, 'exists')
            d.putCallItem('isAbsolute', 'bool', value, 'isAbsolute')
            d.putCallItem('isDir', 'bool', value, 'isDir')
            d.putCallItem('isExecutable', 'bool', value, 'isExecutable')
            d.putCallItem('isFile', 'bool', value, 'isFile')
            d.putCallItem('isHidden', 'bool', value, 'isHidden')
            d.putCallItem('isReadable', 'bool', value, 'isReadable')
            d.putCallItem('isRelative', 'bool', value, 'isRelative')
            d.putCallItem('isRoot', 'bool', value, 'isRoot')
            d.putCallItem('isSymLink', 'bool', value, 'isSymLink')
            d.putCallItem('isWritable', 'bool', value, 'isWritable')
            d.putCallItem('created', 'bool', value, 'created')
            d.putCallItem('lastModified', 'bool', value, 'lastModified')
            d.putCallItem('lastRead', 'bool', value, 'lastRead')
            d.putFields(value)


def qdump__QFixed(d, value):
    v = value.split('i')[0]
    d.putValue('%s/64 = %s' % (v, v / 64.0))


def qform__QFiniteStack():
    return [DisplayFormat.ArrayPlot]


def qdump__QFiniteStack(d, value):
    array, alloc, size = value.split('pii')
    d.check(0 <= size and size <= alloc and alloc <= 1000 * 1000 * 1000)
    d.putItemCount(size)
    d.putPlotData(array, size, value.type[0])


def qdump__QFlags(d, value):
    i = value.split('{int}')[0]
    enumType = value.type[0]
    v = i.cast(enumType.name)
    d.putValue(v.displayEnum('0x%04x', bitsize=32))


def qform__QHash():
    return [DisplayFormat.CompactMap]


def qdump__QHash(d, value):
    qdumpHelper_QHash(d, value, value.type[0], value.type[1])


def qdump__QVariantHash(d, value):
    qdumpHelper_QHash(d, value, d.createType('@QString'), d.createType('@QVariant'))


def qdumpHelper_QHash(d, value, keyType, valueType):
    if d.qtVersion() >= 0x60000:
        qdumpHelper_QHash_6(d, value, keyType, valueType)
    else:
        qdumpHelper_QHash_5(d, value, keyType, valueType)

def qdumpHelper_QHash_5(d, value, keyType, valueType):
    def hashDataFirstNode():
        b = buckets
        n = numBuckets
        while n:
            n -= 1
            bb = d.extractPointer(b)
            if bb != dptr:
                return bb
            b += ptrSize
        return dptr

    def hashDataNextNode(node):
        (nextp, h) = d.split('pI', node)
        if d.extractPointer(nextp):
            return nextp
        start = (h % numBuckets) + 1
        b = buckets + start * ptrSize
        n = numBuckets - start
        while n:
            n -= 1
            bb = d.extractPointer(b)
            if bb != nextp:
                return bb
            b += ptrSize
        return nextp

    ptrSize = d.ptrSize()
    dptr = d.extractPointer(value)
    (fakeNext, buckets, ref, size, nodeSize, userNumBits, numBits, numBuckets) = \
        d.split('ppiiihhi', dptr)

    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.check(-1 <= ref and ref < 100000)

    d.putItemCount(size)
    if d.isExpanded():
        isShort = d.qtVersion() < 0x050000 and keyType.name == 'int'
        with Children(d, size):
            node = hashDataFirstNode()
            for i in d.childRange():
                if isShort:
                    typeCode = 'P{%s}@{%s}' % (keyType.name, valueType.name)
                    (pnext, key, padding2, val) = d.split(typeCode, node)
                else:
                    typeCode = 'Pi@{%s}@{%s}' % (keyType.name, valueType.name)
                    (pnext, hashval, padding1, key, padding2, val) = d.split(typeCode, node)
                d.putPairItem(i, (key, val), 'key', 'value')
                node = hashDataNextNode(node)


def qdumpHelper_QHash_6(d, value, keyType, valueType):
    dptr = d.extractPointer(value)
    if dptr == 0:
        d.putItemCount(0)
        return

    ref, _, size, buckets, seed, spans = d.split('i@qqqp', dptr)

    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.check(-1 <= ref and ref < 100000)
    #d.putValue("%d 0x%x 0x%x 0x%x" % (ref, size, buckets, seed));
    d.putItemCount(size)

    if d.isExpanded():
        type_code = '{%s}@{%s}' % (keyType.name, valueType.name)
        _, entry_size, _ = d.describeStruct(type_code)
        with Children(d, size):
            span_size = 128 + 2 * d.ptrSize() # Including tail padding.
            nspans = int((buckets + 127) / 128)
            count = 0
            for b in range(nspans):
                span = spans + b * span_size
                offsets, entries, allocated, next_free = d.split('128spbb', span)
                #with SubItem(d, 'span %d' % b):
                #   d.putValue('span: 0x%x  %s alloc: %s next: %s'
                #       % (span, d.hexencode(offsets), allocated, next_free))
                entry_pos = 0
                for i in range(128):
                    offset = offsets[i]
                    #with SubItem(d, 'offset %i' % i):
                    #    d.putValue('i: %s off: %s' % (i, offset))
                    if offset != 255: # Entry is used
                        entry = entries + offset * entry_size
                        key, _, val = d.split(type_code, entry)
                        #with SubItem(d, 'count %d  entry %d' % (count, i)):
                        #    d.putValue('i: %s entry: 0x%x' % (i, entry))
                        d.putPairItem(count, (key, val), 'key', 'value')
                        count += 1
                        entry_pos += 1
            #with SubItem(d, 'total'):
            #    d.putValue('total: %s item size: %s' % (count, entry_size))


def qform__QHashNode():
    return [DisplayFormat.CompactMap]


def qdump__QHashNode(d, value):
    d.putPairItem(None, value)


def qHashIteratorHelper(d, value):
    typeName = value.type.name
    hashTypeName = typeName[0:typeName.rfind('::')]
    hashType = d.lookupType(hashTypeName)
    keyType = hashType[0]
    valueType = hashType[1]
    d.putExpandable()
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            node = d.extractPointer(value)
            isShort = d.qtVersion() < 0x050000 and keyType.name == 'int'
            if isShort:
                typeCode = 'P{%s}@{%s}' % (keyType.name, valueType.name)
                (pnext, key, padding2, val) = d.split(typeCode, node)
            else:
                typeCode = 'Pi@{%s}@{%s}' % (keyType.name, valueType.name)
                (pnext, hashval, padding1, key, padding2, val) = d.split(typeCode, node)
            d.putSubItem('key', key)
            d.putSubItem('value', val)


def qdump__QHash__const_iterator(d, value):
    qHashIteratorHelper(d, value)


def qdump__QHash__iterator(d, value):
    qHashIteratorHelper(d, value)


def qdump__QHostAddress(d, value):
    dd = d.extractPointer(value)
    qtVersion = d.qtVersion()
    tiVersion = d.qtTypeInfoVersion()
    #DumperBase.warn('QT: %x, TI: %s' % (qtVersion, tiVersion))
    mayNeedParse = True
    if tiVersion is not None:
        if tiVersion >= 16:
            # After a6cdfacf
            p, scopeId, a6, a4, protocol = d.split('p{@QString}16s{@quint32}B', dd)
            mayNeedParse = False
        elif tiVersion >= 5:
            # Branch 5.8.0 at f70b4a13  TI: 15
            # Branch 5.7.0 at b6cf0418  TI: 5
            (ipString, scopeId, a6, a4, protocol, isParsed) \
                = d.split('{@QString}{@QString}16s{@quint32}B{bool}', dd)
        else:
            (ipString, scopeId, a4, pad, a6, protocol, isParsed) \
                = d.split('{@QString}{@QString}{@quint32}I16sI{bool}', dd)
    elif qtVersion >= 0x050600:  # 5.6.0 at f3aabb42
        if d.ptrSize() == 8 or d.isWindowsTarget():
            (ipString, scopeId, a4, pad, a6, protocol, isParsed) \
                = d.split('{@QString}{@QString}{@quint32}I16sI{bool}', dd)
        else:
            (ipString, scopeId, a4, a6, protocol, isParsed) \
                = d.split('{@QString}{@QString}{@quint32}16sI{bool}', dd)
    elif qtVersion >= 0x050000:  # 5.2.0 at 62feb088
        (ipString, scopeId, a4, a6, protocol, isParsed) \
            = d.split('{@QString}{@QString}{@quint32}16sI{bool}', dd)
    else:  # 4.8.7 at b05d05f
        (a4, a6, protocol, pad, ipString, isParsed, pad, scopeId) \
            = d.split('{@quint32}16sB@{@QString}{bool}@{@QString}', dd)

    if mayNeedParse:
        ipStringData, ipStringSize, ipStringAlloc = d.stringData(ipString)
    if mayNeedParse and isParsed.integer() and ipStringSize > 0:
        d.putStringValue(ipString)
    else:
        # value.d.d->protocol:
        #  QAbstractSocket::IPv4Protocol = 0
        #  QAbstractSocket::IPv6Protocol = 1
        if protocol == 1:
            # value.d.d->a6
            data = d.hexencode(a6)
            address = ':'.join('%x' % int(data[i:i + 4], 16) for i in range(0, 32, 4))
            d.putValue(address)
        elif protocol == 0:
            # value.d.d->a
            a = a4.integer()
            a, n4 = divmod(a, 256)
            a, n3 = divmod(a, 256)
            a, n2 = divmod(a, 256)
            a, n1 = divmod(a, 256)
            d.putValue('%d.%d.%d.%d' % (n1, n2, n3, n4))
        else:
            d.putValue('<unspecified protocol %s>' % protocol)

    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            if mayNeedParse:
                d.putSubItem('ipString', ipString)
                d.putSubItem('isParsed', isParsed)
            d.putSubItem('scopeId', scopeId)
            d.putSubItem('a', a4)


def qdump__QIPv6Address(d, value):
    raw = d.split('16s', value)[0]
    data = d.hexencode(raw)
    d.putValue(':'.join('%x' % int(data[i:i + 4], 16) for i in range(0, 32, 4)))
    d.putArrayData(value.address(), 16, d.lookupType('unsigned char'))


def qform__QList():
    return [DisplayFormat.DirectQListStorage, DisplayFormat.IndirectQListStorage]


def qdump__QList(d, value):
    return qdumpHelper_QList(d, value, d.createType(value.type[0]))


def qdump__QVariantList(d, value):
    qdumpHelper_QList(d, value, d.createType('@QVariant'))


def qdumpHelper_QList(d, value, innerType):
    data, size = d.listData(value, check=True)
    d.putItemCount(size)

    if d.qtVersion() >= 0x60000:
        d.putPlotData(data, size, innerType)
        return

    if d.isExpanded():
        innerSize = innerType.size()
        stepSize = d.ptrSize()
        # The exact condition here is:
        #  QTypeInfo<T>::isLarge || QTypeInfo<T>::isStatic
        # but this data is available neither in the compiled binary nor
        # in the frontend.
        # So as first approximation only do the 'isLarge' check:
        displayFormat = d.currentItemFormat()
        if displayFormat == DisplayFormat.DirectQListStorage:
            isInternal = True
        elif displayFormat == DisplayFormat.IndirectQListStorage:
            isInternal = False
        else:
            isInternal = innerSize <= stepSize and innerType.isMovableType()
        if isInternal:
            if innerSize == stepSize:
                d.putArrayData(data, size, innerType)
            else:
                with Children(d, size, childType=innerType):
                    for i in d.childRange():
                        p = d.createValue(data + i * stepSize, innerType)
                        d.putSubItem(i, p)
        else:
            # about 0.5s / 1000 items
            with Children(d, size, maxNumChild=2000, childType=innerType):
                for i in d.childRange():
                    p = d.extractPointer(data + i * stepSize)
                    x = d.createValue(p, innerType)
                    d.putSubItem(i, x)


def qform__QImage():
    return [DisplayFormat.Simple, DisplayFormat.Separate]


def qdump__QImage(d, value):
    if d.qtVersion() >= 0x060000:
        vtbl, painters, image_data = value.split('ppp')
    elif d.qtVersion() >= 0x050000:
        vtbl, painters, reserved, image_data = value.split('pppp')
    else:
        vtbl, painters, image_data = value.split('ppp')

    if image_data == 0:
        d.putValue('(invalid)')
        return

    ref, width, height = d.split('iii', image_data)
    d.putValue('(%dx%d)' % (width, height))

    d.putExpandable()
    if d.isExpanded():
        if d.qtVersion() < 0x060000:
            (ref, width, height, depth, nbytes, pad, devicePixelRatio, colorTable,
                bits, iformat) = d.split('iiiii@dppi', image_data)
        else:
            (ref, width, height, depth, nbytes, pad, devicePixelRatio, _, _, _,
                bits, iformat) = d.split('iiiii@dppppi', image_data)
        with Children(d):
            d.putIntItem('width', width)
            d.putIntItem('height', height)
            d.putIntItem('nbytes', nbytes)
            d.putIntItem('format', iformat)
            with SubItem(d, 'data'):
                d.putValue('0x%x' % bits)
                d.putType('void *')

    displayFormat = d.currentItemFormat()
    if displayFormat == DisplayFormat.Separate:
        d.putDisplay('imagedata:separate', '%08x%08x%08x%08x' % (width, height, nbytes, iformat)
                     + d.readMemory(bits, nbytes))


def qdump__QLinkedList(d, value):
    dd = d.extractPointer(value)
    ptrSize = d.ptrSize()
    n = d.extractInt(dd + 4 + 2 * ptrSize)
    ref = d.extractInt(dd + 2 * ptrSize)
    d.check(0 <= n and n <= 100 * 1000 * 1000)
    d.check(-1 <= ref and ref <= 1000)
    d.putItemCount(n)
    if d.isExpanded():
        innerType = value.type[0]
        with Children(d, n, maxNumChild=1000, childType=innerType):
            pp = d.extractPointer(dd)
            for i in d.childRange():
                d.putSubItem(i, d.createValue(pp + 2 * ptrSize, innerType))
                pp = d.extractPointer(pp)


qqLocalesCount = None


def qdump__QLocale(d, value):
    # Check for uninitialized 'index' variable. Retrieve size of
    # QLocale data array from variable in qlocale.cpp.
    # Default is 368 in Qt 4.8, 438 in Qt 5.0.1, the last one
    # being 'System'.
    #global qqLocalesCount
    #if qqLocalesCount is None:
    #    #try:
    #        qqLocalesCount = int(value(ns + 'locale_data_size'))
    #    #except:
    #        qqLocalesCount = 438
    #try:
    #    index = int(value['p']['index'])
    #except:
    #    try:
    #        index = int(value['d']['d']['m_index'])
    #    except:
    #        index = int(value['d']['d']['m_data']...)
    #d.check(index >= 0)
    #d.check(index <= qqLocalesCount)
    if d.qtVersion() < 0x50000:
        d.putStringValue(d.call('const char *', value, 'name'))
        d.putPlainChildren(value)
        return

    ns = d.qtNamespace()
    dd = value.extractPointer()
    (data, ref, numberOptions) = d.split('pi4s', dd)
    (languageId, scriptId, countryId,
     decimal, group, listt, percent, zero,
     minus, plus, exponential) \
        = d.split('2s{short}2s'
                  + '{@QChar}{@QChar}{short}{@QChar}{@QChar}'
                  + '{@QChar}{@QChar}{@QChar}', data)
    try:
        d.putStringValue(d.call('const char *', value, 'name'))
    except:
        pass
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            prefix = ns + 'QLocale::'
            d.putSubItem('country', d.createValue(countryId, prefix + 'Country'))
            d.putSubItem('language', d.createValue(languageId, prefix + 'Language'))
            d.putSubItem('numberOptions', d.createValue(numberOptions, prefix + 'NumberOptions'))
            d.putSubItem('decimalPoint', decimal)
            d.putSubItem('exponential', exponential)
            d.putSubItem('percent', percent)
            d.putSubItem('zeroDigit', zero)
            d.putSubItem('groupSeparator', group)
            d.putSubItem('negativeSign', minus)
            d.putSubItem('positiveSign', plus)
            d.putCallItem('measurementSystem', '@QLocale::MeasurementSystem',
                          value, 'measurementSystem')
            d.putCallItem('timeFormat_(short)', '@QString',
                          value, 'timeFormat', ns + 'QLocale::ShortFormat')
            d.putCallItem('timeFormat_(long)', '@QString',
                          value, 'timeFormat', ns + 'QLocale::LongFormat')
            d.putFields(value)


def qdump__QMapNode(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putSubItem('key', value['key'])
            d.putSubItem('value', value['value'])


def qdumpHelper_Qt4_QMap(d, value, keyType, valueType):
    dd = value.extractPointer()
    (dummy, it, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11,
        ref, toplevel, n) = d.split('p' * 13 + 'iii', dd)
    d.check(0 <= n and n <= 100 * 1000 * 1000)
    d.checkRef(ref)
    d.putItemCount(n)
    if d.isExpanded():
        if n > 10000:
            n = 10000

        typeCode = '{%s}@{%s}' % (keyType.name, valueType.name)
        pp, payloadSize, fields = d.describeStruct(typeCode)

        with Children(d, n):
            for i in d.childRange():
                key, pad, value = d.split(typeCode, it - payloadSize)
                d.putPairItem(i, (key, value), 'key', 'value')
                dummy, it = d.split('Pp', it)


def qdumpHelper_Qt5_QMap(d, value, keyType, valueType):
    dptr = d.extractPointer(value)
    (ref, n) = d.split('ii', dptr)
    d.check(0 <= n and n <= 100 * 1000 * 1000)
    d.check(-1 <= ref and ref < 100000)

    d.putItemCount(n)
    if d.isExpanded():
        if n > 10000:
            n = 10000

        typeCode = 'ppp@{%s}@{%s}' % (keyType.name, valueType.name)

        def helper(node):
            (p, left, right, padding1, key, padding2, value) = d.split(typeCode, node)
            if left:
                for res in helper(left):
                    yield res
            yield (key, value)
            if right:
                for res in helper(right):
                    yield res

        with Children(d, n):
            for (pair, i) in zip(helper(dptr + 8), range(n)):
                d.putPairItem(i, pair, 'key', 'value')


def qdumpHelper_Qt6_QMap(d, value, keyType, valueType):
    d_ptr = d.extractPointer(value)
    if d_ptr == 0:
        d.putItemCount(0)
        return
    m = value['d']['d']['m']
    d.putItem(m)
    d.putBetterType('@QMap<%s, %s>' % (keyType.name, valueType.name))


def qform__QMap():
    return [DisplayFormat.CompactMap]


def qdump__QMap(d, value):
    qdumpHelper_QMap(d, value, value.type[0], value.type[1])


def qdumpHelper_QMap(d, value, keyType, valueType):
    if d.qtVersion() >= 0x60000:
        qdumpHelper_Qt6_QMap(d, value, keyType, valueType)
    elif d.qtVersion() >= 0x50000:
        qdumpHelper_Qt5_QMap(d, value, keyType, valueType)
    else:
        qdumpHelper_Qt4_QMap(d, value, keyType, valueType)


def qform__QMultiMap():
    return [DisplayFormat.CompactMap]


def qdumpHelper_Qt6_QMultiMap(d, value, keyType, valueType):
    d_ptr = d.extractPointer(value)
    if d_ptr == 0:
        d.putItemCount(0)
        return
    m = value['d']['d']['m']
    d.putItem(m)
    d.putBetterType('@QMultiMap<%s, %s>' % (keyType.name, valueType.name))

def qdump__QMultiMap(d, value):
    if d.qtVersion() >= 0x60000:
        qdumpHelper_Qt6_QMultiMap(d, value, value.type[0], value.type[1])
    else:
        qdump__QMap(d, value)


def qform__QVariantMap():
    return [DisplayFormat.CompactMap]


def qdump__QVariantMap(d, value):
    qdumpHelper_QMap(d, value, d.createType('@QString'), d.createType('@QVariant'))
    d.putBetterType('@QVariantMap')


def qdump__QMetaMethod(d, value):
    d.putQMetaStuff(value, 'QMetaMethod')


def qdump__QMetaEnum(d, value):
    d.putQMetaStuff(value, 'QMetaEnum')


def qdump__QMetaProperty(d, value):
    d.putQMetaStuff(value, 'QMetaProperty')


def qdump__QMetaClassInfo(d, value):
    d.putQMetaStuff(value, 'QMetaClassInfo')


def qdump__QMetaObject(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putQObjectGutsHelper(0, 0, -1, value.address(), 'QMetaObject')
            d.putMembersItem(value)


if False:
    def qdump__QObjectPrivate__ConnectionList(d, value):
        d.putExpandable()
        if d.isExpanded():
            i = 0
            with Children(d):
                first, last = value.split('pp')
                currentConnection = first
                connectionType = d.createType('@QObjectPrivate::Connection')
                while currentConnection and currentConnection != last:
                    sender, receiver, slotObj, nextConnectionList, nextp, prev = \
                        d.split('pppppp', currentConnection)
                    d.putSubItem(i, d.createValue(currentConnection, connectionType))
                    currentConnection = nextp
                    i += 1
                d.putFields(value)
            d.putItemCount(i)
        else:
            d.putSpecialValue('minimumitemcount', 0)


def qdump__QPair(d, value):
    typeCode = '{%s}@{%s}' % (value.type[0].name, value.type[1].name)
    first, pad, second = value.split(typeCode)
    with Children(d):
        key = d.putSubItem('first', first)
        value = d.putSubItem('second', second)
    key = key.value if key.encoding is None else "..."
    value = value.value if value.encoding is None else "..."
    d.putValue('(%s, %s)' % (key, value))


def qdump__QProcEnvKey(d, value):
    d.putByteArrayValue(value)
    d.putPlainChildren(value)


def qdump__QPixmap(d, value):
    if d.qtVersion() >= 0x060000:
        vtbl, painters, data = value.split('ppp')
    elif d.qtVersion() >= 0x050000:
        vtbl, painters, reserved, data = s = d.split('pppp', value)
    else:
        vtbl, painters, data = value.split('ppp')

    if data == 0:
        d.putValue('(invalid)')
    else:
        _, width, height = d.split('pii', data)
        d.putValue('(%dx%d)' % (width, height))

    d.putPlainChildren(value)


def qdump__QMargins(d, value):
    d.putValue('left:%s, top:%s, right:%s, bottom:%s' % (value.split('iiii')))
    d.putPlainChildren(value)


def qdump__QPoint(d, value):
    d.putValue('(%s, %s)' % (value.split('ii')))
    d.putPlainChildren(value)


def qdump__QPointF(d, value):
    d.putValue('(%s, %s)' % (value.split('dd')))
    d.putPlainChildren(value)


def qdump__QRect(d, value):
    def pp(l): return ('+' if l >= 0 else '') + str(l)
    (x1, y1, x2, y2) = d.split('iiii', value)
    d.putValue('%sx%s%s%s' % (x2 - x1 + 1, y2 - y1 + 1, pp(x1), pp(y1)))
    d.putPlainChildren(value)


def qdump__QRectF(d, value):
    def pp(l): return ('+' if l >= 0 else '') + str(l)
    (x, y, w, h) = value.split('dddd')
    d.putValue('%sx%s%s%s' % (w, h, pp(x), pp(y)))
    d.putPlainChildren(value)


def qdump__QRegExp(d, value):
    # value.priv.engineKey.pattern
    privAddress = d.extractPointer(value)
    (eng, pattern) = d.split('p{@QString}', privAddress)
    d.putStringValue(pattern)
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            try:
                d.call('void', value, 'capturedTexts')  # Warm up internal cache.
            except:
                # Might fail (LLDB, Core files, ...), still cache might be warm.
                pass
            (patternSyntax, caseSensitive, minimal, pad, t, captures) \
                = d.split('{int}{int}B@{@QString}{@QStringList}', privAddress + 2 * d.ptrSize())
            d.putSubItem('syntax', patternSyntax.cast(d.qtNamespace() + 'QRegExp::PatternSyntax'))
            d.putSubItem('captures', captures)


def qdump__QRegion(d, value):
    d_ptr = d.extractPointer(value)
    if d_ptr == 0:
        d.putSpecialValue('empty')
    else:
        if d.qtVersion() >= 0x060000:
            ref, _, rgn = d.split('i@p', d_ptr)
            numRects, innerArea, rects, extents, innerRect = \
                d.split('ii{@QList<@QRect>}{@QRect}{@QRect}', rgn)
        elif d.qtVersion() >= 0x050400:  # Padding removed in ee324e4ed
            ref, _, rgn = d.split('i@p', d_ptr)
            numRects, innerArea, rects, extents, innerRect = \
                d.split('ii{@QVector<@QRect>}{@QRect}{@QRect}', rgn)
        elif d.qtVersion() >= 0x050000:
            ref, _, rgn = d.split('i@p', d_ptr)
            numRects, _, rects, extents, innerRect, innerArea = \
                d.split('i@{@QVector<@QRect>}{@QRect}{@QRect}i', rgn)
        else:
            if d.isWindowsTarget():
                ref, _, rgn = d.split('i@p', d_ptr)
            else:
                ref, _, xrgn, xrectangles, rgn = d.split('i@ppp', d_ptr)
            if rgn == 0:
                numRects = 0
            else:
                numRects, _, rects, extents, innerRect, innerArea = \
                    d.split('i@{@QVector<@QRect>}{@QRect}{@QRect}i', rgn)

        d.putItemCount(numRects)
        if d.isExpanded():
            with Children(d):
                d.putIntItem('numRects', numRects)
                d.putIntItem('innerArea', innerArea)
                d.putSubItem('extents', extents)
                d.putSubItem('innerRect', innerRect)
                d.putSubItem('rects', rects)


def qdump__QScopedPointer(d, value):
    if value.pointer() == 0:
        d.putValue('(null)')
    else:
        d.putItem(value['d'])
        d.putValue(d.currentValue.value, d.currentValue.encoding)
    typeName = value.type.name
    if value.type[1].name == d.qtNamespace() + 'QScopedPointerDeleter<%s>' % value.type[0].name:
        typeName = '@QScopedPointer<%s>' % value.type[0].name
    d.putBetterType(typeName)


def qdump__QSet(d, value):
    if d.qtVersion() >= 0x060000:
        qdumpHelper_QSet6(d, value)
    else:
        qdumpHelper_QSet45(d, value)

def qdumpHelper_QSet6(d, value):
    dptr = d.extractPointer(value)
    if dptr == 0:
        d.putItemCount(0)
        return

    ref, _, size, buckets, seed, spans = d.split('i@qqqp', dptr)

    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.check(-1 <= ref and ref < 100000)
    d.putItemCount(size)

    if d.isExpanded():
        value_type = value.type[0]
        value_size = value_type.size()
        with Children(d, size):
            span_size = 128 + 2 * d.ptrSize() # Including tail padding.
            nspans = int((buckets + 127) / 128)
            count = 0
            for b in range(nspans):
                span = spans + b * span_size
                offsets, entries, allocated, next_free = d.split('128spbb', span)
                for i in range(128):
                    offset = offsets[i]
                    if offset != 255: # Entry is used
                        entry = entries + offset * value_size
                        d.putSubItem(count, d.createValue(entry, value_type))
                        count += 1


def qdumpHelper_QSet45(d, value):
    def hashDataFirstNode():
        b = buckets
        n = numBuckets
        while n:
            n -= 1
            bb = d.extractPointer(b)
            if bb != dptr:
                return bb
            b += ptrSize
        return dptr

    def hashDataNextNode(node):
        (nextp, h) = d.split('pI', node)
        if d.extractPointer(nextp):
            return nextp
        start = (h % numBuckets) + 1
        b = buckets + start * ptrSize
        n = numBuckets - start
        while n:
            n -= 1
            bb = d.extractPointer(b)
            if bb != nextp:
                return bb
            b += ptrSize
        return nextp

    ptrSize = d.ptrSize()
    dptr = d.extractPointer(value)
    (fakeNext, buckets, ref, size, nodeSize, userNumBits, numBits, numBuckets) = \
        d.split('ppiiihhi', dptr)

    d.check(0 <= size and size <= 100 * 1000 * 1000)
    d.check(-1 <= ref and ref < 100000)

    d.putItemCount(size)
    if d.isExpanded():
        keyType = value.type[0]
        isShort = d.qtVersion() < 0x050000 and keyType.name == 'int'
        with Children(d, size, childType=keyType):
            node = hashDataFirstNode()
            for i in d.childRange():
                if isShort:
                    typeCode = 'P{%s}' % keyType.name
                    (pnext, key) = d.split(typeCode, node)
                else:
                    typeCode = 'Pi@{%s}' % keyType.name
                    (pnext, hashval, padding1, key) = d.split(typeCode, node)
                with SubItem(d, i):
                    d.putItem(key)
                node = hashDataNextNode(node)


def qdump__QSharedData(d, value):
    d.putValue('ref: %s' % value.to('i'))


def qdump__QSharedDataPointer(d, value):
    d_ptr = value['d']
    if d_ptr.pointer() == 0:
        d.putValue('(null)')
    else:
        # This replaces the pointer by the pointee, making the
        # pointer transparent.
        try:
            innerType = value.type[0]
        except:
            d.putValue(d_ptr)
            d.putPlainChildren(value)
            return
        d.putBetterType(d.currentType)
        d.putItem(d_ptr.dereference())


def qdump__QSize(d, value):
    d.putValue('(%s, %s)' % value.split('ii'))
    d.putPlainChildren(value)


def qdump__QSizeF(d, value):
    d.putValue('(%s, %s)' % value.split('dd'))
    d.putPlainChildren(value)


def qdump__QSizePolicy__Policy(d, value):
    d.putEnumValue(value.integer(), {
        0:  'QSizePolicy::Fixed',
        1:  'QSizePolicy::GrowFlag',
        2:  'QSizePolicy::ExpandFlag',
        3:  'QSizePolicy::MinimumExpanding (GrowFlag|ExpandFlag)',
        4:  'QSizePolicy::ShrinkFlag',
        5:  'QSizePolicy::Preferred (GrowFlag|ShrinkFlag)',
        7:  'QSizePolicy::Expanding (GrowFlag|ShrinkFlag|ExpandFlag)',
        8:  'QSizePolicy::IgnoreFlag',
        13: 'QSizePolicy::Ignored (ShrinkFlag|GrowFlag|IgnoreFlag)',
    })


def qdump__QSizePolicy(d, value):
    bits = value.integer()
    d.putEmptyValue(-99)
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putIntItem('horStretch', (bits >> 0) & 0xff)
            d.putIntItem('verStretch', (bits >> 8) & 0xff)
            d.putEnumItem('horPolicy', (bits >> 16) & 0xf, "@QSizePolicy::Policy")
            d.putEnumItem('verPolicy', (bits >> 20) & 0xf, "@QSizePolicy::Policy")


def qform__QStack():
    return [DisplayFormat.ArrayPlot]


def qdump__QStack(d, value):
    if d.qtVersion() >= 0x60000:
        qdump__QList(d, value)
    else:
        qdump__QVector(d, value)


def qdump__QPolygonF(d, value):
    data, size = d.vectorData(value)
    d.putItemCount(size)
    d.putPlotData(data, size, d.createType('@QPointF'))


def qdump__QPolygon(d, value):
    data, size = d.vectorData(value)
    d.putItemCount(size)
    d.putPlotData(data, size, d.createType('@QPoint'))


def qdump__QGraphicsPolygonItem(d, value):
    (vtbl, dptr) = value.split('pp')
    if d.qtVersion() >= 0x060000:
        if d.ptrSize() == 8:
            offset = 424 # sizeof(QGraphicsPolygonItemPrivate), the base
        else:
            # Chicken out. Not worth maintaining.
            d.putPlainChildren(value)
            return
    else:
        if d.ptrSize() == 8:
            offset = 384
        elif d.isWindowsTarget():
            offset = 328 if d.isMsvcTarget() else 320
        else:
            offset = 308
    data, size = d.vectorData(dptr + offset)
    d.putItemCount(size)
    d.putPlotData(data, size, d.createType('@QPointF'))


def qedit__QString(d, value, data):
    d.call('void', value, 'resize', str(len(data)))
    (base, size, alloc) = d.stringData(value)
    d.setValues(base, 'short', [ord(c) for c in data])


def qform__QString():
    return [DisplayFormat.Simple, DisplayFormat.Separate]


def qdump__QString(d, value):
    d.putStringValue(value)
    data, size, _ = d.stringData(value)
    displayFormat = d.currentItemFormat()
    if displayFormat == DisplayFormat.Separate:
        d.putDisplay('utf16:separate', d.encodeString(value, limit=100000))
    if (size > 0):
        d.putExpandable()
        if d.isExpanded():
            d.putArrayData(data, size, d.createType('@QChar'))


def qdump__QSettingsKey(d, value):
    qdump__QString(d, value)
    d.putBetterType(value.type)


def qdump__QStaticStringData(d, value):
    size = value.type[0]
    (ref, size, alloc, pad, offset, data) = value.split('iii@p%ss' % (2 * size))
    d.putValue(d.hexencode(data), 'utf16')
    d.putPlainChildren(value)


def qdump__QTypedArrayData(d, value):
    if value.type[0].name == 'unsigned short':
        qdump__QStringData(d, value)
#    else:
#        qdump__QArrayData(d, value)


def qdump__QStringData(d, value):
    (ref, size, alloc, pad, offset) = value.split('III@p')
    elided, shown = d.computeLimit(size, d.displayStringLimit)
    data = d.readMemory(value.address() + offset, shown * 2)
    d.putValue(data, 'utf16', elided=elided)
    d.putPlainChildren(value)


def qdump__QAnyStringView(d, value):
    data, size = value.split('pp')
    bits = d.ptrSize() * 8 - 2
    tag = size >> bits
    size = size & (2**bits - 1)
    elided, shown = d.computeLimit(size, d.displayStringLimit)
    if tag == 0:
        mem = d.readMemory(data, shown)
        d.putValue(mem, 'utf8', elided=elided)
    elif tag == 1:
        mem = d.readMemory(data, shown)
        d.putValue(mem, 'latin1', elided=elided)
    elif tag == 2:
        mem = d.readMemory(data, shown * 2)
        d.putValue(mem, 'utf16', elided=elided)
    else:
        d.putSpecialValue('empty')
    d.putPlainChildren(value)


def qform__QStringView():
    return [DisplayFormat.Simple, DisplayFormat.Separate]


def qdump__QStringView(d, value):
    data = value['m_data']
    idata = data.integer()
    if idata == 0:
        d.putValue('(null)')
        return
    size = value['m_size']
    isize = size.integer()
    elided, shown = d.computeLimit(isize, d.displayStringLimit)
    mem = d.readMemory(idata, shown * 2)
    d.putValue(mem, 'utf16', elided=elided)
    if d.currentItemFormat() == DisplayFormat.Separate:
        d.putDisplay('utf16:separate', mem)
    d.putExpandable()
    if d.isExpanded():
        d.putArrayData(idata, isize, d.createType('char16_t'))


def qdump__QHashedString(d, value):
    qdump__QString(d, value)
    d.putBetterType(value.type)


def qdump__QQmlRefCount(d, value):
    d.putItem(value['refCount'])
    d.putBetterType(value.type)


def qdump__QStringRef(d, value):
    (stringptr, pos, size) = value.split('pii')
    if stringptr == 0:
        d.putValue('(null)')
        return
    data, ssize, alloc = d.stringData(d.createValue(stringptr, '@QString'))
    d.putValue(d.readMemory(data + 2 * pos,  2 * size), 'utf16')
    d.putPlainChildren(value)


def qdump__QStringList(d, value):
    qdumpHelper_QList(d, value, d.createType('@QString'))
    d.putBetterType(value.type)


def qdump__QTemporaryFile(d, value):
    qdump__QFile(d, value)


def qdump__QTextCodec(d, value):
    name = d.call('const char *', value, 'name')
    d.putValue(d.encodeByteArray(name, limit=100), 6)
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putCallItem('name', '@QByteArray', value, 'name')
            d.putCallItem('mibEnum', 'int', value, 'mibEnum')
            d.putFields(value)


def qdump__QTextCursor(d, value):
    privAddress = d.extractPointer(value)
    if privAddress == 0:
        d.putValue('(invalid)')
    else:
        positionAddress = privAddress + 2 * d.ptrSize() + 8
        d.putValue(d.extractInt(positionAddress))
        d.putExpandable()
    if d.isExpanded():
        with Children(d):
            positionAddress = privAddress + 2 * d.ptrSize() + 8
            d.putIntItem('position', d.extractInt(positionAddress))
            d.putIntItem('anchor', d.extractInt(positionAddress + 4))
            d.putCallItem('selected', '@QString', value, 'selectedText')
            d.putFields(value)


def qdump__QTextDocument(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putCallItem('blockCount', 'int', value, 'blockCount')
            d.putCallItem('characterCount', 'int', value, 'characterCount')
            d.putCallItem('lineCount', 'int', value, 'lineCount')
            d.putCallItem('revision', 'int', value, 'revision')
            d.putCallItem('toPlainText', '@QString', value, 'toPlainText')
            d.putFields(value)


def qform__QUrl():
    return [DisplayFormat.Simple, DisplayFormat.Separate]


def qdump__QUrl(d, value):
    privAddress = d.extractPointer(value)
    if not privAddress:
        # d == 0 if QUrl was constructed with default constructor
        d.putValue('<invalid>')
        return

    if d.qtVersion() < 0x050000:
        d.call('void', value, 'port')  # Warm up internal cache.
        d.call('void', value, 'path')
        st = '{@QString}'
        ba = '{@QByteArray}'
        (ref, dummy,
         scheme, userName, password, host, path,  # QString
         query,  # QByteArray
         fragment,  # QString
         encodedOriginal, encodedUserName, encodedPassword,
         encodedPath, encodedFragment,  # QByteArray
         port) \
            = d.split('i@' + st * 5 + ba + st + ba * 5 + 'i', privAddress)
    else:
        (ref, port, scheme, userName, password, host, path, query, fragment) \
            = d.split('ii' + '{@QString}' * 7, privAddress)

    userNameEnc = d.encodeString(userName)
    hostEnc = d.encodeString(host)
    elided, pathEnc = d.encodeStringHelper(path, d.displayStringLimit)
    url = d.encodeString(scheme)
    url += '3a002f002f00'  # '://'
    if len(userNameEnc):
        url += userNameEnc + '4000'  # '@'
    url += hostEnc
    if port >= 0:
        url += '3a00' + ''.join(['%02x00' % ord(c) for c in str(port)])
    url += pathEnc
    d.putValue(url, 'utf16', elided=elided)

    displayFormat = d.currentItemFormat()
    if displayFormat == DisplayFormat.Separate:
        d.putDisplay('utf16:separate', url)

    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putIntItem('port', port)
            d.putSubItem('scheme', scheme)
            d.putSubItem('userName', userName)
            d.putSubItem('password', password)
            d.putSubItem('host', host)
            d.putSubItem('path', path)
            d.putSubItem('query', query)
            d.putSubItem('fragment', fragment)
            d.putFields(value)


def qdump__QUuid(d, value):
    r = value.split('IHHBBBBBBBB')
    d.putValue('{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}' % r)
    d.putPlainChildren(value)


def qdumpHelper_QVariant_0(d, value):
    # QVariant::Invalid
    d.putBetterType('@QVariant (invalid)')
    d.putValue('(invalid)')


def qdumpHelper_QVariant_1(d, value):
    # QVariant::Bool
    d.putBetterType('@QVariant (bool)')
    d.putValue('true' if value.to('b') else 'false')


def qdumpHelper_QVariant_2(d, value):
    # QVariant::Int
    d.putBetterType('@QVariant (int)')
    d.putValue(value.to('i'))


def qdumpHelper_QVariant_3(d, value):
    # uint
    d.putBetterType('@QVariant (uint)')
    d.putValue(value.to('I'))


def qdumpHelper_QVariant_4(d, value):
    # qlonglong
    d.putBetterType('@QVariant (qlonglong)')
    d.putValue(value.to('q'))


def qdumpHelper_QVariant_5(d, value):
    # qulonglong
    d.putBetterType('@QVariant (qulonglong)')
    d.putValue(value.to('Q'))


def qdumpHelper_QVariant_6(d, value):
    # QVariant::Double
    d.putBetterType('@QVariant (double)')
    d.putValue(value.to('d'))


qdumpHelper_QVariants_A = [
    qdumpHelper_QVariant_0,
    qdumpHelper_QVariant_1,
    qdumpHelper_QVariant_2,
    qdumpHelper_QVariant_3,
    qdumpHelper_QVariant_4,
    qdumpHelper_QVariant_5,
    qdumpHelper_QVariant_6
]


qdumpHelper_QVariants_B = [
    'QChar',         # 7
    'QVariantMap',   # 8
    'QVariantList',  # 9
    'QString',       # 10
    'QStringList',   # 11
    'QByteArray',    # 12
    'QBitArray',     # 13
    'QDate',         # 14
    'QTime',         # 15
    'QDateTime',     # 16
    'QUrl',          # 17
    'QLocale',       # 18
    'QRect',         # 19
    'QRectF',        # 20
    'QSize',         # 21
    'QSizeF',        # 22
    'QLine',         # 23
    'QLineF',        # 24
    'QPoint',        # 25
    'QPointF',       # 26
    'QRegExp',       # 27
    'QVariantHash',  # 28
]


def qdumpHelper_QVariant_31(d, value):
    # QVariant::VoidStar
    d.putBetterType('@QVariant (void *)')
    d.putValue('0x%x' % d.extractPointer(value))


def qdumpHelper_QVariant_32(d, value):
    # QVariant::Long
    d.putBetterType('@QVariant (long)')
    if d.ptrSize() == 4:
        d.putValue('%s' % d.extractInt(value))
    else:
        d.putValue('%s' % d.extractInt64(value))  # sic!


def qdumpHelper_QVariant_33(d, value):
    # QVariant::Short
    d.putBetterType('@QVariant (short)')
    d.putValue('%s' % d.extractShort(value))


def qdumpHelper_QVariant_34(d, value):
    # QVariant::Char
    d.putBetterType('@QVariant (char)')
    d.putValue('%s' % d.extractByte(value))


def qdumpHelper_QVariant_35(d, value):
    # QVariant::ULong
    d.putBetterType('@QVariant (unsigned long)')
    if d.ptrSize() == 4:
        d.putValue('%s' % d.extractUInt(value))
    else:
        d.putValue('%s' % d.extractUInt64(value))  # sic!


def qdumpHelper_QVariant_36(d, value):
    # QVariant::UShort
    d.putBetterType('@QVariant (unsigned short)')
    d.putValue('%s' % d.extractUShort(value))


def qdumpHelper_QVariant_37(d, value):
    # QVariant::UChar
    d.putBetterType('@QVariant (unsigned char)')
    d.putValue('%s' % d.extractByte(value))


def qdumpHelper_QVariant_38(d, value):
    # QVariant::Float
    d.putBetterType('@QVariant (float)')
    d.putValue(value.to('f'))


qdumpHelper_QVariants_D = [
    qdumpHelper_QVariant_31,
    qdumpHelper_QVariant_32,
    qdumpHelper_QVariant_33,
    qdumpHelper_QVariant_34,
    qdumpHelper_QVariant_35,
    qdumpHelper_QVariant_36,
    qdumpHelper_QVariant_37,
    qdumpHelper_QVariant_38
]

qdumpHelper_QVariants_E = [
    'QFont',       # 64
    'QPixmap',     # 65
    'QBrush',      # 66
    'QColor',      # 67
    'QPalette',    # 68
    'QIcon',       # 69
    'QImage',      # 70
    'QPolygon',    # 71
    'QRegion',     # 72
    'QBitmap',     # 73
    'QCursor',     # 74
]

qdumpHelper_QVariants_F = [
    # Qt 5. In Qt 4 add one.
    'QKeySequence',  # 75
    'QPen',          # 76
    'QTextLength',   # 77
    'QTextFormat',   # 78
    'X',
    'QTransform',    # 80
    'QMatrix4x4',    # 81
    'QVector2D',     # 82
    'QVector3D',     # 83
    'QVector4D',     # 84
    'QQuaternion',   # 85
    'QPolygonF'      # 86
]


def qdump__QVariant(d, value):
    if d.qtVersion() >= 0x060000:
        qdumpHelper__QVariant6(d, value)
    else:
        qdumpHelper__QVariant45(d, value)

def qdumpHelper__QVariant6(d, value):
    data, typeStuff = d.split('24sp', value)
    packedType = typeStuff >> 2
    metaTypeInterface = typeStuff - (typeStuff & 3)

    if metaTypeInterface == 0:
        qdumpHelper_QVariant_0(d, value)
        return

    revision, alignment, size, flags, variantType, metaObjectPtr, name = \
        d.split('HHIIIpp', metaTypeInterface)

    # Well-known simple type.
    if variantType >= 1 and variantType <= 6:
        qdumpHelper_QVariants_A[variantType](d, value)
        return None

    # Extended Core type (Qt 5+)
    if variantType >= 31 and variantType <= 38:
        qdumpHelper_QVariants_D[variantType - 31](d, value)
        return None

    typeName = d.extractCString(name).decode('utf8')
    isShared = bool(typeStuff & 0x1)

    if isShared:
        # indirectly stored items (QRectF, ...)
        ptr = d.extractPointer(value)
        _, data  = d.split('8s{%s}' % typeName, ptr)
        d.putItem(data)
    else:
        val = d.createValue(data, typeName)
        val.laddress = value.laddress
        d.putItem(val)

    d.putBetterType('@QVariant (%s)' % typeName)


def qdumpHelper__QVariant45(d, value):
    data, typeStuff = d.split('8sI', value)
    variantType = typeStuff & 0x3fffffff
    isShared = bool(typeStuff & 0x40000000)

    # Well-known simple type.
    if variantType <= 6:
        qdumpHelper_QVariants_A[variantType](d, value)
        return None

    # Extended Core type (Qt 5)
    if variantType >= 31 and variantType <= 38 and d.qtVersion() >= 0x050000:
        qdumpHelper_QVariants_D[variantType - 31](d, value)
        return None

    # Extended Core type (Qt 4)
    if variantType >= 128 and variantType <= 135 and d.qtVersion() < 0x050000:
        if variantType == 128:
            d.putBetterType('@QVariant (void *)')
            d.putValue('0x%x' % value.extractPointer())
        else:
            if variantType == 135:  # Float
                blob = value
            else:
                p = d.extractPointer(value)
                blob = d.extractUInt64(p)
            qdumpHelper_QVariants_D[variantType - 128](d, blob)
        return None

    #DumperBase.warn('TYPE: %s' % variantType)

    if variantType <= 86:
        # Known Core or Gui type.
        if variantType <= 28:
            innert = qdumpHelper_QVariants_B[variantType - 7]
        elif variantType <= 74:
            innert = qdumpHelper_QVariants_E[variantType - 64]
        elif d.qtVersion() < 0x050000:
            innert = qdumpHelper_QVariants_F[variantType - 76]
        else:
            innert = qdumpHelper_QVariants_F[variantType - 75]

        #data = value['d']['data']
        innerType = d.qtNamespace() + innert

        #DumperBase.warn('SHARED: %s' % isShared)
        if isShared:
            base1 = d.extractPointer(value)
            #DumperBase.warn('BASE 1: %s %s' % (base1, innert))
            base = d.extractPointer(base1)
            #DumperBase.warn('SIZE 1: %s' % size)
            val = d.createValue(base, innerType)
        else:
            #DumperBase.warn('DIRECT ITEM 1: %s' % innerType)
            val = d.createValue(data, innerType)
            val.laddress = value.laddress

        d.putEmptyValue(-99)
        d.putItem(val)
        d.putBetterType('@QVariant (%s)' % innert)

        return innert

    # User types.
    ns = d.qtNamespace()
    d.putEmptyValue(-99)
    d.putType('%sQVariant (%s)' % (ns, variantType))
    d.putExpandable()
    if d.isExpanded():
        innerType = None
        with Children(d):
            try:
                p = d.call('const char *', value, 'typeName')
            except:
                d.putSpecialValue('notcallable')
                return None
            ptr = p.pointer()
            (elided, blob) = d.encodeCArray(ptr, 1, 100)
            innerType = d.hexdecode(blob)

            # Prefer namespaced version.
            if len(ns) > 0:
                if not d.lookupNativeType(ns + innerType) is None:
                    innerType = ns + innerType

            if isShared:
                base1 = d.extractPointer(value)
                base = d.extractPointer(base1)
                val = d.createValue(base, innerType)
            else:
                val = d.createValue(data, innerType)
                val.laddress = value.laddress
            d.putSubItem('data', val)

        if innerType is not None:
            d.putBetterType('%sQVariant (%s)' % (ns, innerType))
    return None


def qedit__QVector(d, value, data):
    values = data.split(',')
    d.call('void', value, 'resize', str(len(values)))
    base, vsize = d.vectorData(value)
    d.setValues(base, value.type[0].name, values)


def qform__QVector():
    return [DisplayFormat.ArrayPlot]


def qdump__QVector(d, value):
    if d.qtVersion() >= 0x060000:
        data, size = d.listData(value)
        d.putItemCount(size)
        d.putPlotData(data, size, d.createType(value.type.ltarget[0]))
        # g++ 9.3 does not add the template parameter list to the debug info.
        # Fake it for the common case:
        if value.type.name == d.qtNamespace() + "QVector":
            d.putBetterType(value.type.name + '<' + value.type.ltarget[0].name + '>')
    else:
        data, size = d.vectorData(value)
        d.putItemCount(size)
        d.putPlotData(data, size, d.createType(value.type[0]))


if False:
    def qdump__QObjectConnectionList(d, value):
        data, size = d.vectorData(value)
        d.putItemCount(size)
        d.putPlotData(data, size, d.createType('@QObjectPrivate::ConnectionList'))


def qdump__QVarLengthArray(d, value):
    if d.qtVersion() >= 0x060000:
        cap, size, data = value.split('QQp')
    else:
        cap, size, data = value.split('iip')
    d.check(0 <= size)
    d.putItemCount(size)
    d.putPlotData(data, size, value.type[0])


def qdump__QSharedPointer(d, value):
    qdump_QWeakPointerHelper(d, value, False)


def qdump__QWeakPointer(d, value):
    qdump_QWeakPointerHelper(d, value, True)


def qdump__QPointer(d, value):
    # actually, we'd use value['wp'] instead of value, but since we
    # only split() on the result and the (sub-)object address is the
    # same it does not matter but saves some cycles.
    qdump_QWeakPointerHelper(d, value, True, value.type[0])


def qdump_QWeakPointerHelper(d, value, isWeak, innerType=None):
    if isWeak:
        (d_ptr, val) = value.split('pp')
    else:
        (val, d_ptr) = value.split('pp')
    if d_ptr == 0 and val == 0:
        d.putValue('(null)')
        return
    if d_ptr == 0 or val == 0:
        d.putValue('<invalid>')
        return

    if d.qtVersion() >= 0x050000:
        (weakref, strongref) = d.split('ii', d_ptr)
    else:
        (vptr, weakref, strongref) = d.split('pii', d_ptr)
    d.check(strongref >= -1)
    d.check(strongref <= weakref)
    d.check(weakref <= 10 * 1000 * 1000)

    if innerType is None:
        innerType = value.type[0]
    with Children(d):
        dataAddress = value.laddress
        if isWeak:
            dataAddress = dataAddress + d.ptrSize()
        short = d.putSubItem('data', d.createValue(dataAddress, d.createPointerType(innerType)))
        d.putIntItem('weakref', weakref)
        d.putIntItem('strongref', strongref)
    d.putValue(short.value, short.encoding)


def qdump__QXmlAttributes__Attribute(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            (qname, uri, localname, val) = value.split('{@QString}' * 4)
            d.putSubItem('qname', qname)
            d.putSubItem('uri', uri)
            d.putSubItem('localname', localname)
            d.putSubItem('value', val)


def qdump__QXmlAttributes(d, value):
    vptr, atts = value.split('p{@QList<@QXmlAttributes::Attribute>}')
    _, att_size, _ = d.describeStruct('{@QString}' * 4)
    innerType = d.createType('@QXmlAttributes::Attribute', att_size)
    qdumpHelper_QList(d, atts, innerType)


def qdump__QXmlStreamStringRef(d, value):
    s = value['m_string']
    (data, size, alloc) = d.stringData(s)
    data += 2 * int(value['m_position'])
    size = int(value['m_size'])
    s = d.readMemory(data, 2 * size)
    d.putValue(s, 'utf16')
    d.putPlainChildren(value)


def qdump__QXmlStreamAttribute(d, value):
    s = value['m_name']['m_string']
    (data, size, alloc) = d.stringData(s)
    data += 2 * int(value['m_name']['m_position'])
    size = int(value['m_name']['m_size'])
    s = d.readMemory(data, 2 * size)
    d.putValue(s, 'utf16')
    d.putPlainChildren(value)


#######################################################################
#
# V4
#
#######################################################################

def extractQmlData(d, value):
    #if value.type.code == TypeCode.Pointer:
    #    value = value.dereference()
    base = value.split('p')[0]
    #mmdata = d.split('Q', base)[0]
    #PointerMask = 0xfffffffffffffffd
    #vtable = mmdata & PointerMask
    #DumperBase.warn('QML DATA: %s' % value.stringify())
    #data = value['data']
    #return #data.cast(d.lookupType(value.type.name.replace('QV4::', 'QV4::Heap::')))
    typeName = value.type.name.replace('QV4::', 'QV4::Heap::')
    #DumperBase.warn('TYOE DATA: %s' % typeName)
    return d.createValue(base, typeName)


def qdump__QV4__Heap__Base(d, value):
    mm_data = value.extractPointer()
    d.putValue('[%s]' % mm_data)
    if d.isExpanded():
        with Children(d):
            with SubItem(d, 'vtable'):
                d.putItem(d.createValue(mm_data & (~3), d.qtNamespace() + 'QV4::VTable'))
            d.putBoolItem('isMarked', mm_data & 1)
            d.putBoolItem('inUse', (mm_data & 2) == 0)
            with SubItem(d, 'nextFree'):
                d.putItem(d.createValue(mm_data & (~3), value.type))


def qdump__QV4__Heap__String(d, value):
    # Note: There's also the 'Identifier' case. And the largestSubLength != 0 case.
    (baseClass, textOrLeft, idOrRight, subtype, stringHash, largestSub, length, mm) \
        = value.split('QppIIIIp')
    textPtr = d.split('{@QStringDataPtr}', textOrLeft)[0]
    qdump__QStringData(d, d.createValue(textOrLeft, d.qtNamespace() + 'QStringData'))
    if d.isExpanded():
        with Children(d):
            d.putFields(value)


def qmlPutHeapChildren(d, value):
    d.putItem(extractQmlData(d, value))


def qdump__QV4__Object(d, value):
    qmlPutHeapChildren(d, value)


def qdump__QV4__FunctionObject(d, value):
    #qmlPutHeapChildren(d, value)
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            d.putFields(value)
            d.putSubItem('heap', extractQmlData(d, value))
            d.putCallItem('sourceLocation', '@QQmlSourceLocation',
                          value, 'sourceLocation')


def qdump__QV4__CompilationUnit(d, value):
    qmlPutHeapChildren(d, value)


def qdump__QV4__CallContext(d, value):
    qmlPutHeapChildren(d, value)


def qdump__QV4__ScriptFunction(d, value):
    qmlPutHeapChildren(d, value)


def qdump__QV4__SimpleScriptFunction(d, value):
    qdump__QV4__FunctionObject(d, value)


def qdump__QV4__ExecutionContext(d, value):
    qmlPutHeapChildren(d, value)


def qdump__QQmlSourceLocation(d, value):
    (sourceFile, line, col) = value.split('pHH')
    (data, size, alloc) = d.stringData(value)
    d.putValue(d.readMemory(data, 2 * size), 'utf16')
    d.putField('valuesuffix', ':%s:%s' % (line, col))
    d.putPlainChildren(value)


#def qdump__QV4__CallData(d, value):
#    argc = value['argc'].integer()
#    d.putItemCount(argc)
#    if d.isExpanded():
#        with Children(d):
#            d.putSubItem('[this]', value['thisObject'])
#            for i in range(0, argc):
#                d.putSubItem(i, value['args'][i])
#

def qdump__QV4__String(d, value):
    qmlPutHeapChildren(d, value)


def qdump__QV4__Identifier(d, value):
    d.putStringValue(value)
    d.putPlainChildren(value)


def qdump__QV4__PropertyHash(d, value):
    data = value.extractPointer()
    (ref, alloc, size, numBits, entries) = d.split('iiiip', data)
    n = 0
    with Children(d):
        for i in range(alloc):
            (identifier, index) = d.split('pI', entries + i * 2 * d.ptrSize())
            if identifier != 0:
                n += 1
                with SubItem(d):
                    d.putItem(d, d.createValue(identifier, '@QV4::Identifier'))
                    d.put('keysuffix', ' %d' % index)
    d.putItemCount(n)
    d.putPlainChildren(value)


def qdump__QV4__InternalClass__Transition(d, value):
    identifier = d.createValue(value.extractPointer(), '@QV4::Identifier')
    d.putStringValue(identifier)
    d.putPlainChildren(value)


def qdump__QV4__InternalClassTransition(d, value):
    qdump__QV4__InternalClass__Transition(d, value)


def qdump__QV4__SharedInternalClassData(d, value):
    (ref, alloc, size, pad, data) = value.split('iIIip')
    val = d.createValue(data, value.type[0])
    with Children(d):
        with SubItem(d, 'data'):
            d.putItem(val)
            short = d.currentValue
        d.putIntItem('size', size)
        d.putIntItem('alloc', alloc)
        d.putIntItem('refcount', ref)
    d.putValue(short.value, short.encoding)


def qdump__QV4__IdentifierTable(d, value):
    (engine, alloc, size, numBits, pad, entries) = value.split('piiiip')
    n = 0
    with Children(d):
        for i in range(alloc):
            identifierPtr = d.extractPointer(entries + i * d.ptrSize())
            if identifierPtr != 0:
                n += 1
                with SubItem(d, None):
                    d.putItem(d.createValue(identifierPtr, '@QV4::Heap::String'))
    d.putItemCount(n)
    d.putPlainChildren(value)


if False:
    # 32 bit.
    QV4_Masks_SilentNaNBit =           0x00040000
    QV4_Masks_NaN_Mask =               0x7ff80000
    QV4_Masks_NotDouble_Mask =         0x7ffa0000
    QV4_Masks_Type_Mask =              0xffffc000
    QV4_Masks_Immediate_Mask =         QV4_Masks_NotDouble_Mask | 0x00004000 | QV4_Masks_SilentNaNBit
    QV4_Masks_IsNullOrUndefined_Mask = QV4_Masks_Immediate_Mask | 0x08000
    QV4_Masks_Tag_Shift = 32

    QV4_ValueType_Undefined_Type =     QV4_Masks_Immediate_Mask | 0x00000
    QV4_ValueType_Null_Type =          QV4_Masks_Immediate_Mask | 0x10000
    QV4_ValueType_Boolean_Type =       QV4_Masks_Immediate_Mask | 0x08000
    QV4_ValueType_Integer_Type =       QV4_Masks_Immediate_Mask | 0x18000
    QV4_ValueType_Managed_Type =       QV4_Masks_NotDouble_Mask | 0x00000 | QV4_Masks_SilentNaNBit
    QV4_ValueType_Empty_Type =         QV4_Masks_NotDouble_Mask | 0x18000 | QV4_Masks_SilentNaNBit

    QV4_ConvertibleToInt =             QV4_Masks_Immediate_Mask | 0x1

    QV4_ValueTypeInternal_Null_Type_Internal =    QV4_ValueType_Null_Type | QV4_ConvertibleToInt
    QV4_ValueTypeInternal_Boolean_Type_Internal = QV4_ValueType_Boolean_Type | QV4_ConvertibleToInt
    QV4_ValueTypeInternal_Integer_Type_Internal = QV4_ValueType_Integer_Type | QV4_ConvertibleToInt


def QV4_getValue(d, jsval):  # (Dumper, QJSValue *jsval) -> QV4::Value *
    dd = d.split('Q', jsval)[0]
    if dd & 3:
        return 0
    return dd


def QV4_getVariant(d, jsval):  # (Dumper, QJSValue *jsval) -> QVariant *
    dd = d.split('Q', jsval)[0]
    if dd & 1:
        return dd & ~3
    return 0


def QV4_valueForData(d, jsval):  # (Dumper, QJSValue *jsval) -> QV4::Value *
    v = QV4_getValue(d, jsval)
    if v:
        return v
    d.warn('Not implemented: VARIANT')
    return 0


def QV4_putObjectValue(d, objectPtr):
    ns = d.qtNamespace()
    base = d.extractPointer(objectPtr)
    (inlineMemberOffset, inlineMemberSize, internalClass, prototype,
     memberData, arrayData) = d.split('IIpppp', base)
    d.putValue('PTR: 0x%x' % objectPtr)
    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                d.putValue('[0x%x]' % objectPtr)
                d.putType(' ')
            d.putIntItem('inlineMemberOffset', inlineMemberOffset)
            d.putIntItem('inlineMemberSize', inlineMemberSize)
            d.putIntItem('internalClass', internalClass)
            d.putIntItem('prototype', prototype)
            d.putPtrItem('memberData', memberData)
            d.putPtrItem('arrayData', arrayData)
            d.putSubItem('OBJ', d.createValue(objectPtr, ns + 'QV4::Object'))
            #d.putFields(value)


def qdump__QV4_Object(d, value):
    ns = d.qtNamespace()
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                base = d.extractPointer(objectPtr)
                (inlineMemberOffset, inlineMemberSize, internalClass, prototype,
                 memberData, arrayData) = d.split('IIpppp', base)
                d.putValue('PTR: 0x%x' % objectPtr)


def qdump__QV4__Value(d, value):
    if d.ptrSize() == 4:
        qdump_32__QV4__Value(d, value)
    else:
        qdump_64__QV4__Value(d, value)


def qdump_32__QV4__Value(d, value):
    # QV4_Masks_SilentNaNBit           = 0x00040000
    # QV4_Masks_NaN_Mask               = 0x7ff80000
    # QV4_Masks_NotDouble_Mask         = 0x7ffa0000
    # QV4_Masks_Type_Mask              = 0xffffc000
    ns = d.qtNamespace()
    v = value.split('Q')[0]
    tag = v >> 32
    val = v & 0xffffffff
    if (tag & 0x7fff2000) == 0x7fff2000:  # Int
        d.putValue(val)
        d.putBetterType('@QV4::Value (int32)')
    elif (tag & 0x7fff4000) == 0x7fff4000:  # Bool
        d.putValue(val)
        d.putBetterType('@QV4::Value (bool)')
    elif (tag & 0x7fff0000) == 0x7fff0000:  # Null
        d.putValue(val)
        d.putBetterType('@QV4::Value (null)')
    elif (tag & 0x7ffa0000) != 0x7ffa0000:  # Double
        d.putValue(value.split('d')[0])
        d.putBetterType('@QV4::Value (double)')
    elif tag == 0x7ffa0000:
        if val == 0:
            d.putValue('(undefined)')
            d.putBetterType('@QV4::Value (undefined)')
        else:
            managed = d.createValue(val, ns + 'QV4::Heap::Base')
            qdump__QV4__Heap__Base(d, managed)
            #d.putValue('[0x%x]' % v)
    #d.putPlainChildren(value)
    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                d.putValue('[0x%x]' % v)
                d.putType(' ')
            with SubItem(d, '[val]'):
                d.putValue('[0x%x]' % val)
                d.putType(' ')
            with SubItem(d, '[tag]'):
                d.putValue('[0x%x]' % tag)
                d.putType(' ')
            #with SubItem(d, '[vtable]'):
            #    d.putItem(d.createValue(vtable, ns + 'QV4::VTable'))
            #    d.putType(' ');
            d.putFields(value)


def qdump_64__QV4__Value(d, value):
    dti = d.qtDeclarativeTypeInfoVersion()
    new = dti is not None and dti >= 2
    if new:
        QV4_NaNEncodeMask =         0xfffc000000000000
        QV4_Masks_Immediate_Mask =  0x00020000  # bit 49

        QV4_ValueTypeInternal_Empty_Type_Internal =     QV4_Masks_Immediate_Mask | 0
        QV4_ConvertibleToInt =                          QV4_Masks_Immediate_Mask | 0x10000  # bit 48
        QV4_ValueTypeInternal_Null_Type_Internal =      QV4_ConvertibleToInt | 0x08000
        QV4_ValueTypeInternal_Boolean_Type_Internal =   QV4_ConvertibleToInt | 0x04000
        QV4_ValueTypeInternal_Integer_Type_Internal =   QV4_ConvertibleToInt | 0x02000

        QV4_ValueType_Undefined_Type = 0  # Dummy to make generic code below pass.

    else:
        QV4_NaNEncodeMask =         0xffff800000000000
        QV4_Masks_Immediate_Mask =  0x00018000

        QV4_IsInt32Mask =           0x0002000000000000
        QV4_IsDoubleMask =          0xfffc000000000000
        QV4_IsNumberMask =          QV4_IsInt32Mask | QV4_IsDoubleMask
        QV4_IsNullOrUndefinedMask = 0x0000800000000000
        QV4_IsNullOrBooleanMask =   0x0001000000000000

        QV4_Masks_NaN_Mask =                0x7ff80000
        QV4_Masks_Type_Mask =               0xffff8000
        QV4_Masks_IsDouble_Mask =           0xfffc0000
        QV4_Masks_IsNullOrUndefined_Mask =  0x00008000
        QV4_Masks_IsNullOrBoolean_Mask =    0x00010000

        QV4_ValueType_Undefined_Type =  QV4_Masks_IsNullOrUndefined_Mask
        QV4_ValueType_Null_Type =       QV4_Masks_IsNullOrUndefined_Mask \
            | QV4_Masks_IsNullOrBoolean_Mask
        QV4_ValueType_Boolean_Type =    QV4_Masks_IsNullOrBoolean_Mask
        QV4_ValueType_Integer_Type =    0x20000 | QV4_Masks_IsNullOrBoolean_Mask
        QV4_ValueType_Managed_Type =    0
        QV4_ValueType_Empty_Type =      QV4_ValueType_Undefined_Type | 0x4000

        QV4_ValueTypeInternal_Null_Type_Internal =      QV4_ValueType_Null_Type
        QV4_ValueTypeInternal_Boolean_Type_Internal =   QV4_ValueType_Boolean_Type
        QV4_ValueTypeInternal_Integer_Type_Internal =   QV4_ValueType_Integer_Type

    QV4_PointerMask = 0xfffffffffffffffd

    QV4_Masks_Tag_Shift =           32
    QV4_IsDouble_Shift =            64 - 14
    QV4_IsNumber_Shift =            64 - 15
    QV4_IsConvertibleToInt_Shift =  64 - 16
    QV4_IsManaged_Shift =           64 - 17

    v = value.split('Q')[0]
    tag = v >> QV4_Masks_Tag_Shift
    vtable = v & QV4_PointerMask
    ns = d.qtNamespace()
    if (v >> QV4_IsNumber_Shift) == 1:
        d.putBetterType('@QV4::Value (int32)')
        vv = v & 0xffffffff
        vv = vv if vv < 0x80000000 else -(0x100000000 - vv)
        d.putBetterType('@QV4::Value (int32)')
        d.putValue('%d' % vv)
    elif (v >> QV4_IsDouble_Shift):
        d.putBetterType('@QV4::Value (double)')
        d.putValue('%0.16x' % (v ^ QV4_NaNEncodeMask), 'float:8')
    elif tag == QV4_ValueType_Undefined_Type and not new:
        d.putBetterType('@QV4::Value (undefined)')
        d.putValue('(undefined)')
    elif tag == QV4_ValueTypeInternal_Null_Type_Internal:
        d.putBetterType('@QV4::Value (null?)')
        d.putValue('(null?)')
    elif v == 0:
        if new:
            d.putBetterType('@QV4::Value (undefined)')
            d.putValue('(undefined)')
        else:
            d.putBetterType('@QV4::Value (null)')
            d.putValue('(null)')
    #elif ((v >> QV4_IsManaged_Shift) & ~1) == 1:
    #    d.putBetterType('@QV4::Value (null/undef)')
    #    d.putValue('(null/undef)')
    #elif v & QV4_IsNullOrBooleanMask:
    #    d.putBetterType('@QV4::Value (null/bool)')
    #    d.putValue('(null/bool)')
    #    d.putValue(v & 1)
    else:
        (parentv, flags, pad, className) = d.split('pIIp', vtable)
        #vtable = value['m']['vtable']
        if flags & 2:  # isString'
            d.putBetterType('@QV4::Value (string)')
            qdump__QV4__Heap__String(d, d.createValue(v, ns + 'QV4::Heap::String'))
            #d.putStringValue(d.extractPointer(value) + 2 * d.ptrSize())
            #d.putValue('ptr: 0x%x' % d.extractPointer(value))
            return
        elif flags & 4:  # isObject
            d.putBetterType('@QV4::Value (object)')
            #QV4_putObjectValue(d, d.extractPointer(value) + 2 * d.ptrSize())
            arrayVTable = d.symbolAddress(ns + 'QV4::ArrayObject::static_vtbl')
            #DumperBase.warn('ARRAY VTABLE: 0x%x' % arrayVTable)
            d.putExpandable()
            d.putItem(d.createValue(d.extractPointer(value) + 2 * d.ptrSize(), ns + 'QV4::Object'))
            return
        elif flags & 8:  # isFunction
            d.putBetterType('@QV4::Value (function)')
            d.putEmptyValue()
        else:
            d.putBetterType('@QV4::Value (unknown)')
            #d.putValue('[0x%x]' % v)
            d.putValue('[0x%x : flag 0x%x : tag 0x%x]' % (v, flags, tag))
    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                d.putValue('[0x%x]' % v)
                d.putType(' ')
            with SubItem(d, '[vtable]'):
                d.putItem(d.createValue(vtable, ns + 'QV4::VTable'))
                d.putType(' ')
            d.putFields(value)


def qdump__QV__PropertyHashData(d, value):
    (ref, alloc, size, numBits, entries) = value.split('IIIIp')
    d.putItemCount(size)
    if d.isExpanded():
        with Children(d):
            d.putFields(value)


def qdump__QV__PropertyHash(d, value):
    qdump__QV__PropertyHashData(d, d.createValue(d.extractPointer(), value.type.name + 'Data'))


def qdump__QV4__Scoped(d, value):
    innerType = value.type[0]
    d.putItem(d.createValue(value.extractPointer(), innerType))
    #d.putEmptyValue()
    #if d.isExpanded():
    #    with Children(d):
    #        d.putSubItem('[]', d.createValue(value.extractPointer(), innerType))
    #        d.putFields(value)


def qdump__QV4__ScopedString(d, value):
    innerType = value.type[0]
    qdump__QV4__String(d, d.createValue(value.extractPointer(), innerType))


def qdump__QJSValue(d, value):
    if d.ptrSize() == 4:
        qdump_32__QJSValue(d, value)
    else:
        if d.qtVersion() >= 0x60000:
            qdump_64__QJSValue_6(d, value)
        else:
            qdump_64__QJSValue_5(d, value)


def qdump_32__QJSValue(d, value):
    ns = d.qtNamespace()
    dd = value.split('I')[0]
    d.putValue('[0x%x]' % dd)
    if dd == 0:
        d.putValue('(null)')
        d.putType(value.type.name + ' (null)')
    elif dd & 1:
        variant = d.createValue(dd & ~3, ns + 'QVariant')
        qdump__QVariant(d, variant)
        d.putBetterType(d.currentType.value.replace('QVariant', 'QJSValue', 1))
    elif dd & 3 == 0:
        v4value = d.createValue(dd, ns + 'QV4::Value')
        qdump_32__QV4__Value(d, v4value)
        d.putBetterType(d.currentType.value.replace('QV4::Value', 'QJSValue', 1))
        return
    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                d.putValue('[0x%x]' % dd)
                d.putType(' ')
            d.putFields(value)

def qdump_64__QJSValue_6(d, value):
    dd = value.split('Q')[0]
    if dd == 0:
        d.putValue('(undefined)')
        d.putType(value.type.name + ' (undefined)')
    if d.qtVersion() < 0x60500:
        typ = dd >> 47
        if typ == 5:
            d.putValue('(null)')
            d.putType(value.type.name + ' (null)')
        elif typ == 6:
            d.putValue('true' if dd & 1 else 'false')
            d.putType(value.type.name + ' (bool)')
        elif typ == 7:
            d.putValue(dd & 0xfffffffff)
            d.putType(value.type.name + ' (int)')
        elif typ > 7:
            val = d.Value(d)
            val.ldata = struct.pack('q', dd ^ 0xfffc000000000000)
            val._type = d.createType('double')
            d.putItem(val)
            d.putType(value.type.name + ' (double)')
        elif typ <= 3: # Heap
            if dd & 1: # String
                val = d.Value(d)
                val.ldata = struct.pack('q', dd & ~1)
                val._type = d.createType('@QString*')
                d.putItem(val)
                d.putType(value.type.name + ' (QString)')
            else:
                # FIXME: Arrays, Objects missing.
                val = d.split('{@QV4::Managed*}', value)[0]
                d.putItem(val)
                d.putItemCount(1)
        else:
            d.putEmptyValue()
            d.putItemCount(1)
            d.putPlainChildren(value)
            return

    else:
        typ = dd & 7
        isPointer = typ & 1
        if typ == 0:
            d.putValue('(undefined)')
            d.putType(value.type.name + ' (undefined)')
        elif typ == 2:
            d.putValue('(null)')
            d.putType(value.type.name + ' (null)')
        elif typ == 4:
            d.putValue(dd >> 32)
            d.putType(value.type.name + ' (int)')
        elif typ == 6:
            d.putValue('true' if dd >> 32 & 1 else 'false')
            d.putType(value.type.name + ' (bool)')
        elif isPointer:
            pointer = dd >> 3
            pointer = pointer << 3
            val = d.Value(d)
            val.ldata = struct.pack('q', pointer)
            if typ == 1:
                val._type = d.createType('double*')
                d.putItem(val)
                d.putType(value.type.name + ' (double)')
            elif typ == 3:
                val._type = d.createType('@QV4::Value*')
                d.putItem(val)
                d.putType(value.type.name + ' (QV4::Value)')
            elif typ == 5:
                val._type = d.createType('@QString*')
                d.putItem(val)
                d.putType(value.type.name + ' (QString)')

        else:
            d.putEmptyValue()
            d.putItemCount(1)
            d.putPlainChildren(value)
            return

    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                d.putValue('[0x%x]' % dd)
                d.putType(' ')
            d.putFields(value)


def qdump_64__QJSValue_5(d, value):
    ns = d.qtNamespace()
    dd = value.split('Q')[0]
    if dd == 0:
        d.putValue('(null)')
        d.putType(value.type.name + ' (null)')
    elif dd & 1:
        variant = d.createValue(dd & ~3, ns + 'QVariant')
        qdump__QVariant(d, variant)
        d.putBetterType(d.currentType.value.replace('QVariant', 'QJSValue', 1))
    else:
        d.putEmptyValue()
        #qdump__QV4__Value(d, d.createValue(dd, ns + 'QV4::Value'))
        #return
    if d.isExpanded():
        with Children(d):
            with SubItem(d, '[raw]'):
                d.putValue('[0x%x]' % dd)
                d.putType(' ')
            d.putFields(value)


def qdump__QQmlBinding(d, value):
    d.putEmptyValue()
    if d.isExpanded():
        with Children(d):
            d.putCallItem('expressionIdentifier', '@QString',
                          value, 'expressionIdentifier')
            d.putFields(value)


#######################################################################
#
# Webkit
#
#######################################################################


def jstagAsString(tag):
    # enum { Int32Tag =        0xffffffff };
    # enum { CellTag =         0xfffffffe };
    # enum { TrueTag =         0xfffffffd };
    # enum { FalseTag =        0xfffffffc };
    # enum { NullTag =         0xfffffffb };
    # enum { UndefinedTag =    0xfffffffa };
    # enum { EmptyValueTag =   0xfffffff9 };
    # enum { DeletedValueTag = 0xfffffff8 };
    if tag == -1:
        return 'Int32'
    if tag == -2:
        return 'Cell'
    if tag == -3:
        return 'True'
    if tag == -4:
        return 'Null'
    if tag == -5:
        return 'Undefined'
    if tag == -6:
        return 'Empty'
    if tag == -7:
        return 'Deleted'
    return 'Unknown'


def qdump__QTJSC__JSValue(d, value):
    d.putEmptyValue()
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            tag = value['u']['asBits']['tag']
            payload = value['u']['asBits']['payload']
            #d.putIntItem('tag', tag)
            with SubItem(d, 'tag'):
                d.putValue(jstagAsString(int(tag)))
                d.putNoType()

            d.putIntItem('payload', int(payload))
            d.putFields(value['u'])

            if tag == -2:
                cellType = d.lookupType('QTJSC::JSCell').pointer()
                d.putSubItem('cell', payload.cast(cellType))

            try:
                # FIXME: This might not always be a variant.
                delegateType = d.lookupType(d.qtNamespace() + 'QScript::QVariantDelegate').pointer()
                delegate = scriptObject['d']['delegate'].cast(delegateType)
                #d.putSubItem('delegate', delegate)
                variant = delegate['m_value']
                d.putSubItem('variant', variant)
            except:
                pass


def qdump__QScriptValue(d, value):
    # structure:
    #  engine        QScriptEnginePrivate
    #  jscValue      QTJSC::JSValue
    #  next          QScriptValuePrivate *
    #  numberValue   5.5987310416280426e-270 myns::qsreal
    #  prev          QScriptValuePrivate *
    #  ref           QBasicAtomicInt
    #  stringValue   QString
    #  type          QScriptValuePrivate::Type: { JavaScriptCore, Number, String }
    #d.putEmptyValue()
    dd = value['d_ptr']['d']
    ns = d.qtNamespace()
    if dd.pointer() == 0:
        d.putValue('(invalid)')
        return
    if int(dd['type']) == 1:  # Number
        d.putValue(dd['numberValue'])
        d.putType('%sQScriptValue (Number)' % ns)
        return
    if int(dd['type']) == 2:  # String
        d.putStringValue(dd['stringValue'])
        d.putType('%sQScriptValue (String)' % ns)
        return

    d.putType('%sQScriptValue (JSCoreValue)' % ns)
    x = dd['jscValue']['u']
    tag = x['asBits']['tag']
    payload = x['asBits']['payload']
    #isValid = int(x['asBits']['tag']) != -6   # Empty
    #isCell = int(x['asBits']['tag']) == -2
    #DumperBase.warn('IS CELL: %s ' % isCell)
    #isObject = False
    #className = 'UNKNOWN NAME'
    #if isCell:
    #    # isCell() && asCell()->isObject();
    #    # in cell: m_structure->typeInfo().type() == ObjectType;
    #    cellType = d.lookupType('QTJSC::JSCell').pointer()
    #    cell = payload.cast(cellType).dereference()
    #    dtype = 'NO DYNAMIC TYPE'
    #    try:
    #        dtype = cell.dynamic_type
    #    except:
    #        pass
    #    warn('DYNAMIC TYPE: %s' % dtype)
    #    warn('STATUC  %s' % cell.type)
    #    type = cell['m_structure']['m_typeInfo']['m_type']
    #    isObject = int(type) == 7 # ObjectType;
    #    className = 'UNKNOWN NAME'
    #DumperBase.warn('IS OBJECT: %s ' % isObject)

    #inline bool JSCell::inherits(const ClassInfo* info) const
    #for (const ClassInfo* ci = classInfo(); ci; ci = ci->parentClass) {
    #    if (ci == info)
    #        return true;
    #return false;

    try:
        # This might already fail for 'native' payloads.
        scriptObjectType = d.lookupType(ns + 'QScriptObject').pointer()
        scriptObject = payload.cast(scriptObjectType)

        # FIXME: This might not always be a variant.
        delegateType = d.lookupType(ns + 'QScript::QVariantDelegate').pointer()
        delegate = scriptObject['d']['delegate'].cast(delegateType)
        #d.putSubItem('delegate', delegate)

        variant = delegate['m_value']
        #d.putSubItem('variant', variant)
        t = qdump__QVariant(d, variant)
        # Override the 'QVariant (foo)' output
        d.putBetterType('@QScriptValue (%s)' % t)
        if t != 'JSCoreValue':
            return
    except:
        pass

    # This is a 'native' JSCore type for e.g. QDateTime.
    d.putValue('<native>')
    d.putExpandable()
    if d.isExpanded():
        with Children(d):
            d.putSubItem('jscValue', dd['jscValue'])


def qdump__QQmlAccessorProperties__Properties(d, value):
    size = int(value['count'])
    d.putItemCount(size)
    if d.isExpanded():
        d.putArrayData(value['properties'], size)


#
# QJson
#

def qdumpHelper_qle_cutBits(value, offset, length):
    return (value >> offset) & ((1 << length) - 1)


def qdump__QJsonPrivate__qle_bitfield(d, value):
    offset = value.type[0]
    length = value.type[1]
    val = value['val'].integer()
    d.putValue('%s' % qdumpHelper_qle_cutBits(val, offset, length))


def qdumpHelper_qle_signedbitfield_value(d, value):
    offset = value.type[0]
    length = value.type[1]
    val = value['val'].integer()
    val = (val >> offset) & ((1 << length) - 1)
    if val >= (1 << (length - 1)):
        val -= (1 << (length - 1))
    return val


def qdump__QJsonPrivate__qle_signedbitfield(d, value):
    d.putValue('%s' % qdumpHelper_qle_signedbitfield_value(d, value))


def qdump__QJsonPrivate__q_littleendian(d, value):
    d.putValue('%s' % value['val'].integer())


def qdumpHelper_QJsonValue(d, data, base, pv):
    """
    Parameters are the parameters to the
    QJsonValue(QJsonPrivate::Data *data, QJsonPrivate::Base *base,
               const QJsonPrivate::Value& pv)
    constructor. We 'inline' the construction here.

    data is passed as pointer integer
    base is passed as pointer integer
    pv is passed as 32 bit integer.
    """
    d.checkIntType(data)
    d.checkIntType(base)
    d.checkIntType(pv)

    t = qdumpHelper_qle_cutBits(pv, 0, 3)
    v = qdumpHelper_qle_cutBits(pv, 5, 27)
    latinOrIntValue = qdumpHelper_qle_cutBits(pv, 3, 1)

    if t == 0:
        d.putType('QJsonValue (Null)')
        d.putValue('Null')
        return
    if t == 1:
        d.putType('QJsonValue (Bool)')
        d.putValue('true' if v else 'false')
        return
    if t == 2:
        d.putType('QJsonValue (Number)')
        if latinOrIntValue:
            w = toInteger(v)
            if w >= 0x4000000:
                w -= 0x8000000
            d.putValue(w)
        else:
            data = base + v
            f = d.split('d', data)[0]
            d.putValue(str(f))
        return
    if t == 3:
        d.putType('QJsonValue (String)')
        data = base + v
        if latinOrIntValue:
            length = d.extractUShort(data)
            d.putValue(d.readMemory(data + 2, length), 'latin1')
        else:
            length = d.extractUInt(data)
            d.putValue(d.readMemory(data + 4, length * 2), 'utf16')
        return
    if t == 4:
        d.putType('QJsonValue (Array)')
        qdumpHelper_QJsonArray(d, data, base + v)
        return
    if t == 5:
        d.putType('QJsonValue (Object)')
        qdumpHelper_QJsonObject(d, data, base + v)


def qdumpHelper_QJsonArray(d, data, array):
    """
    Parameters are the parameters to the
    QJsonArray(QJsonPrivate::Data *data, QJsonPrivate::Array *array)
    constructor. We 'inline' the construction here.

    array is passed as integer pointer to the QJsonPrivate::Base object.
    """

    if data:
        # The 'length' part of the _dummy member:
        n = qdumpHelper_qle_cutBits(d.extractUInt(array + 4), 1, 31)
    else:
        n = 0

    d.putItemCount(n)
    if d.isExpanded():
        with Children(d, maxNumChild=1000):
            table = array + d.extractUInt(array + 8)
            for i in range(n):
                with SubItem(d, i):
                    qdumpHelper_QJsonValue(d, data, array, d.extractUInt(table + 4 * i))


def qdumpHelper_QJsonObject(d, data, obj):
    """
    Parameters are the parameters to the
    QJsonObject(QJsonPrivate::Data *data, QJsonPrivate::Object *object);
    constructor. We "inline" the construction here.

    obj is passed as integer pointer to the QJsonPrivate::Base object.
    """

    if data:
        # The 'length' part of the _dummy member:
        n = qdumpHelper_qle_cutBits(d.extractUInt(obj + 4), 1, 31)
    else:
        n = 0

    d.putItemCount(n)
    if d.isExpanded():
        with Children(d, maxNumChild=1000):
            table = obj + d.extractUInt(obj + 8)
            for i in range(n):
                with SubItem(d, i):
                    entryPtr = table + 4 * i  # entryAt(i)
                    entryStart = obj + d.extractUInt(entryPtr)  # Entry::value
                    keyStart = entryStart + 4  # sizeof(QJsonPrivate::Entry) == 4
                    val = d.extractInt(entryStart)
                    key = d.extractInt(keyStart)
                    isLatinKey = qdumpHelper_qle_cutBits(val, 4, 1)
                    if isLatinKey:
                        keyLength = d.extractUShort(keyStart)
                        d.putField('key', d.readMemory(keyStart + 2, keyLength))
                        d.putField('keyencoded', 'latin1')
                    else:
                        keyLength = d.extractUInt(keyStart)
                        d.putField('key', d.readMemory(keyStart + 4, keyLength))
                        d.putField('keyencoded', 'utf16')

                    qdumpHelper_QJsonValue(d, data, obj, val)


def qdump__QJsonValue(d, value):
    (data, dd, t) = value.split('QpI')

    if d.qtVersion() >= 0x050f00:
        value = d.createProxyValue((data, dd, t, False), 'QCborValue_proxy')
        d.putItem(value)
        return

    if t == 0:
        d.putType('QJsonValue (Null)')
        d.putValue('Null')
        return
    if t == 1:
        d.putType('QJsonValue (Bool)')
        v = value.split('b')
        d.putValue('true' if v else 'false')
        return
    if t == 2:
        d.putType('QJsonValue (Number)')
        d.putValue(value.split('d'))
        return
    if t == 3:
        d.putType('QJsonValue (String)')
        string = value.split('{@QString}')[0]
        elided, base = d.encodeString(string, d.displayStringLimit)
        d.putValue(base, 'utf16', elided=elided)
        return
    if t == 4:
        d.putType('QJsonValue (Array)')
        qdumpHelper_QJsonArray(d, dd, data)
        return
    if t == 5:
        d.putType('QJsonValue (Object)')
        qdumpHelper_QJsonObject(d, dd, data)
        return
    d.putType('QJsonValue (Undefined)')
    d.putEmptyValue()


def qdump__QJsonArray(d, value):
    if d.qtVersion() >= 0x060000:
        dptr = d.extractPointer(value)
        if not dptr:
            d.putItemCount(0)
        else:
            qdumpHelper_QCbor_array(d, dptr, False)
    elif d.qtVersion() >= 0x050f00:
        _, container_ptr = value.split('pp')
        qdumpHelper_QCbor_array(d, container_ptr, False)
    else:
        qdumpHelper_QJsonArray(d, value['d'].pointer(), value['a'].pointer())


def qdump__QJsonObject(d, value):
    if d.qtVersion() >= 0x060000:
        dptr = d.extractPointer(value)
        if not dptr:
            d.putItemCount(0)
        else:
            qdumpHelper_QCbor_map(d, dptr, False)
    elif d.qtVersion() >= 0x050f00:
        _, container_ptr = value.split('pp')
        qdumpHelper_QCbor_map(d, container_ptr, False)
    else:
        qdumpHelper_QJsonObject(d, value['d'].pointer(), value['o'].pointer())


def qdump__QSqlResultPrivate(d, value):
    # QSqlResult *q_ptr;
    # QPointer<QSqlDriver> sqldriver;
    # int idx;
    # QString sql;
    # bool active;
    # bool isSel;
    # QSqlError error;
    # bool forwardOnly;
    # QSql::NumericalPrecisionPolicy precisionPolicy;
    # int bindCount;
    # QSqlResult::BindingSyntax binds;
    # QString executedQuery;
    # QHash<int, QSql::ParamType> types;
    # QVector<QVariant> values;
    # QHash<QString, QList<int> > indexes;
    # QVector<QHolder> holders
    vptr, qptr, sqldriver1, sqldriver2, idx, pad, sql, active, isSel, pad, \
        error1, error2, error3, \
        forwardOnly, pad, precisionPolicy, bindCount, \
        binds, executedQuery, types, values, indexes, holders = \
        value.split('ppppi@{@QString}bb@pppb@iiii{@QString}ppp')

    d.putStringValue(sql)
    d.putPlainChildren(value)


def qdump__QSqlField(d, value):
    val, dptr = value.split('{@QVariant}p')
    qdump__QVariant(d, val)
    d.putBetterType(d.currentType.value.replace('QVariant', 'QSqlField'))
    d.putPlainChildren(value)


def qdump__QLazilyAllocated(d, value):
    p = value.extractPointer()
    if p == 0:
        d.putValue("(null)")
    else:
        d.putItem(d.createValue(p, value.type[0]))
        d.putBetterType(value.type)


def qdump__qfloat16(d, value):
    h = value.split('H')[0]
    # Stole^H^H^HHeavily inspired by J.F. Sebastian at
    # http://math.stackexchange.com/questions/1128204/how-to-convert-
    # from-floating-point-binary-to-decimal-in-half-precision16-bits
    sign = h >> 15
    exp = (h >> 10) & 0b011111
    fraction = h & (2**10 - 1)
    if exp == 0:
        if fraction == 0:
            res = -0.0 if sign else 0.0
        else:
            res = (-1)**sign * fraction / 2**10 * 2**(-14)  # subnormal
    elif exp == 0b11111:
        res = ('-inf' if sign else 'inf') if fraction == 0 else 'nan'
    else:
        res = (-1)**sign * (1 + 1. * fraction / 2**10) * 2**(exp - 15)
    d.putValue(res)
    d.putPlainChildren(value)



def qdumpHelper_QCbor_string(d, container_ptr, element_index, is_bytes):
    # d.split('i@{@QByteArray::size_type}pp', container_ptr) doesn't work with CDB,
    # so be explicit:
    data_pos = container_ptr + (2 * d.ptrSize() if d.qtVersion() >= 0x060000 else 8)
    elements_pos = data_pos + (3 * d.ptrSize() if d.qtVersion() >= 0x060000 else d.ptrSize())
    elements_data_ptr, elements_size = d.vectorData(elements_pos)
    element_at_n_addr = elements_data_ptr + element_index * 16 # sizeof(QtCbor::Element) == 16
    element_value, _, element_flags = d.split('qII', element_at_n_addr)
    enc = 'latin1' if is_bytes or (element_flags & 8) else 'utf16'
    bytedata, _, _ = d.qArrayData(data_pos)
    bytedata += element_value
    if d.qtVersion() >= 0x060000:
        bytedata_len = d.extractInt64(bytedata)
        bytedata_data = bytedata + 8
    else:
        bytedata_len = d.extractInt(bytedata)
        bytedata_data = bytedata + 4 # sizeof(QtCbor::ByteData) header part

    elided, shown = d.computeLimit(bytedata_len, d.displayStringLimit)
    res = d.readMemory(bytedata_data, shown)
    d.putValue(res, enc, elided=elided)


def qdumpHelper_QCborArray_valueAt(d, container_ptr, elements_data_ptr, idx, bytedata, is_cbor):
    element_at_n_addr = elements_data_ptr + idx * 16 # sizeof(QtCbor::Element) == 15
    element_value, element_type, element_flags = d.split('qII', element_at_n_addr)
    element_container, _, _ = d.split('pII', element_at_n_addr)
    if element_flags & 1: # QtCbor::Element::IsContainer
        return d.createProxyValue((-1, element_container, element_type, is_cbor), 'QCborValue_proxy')
    if element_flags & 2: # QtCbor::Element::HasByteData
        return d.createProxyValue((idx, container_ptr, element_type, is_cbor), 'QCborValue_proxy')
    return d.createProxyValue((element_value, 0, element_type, is_cbor), 'QCborValue_proxy')


def qdumpHelper_QCbor_array(d, container_ptr, is_cbor):
    if not container_ptr:
        d.putItemCount(0)
        return
    # d.split('i@{@QByteArray::size_type}pp', container_ptr) doesn't work with CDB,
    # so be explicit:
    data_pos = container_ptr + (2 * d.ptrSize() if d.qtVersion() >= 0x060000 else 8)
    elements_pos = data_pos + (3 * d.ptrSize() if d.qtVersion() >= 0x060000 else d.ptrSize())
    elements_data_ptr, elements_size = d.vectorData(elements_pos)
    d.putItemCount(elements_size)
    if d.isExpanded():
        bytedata, _, _ = d.qArrayData(data_pos)
        with Children(d, maxNumChild=1000):
            for i in range(elements_size):
                d.putSubItem(i, qdumpHelper_QCborArray_valueAt(d, container_ptr, elements_data_ptr, i, bytedata, is_cbor))


def qdump__QCborArray(d, value):
    container_ptr = value.extractPointer()
    qdumpHelper_QCbor_array(d, container_ptr, True)


def qdumpHelper_QCbor_map(d, container_ptr, is_cbor):
    if not container_ptr:
        d.putItemCount(0)
        return
    # d.split('i@{@QByteArray::size_type}pp', container_ptr) doesn't work with CDB,
    # so be explicit:
    data_pos = container_ptr + (2 * d.ptrSize() if d.qtVersion() >= 0x060000 else 8)
    elements_pos = data_pos + (3 * d.ptrSize() if d.qtVersion() >= 0x060000 else d.ptrSize())
    elements_data_ptr, elements_size = d.vectorData(elements_pos)
    elements_size = int(elements_size / 2)
    d.putItemCount(elements_size)
    if d.isExpanded():
        bytedata, _, _ = d.qArrayDataHelper(data_pos)
        with Children(d, maxNumChild=1000):
            for i in range(elements_size):
                key = qdumpHelper_QCborArray_valueAt(d, container_ptr, elements_data_ptr, 2 * i, bytedata, is_cbor)
                val = qdumpHelper_QCborArray_valueAt(d, container_ptr, elements_data_ptr, 2 * i + 1, bytedata, is_cbor)
                d.putPairItem(i, (key, val), 'key', 'value')


def qdump__QCborMap(d, value):
    container_ptr = value.extractPointer()
    qdumpHelper_QCbor_map(d, container_ptr, True)


def qdump__QCborValue(d, value):
    item_data, container_ptr, item_type = value.split('qpi')
    d.putItem(d.createProxyValue((item_data, container_ptr, item_type, True), 'QCborValue_proxy'))
    d.putPlainChildren(value)

def qdump__QCborValue_proxy(d, value):
    item_data, container_ptr, item_type, is_cbor = value.data()

    def typename(key, is_cbor):
        if is_cbor:
            return {
                'invalid'   : 'QCborValue (Invalid)',
                'int'       : 'QCborValue (Integer)',
                'false'     : 'QCborValue (False)',
                'true'      : 'QCborValue (True)',
                'null'      : 'QCborValue (Null)',
                'undefined' : 'QCborValue (Undefined)',
                'double'    : 'QCborValue (Double)',
                'bytes'     : 'QCborValue (ByteArray)',
                'string'    : 'QCborValue (String)',
                'map'       : 'QCborValue (Map)',
                'array'     : 'QCborValue (Array)'
            }.get(key, 'QCborValue (Unknown)')
        else:
            return {
                'invalid'   : 'QJsonValue (Invalid)',
                'int'       : 'QJsonValue (Number)',
                'false'     : 'QJsonValue (Bool)',
                'true'      : 'QJsonValue (Bool)',
                'null'      : 'QJsonValue (Null)',
                'undefined' : 'QJsonValue (Undefined)',
                'double'    : 'QJsonValue (Number)',
                'bytes'     : 'QJsonValue (ByteArray)',
                'string'    : 'QJsonValue (String)',
                'map'       : 'QJsonValue (Object)',
                'array'     : 'QJsonValue (Array)'
            }.get(key, 'QJsonValue (Unknown)')


    if item_type == 0xffffffffffffffff:
        d.putType(typename('invalid', is_cbor))
        d.putValue('Invalid')

    elif item_type == 0x00:
        d.putType(typename('int', is_cbor))
        d.putValue(item_data)

    elif item_type == 0x100 + 20:
        d.putType(typename('false', is_cbor))
        d.putValue('False')

    elif item_type == 0x100 + 21:
        d.putType(typename('true', is_cbor))
        d.putValue('True')

    elif item_type == 0x100 + 22:
        d.putType(typename('null', is_cbor))
        d.putValue('Null')

    elif item_type == 0x100 + 23:
        d.putType(typename('undefined', is_cbor))
        d.putValue('Undefined')

    elif item_type == 0x202:
        val = struct.unpack('d', struct.pack('q', item_data))
        d.putType(typename('double', is_cbor))
        d.putValue('%f' % val)

    elif item_type == 0x40:
        d.putType(typename('bytes', is_cbor))
        qdumpHelper_QCbor_string(d, container_ptr, item_data, True)

    elif item_type == 0x60:
        d.putType(typename('string', is_cbor))
        qdumpHelper_QCbor_string(d, container_ptr, item_data, False)

    elif item_type == 0x80:
        d.putType(typename('array', is_cbor))
        qdumpHelper_QCbor_array(d, container_ptr, is_cbor)

    elif item_type == 0xa0:
        d.putType(typename('map', is_cbor))
        qdumpHelper_QCbor_map(d, container_ptr, is_cbor)

    elif item_type == 0x10000:
        d.putType('QCborValue (DateTime)')
        d.putValue('DateTime')

    elif item_type == 0x10020:
        d.putType('QCborValue (Url)')
        d.putValue('Url')

    elif item_type == 0x10023:
        d.putType('QCborValue (RegularExpression)')
        d.putValue('RegularExpression')

    elif item_type == 0x10025:
        d.putType('QCborValue (Uuid)')
        d.putValue('Uuid')

    else:
        d.putType('QCborValue (Unknown)')
        d.putValue(item_data)


def qendian_helper(d, value, code, endian):
    data = value.data()
    size = len(data)
    v = struct.unpack_from(endian + code, data)
    d.putValue('%d (%s)' % (v[0], 'LE' if endian == '<' else 'BE'))
    d.putNumChild(size)
    if d.isExpanded():
        s = struct.unpack_from(endian + 'b' * size, data)
        with Children(d):
            for i in range(size):
                with SubItem(d, '[%d]' % i):
                    d.putValue(s[i])
                    d.putType('byte')


def qdump__quint16_le(d, value):
    qendian_helper(d, value, 'H', '<')

def qdump__quint32_le(d, value):
    qendian_helper(d, value, 'I', '<')

def qdump__quint64_le(d, value):
    qendian_helper(d, value, 'Q', '<')

def qdump__quint16_be(d, value):
    qendian_helper(d, value, 'H', '>')

def qdump__quint32_be(d, value):
    qendian_helper(d, value, 'I', '>')

def qdump__quint64_be(d, value):
    qendian_helper(d, value, 'Q', '>')
