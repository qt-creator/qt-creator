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

#ifndef GRADIENTLINEQMLADAPTOR_H
#define GRADIENTLINEQMLADAPTOR_H

#include <qmleditorwidgets/gradientline.h>
#include <propertyeditorvalue.h>
#include <qmlitemnode.h>

namespace QmlDesigner {

class GradientLineQmlAdaptor : public QmlEditorWidgets::GradientLine
{
    Q_OBJECT

    Q_PROPERTY(QVariant itemNode READ itemNode WRITE setItemNode NOTIFY itemNodeChanged)

public:
    explicit GradientLineQmlAdaptor(QWidget *parent = 0);

    static void registerDeclarativeType();

signals:
    void itemNodeChanged();

public slots:
    void setupGradient();
    void writeGradient();
    void deleteGradient();

private:
    QVariant itemNode() const { return QVariant::fromValue(m_itemNode.modelNode()); }
    void setItemNode(const QVariant &itemNode);

    QmlItemNode m_itemNode;

};

} //QmlDesigner

#endif // GRADIENTLINEQMLADAPTOR_H
