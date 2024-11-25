// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "kit.h"

#include <QSet>

#include <functional>

namespace Utils { class FilePath; }

namespace ProjectExplorer {
class KitAspectFactory;

namespace Internal {
class KitManagerConfigWidget;
} // namespace Internal

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
    static void deregisterKits(const QList<Kit *> kits);
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
