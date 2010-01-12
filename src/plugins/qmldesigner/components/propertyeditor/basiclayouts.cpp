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

/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the $MODULE$ of the Qt Toolkit.
**
** $TROLLTECH_DUAL_LICENSE$
**
****************************************************************************/

#include "basiclayouts.h"


QT_BEGIN_NAMESPACE
QML_DEFINE_TYPE(Qt,4,6,QBoxLayout,QBoxLayoutObject);

QBoxLayoutObject::QBoxLayoutObject(QObject *parent)
: QLayoutObject(parent), _widgets(this), _layout(0)
{
}

QBoxLayoutObject::QBoxLayoutObject(QBoxLayout *layout, QObject *parent)
: QLayoutObject(parent), _widgets(this), _layout(layout)
{
}

QLayout *QBoxLayoutObject::layout() const
{
    return _layout;
}

void QBoxLayoutObject::addWidget(QWidget *wid)
{
    _layout->addWidget(wid);
}

void QBoxLayoutObject::clearWidget()
{
}

QML_DEFINE_TYPE(Qt,4,6,QHBoxLayout,QHBoxLayoutObject);
QHBoxLayoutObject::QHBoxLayoutObject(QObject *parent)
: QBoxLayoutObject(new QHBoxLayout, parent)
{
}


QML_DEFINE_TYPE(Qt,4,6,QVBoxLayout,QVBoxLayoutObject);
QVBoxLayoutObject::QVBoxLayoutObject(QObject *parent)
: QBoxLayoutObject(new QVBoxLayout, parent)
{
}
QT_END_NAMESPACE
