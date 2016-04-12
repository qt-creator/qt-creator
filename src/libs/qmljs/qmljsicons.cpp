/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsicons.h"

#include <cplusplus/Icons.h>

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
    QHash<QPair<QString,QString>,QIcon> iconHash;
    QString resourcePath;
};

} // namespace QmlJS

Icons::Icons()
    : d(new IconsPrivate)
{
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

QIcon Icons::icon(Node *node)
{
    if (dynamic_cast<AST::UiObjectDefinition*>(node))
        return objectDefinitionIcon();
    if (dynamic_cast<AST::UiScriptBinding*>(node))
        return scriptBindingIcon();

    return QIcon();
}

QIcon Icons::objectDefinitionIcon()
{
    return CPlusPlus::Icons::iconForType(CPlusPlus::Icons::ClassIconType);
}

QIcon Icons::scriptBindingIcon()
{
    return CPlusPlus::Icons::iconForType(CPlusPlus::Icons::VarPublicIconType);
}

QIcon Icons::publicMemberIcon()
{
    return CPlusPlus::Icons::iconForType(CPlusPlus::Icons::FuncPublicIconType);
}

QIcon Icons::functionDeclarationIcon()
{
    return CPlusPlus::Icons::iconForType(CPlusPlus::Icons::FuncPublicIconType);
}
