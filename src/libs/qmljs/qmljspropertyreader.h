/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QMLJSPROPERTYREADER_H
#define QMLJSPROPERTYREADER_H

#include <qmljs/qmljs_global.h>
#include <qmljs/parser/qmljsastfwd_p.h>
#include <qmljs/qmljsdocument.h>

#include <QHash>
#include <QList>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QLinearGradient>

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

    QLinearGradient parseGradient(const QString &propertyName) const;

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
