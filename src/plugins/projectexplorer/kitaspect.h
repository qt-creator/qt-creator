// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "task.h"

#include <utils/aspects.h>
#include <utils/guard.h>

#include <QPair>
#include <QPushButton>
#include <QSet>

#include <functional>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
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

    virtual void onKitsLoaded() {}

    static void handleKitsLoaded();
    static const QList<KitAspectFactory *> kitAspectFactories();

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
    enum ItemRole { IdRole = Qt::UserRole + 100, IsNoneRole, QualityRole };

    KitAspect(Kit *kit, const KitAspectFactory *factory);
    ~KitAspect();

    virtual void refresh();

    void addToLayoutImpl(Layouting::Layout &layout) override;
    static QString msgManage();

    Kit *kit() const { return m_kit; }
    const KitAspectFactory *factory() const { return m_factory; }
    QAction *mutableAction() const { return m_mutableAction; }
    void addMutableAction(QWidget *child);
    void setManagingPage(Utils::Id pageId) { m_managingPageId = pageId; }

    void makeStickySubWidgetsReadOnly();

protected:
    virtual void makeReadOnly();
    virtual void addToInnerLayout(Layouting::Layout &parentItem);
    virtual Utils::Id settingsPageItemToPreselect() const { return {}; }

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
    void setListAspectSpec(ListAspectSpec &&listAspectSpec);

private:
    Kit *m_kit;
    const KitAspectFactory *m_factory;
    QAction *m_mutableAction = nullptr;
    Utils::Id m_managingPageId;
    QPushButton *m_manageButton = nullptr;
    QComboBox *m_comboBox = nullptr;
    std::optional<ListAspectSpec> m_listAspectSpec;
    Utils::Guard m_ignoreChanges;
};

} // namespace ProjectExplorer
