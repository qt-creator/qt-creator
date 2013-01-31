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

#ifndef KNOWNTYPE_H
#define KNOWNTYPE_H

// Enumeration describing a type.
enum KnownType
{
    KT_Unknown =0,
    // Flags to be used in type values.
    KT_POD_Type = 0x10000,
    KT_Qt_Type = 0x20000,
    KT_Qt_PrimitiveType = 0x40000,
    KT_Qt_MovableType = 0x80000,
    KT_STL_Type = 0x100000,
    KT_ContainerType = 0x200000,
    KT_HasSimpleDumper = 0x400000,
    KT_HasComplexDumper = 0x800000, // Non-container complex dumper
    KT_Editable = 0x1000000, // Editable complex type
    // Types: PODs
    KT_Char = KT_POD_Type + 1,
    KT_UnsignedChar = KT_POD_Type + 2,
    KT_IntType = KT_POD_Type + 3,         // any signed short, long, int
    KT_UnsignedIntType = KT_POD_Type + 4, // any unsigned int
    KT_FloatType = KT_POD_Type + 5,       // float, double
    KT_POD_PointerType = KT_POD_Type + 6,     // pointer to some POD
    KT_PointerType = KT_POD_Type + 7,     // pointer to class or complex type
    // Types: Qt Basic
    KT_QChar = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 1,
    KT_QByteArray = KT_Qt_Type + KT_Editable + KT_Qt_MovableType + KT_HasComplexDumper + KT_HasSimpleDumper + 2,
    KT_QString = KT_Qt_Type + KT_Editable + KT_Qt_MovableType + KT_HasSimpleDumper + 3,
    KT_QColor = KT_Qt_Type + KT_HasSimpleDumper + 4,
    KT_QFlags = KT_Qt_Type + KT_HasSimpleDumper + 5,
    KT_QDate = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 6,
    KT_QTime = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 7,
    KT_QPoint = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 8,
    KT_QPointF = KT_Qt_Type +KT_Qt_MovableType + KT_HasSimpleDumper + 9,
    KT_QSize = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 11,
    KT_QSizeF = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 12,
    KT_QLine = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 13,
    KT_QLineF = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 14,
    KT_QRect = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 15,
    KT_QRectF = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 16,
    KT_QVariant = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + KT_HasComplexDumper + 17,
    KT_QBasicAtomicInt = KT_Qt_Type + KT_HasSimpleDumper + 18,
    KT_QAtomicInt = KT_Qt_Type + KT_HasSimpleDumper + 19,
    KT_QStringRef = KT_Qt_Type + KT_HasSimpleDumper + 20,
    KT_QObject = KT_Qt_Type + KT_HasSimpleDumper + KT_HasComplexDumper + 20,
    KT_QWindow = KT_Qt_Type + KT_HasSimpleDumper + KT_HasComplexDumper + 21,
    KT_QWidget = KT_Qt_Type + KT_HasSimpleDumper + KT_HasComplexDumper + 22,
    KT_QSharedPointer = KT_Qt_Type + KT_HasSimpleDumper + KT_HasComplexDumper + 23,
    // Types: Various QT movable types
    KT_QPen = KT_Qt_Type + KT_Qt_MovableType + 30,
    KT_QUrl = KT_Qt_Type + KT_Qt_MovableType + 31 + KT_HasSimpleDumper,
    KT_QIcon = KT_Qt_Type + KT_Qt_MovableType + 32,
    KT_QBrush = KT_Qt_Type + KT_Qt_MovableType + 33,
    KT_QImage = KT_Qt_Type + KT_HasSimpleDumper + KT_Qt_MovableType + 35,
    KT_QLocale = KT_Qt_Type + KT_Qt_MovableType + 36,
    KT_QMatrix = KT_Qt_Type + KT_Qt_MovableType + 37,
    KT_QRegExp = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 38,
    KT_QMargins = KT_Qt_Type + KT_Qt_MovableType + 39,
    KT_QXmltem = KT_Qt_Type + KT_Qt_MovableType + 40,
    KT_QXmlName = KT_Qt_Type + KT_Qt_MovableType + 41,
    KT_QBitArray = KT_Qt_Type + KT_Qt_MovableType + 42,
    KT_QDateTime = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 43,
    KT_QFileInfo = KT_Qt_Type + KT_Qt_MovableType + KT_HasSimpleDumper + 44,
    KT_QMetaEnum = KT_Qt_Type + KT_Qt_MovableType + 45,
    KT_QVector2D = KT_Qt_Type + KT_Qt_MovableType + 46,
    KT_QVector3D = KT_Qt_Type + KT_Qt_MovableType + 47,
    KT_QVector4D = KT_Qt_Type + KT_Qt_MovableType + 48,
    KT_QMatrix4x4 = KT_Qt_Type + KT_Qt_MovableType + 49,
    KT_QTextBlock = KT_Qt_Type + KT_Qt_MovableType + 50,
    KT_QTransform = KT_Qt_Type + KT_Qt_MovableType + 51,
    KT_QBasicTimer = KT_Qt_Type + KT_Qt_MovableType + 52,
    KT_QMetaMethod = KT_Qt_Type + KT_Qt_MovableType + 53,
    KT_QModelIndex = KT_Qt_Type + KT_Qt_MovableType + 54,
    KT_QQuaternion = KT_Qt_Type + KT_Qt_MovableType + 55,
    KT_QScriptItem = KT_Qt_Type + KT_Qt_MovableType + 56,
    KT_QKeySequence = KT_Qt_Type + KT_Qt_MovableType + 57,
    KT_QTextFragment = KT_Qt_Type + KT_Qt_MovableType + 58,
    KT_QTreeViewItem = KT_Qt_Type + KT_Qt_MovableType + 59,
    KT_QMetaClassInfo = KT_Qt_Type + KT_Qt_MovableType + 60,
    KT_QNetworkCookie = KT_Qt_Type + KT_Qt_MovableType + 61,
    KT_QHashDummyValue = KT_Qt_Type + KT_Qt_MovableType + 62,
    KT_QSourceLocation = KT_Qt_Type + KT_Qt_MovableType + 63,
    KT_QNetworkProxyQuery = KT_Qt_Type + KT_Qt_MovableType + 64,
    KT_QXmlNodeModelIndex = KT_Qt_Type + KT_Qt_MovableType + 65,
    KT_QItemSelectionRange = KT_Qt_Type + KT_Qt_MovableType + 66,
    KT_QPaintBufferCommand = KT_Qt_Type + KT_Qt_MovableType + 67,
    KT_QTextHtmlParserNode = KT_Qt_Type + KT_Qt_MovableType + 68,
    KT_QXmlStreamAttribute = KT_Qt_Type + KT_Qt_MovableType + 69,
    KT_QTextBlock_iterator = KT_Qt_Type + KT_Qt_MovableType + 70,
    KT_QTextFrame_iterator = KT_Qt_Type + KT_Qt_MovableType + 71,
    KT_QPersistentModelIndex = KT_Qt_Type + KT_Qt_MovableType + 72,
    KT_QObjectPrivate_Sender = KT_Qt_Type + KT_Qt_MovableType + 73,
    KT_QPatternist_AtomicValue = KT_Qt_Type + KT_Qt_MovableType + 74,
    KT_QPatternist_Cardinality = KT_Qt_Type + KT_Qt_MovableType + 75,
    KT_QObjectPrivate_Connection = KT_Qt_Type + KT_Qt_MovableType + 76,
    KT_QPatternist_ItemCacheCell = KT_Qt_Type + KT_Qt_MovableType + 77,
    KT_QPatternist_ItemType_Ptr = KT_Qt_Type + KT_Qt_MovableType + 78,
    KT_QPatternist_NamePool_Ptr = KT_Qt_Type + KT_Qt_MovableType + 79,
    KT_QXmlStreamEntityDeclaration = KT_Qt_Type + KT_Qt_MovableType + 80,
    KT_QPatternist_Expression_Ptr = KT_Qt_Type + KT_Qt_MovableType + 81,
    KT_QXmlStreamNotationDeclaration = KT_Qt_Type + KT_Qt_MovableType + 82,
    KT_QPatternist_SequenceType_Ptr = KT_Qt_Type + KT_Qt_MovableType + 83,
    KT_QXmlStreamNamespaceDeclaration = KT_Qt_Type + KT_Qt_MovableType + 84,
    KT_QPatternist_Item_Iterator_Ptr = KT_Qt_Type + KT_Qt_MovableType + 85,
    KT_QPatternist_ItemSequenceCacheCell = KT_Qt_Type + KT_Qt_MovableType + 86,
    KT_QNetworkHeadersPrivate_RawHeaderPair = KT_Qt_Type + KT_Qt_MovableType + 87,
    KT_QPatternist_AccelTree_BasicNodeData = KT_Qt_Type + KT_Qt_MovableType + 88,
    KT_QFile = KT_Qt_Type + KT_HasSimpleDumper + 89,
    KT_QDir  = KT_Qt_Type + KT_HasSimpleDumper + 90,
    KT_QScriptValue = KT_Qt_Type + KT_HasSimpleDumper + 91,
    KT_QHostAddress = KT_Qt_Type + KT_HasSimpleDumper + 92,
    KT_QProcess = KT_Qt_Type + KT_HasSimpleDumper + 93,
    // Types: Qt primitive types
    KT_QFixed = KT_Qt_Type + KT_Qt_PrimitiveType + 90,
    KT_QTextItem = KT_Qt_Type + KT_Qt_PrimitiveType + 91,
    KT_QFixedSize = KT_Qt_Type + KT_Qt_PrimitiveType + 92,
    KT_QFixedPoint = KT_Qt_Type + KT_Qt_PrimitiveType + 93,
    KT_QScriptLine = KT_Qt_Type + KT_Qt_PrimitiveType + 94,
    KT_QScriptAnalysis = KT_Qt_Type + KT_Qt_PrimitiveType + 95,
    KT_QTextUndoCommand = KT_Qt_Type + KT_Qt_PrimitiveType + 96,
    KT_QGlyphJustification = KT_Qt_Type + KT_Qt_PrimitiveType + 97,
    KT_QPainterPath_Element = KT_Qt_Type + KT_Qt_PrimitiveType + 98,
    // Types: Qt Containers
    KT_QStringList = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 1,
    KT_QList = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 2,
    KT_QLinkedList = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 3,
    KT_QVector = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 4,
    KT_QStack = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 5,
    KT_QQueue = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 6,
    KT_QSet = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 7,
    KT_QHash = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 8,
    KT_QMultiHash = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 9,
    KT_QMap = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 10,
    KT_QMultiMap = KT_Qt_Type + KT_ContainerType + KT_HasSimpleDumper + 11,
    // Types: STL
    KT_StdString = KT_STL_Type + KT_Editable + KT_HasSimpleDumper + 1,
    KT_StdWString = KT_STL_Type + KT_Editable + KT_HasSimpleDumper + 2,
    // Types: STL containers
    KT_StdVector =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 1,
    KT_StdList =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 2,
    KT_StdStack =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 3,
    KT_StdDeque =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 4,
    KT_StdSet =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 5,
    KT_StdMap =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 6,
    KT_StdMultiMap =  KT_STL_Type + KT_ContainerType + KT_HasSimpleDumper + 7
};

#endif // KNOWNTYPE_H
