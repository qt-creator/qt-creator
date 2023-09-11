// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "kit.h"

#include <coreplugin/featureprovider.h>

#include <utils/aspects.h>

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
class KitAspect;
class KitManager;

namespace Internal {
class KitManagerConfigWidget;
} // namespace Internal

/**
 * @brief The KitAspectFactory class
 *
 * A KitAspectFactory can create instances of one type of KitAspect.
 * A KitAspect handles a specific piece of information stored in the kit.
 *
 * They auto-register with the \a KitManager for their life time
 */
class PROJECTEXPLORER_EXPORT KitAspectFactory : public QObject
{
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

    virtual KitAspect *createKitAspect(Kit *) const = 0;

    virtual void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const;
    virtual void addToRunEnvironment(const Kit *k, Utils::Environment &env) const;

    virtual QList<Utils::OutputLineParser *> createOutputParsers(const Kit *k) const;

    virtual QString displayNamePostfix(const Kit *k) const;

    virtual QSet<Utils::Id> supportedPlatforms(const Kit *k) const;
    virtual QSet<Utils::Id> availableFeatures(const Kit *k) const;

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

    virtual bool isApplicableToKit(const Kit *) const { return true; }

    virtual void onKitsLoaded() {}

protected:
    KitAspectFactory();
    ~KitAspectFactory();

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

class PROJECTEXPLORER_EXPORT KitAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    KitAspect(Kit *kit, const KitAspectFactory *factory);
    ~KitAspect();

    virtual void makeReadOnly() = 0;
    virtual void refresh() = 0;

    void addToLayout(Layouting::LayoutItem &parentItem) override;

    static QString msgManage();

    Kit *kit() const { return m_kit; }
    const KitAspectFactory *factory() const { return m_factory; }
    QAction *mutableAction() const { return m_mutableAction; }
    void addMutableAction(QWidget *child);
    QWidget *createManageButton(Utils::Id pageId);

protected:
    virtual void addToLayoutImpl(Layouting::LayoutItem &parentItem) = 0;

    Kit *m_kit;
    const KitAspectFactory *m_factory;
    QAction *m_mutableAction = nullptr;
};

class PROJECTEXPLORER_EXPORT KitManager final : public QObject
{
    Q_OBJECT

public:
    static KitManager *instance();

    static const QList<Kit *> kits();
    static const QList<Kit *> sortedKits(); // Avoid sorting whenever possible!
    static Kit *kit(const Kit::Predicate &predicate);
    static Kit *kit(Utils::Id id);
    static Kit *defaultKit();

    static const QList<KitAspectFactory *> kitAspectFactories();
    static const QSet<Utils::Id> irrelevantAspects();
    static void setIrrelevantAspects(const QSet<Utils::Id> &aspects);

    static Kit *registerKit(const std::function<void(Kit *)> &init, Utils::Id id = {});
    static void deregisterKit(Kit *k);
    static void setDefaultKit(Kit *k);

    static void saveKits();

    static bool isLoaded();
    static bool waitForLoaded(const int timeout = 60 * 1000); // timeout in ms
    static void showLoadingProgress();

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
    ~KitManager() override;

    static void destroy();

    static void setBinaryForKit(const Utils::FilePath &binary);

    // Make sure the this is only called after all
    // KitAspects are registered!
    static void restoreKits();

    static void notifyAboutUpdate(Kit *k);
    static void completeKit(Kit *k);

    friend class ProjectExplorerPlugin; // for constructor
    friend class Kit;
    friend class Internal::KitManagerConfigWidget;
};

} // namespace ProjectExplorer
