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

#include "qmljsicons.h"
#include <QDebug>
#include <QDir>
#include <QHash>
#include <QPair>

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
    : d(new IconsPrivate)
{
    d->elementIcon = QIcon(QLatin1String(":/qmljs/images/element.png"));
    d->propertyIcon = QIcon(QLatin1String(":/qmljs/images/property.png"));
    d->publicMemberIcon = QIcon(QLatin1String(":/qmljs/images/publicmember.png"));
    d->functionDeclarationIcon = QIcon(QLatin1String(":/qmljs/images/func.png"));
}

Icons::~Icons()
{
    m_instance = 0;
    delete d;
}

Icons *Icons::instance()
{
    if (!m_instance)
        m_instance = new Icons();
    return m_instance;
}

void Icons::setIconFilesPath(const QString &iconPath)
{
    if (iconPath == d->resourcePath)
        return;

    d->resourcePath = iconPath;

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
            d->iconHash.insert(element, icon);
        }
    }
}

QIcon Icons::icon(const QString &packageName, const QString typeName) const
{
    QPair<QString,QString> element(packageName, typeName);
    if (debug)
        qDebug() << "QmlJSIcons - icon for" << packageName << typeName << "requested" << d->iconHash.contains(element);
    return d->iconHash.value(element);
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
    return d->elementIcon;
}

QIcon Icons::scriptBindingIcon() const
{
    return d->propertyIcon;
}

QIcon Icons::publicMemberIcon() const
{
    return d->publicMemberIcon;
}

QIcon Icons::functionDeclarationIcon() const
{
    return d->functionDeclarationIcon;
}
