/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 BogDan Vatra <bog_dan_ro@yahoo.com>
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QT4ANDROIDTARGETFACTORY_H
#define QT4ANDROIDTARGETFACTORY_H

#include "qt4projectmanager/qt4basetargetfactory.h"
#include "qt4projectmanager/qt4target.h"

namespace Android {
namespace Internal {

class AndroidTargetFactory : public Qt4ProjectManager::Qt4BaseTargetFactory
{
    Q_OBJECT
public:
    AndroidTargetFactory(QObject *parent = 0);
    ~AndroidTargetFactory();

    QStringList supportedTargetIds() const;
    QString displayNameForId(const QString &id) const;
    QIcon iconForId(const QString &id) const;

    bool canCreate(ProjectExplorer::Project *parent, const QString &id) const;
    bool canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const;
    Qt4ProjectManager::Qt4BaseTarget *restore(ProjectExplorer::Project *parent, const QVariantMap &map);

    bool supportsTargetId(const QString &id) const;
    virtual QSet<QString> targetFeatures(const QString &id) const;

    Qt4ProjectManager::Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id);
    Qt4ProjectManager::Qt4BaseTarget *create(ProjectExplorer::Project *parent, const QString &id,
                                             const QList<Qt4ProjectManager::BuildConfigurationInfo> &infos);

    QString buildNameForId(const QString &id) const;
};

} // namespace Internal
} // namespace Android

#endif // QT4ANDROIDTARGETFACTORY_H
