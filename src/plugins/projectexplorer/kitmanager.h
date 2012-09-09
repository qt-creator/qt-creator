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

#ifndef KITMANAGER_H
#define KITMANAGER_H

#include "projectexplorer_export.h"

#include "task.h"

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QPair>

namespace Utils { class Environment; }

namespace ProjectExplorer {
class Kit;
class KitConfigWidget;

namespace Internal {
class KitManagerPrivate;
class KitModel;
} // namespace Internal

/**
 * @brief The KitInformation class
 *
 * One piece of information stored in the kit.
 *
 * This needs to get registered with the \a KitManager.
 */
class PROJECTEXPLORER_EXPORT KitInformation : public QObject
{
    Q_OBJECT

public:
    typedef QPair<QString, QString> Item;
    typedef QList<Item> ItemList;

    virtual Core::Id dataId() const = 0;

    virtual unsigned int priority() const = 0; // the higher the closer to the top.

    virtual bool visibleIn(Kit *) { return true; }
    virtual QVariant defaultValue(Kit *) const = 0;

    virtual QList<Task> validate(Kit *) const = 0;

    virtual ItemList toUserOutput(Kit *) const = 0;

    virtual KitConfigWidget *createConfigWidget(Kit *) const = 0;

    virtual void addToEnvironment(const Kit *k, Utils::Environment &env) const;

    virtual QString displayNamePostfix(const Kit *k) const;

signals:
    void validationNeeded();
};

class PROJECTEXPLORER_EXPORT KitMatcher
{
public:
    virtual ~KitMatcher() { }
    virtual bool matches(const Kit *k) const = 0;
};

class PROJECTEXPLORER_EXPORT KitManager : public QObject
{
    Q_OBJECT

public:
    static KitManager *instance();
    ~KitManager();

    QList<Kit *> kits(const KitMatcher *m = 0) const;
    Kit *find(const Core::Id &id) const;
    Kit *find(const KitMatcher *m) const;
    Kit *defaultKit();

    QList<KitInformation *> kitInformation() const;

    KitConfigWidget *createConfigWidget(Kit *k) const;

public slots:
    bool registerKit(ProjectExplorer::Kit *k);
    void deregisterKit(ProjectExplorer::Kit *k);
    QList<Task> validateKit(ProjectExplorer::Kit *k);
    void setDefaultKit(ProjectExplorer::Kit *k);

    void saveKits();

    void registerKitInformation(ProjectExplorer::KitInformation *ki);
    void deregisterKitInformation(ProjectExplorer::KitInformation *ki);

signals:
    void kitAdded(ProjectExplorer::Kit *);
    // Kit is still valid when this call happens!
    void kitRemoved(ProjectExplorer::Kit *);
    // Kit was updated.
    void kitUpdated(ProjectExplorer::Kit *);
    // Default kit was changed.
    void defaultkitChanged();
    // Something changed.
    void kitsChanged();

private slots:
    void validateKits();

private:
    explicit KitManager(QObject *parent = 0);

    // Make sure the this is only called after all
    // KitInformation are registered!
    void restoreKits();
    class KitList
    {
    public:
        KitList()
        { }
        Core::Id defaultKit;
        QList<Kit *> kits;
    };
    KitList restoreKits(const Utils::FileName &fileName);

    void notifyAboutUpdate(ProjectExplorer::Kit *k);
    void addKit(Kit *k);

    Internal::KitManagerPrivate *const d;

    static KitManager *m_instance;

    friend class Internal::KitManagerPrivate; // for the restoreToolChains methods
    friend class ProjectExplorerPlugin; // for constructor
    friend class Kit;
    friend class Internal::KitModel;
};

} // namespace ProjectExplorer

#endif // KITMANAGER_H
