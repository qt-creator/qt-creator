/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef CORELIB_GLOBAL_H
#define CORELIB_GLOBAL_H

#include <QtGlobal>
#include <QList>


// Unnecessary since core isn't a dll any more.

#define TEST_CORESHARED_EXPORT

#if defined(DESIGNER_CORE_LIBRARY)
#  define QMLDESIGNERCORE_EXPORT Q_DECL_EXPORT
#else
#  define QMLDESIGNERCORE_EXPORT Q_DECL_IMPORT
#endif
namespace QmlDesigner {
typedef QByteArray PropertyName;
typedef QList<PropertyName> PropertyNameList;
typedef QByteArray TypeName;
typedef QList<PropertyName> PropertyTypeList;
typedef QByteArray IdName;

enum AnchorLineType {
    AnchorLineInvalid = 0x0,
    AnchorLineNoAnchor = AnchorLineInvalid,
    AnchorLineLeft = 0x01,
    AnchorLineRight = 0x02,
    AnchorLineTop = 0x04,
    AnchorLineBottom = 0x08,
    AnchorLineHorizontalCenter = 0x10,
    AnchorLineVerticalCenter = 0x20,
    AnchorLineBaseline = 0x40,

    AnchorLineFill =  AnchorLineLeft | AnchorLineRight | AnchorLineTop | AnchorLineBottom,
    AnchorLineCenter = AnchorLineVerticalCenter | AnchorLineHorizontalCenter,
    AnchorLineHorizontalMask = AnchorLineLeft | AnchorLineRight | AnchorLineHorizontalCenter,
    AnchorLineVerticalMask = AnchorLineTop | AnchorLineBottom | AnchorLineVerticalCenter | AnchorLineBaseline,
    AnchorLineAllMask = AnchorLineVerticalMask | AnchorLineHorizontalMask
};
}
//#if defined(TEST_EXPORTS)
//#if defined(CORE_LIBRARY)
//#  define TEST_CORESHARED_EXPORT Q_DECL_EXPORT
//#else
//#  define TEST_CORESHARED_EXPORT Q_DECL_IMPORT
//#endif
//#else
//#  define TEST_CORESHARED_EXPORT
//#endif

#include <qglobal.h>

#endif // CORELIB_GLOBAL_H
