/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SIMPLEPROEDITORINFO_H
#define SIMPLEPROEDITORINFO_H

#include <QtCore/QHash>
#include <QtCore/QVariant>
#include <QtGui/QPixmap>

namespace Qt4ProjectManager {
namespace Internal {

class SPEInfoItem
{
public:
    enum InfoKind {
        Configuration,
        Platform,
        Variable,
        QtModule,
        Template,
        Operator
    };

    SPEInfoItem(const QString &id, InfoKind kind);
    virtual ~SPEInfoItem() {}

    QString id(void) const;
    InfoKind infoKind(void) const;
    virtual QString name(void) const;
    virtual QString description(void) const;
    QVariant data(const QString &key) const;
    const SPEInfoItem *parentItem(void) const;
    void setParentItem(const SPEInfoItem *parentItem);

    bool isAncestorOf(const SPEInfoItem *ancestor) const;

    static const QString keyType;
    static const QString valueFile;
    static const QString valuePath;
    static const QString keyIncludedByDefault;
    static const QString keyImageFileName;

protected:
    QHash<QString, QVariant> m_data;

private:
    QString m_id;
    InfoKind m_infoKind;
    QPixmap m_image;
    const class SPEInfoItem *m_parentItem;
};

class SPEInfo
{
public:
    ~SPEInfo();

    static const QList<SPEInfoItem*> *list(SPEInfoItem::InfoKind kind);
    static const SPEInfoItem *defaultInfoOfKind(SPEInfoItem::InfoKind kind);
    static const SPEInfoItem *platformInfoForId(const QString &id);
    static const SPEInfoItem *configurationInfoForId(const QString &id);
    static const SPEInfoItem *infoOfKindForId(SPEInfoItem::InfoKind kind,
        const QString &id, const SPEInfoItem *defaultInfoItem = NULL);

private:
    static void addListToHash(const QList<SPEInfoItem*> &list);
    static void initializeLists(void);
    static void deleteLists(void);

    static QList<SPEInfoItem*> m_configurationList;
    static QList<SPEInfoItem*> m_platformList;
    static QList<SPEInfoItem*> m_variableList;
    static QList<SPEInfoItem*> m_qtmoduleList;
    static QList<SPEInfoItem*> m_templateList;
    static QList<SPEInfoItem*> m_operatorList;

    static QHash<QPair<SPEInfoItem::InfoKind, QString> ,SPEInfoItem* > m_itemHash;

    static bool m_listsInitialized;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // SIMPLEPROEDITORINFO_H
