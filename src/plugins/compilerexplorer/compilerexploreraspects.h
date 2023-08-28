// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QComboBox>
#include <QItemSelectionModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardItemModel>

namespace CompilerExplorer {

class StringSelectionAspect : public Utils::TypedAspect<QString>
{
    Q_OBJECT
public:
    StringSelectionAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    using ResultCallback = std::function<void(QList<QStandardItem *> items)>;
    using FillCallback = std::function<void(ResultCallback)>;
    void setFillCallback(FillCallback callback) { m_fillCallback = callback; }

    void refill() { emit refillRequested(); }

    void bufferToGui() override;
    bool guiToBuffer() override;

signals:
    void refillRequested();

private:
    FillCallback m_fillCallback;
    QStandardItemModel *m_model{nullptr};
    QItemSelectionModel *m_selectionModel{nullptr};
};

// QMap<Library.Id, Library.Version.Id>
class LibrarySelectionAspect : public Utils::TypedAspect<QMap<QString, QString>>
{
    Q_OBJECT
public:
    enum Roles {
        LibraryData = Qt::UserRole + 1,
        SelectedVersion,
    };

    LibrarySelectionAspect(Utils::AspectContainer *container = nullptr);

    void addToLayout(Layouting::LayoutItem &parent) override;

    using ResultCallback = std::function<void(QList<QStandardItem *>)>;
    using FillCallback = std::function<void(ResultCallback)>;
    void setFillCallback(FillCallback callback) { m_fillCallback = callback; }
    void refill() { emit refillRequested(); }

    void bufferToGui() override;
    bool guiToBuffer() override;

signals:
    void refillRequested();

private:
    FillCallback m_fillCallback;
    QStandardItemModel *m_model{nullptr};
};

template<class T>
class AspectListAspect : public Utils::BaseAspect
{
public:
    using ToBaseAspectPtr = std::function<Utils::BaseAspect *(const T &)>;
    using CreateItem = std::function<T()>;
    using ItemCallback = std::function<void(const T &)>;
    using IsDirty = std::function<bool(const T &)>;
    using Apply = std::function<void(const T &)>;

    AspectListAspect(Utils::AspectContainer *container = nullptr)
        : Utils::BaseAspect(container)
    {}

    void fromMap(const Utils::Store &map) override
    {
        QTC_ASSERT(!settingsKey().isEmpty(), return);

        QVariantList list = map[settingsKey()].toList();
        for (const QVariant &entry : list) {
            T item = m_createItem();
            m_toBaseAspect(item)->fromMap(Utils::storeFromVariant(entry));
            m_volatileItems.append(item);
        }
        m_items = m_volatileItems;
    }

    QVariantList toList(bool v) const
    {
        QVariantList list;
        const auto &items = v ? m_volatileItems : m_items;

        for (const auto &item : items) {
            Utils::Store childMap;
            if (v)
                m_toBaseAspect(item)->volatileToMap(childMap);
            else
                m_toBaseAspect(item)->toMap(childMap);

            list.append(Utils::variantFromStore(childMap));
        }

        return list;
    }

    void toMap(Utils::Store &map) const override
    {
        QTC_ASSERT(!settingsKey().isEmpty(), return);
        const Utils::Key key = settingsKey();
        map[key] = toList(false);
    }

    void volatileToMap(Utils::Store &map) const override
    {
        QTC_ASSERT(!settingsKey().isEmpty(), return);
        const Utils::Key key = settingsKey();
        map[key] = toList(true);
    }

    T addItem(T item)
    {
        m_volatileItems.append(item);
        if (m_itemAdded)
            m_itemAdded(item);
        emit volatileValueChanged();
        if (isAutoApply())
            apply();
        return item;
    }

    void removeItem(T item)
    {
        m_volatileItems.removeOne(item);
        if (m_itemRemoved)
            m_itemRemoved(item);
        emit volatileValueChanged();
        if (isAutoApply())
            apply();
    }

    void apply() override
    {
        m_items = m_volatileItems;
        if (m_apply)
            forEachItem(m_apply);
        emit changed();
    }

    void setToBaseAspectFunction(ToBaseAspectPtr toBaseAspect) { m_toBaseAspect = toBaseAspect; }
    void setCreateItemFunction(CreateItem createItem) { m_createItem = createItem; }
    void setIsDirtyFunction(IsDirty isDirty) { m_isDirty = isDirty; }
    void setApplyFunction(Apply apply) { m_apply = apply; }

    void forEachItem(std::function<void(const T &)> callback)
    {
        for (const auto &item : m_volatileItems)
            callback(item);
    }

    void forEachItem(std::function<void(const T &, int)> callback)
    {
        int idx = 0;
        for (const auto &item : m_volatileItems)
            callback(item, idx++);
    }

    void setItemAddedCallback(const ItemCallback &callback) { m_itemAdded = callback; }
    void setItemRemovedCallback(const ItemCallback &callback) { m_itemRemoved = callback; }

    qsizetype size() { return m_volatileItems.size(); }
    bool isDirty() override
    {
        if (m_isDirty) {
            for (const auto &item : m_volatileItems) {
                if (m_isDirty(item))
                    return true;
            }
        }
        return false;
    }

    QVariant volatileVariantValue() const override { return {}; }

private:
    QList<T> m_items;
    QList<T> m_volatileItems;
    ToBaseAspectPtr m_toBaseAspect;
    CreateItem m_createItem;
    IsDirty m_isDirty;
    Apply m_apply;
    ItemCallback m_itemAdded;
    ItemCallback m_itemRemoved;
};

} // namespace CompilerExplorer
