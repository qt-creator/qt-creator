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

#pragma once

#include "projectexplorer_export.h"

#include "kit.h"

#include <coreplugin/id.h>
#include <coreplugin/featureprovider.h>

#include <QObject>
#include <QPair>

#include <functional>

namespace Utils {
class Environment;
class FileName;
class MacroExpander;
}

namespace ProjectExplorer {
class Task;
class IOutputParser;
class KitConfigWidget;
class KitManager;

namespace Internal {
class KitManagerConfigWidget;
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

    Core::Id id() const { return m_id; }
    int priority() const { return m_priority; }

    virtual QVariant defaultValue(const Kit *) const = 0;

    // called to find issues with the kit
    virtual QList<Task> validate(const Kit *) const = 0;
    // called after restoring a kit, so upgrading of kit information settings can be done
    virtual void upgrade(Kit *) { return; }
    // called to fix issues with this kitinformation. Does not modify the rest of the kit.
    virtual void fix(Kit *) { return; }
    // called on initial setup of a kit.
    virtual void setup(Kit *) { return; }

    virtual ItemList toUserOutput(const Kit *) const = 0;

    virtual KitConfigWidget *createConfigWidget(Kit *) const = 0;

    virtual void addToEnvironment(const Kit *k, Utils::Environment &env) const;
    virtual IOutputParser *createOutputParser(const Kit *k) const;

    virtual QString displayNamePostfix(const Kit *k) const;

    virtual QSet<Core::Id> supportedPlatforms(const Kit *k) const;
    virtual QSet<Core::Id> availableFeatures(const Kit *k) const;

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

protected:
    void setId(Core::Id id) { m_id = id; }
    void setPriority(int priority) { m_priority = priority; }
    void notifyAboutUpdate(Kit *k);

private:
    Core::Id m_id;
    int m_priority = 0; // The higher the closer to the top.
};

class PROJECTEXPLORER_EXPORT KitManager : public QObject
{
    Q_OBJECT

public:
    static KitManager *instance();
    ~KitManager() override;

    static QList<Kit *> kits(const Kit::Predicate &predicate = Kit::Predicate());
    static Kit *kit(const Kit::Predicate &predicate);
    static Kit *kit(Core::Id id);
    static Kit *defaultKit();

    static QList<KitInformation *> kitInformation();

    static Internal::KitManagerConfigWidget *createConfigWidget(Kit *k);

    static void deleteKit(Kit *k);

    static bool registerKit(Kit *k);
    static void deregisterKit(Kit *k);
    static void setDefaultKit(Kit *k);

    static void registerKitInformation(KitInformation *ki);
    static void deregisterKitInformation(KitInformation *ki);

    static QSet<Core::Id> supportedPlatforms();
    static QSet<Core::Id> availableFeatures(Core::Id platformId);

    static QList<Kit *> sortKits(const QList<Kit *> kits); // Avoid sorting whenever possible!

    static void saveKits();

    static bool isLoaded();

signals:
    void kitAdded(ProjectExplorer::Kit *);
    // Kit is still valid when this call happens!
    void kitRemoved(ProjectExplorer::Kit *);
    // Kit was updated.
    void kitUpdated(ProjectExplorer::Kit *);
    void unmanagedKitUpdated(ProjectExplorer::Kit *);
    // Default kit was changed.
    void defaultkitChanged();
    // Something changed.
    void kitsChanged();

    void kitsLoaded();

private:
    explicit KitManager(QObject *parent = nullptr);

    // Make sure the this is only called after all
    // KitInformation are registered!
    void restoreKits();
    class KitList
    {
    public:
        KitList() { }
        Core::Id defaultKit;
        QList<Kit *> kits;
    };
    KitList restoreKits(const Utils::FileName &fileName);

    static void notifyAboutUpdate(Kit *k);
    void addKit(Kit *k);

    friend class ProjectExplorerPlugin; // for constructor
    friend class Kit;
    friend class Internal::KitModel;
    friend class KitInformation; // for notifyAbutUpdate
};

} // namespace ProjectExplorer
