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

#include "qmljsicons.h"

#include <QDir>
#include <QHash>
#include <QIcon>
#include <QLoggingCategory>
#include <QPair>

using namespace QmlJS;
using namespace QmlJS::AST;

enum {
    debug = false
};

static Q_LOGGING_CATEGORY(iconsLog, "qtc.qmljs.icons")

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
        qCDebug(iconsLog) << "parsing" << iconPath;
    QDir topDir(iconPath);
    foreach (const QFileInfo &subDirInfo, topDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (debug)
            qCDebug(iconsLog) << "parsing" << subDirInfo.absoluteFilePath();
        const QString packageName = subDirInfo.fileName();
        QDir subDir(subDirInfo.absoluteFilePath() + QLatin1String("/16x16"));
        foreach (const QFileInfo &iconFile, subDir.entryInfoList(QDir::Files)) {
            QIcon icon(iconFile.absoluteFilePath());
            if (icon.isNull()) {
                if (debug)
                    qCDebug(iconsLog) << "skipping" << iconFile.absoluteFilePath();
                continue;
            }
            if (debug)
                qCDebug(iconsLog) << "adding" << packageName << iconFile.baseName() << "icon to database";
            QPair<QString,QString> element(packageName, iconFile.baseName());
            d->iconHash.insert(element, icon);
        }
    }
}

QIcon Icons::icon(const QString &packageName, const QString typeName) const
{
    QPair<QString,QString> element(packageName, typeName);
    if (debug)
        qCDebug(iconsLog) << "icon for" << packageName << typeName << "requested" << d->iconHash.contains(element);
    return d->iconHash.value(element);
}

QIcon Icons::icon(Node *node) const
{
    if (dynamic_cast<AST::UiObjectDefinition*>(node))
        return objectDefinitionIcon();
    if (dynamic_cast<AST::UiScriptBinding*>(node))
        return scriptBindingIcon();

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
