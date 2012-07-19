/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLJSPROPERTYREADER_H
#define QMLJSPROPERTYREADER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsastfwd_p.h>
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
