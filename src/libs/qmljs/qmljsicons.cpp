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

#include "qmljsicons.h"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QPair>

using namespace QmlJS;
using namespace QmlJS::AST;

enum {
    debug = false
};

namespace QmlJS {

Icons *Icons::m_instance = 0;

class IconsPrivate
{
public:
    QIcon elementIcon;
    QIcon propertyIcon;
    QIcon publicMemberIcon;
    QIcon functionDeclarationIcon;
    QHash<QPair<QString,QString>,QIcon> iconHash;
    QString resourcePath;
};

} // namespace QmlJS

Icons::Icons()
    : m_d(new IconsPrivate)
{
    m_d->elementIcon = QIcon(QLatin1String(":/qmljs/images/element.png"));
    m_d->propertyIcon = QIcon(QLatin1String(":/qmljs/images/property.png"));
    m_d->publicMemberIcon = QIcon(QLatin1String(":/qmljs/images/publicmember.png"));
    m_d->functionDeclarationIcon = QIcon(QLatin1String(":/qmljs/images/func.png"));
}

Icons::~Icons()
{
    m_instance = 0;
    delete m_d;
}

Icons *Icons::instance()
{
    if (!m_instance)
        m_instance = new Icons();
    return m_instance;
}

void Icons::setIconFilesPath(const QString &iconPath)
{
    if (iconPath == m_d->resourcePath)
        return;

    m_d->resourcePath = iconPath;

    if (debug)
        qDebug() << "QmlJSIcons -" << "parsing" << iconPath;
    QDir topDir(iconPath);
    foreach (const QFileInfo &subDirInfo, topDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (debug)
            qDebug() << "QmlJSIcons - parsing" << subDirInfo.absoluteFilePath();
        const QString packageName = subDirInfo.fileName();
        QDir subDir(subDirInfo.absoluteFilePath() + QLatin1String("/16x16"));
        foreach (const QFileInfo &iconFile, subDir.entryInfoList(QDir::Files)) {
            QIcon icon(iconFile.absoluteFilePath());
            if (icon.isNull()) {
                if (debug)
                    qDebug() << "QmlJSIcons - skipping" << iconFile.absoluteFilePath();
                continue;
            }
            if (debug)
                qDebug() << "QmlJSIcons - adding" << packageName << iconFile.baseName() << "icon to database";
            QPair<QString,QString> element(packageName, iconFile.baseName());
            m_d->iconHash.insert(element, icon);
        }
    }
}

QIcon Icons::icon(const QString &packageName, const QString typeName) const
{
    QPair<QString,QString> element(packageName, typeName);
    if (debug)
        qDebug() << "QmlJSIcons - icon for" << packageName << typeName << "requested" << m_d->iconHash.contains(element);
    return m_d->iconHash.value(element);
}

QIcon Icons::icon(Node *node) const
{
    if (dynamic_cast<AST::UiObjectDefinition*>(node)) {
        return objectDefinitionIcon();
    }
    if (dynamic_cast<AST::UiScriptBinding*>(node)) {
        return scriptBindingIcon();
    }

    return QIcon();
}

QIcon Icons::objectDefinitionIcon() const
{
    return m_d->elementIcon;
}

QIcon Icons::scriptBindingIcon() const
{
    return m_d->propertyIcon;
}

QIcon Icons::publicMemberIcon() const
{
    return m_d->publicMemberIcon;
}

QIcon Icons::functionDeclarationIcon() const
{
    return m_d->functionDeclarationIcon;
}
