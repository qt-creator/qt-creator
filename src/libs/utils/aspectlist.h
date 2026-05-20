// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aspects.h"
#include "covariantcallback.h"

namespace Utils {

namespace Internal { class AspectListPrivate; }

class QTCREATOR_UTILS_EXPORT AspectList : public Utils::BaseAspect
{
    Q_OBJECT
    friend class Internal::AspectListPrivate;
public:
    using CreateItem = std::function<std::shared_ptr<BaseAspect>()>;

    AspectList(Utils::AspectContainer *container = nullptr);
    ~AspectList() override;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    void volatileToMap(Utils::Store &map) const override;
    QVariantList toList(bool v) const;

    QList<std::shared_ptr<BaseAspect>> items() const;
    QList<std::shared_ptr<BaseAspect>> volatileItems() const;

    std::shared_ptr<BaseAspect> createAndAddItem();
    std::shared_ptr<BaseAspect> addItem(const std::shared_ptr<BaseAspect> &item);
    std::shared_ptr<BaseAspect> actualAddItem(const std::shared_ptr<BaseAspect> &item);

    void removeItem(const std::shared_ptr<BaseAspect> &item);
    void actualRemoveItem(const std::shared_ptr<BaseAspect> &item);
    void clear();

    void apply() override;
    void cancel() override;
    void setAutoApply(bool on) override;

    void setCreateItemFunction(CreateItem createItem);

    void forEachItem(const CovariantCallback<void(std::shared_ptr<BaseAspect>)> &callback) const
    {
        for (const auto &item : volatileItems())
            callback(item);
    }

    void forEachItem(const CovariantCallback<void(std::shared_ptr<BaseAspect>, int)> &callback) const
    {
        int idx = 0;
        for (const auto &item : volatileItems())
            callback(item, idx++);
    }

    qsizetype size() const;
    bool isDirty() const override;

    QVariant variantValue() const override { return toList(false); }
    void setVariantValue(const QVariant &value, Announcement howToAnnounce = DoEmit) override;
    QVariant volatileVariantValue() const override { return {}; } // ??

    enum class DisplayStyle { InlineList, ListViewWithDetails };
    void setDisplayStyle(DisplayStyle displayStyle);

    void addExtraButton(const QString &text, std::function<void()> callback);

    CovariantCallback<QVariant(BaseAspect *, int)> listViewDataCallback;

    CovariantCallback<void(std::shared_ptr<BaseAspect>)> itemAddedCallback;
    CovariantCallback<void(std::shared_ptr<BaseAspect>)> itemRemovedCallback;

    void addToLayoutImpl(Layouting::Layout &parent) override;

signals:
    void volatileItemListChanged();

private:
    std::unique_ptr<Internal::AspectListPrivate> d;
};

} // namespace Utils
