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

#ifndef KITMANAGER_H
#define KITMANAGER_H

#include "projectexplorer_export.h"

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
class Kit;
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

    virtual QVariant defaultValue(Kit *) const = 0;

    // called to find issues with the kit
    virtual QList<Task> validate(const Kit *) const = 0;
    // called to fix issues with this kitinformation. Does not modify the rest of the kit.
    virtual void fix(Kit *) { return; }
    // called on initial setup of a kit.
    virtual void setup(Kit *) { return; }

    virtual ItemList toUserOutput(const Kit *) const = 0;

    virtual KitConfigWidget *createConfigWidget(Kit *) const = 0;

    virtual void addToEnvironment(const Kit *k, Utils::Environment &env) const;
    virtual IOutputParser *createOutputParser(const Kit *k) const;

    virtual QString displayNamePostfix(const Kit *k) const;

    virtual QSet<QString> availablePlatforms(const Kit *k) const;
    virtual QString displayNameForPlatform(const Kit *k, const QString &platform) const;
    virtual Core::FeatureSet availableFeatures(const Kit *k) const;

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

protected:
    void setId(Core::Id id) { m_id = id; }
    void setPriority(int priority) { m_priority = priority; }
    void notifyAboutUpdate(Kit *k);

private:
    Core::Id m_id;
    int m_priority; // The higher the closer to the top.
};

class PROJECTEXPLORER_EXPORT KitMatcher
{
public:
    typedef std::function<bool(const Kit *)> Matcher;
    KitMatcher(const Matcher &m) : m_matcher(m) {}
    KitMatcher() {}

    bool isValid() const { return !!m_matcher; }
    bool matches(const Kit *kit) const { return m_matcher(kit); }

private:
    Matcher m_matcher;
};

class PROJECTEXPLORER_EXPORT KitManager : public QObject
{
    Q_OBJECT

public:
    static KitManager *instance();
    ~KitManager();

    static QList<Kit *> kits();
    static QList<Kit *> matchingKits(const KitMatcher &matcher);
    static Kit *find(Core::Id id);
    static Kit *find(const KitMatcher &matcher);
    static Kit *defaultKit();

    static QList<KitInformation *> kitInformation();

    static Internal::KitManagerConfigWidget *createConfigWidget(Kit *k);

    static void deleteKit(Kit *k);

    static bool registerKit(Kit *k);
    static void deregisterKit(Kit *k);
    static void setDefaultKit(Kit *k);

    static void registerKitInformation(KitInformation *ki);
    static void deregisterKitInformation(KitInformation *ki);

    static QSet<QString> availablePlatforms();
    static QString displayNameForPlatform(const QString &platform);
    static Core::FeatureSet availableFeatures(const QString &platform);

    static QList<Kit *> sortKits(const QList<Kit *> kits); // Avoid sorting whenever possible!

public slots:
    void saveKits();

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

    static void notifyAboutUpdate(Kit *k);
    void addKit(Kit *k);

    friend class ProjectExplorerPlugin; // for constructor
    friend class Kit;
    friend class Internal::KitModel;
    friend class KitInformation; // for notifyAbutUpdate
};

} // namespace ProjectExplorer

#endif // KITMANAGER_H
