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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSPROPERTYREADER_H
#define QMLJSPROPERTYREADER_H

#include <qmljs/qmljsdocument.h>

#include <QHash>
#include <QVariant>
#include <QStringList>

QT_FORWARD_DECLARE_CLASS(QLinearGradient)

namespace QmlJS {

class Document;

class QMLJS_EXPORT PropertyReader
{
public:

    PropertyReader(Document::Ptr doc, AST::UiObjectInitializer *ast);

    bool hasProperty(const QString &propertyName) const
    { return m_properties.contains(propertyName); }

    QVariant readProperty(const QString &propertyName) const
    {
        if (hasProperty(propertyName))
            return m_properties.value(propertyName);
        else
            return QVariant();
    }

    QLinearGradient parseGradient(const QString &propertyName, bool *isBound) const;

    QStringList properties() const
    { return m_properties.keys(); }

    bool isBindingOrEnum(const QString &propertyName) const
    { return m_bindingOrEnum.contains(propertyName); }

private:
    QHash<QString, QVariant> m_properties;
    QList<QString> m_bindingOrEnum;
    AST::UiObjectInitializer *m_ast;
    Document::Ptr m_doc;
};

} //QmlJS

#endif // QMLJSPROPERTYREADER_H
