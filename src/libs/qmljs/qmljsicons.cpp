// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsicons.h"

#include <cplusplus/Icons.h>
#include <utils/appinfo.h>
#include <qmljs/parser/qmljsast_p.h>

#include <QDir>
#include <QHash>
#include <QIcon>
#include <QLoggingCategory>
#include <QPair>
#include <QStringView>

using namespace QmlJS;
using namespace QmlJS::AST;

enum {
    debug = false
};

static Q_LOGGING_CATEGORY(iconsLog, "qtc.qmljs.icons", QtWarningMsg)

namespace QmlJS::Icons {
namespace Internal {
class IconsStorage
{
public:
    using FullTypeName = std::pair<QString /*packageName*/, QString /*typeName*/>;

    explicit IconsStorage();

    QIcon query(QStringView typeName) const;

private:
    const Utils::FilePath m_path = Utils::appInfo().resources / QLatin1String("qmlicons");
    QHash<FullTypeName, QIcon> m_iconsMap;
    QList<QString> m_packageNames;
};

IconsStorage::IconsStorage()
{
    const QString iconsPath = m_path.toFSPathString();
    if (debug)
        qCDebug(iconsLog) << "parsing" << iconsPath;
    QDir topDir(iconsPath);
    const QFileInfoList dirs = topDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo &subDirInfo : dirs) {
        if (debug)
            qCDebug(iconsLog) << "parsing" << subDirInfo.absoluteFilePath();
        const QString packageName = subDirInfo.fileName();
        m_packageNames.append(packageName);
        QDir subDir(subDirInfo.absoluteFilePath() + QLatin1String("/16x16"));
        const QFileInfoList files = subDir.entryInfoList(QDir::Files);
        for (const QFileInfo &iconFile : files) {
            QIcon icon(iconFile.absoluteFilePath());
            if (icon.isNull()) {
                if (debug)
                    qCDebug(iconsLog) << "skipping" << iconFile.absoluteFilePath();
                continue;
            }
            if (debug)
                qCDebug(iconsLog) << "adding" << packageName << iconFile.baseName()
                                  << "icon to database";
            FullTypeName iconId(packageName, iconFile.baseName());
            m_iconsMap.insert(iconId, icon);
        }
    }
}

QIcon IconsStorage::query(QStringView typeName) const
{
    for (const QString &packageName : m_packageNames) {
        const FullTypeName fullTypeName(packageName, typeName.toString());
        if (debug)
            qCDebug(iconsLog) << "icon for" << packageName << typeName << "requested"
                              << m_iconsMap.contains(fullTypeName);
        if (m_iconsMap.contains(fullTypeName)) {
            return m_iconsMap.value(fullTypeName);
        }
    }
    return QIcon();
}
} // namespace Internal

Provider::Provider()
    : m_stockPtr(std::make_unique<const Internal::IconsStorage>())
{
}

Provider &Provider::instance()
{
    static Provider iconsProvider;
    return iconsProvider;
}

QIcon Provider::icon(QStringView typeName) const
{
    return m_stockPtr->query(typeName);
}

QIcon objectDefinitionIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Class);
}

QIcon scriptBindingIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::VarPublic);
}

QIcon publicMemberIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::FuncPublic);
}

QIcon functionDeclarationIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::FuncPublic);
}

QIcon enumMemberIcon()
{
    return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Enum);
}
} // namespace QmlJS::Icons
