// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

static Q_LOGGING_CATEGORY(iconsLog, "qtc.qmljs.icons", QtWarningMsg)

namespace QmlJS {

Icons *Icons::m_instance = nullptr;

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
    m_instance = nullptr;
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
    QList<QFileInfo> dirs = topDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDirInfo : dirs) {
        if (debug)
            qCDebug(iconsLog) << "parsing" << subDirInfo.absoluteFilePath();
        const QString packageName = subDirInfo.fileName();
        QDir subDir(subDirInfo.absoluteFilePath() + QLatin1String("/16x16"));
        QList<QFileInfo> files = subDir.entryInfoList(QDir::Files);
        for (const QFileInfo &iconFile : files) {
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
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Class);
}

QIcon Icons::scriptBindingIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::VarPublic);
}

QIcon Icons::publicMemberIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::FuncPublic);
}

QIcon Icons::functionDeclarationIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::FuncPublic);
}
