/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef DUMMYCONTEXTOBJECT_H
#define DUMMYCONTEXTOBJECT_H

#include <QObject>
#include <QPointer>
#include <qqml.h>

namespace QmlDesigner {

class DummyContextObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject * parent READ parentDummy WRITE setParentDummy NOTIFY parentDummyChanged DESIGNABLE false FINAL)

public:
    explicit DummyContextObject(QObject *parent = 0);

    QObject *parentDummy() const;
    void setParentDummy(QObject *parentDummy);

signals:
    void parentDummyChanged();

private:
    QPointer<QObject> m_dummyParent;
};

} // namespace QmlDesigner

QML_DECLARE_TYPE(QmlDesigner::DummyContextObject)
#endif // DUMMYCONTEXTOBJECT_H
