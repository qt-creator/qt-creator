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

#include <coreplugin/featureprovider.h>

#include <QObject>
#include <QPair>
#include <QSet>

#include <functional>

namespace Utils {
class Environment;
class FilePath;
class MacroExpander;
class OutputLineParser;
} // namespace Utils

namespace ProjectExplorer {
class Task;
class KitAspectWidget;
class KitManager;

namespace Internal {
class KitManagerConfigWidget;
} // namespace Internal

/**
 * @brief The KitAspect class
 *
 * One piece of information stored in the kit.
 *
 * They auto-register with the \a KitManager for their life time
 */
class PROJECTEXPLORER_EXPORT KitAspect : public QObject
{
    Q_OBJECT

public:
    using Item = QPair<QString, QString>;
    using ItemList = QList<Item>;

    Utils::Id id() const { return m_id; }
    int priority() const { return m_priority; }
    QString displayName() const { return m_displayName; }
    QString description() const { return m_description; }
    bool isEssential() const { return m_essential; }

    // called to find issues with the kit
    virtual Tasks validate(const Kit *) const = 0;
    // called after restoring a kit, so upgrading of kit information settings can be done
    virtual void upgrade(Kit *) { return; }
    // called to fix issues with this kitinformation. Does not modify the rest of the kit.
    virtual void fix(Kit *) { return; }
    // called on initial setup of a kit.
    virtual void setup(Kit *) { return; }

    virtual int weight(const Kit *k) const;

    virtual ItemList toUserOutput(const Kit *) const = 0;

    virtual KitAspectWidget *createConfigWidget(Kit *) const = 0;

    virtual void addToEnvironment(const Kit *k, Utils::Environment &env) const;
    virtual QList<Utils::OutputLineParser *> createOutputParsers(const Kit *k) const;

    virtual QString displayNamePostfix(const Kit *k) const;

    virtual QSet<Utils::Id> supportedPlatforms(const Kit *k) const;
    virtual QSet<Utils::Id> availableFeatures(const Kit *k) const;

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

    virtual bool isApplicableToKit(const Kit *) const { return true; }

protected:
    KitAspect();
    ~KitAspect();

    void setId(Utils::Id id) { m_id = id; }
    void setDisplayName(const QString &name) { m_displayName = name; }
    void setDescription(const QString &desc) { m_description = desc; }
    void makeEssential() { m_essential = true; }
    void setPriority(int priority) { m_priority = priority; }
    void notifyAboutUpdate(Kit *k);

private:
    QString m_displayName;
    QString m_description;
    Utils::Id m_id;
    int m_priority = 0; // The higher the closer to the top.
    bool m_essential = false;
};

class PROJECTEXPLORER_EXPORT KitAspectWidget : public QObject
{
    Q_OBJECT

public:
    KitAspectWidget(Kit *kit, const KitAspect *ki);

    Utils::Id kitInformationId() const;

    virtual void makeReadOnly() = 0;
    virtual void refresh() = 0;
    bool visibleInKit() { return m_kitInformation->isApplicableToKit(m_kit); }

    virtual QWidget *mainWidget() const = 0;
    virtual QWidget *buttonWidget() const { return nullptr; }

    bool isSticky() const { return m_isSticky; }

    static QString msgManage();

    Kit *kit() const { return m_kit; }

signals:
    void dirty();

protected:
    Kit *m_kit;
    const KitAspect *m_kitInformation;
    bool m_isSticky;
};

class PROJECTEXPLORER_EXPORT KitManager : public QObject
{
    Q_OBJECT

public:
    static KitManager *instance();
    ~KitManager() override;

    static const QList<Kit *> kits();
    static Kit *kit(const Kit::Predicate &predicate);
    static Kit *kit(Utils::Id id);
    static Kit *defaultKit();

    static const QList<KitAspect *> kitAspects();
    static const QSet<Utils::Id> irrelevantAspects();
    static void setIrrelevantAspects(const QSet<Utils::Id> &aspects);

    static Kit *registerKit(const std::function<void(Kit *)> &init, Utils::Id id = {});
    static void deregisterKit(Kit *k);
    static void setDefaultKit(Kit *k);

    static QList<Kit *> sortKits(const QList<Kit *> &kits); // Avoid sorting whenever possible!

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
    KitManager();

    static void destroy();

    static void registerKitAspect(KitAspect *ki);
    static void deregisterKitAspect(KitAspect *ki);

    static void setBinaryForKit(const Utils::FilePath &binary);

    // Make sure the this is only called after all
    // KitAspects are registered!
    static void restoreKits();

    static void notifyAboutUpdate(Kit *k);
    static void completeKit(Kit *k);

    friend class ProjectExplorerPlugin; // for constructor
    friend class Kit;
    friend class Internal::KitManagerConfigWidget;
    friend class KitAspect; // for notifyAboutUpdate and self-registration
};

} // namespace ProjectExplorer
