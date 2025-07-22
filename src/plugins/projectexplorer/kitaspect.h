// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "devicesupport/idevicefwd.h"
#include "projectexplorer_export.h"
#include "task.h"

#include <tasking/tasktree.h>

#include <utils/aspects.h>

#include <QPair>
#include <QSet>

#include <functional>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
QT_END_NAMESPACE

namespace Utils {
class Environment;
class MacroExpander;
class OutputLineParser;
} // namespace Utils

namespace ProjectExplorer {
class Kit;
class KitAspect;

using LogCallback = std::function<void(const QString &message)>;

PROJECTEXPLORER_EXPORT Tasking::Group kitDetectionRecipe(
    const IDeviceConstPtr &device, const LogCallback &logCallback);

PROJECTEXPLORER_EXPORT Tasking::Group removeDetectedKitsRecipe(
    const IDeviceConstPtr &device, const LogCallback &logCallback);

PROJECTEXPLORER_EXPORT void listAutoDetected(
    const IDeviceConstPtr &device, const LogCallback &logCallback);

class DetectionSource
{
public:
    enum DetectionType {
        Manual,
        FromSystem,
        FromSdk,
        Temporary,
        Uninitialized,
    };

    DetectionSource() = default;
    DetectionSource(DetectionType type, const QString &id = {})
        : type(type), id(id)
    {}

    bool isAutoDetected() const
    {
        return type == FromSystem || type == FromSdk || type == Temporary;
    }

    bool isTemporary() const
    {
        return type == Temporary;
    }

    bool isSdkProvided() const
    {
        return type == FromSdk;
    }

    bool isSystemDetected() const
    {
        return type == FromSystem;
    }

    bool operator==(const DetectionSource &other) const
    {
        return type == other.type && id == other.id;
    }

    DetectionType type = Uninitialized;
    QString id;

private:
    PROJECTEXPLORER_EXPORT friend QDebug operator<<(QDebug dbg, const DetectionSource &source);
};

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

    virtual QString moduleForHeader(const Kit *k, const QString &className) const;

    virtual ItemList toUserOutput(const Kit *) const = 0;

    virtual KitAspect *createKitAspect(Kit *) const = 0;

    virtual void addToBuildEnvironment(const Kit *k, Utils::Environment &env) const;
    virtual void addToRunEnvironment(const Kit *k, Utils::Environment &env) const;

    virtual QList<Utils::OutputLineParser *> createOutputParsers(const Kit *k) const;

    virtual QString displayNamePostfix(const Kit *k) const;

    virtual QSet<Utils::Id> supportedPlatforms(const Kit *k) const;
    virtual QSet<Utils::Id> availableFeatures(const Kit *k) const;

    QList<Utils::Id> embeddableAspects() const { return m_embeddableAspects; }

    virtual void addToMacroExpander(ProjectExplorer::Kit *kit, Utils::MacroExpander *expander) const;

    virtual void onKitsLoaded() {}

    static void handleKitsLoaded();
    static const QList<KitAspectFactory *> kitAspectFactories();

    virtual std::optional<Tasking::ExecutableItem> autoDetect(
        Kit *kit,
        const Utils::FilePaths &searchPaths,
        const QString &detectionSource,
        const LogCallback &logCallback) const;

    virtual std::optional<Tasking::ExecutableItem> removeAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const;

    virtual void listAutoDetected(
        const QString &detectionSource, const LogCallback &logCallback) const;

    virtual Utils::Result<Tasking::ExecutableItem> createAspectFromJson(
        const QString &detectionSource,
        const Utils::FilePath &rootPath,
        Kit *kit,
        const QJsonValue &json,
        const LogCallback &logCallback) const;

protected:
    KitAspectFactory();
    ~KitAspectFactory();

    void setId(Utils::Id id) { m_id = id; }
    void setDisplayName(const QString &name) { m_displayName = name; }
    void setDescription(const QString &desc) { m_description = desc; }
    void makeEssential() { m_essential = true; }
    void setPriority(int priority) { m_priority = priority; }
    void setEmbeddableAspects(const QList<Utils::Id> &aspects) { m_embeddableAspects = aspects; }
    void notifyAboutUpdate(Kit *k);

private:
    QString m_displayName;
    QString m_description;
    Utils::Id m_id;
    QList<Utils::Id> m_embeddableAspects;
    int m_priority = 0; // The higher the closer to the top.
    bool m_essential = false;
};

class PROJECTEXPLORER_EXPORT KitAspect : public Utils::BaseAspect
{
    Q_OBJECT

public:
    enum ItemRole { IdRole = Qt::UserRole + 100, IsNoneRole, TypeRole, QualityRole };

    KitAspect(Kit *kit, const KitAspectFactory *factory);
    ~KitAspect();

    virtual void refresh();

    void addToLayoutImpl(Layouting::Layout &layout) override;
    static QString msgManage();

    Kit *kit() const;
    const KitAspectFactory *factory() const;
    QAction *mutableAction() const;
    void addMutableAction(QWidget *child);
    void setManagingPage(Utils::Id pageId);

    void setAspectsToEmbed(const QList<KitAspect *> &aspects);
    QList<KitAspect *> aspectsToEmbed() const;

    void makeStickySubWidgetsReadOnly();

    // For layouting purposes only.
    QList<QComboBox *> comboBoxes() const;

    virtual void addToInnerLayout(Layouting::Layout &layout);

protected:
    virtual void makeReadOnly();
    virtual Utils::Id settingsPageItemToPreselect() const { return {}; }

    void addLabelToLayout(Layouting::Layout &layout);
    void addListAspectsToLayout(Layouting::Layout &layout);
    void addManageButtonToLayout(Layouting::Layout &layout);

    // Convenience for aspects that provide a list model from which one value can be chosen.
    // It will be exposed via a QComboBox.
    class ListAspectSpec
    {
    public:
        using Getter = std::function<QVariant(const Kit &)>;
        using Setter = std::function<void(Kit &, const QVariant &)>;
        using ResetModel = std::function<void()>;

        ListAspectSpec(
            QAbstractItemModel *model,
            Getter &&getter,
            Setter &&setter,
            ResetModel &&resetModel)
            : model(model)
            , getter(std::move(getter))
            , setter(std::move(setter))
            , resetModel(std::move(resetModel))
        {}

        QAbstractItemModel *model;
        Getter getter;
        Setter setter;
        ResetModel resetModel;
    };
    void addListAspectSpec(const ListAspectSpec &listAspectSpec);

private:
    class Private;
    Private * const d;
};

} // namespace ProjectExplorer
