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

#ifndef QMLTEXTGENERATOR_H
#define QMLTEXTGENERATOR_H

#include <QString>
#include <QStringList>

#include "abstractproperty.h"
#include "modelnode.h"

namespace QmlDesigner {
namespace Internal {

class QmlTextGenerator
{
public:
    explicit QmlTextGenerator(const QStringList &propertyOrder, int indentDepth = 0);

    QString operator()(const AbstractProperty &property) const
    { return toQml(property, m_indentDepth); }

    QString operator()(const ModelNode &modelNode) const
    { return toQml(modelNode, m_indentDepth); }

private:
    QString toQml(const AbstractProperty &property, int indentDepth) const;
    QString toQml(const ModelNode &modelNode, int indentDepth) const;
    QString propertiesToQml(const ModelNode &node, int indentDepth) const;
    QString propertyToQml(const AbstractProperty &property, int indentDepth) const;

    static QString escape(const QString &value);

private:
    QStringList m_propertyOrder;
    int m_indentDepth;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // QMLTEXTGENERATOR_H
