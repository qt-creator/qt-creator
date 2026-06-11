// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aspectlist.h"

#include "algorithm.h"
#include "itemviews.h"
#include "layoutbuilder.h"
#include "qtcassert.h"
#include "qtdesignwidgets.h"
#include "treemodel.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QBoxLayout>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QListView>
#include <QPainter>
#include <QPaintEvent>
#include <QPushButton>
#include <QSizePolicy>
#include <QUndoStack>

using namespace Layouting;

namespace Utils {

class AddItemCommand : public QUndoCommand
{
public:
    AddItemCommand(AspectList *aspect, const std::shared_ptr<BaseAspect> &item)
        : m_aspect(aspect)
        , m_item(item)
    {}

    void undo() override { m_aspect->actualRemoveItem(m_item); }
    void redo() override { m_aspect->actualAddItem(m_item); }

private:
    AspectList *m_aspect;
    std::shared_ptr<BaseAspect> m_item;
};

class RemoveItemCommand : public QUndoCommand
{
public:
    RemoveItemCommand(AspectList *aspect, const std::shared_ptr<BaseAspect> &item)
        : m_aspect(aspect)
        , m_item(item)
    {}

    void undo() override { m_aspect->actualAddItem(m_item); }
    void redo() override { m_aspect->actualRemoveItem(m_item); }

private:
    AspectList *m_aspect;
    std::shared_ptr<BaseAspect> m_item;
};

class AspectListModelItem : public TypedTreeItem<AspectListModelItem>
{
    std::shared_ptr<BaseAspect> m_aspect;
    QObject guard;
    std::function<QVariant(BaseAspect *, int role)> m_dataFunction;

public:
    AspectListModelItem() = default;
    AspectListModelItem(
        const std::shared_ptr<BaseAspect> &aspect,
        const std::function<QVariant(BaseAspect *, int role)> &dataFunction)
        : m_aspect(aspect)
        , m_dataFunction(dataFunction)

    {
        auto upd = [this] { update(); };
        QObject::connect(aspect.get(), &BaseAspect::volatileValueChanged, &guard, upd);
        QObject::connect(aspect.get(), &BaseAspect::changed, &guard, upd);
    }

    QVariant data(int column, int role) const final
    {
        if (column != 0)
            return {};
        if (role == Qt::FontRole) {
            QFont f;
            f.setBold(m_aspect->isDirty() || added);
            f.setStrikeOut(deleted);
            return f;
        }

        QVariant data = m_dataFunction(m_aspect.get(), role);
        if (data.canConvert<QFuture<QVariant>>()) {
            QFuture<QVariant> future = data.value<QFuture<QVariant>>();
            if (!future.isFinished()) {
                future.then(model(), [m = model(), idx = index(), role](const QFuture<QVariant> &f) {
                    if (f.isFinished())
                        m->dataChanged(idx, idx, {role});
                });
                return {};
            }
            return future.result();
        }
        return data;
    }

    bool hasAspect(const std::shared_ptr<BaseAspect> &aspect) const { return aspect == m_aspect; }

    std::shared_ptr<BaseAspect> aspect() const { return m_aspect; }

    void setStatus(bool isAdded, bool isDeleted)
    {
        if (added == isAdded && deleted == isDeleted)
            return;

        aspect()->setEnabled(!isDeleted);

        added = isAdded;
        deleted = isDeleted;
        update();
    }

    bool added = false;
    bool deleted = false;
};

class AspectListModel : public TreeModel<AspectListModelItem, AspectListModelItem>
{
public:
    AspectListModel(const std::function<QVariant(BaseAspect *, int role)> &dataFunction)
        : TreeModel<AspectListModelItem, AspectListModelItem>()
        , m_dataFunction(dataFunction)
    {
    }

    void sync(AspectList &aspect)
    {
        QList<std::shared_ptr<BaseAspect>> volatileItems = aspect.volatileItems();
        QList<std::shared_ptr<BaseAspect>> items = aspect.items();
        QSet<std::shared_ptr<BaseAspect>> itemSet
            = QSet<std::shared_ptr<BaseAspect>>(items.begin(), items.end());
        QSet<std::shared_ptr<BaseAspect>> volatileItemSet
            = QSet<std::shared_ptr<BaseAspect>>(volatileItems.begin(), volatileItems.end());

        auto newItems = volatileItemSet - itemSet;
        auto removedItems = itemSet - volatileItemSet;

        auto itAspect = volatileItems.begin();
        auto modelIdx = 0;

        while (itAspect != volatileItems.end() && modelIdx < rootItem()->childCount()) {
            AspectListModelItem *modelItem = rootItem()->childAt(modelIdx);
            auto inModelAspect = modelItem->aspect();

            const bool inVolatile = volatileItemSet.contains(inModelAspect);
            const bool isGone = !inVolatile && !itemSet.contains(inModelAspect);

            if (*itAspect == inModelAspect) {
                // The item is both in the model and in the volatile part of the aspect, just update its state.
                modelItem->setStatus(
                    newItems.contains(inModelAspect), removedItems.contains(inModelAspect));
                ++itAspect;
                ++modelIdx;
            } else {
                if (!inVolatile) {
                    // The item in the model is not in the volatile part of the aspect
                    if (isGone) {
                        // The item is also not in the applied part of the aspect, remove it from model.
                        delete takeItem(modelItem);
                        modelItem = nullptr;
                    } else {
                        // The item is in the applied part of the aspect, but not in the volatile part, mark it as deleted.
                        modelItem->setStatus(false, true);
                        ++modelIdx;
                    }
                } else {
                    // The item in the aspect is not in the model, add it.
                    modelItem = new AspectListModelItem(*itAspect, m_dataFunction);
                    modelItem->setStatus(newItems.contains(*itAspect), !inVolatile);
                    rootItem()->insertChild(modelIdx, modelItem);
                    ++modelIdx;
                    ++itAspect;
                }
            }
        }

        // Remove remaining items in model.
        if (modelIdx < rootItem()->childCount()) {
            for (; modelIdx < rootItem()->childCount();) {
                auto item = rootItem()->childAt(modelIdx);
                const bool isGone = !itemSet.contains(item->aspect());
                if (isGone)
                    delete takeItem(item);
                else {
                    item->setStatus(false, true);
                    modelIdx++;
                }
            }
        }

        // Add remaining items in aspect to model.
        if (itAspect != volatileItems.end()) {
            for (; itAspect != volatileItems.end(); ++itAspect) {
                auto item = new AspectListModelItem(*itAspect, m_dataFunction);
                item->setStatus(newItems.contains(*itAspect), false);
                rootItem()->appendChild(item);
            }
        }
    }

private:
    std::function<QVariant(BaseAspect *, int)> m_dataFunction;
};

class Internal::AspectListPrivate
{
public:
    QList<std::shared_ptr<BaseAspect>> items;
    QList<std::shared_ptr<BaseAspect>> volatileItems;

    void connectVolatile(const std::shared_ptr<BaseAspect> &aspect, AspectList *list)
    {
        QObject::connect(
            aspect.get(),
            &BaseAspect::volatileValueChanged,
            list,
            &AspectList::volatileValueChanged);
    }
    void disconnectVolatile(const std::shared_ptr<BaseAspect> &aspect, AspectList *list)
    {
        QObject::disconnect(
            aspect.get(),
            &BaseAspect::volatileValueChanged,
            list,
            &AspectList::volatileValueChanged);
    }

    AspectList::CreateItem createItem;

    AspectList::DisplayStyle displayStyle = AspectList::DisplayStyle::InlineList;

    struct ExtraButton { QString text; std::function<void()> callback; };
    QList<ExtraButton> extraButtons;

    AspectListModel model;

    AspectListPrivate(std::function<QVariant(BaseAspect *, int)> dataFunction)
        : model(dataFunction)
    {}

    void addToLayoutImplInlineList(Layouting::Layout &parent, AspectList *aspect)
    {
        using namespace Layouting;
        using namespace Utils::QtDesignWidgets;

        auto fill = [this, aspect] {
            const auto createRow = [aspect](const std::shared_ptr<BaseAspect> &item) {
                // clang-format off
                return Row {
                    *item,
                    IconButton {
                        ::icon(Utils::Icons::EDIT_CLEAR),
                        sizePolicy(QSizePolicy{QSizePolicy::Fixed, QSizePolicy::Fixed}),
                        onClicked(aspect, [aspect, item] {
                            aspect->removeItem(item);
                        })
                    },
                    spacing(5),
                    noMargin,
                };
                // clang-format on
            };

            // clang-format off
            return Column {
                Utils::transform(aspect->volatileItems(), createRow),
                Row {
                    noMargin,
                    st,
                    IconButton {
                        ::icon(Utils::Icons::PLUS),
                        onClicked(aspect, [this, aspect](){
                            aspect->addItem(createItem());
                        })
                    }
                }
            };
            // clang-format on
        };

        // clang-format off
        parent.addItem(
            Group {
                replaceLayoutOn(aspect, &AspectList::volatileItemListChanged, fill)
            }
        );
        // clang-format on
    }

    void addToLayoutImplListView(Layouting::Layout &parent, AspectList *aspect)
    {
        using namespace Layouting;
        using namespace Utils::QtDesignWidgets;

        QPushButton *removeButton = nullptr;
        QWidget *configWidget = nullptr;
        auto listView = aspect->createSubWidget<TreeView>();
        listView->header()->hide();
        listView->setModel(&model);
        listView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        auto add = [aspect, listView, this] {
            auto newItem = aspect->createAndAddItem();
            // Find new index
            auto item = model.findItemAtLevel<1>(
                [newItem](AspectListModelItem *item) { return item->hasAspect(newItem); });
            QModelIndex newIdx = model.indexForItem(item);
            listView->setCurrentIndex(newIdx);
        };

        auto removeCurrent = [listView, this, aspect] {
            QModelIndex currentIndex = listView->currentIndex();
            QTC_ASSERT(currentIndex.isValid(), return);
            const auto item = model.itemForIndex(currentIndex);
            QTC_ASSERT(item, return);
            aspect->removeItem(item->aspect());
        };
        QLayout *layout = nullptr;

        Column buttonColumn {
            PushButton {
                text(Tr::tr("Add")),
                onClicked(aspect, add),
            },
            PushButton {
                bindTo(&removeButton),
                text(Tr::tr("Remove")),
                onClicked(aspect, removeCurrent),
            },
        };
        for (const ExtraButton &eb : extraButtons) {
            buttonColumn.addItem(PushButton {
                text(eb.text),
                onClicked(aspect, eb.callback),
            });
        }
        buttonColumn.addItem(st);

        // clang-format off
        parent.addItem(
            Row {
                Column {
                    bindTo(&layout),
                    listView,
                },
                buttonColumn,
            }
        );
        // clang-format on

        const auto onCurrentChanged =
            [listView, layout, configWidget, this](const QModelIndex &current) mutable {
                QWidget *newConfigWidget = nullptr;
                if (current.isValid()) {
                    const AspectListModelItem *item = model.itemForIndex(current);
                    QTC_ASSERT(item, return);
                    newConfigWidget = new QWidget();

                    if (auto container = dynamic_cast<AspectContainer *>(item->aspect().get()))
                        container->layouter()().attachTo(newConfigWidget);
                    else
                        Column{item->aspect().get()}.attachTo(newConfigWidget);
                }

                if (newConfigWidget) {
                    if (!configWidget) {
                        layout->addWidget(newConfigWidget);
                    } else {
                        delete layout->replaceWidget(configWidget, newConfigWidget);
                        delete configWidget;
                    }
                } else {
                    delete configWidget;
                }
                configWidget = newConfigWidget;
                listView->scrollTo(current, QListView::ScrollHint::EnsureVisible);
            };

        QObject::connect(
            listView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            aspect,
            onCurrentChanged);
    }

    void addToLayoutImpl(Layouting::Layout &parent, AspectList *aspect)
    {
        if (displayStyle == AspectList::DisplayStyle::InlineList)
            addToLayoutImplInlineList(parent, aspect);
        else if (displayStyle == AspectList::DisplayStyle::ListViewWithDetails)
            addToLayoutImplListView(parent, aspect);
    }
};

AspectList::AspectList(Utils::AspectContainer *container)
    : Utils::BaseAspect(container)
    , d(std::make_unique<Internal::AspectListPrivate>(
          [this](BaseAspect *aspect, int role) -> QVariant {
              QTC_ASSERT(listViewDataCallback, return QString("No listViewDataCallback set"));
              return listViewDataCallback(aspect, role);
          }))
{}

AspectList::~AspectList() = default;

void AspectList::fromMap(const Utils::Store &map)
{
    QTC_ASSERT(!settingsKey().isEmpty(), return);

    setVariantValue(map[settingsKey()], BeQuiet);
    d->model.sync(*this);
}

QVariantList AspectList::toList(bool v) const
{
    QVariantList list;
    const auto &items = v ? d->volatileItems : d->items;

    for (const std::shared_ptr<BaseAspect> &item : items) {
        Utils::Store childStore;
        if (v)
            item->volatileToMap(childStore);
        else
            item->toMap(childStore);

        list.append(Utils::variantFromStore(childStore));
    }

    return list;
}

void AspectList::toMap(Utils::Store &map) const
{
    QTC_ASSERT(!settingsKey().isEmpty(), return);
    const Utils::Key key = settingsKey();
    map[key] = toList(false);
}

void AspectList::volatileToMap(Utils::Store &map) const
{
    QTC_ASSERT(!settingsKey().isEmpty(), return);
    const Utils::Key key = settingsKey();
    map[key] = toList(true);
}

std::shared_ptr<BaseAspect> AspectList::actualAddItem(const std::shared_ptr<BaseAspect> &item)
{
    item->setAutoApply(isAutoApply());
    item->setUndoStack(undoStack());

    d->volatileItems.append(item);
    d->connectVolatile(item, this);
    if (itemAddedCallback)
        itemAddedCallback(item);
    d->model.sync(*this);
    emit volatileValueChanged();
    emit volatileItemListChanged();
    if (isAutoApply())
        d->items = d->volatileItems;
    return item;
}

QList<std::shared_ptr<BaseAspect>> AspectList::items() const
{
    return d->items;
}
QList<std::shared_ptr<BaseAspect>> AspectList::volatileItems() const
{
    return d->volatileItems;
}

std::shared_ptr<BaseAspect> AspectList::createAndAddItem()
{
    return addItem(d->createItem());
}

std::shared_ptr<BaseAspect> AspectList::addItem(const std::shared_ptr<BaseAspect> &item)
{
    if (undoStack())
        undoStack()->push(new AddItemCommand(this, item));
    else
        return actualAddItem(item);

    return item;
}

void AspectList::actualRemoveItem(const std::shared_ptr<BaseAspect> &item)
{
    d->disconnectVolatile(item, this);
    d->volatileItems.removeOne(item);
    if (itemRemovedCallback)
        itemRemovedCallback(item);
    d->model.sync(*this);
    emit volatileValueChanged();
    emit volatileItemListChanged();
    if (isAutoApply())
        d->items = d->volatileItems;
}

void AspectList::removeItem(const std::shared_ptr<BaseAspect> &item)
{
    if (undoStack())
        undoStack()->push(new RemoveItemCommand(this, item));
    else
        actualRemoveItem(item);
}

void AspectList::clear()
{
    if (undoStack()) {
        undoStack()->beginMacro("Clear");

        for (const std::shared_ptr<BaseAspect> &item : volatileItems())
            undoStack()->push(new RemoveItemCommand(this, item));

        undoStack()->endMacro();
    } else {
        for (const std::shared_ptr<BaseAspect> &item : volatileItems())
            actualRemoveItem(item);
    }
}

void AspectList::apply()
{
    d->items = d->volatileItems;
    forEachItem([](BaseAspect *aspect) { aspect->apply(); });
    d->model.sync(*this);
    emit changed();
}

void AspectList::cancel()
{
    for (const auto &item : d->volatileItems)
        d->disconnectVolatile(item, this);

    d->volatileItems = d->items;

    for (const auto &item : d->volatileItems)
        d->connectVolatile(item, this);

    forEachItem([](BaseAspect *aspect) { aspect->cancel(); });
    d->model.sync(*this);
    emit volatileValueChanged();
    emit volatileItemListChanged();
}

void AspectList::setAutoApply(bool on)
{
    BaseAspect::setAutoApply(on);
    forEachItem([on](BaseAspect *aspect) { aspect->setAutoApply(on); });
}

void AspectList::setCreateItemFunction(CreateItem createItem)
{
    d->createItem = createItem;
}

qsizetype AspectList::size() const
{
    return d->volatileItems.size();
}

bool AspectList::isDirty() const
{
    if (d->items != d->volatileItems)
        return true;

    for (const std::shared_ptr<BaseAspect> &item : std::as_const(d->volatileItems)) {
        if (item->isDirty())
            return true;
    }
    return false;
}

void AspectList::setVariantValue(const QVariant &value, Announcement howToAnnounce)
{
    const QVariantList list = value.toList();
    for (const std::shared_ptr<BaseAspect> &item : d->volatileItems)
        d->disconnectVolatile(item, this);

    d->volatileItems.clear();
    for (const QVariant &entry : list) {
        auto item = d->createItem();
        item->setAutoApply(isAutoApply());
        item->setUndoStack(undoStack());
        item->fromMap(Utils::storeFromVariant(entry));
        d->volatileItems.append(item);
        d->connectVolatile(item, this);
    }

    d->items = d->volatileItems;
    if (howToAnnounce == DoEmit)
        emit changed();

    d->model.sync(*this);
}

class ColoredRow : public QWidget
{
public:
    ColoredRow(int idx, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_index(idx)
    {}
    void paintEvent(QPaintEvent *event) override
    {
        QPainter p(this);
        QPalette pal = palette();
        if (m_index % 2 == 0)
            p.fillRect(event->rect(), pal.base());
        else
            p.fillRect(event->rect(), pal.alternateBase());
    }

private:
    int m_index;
};

void AspectList::setDisplayStyle(DisplayStyle displayStyle)
{
    d->displayStyle = displayStyle;
}

void AspectList::addExtraButton(const QString &text, std::function<void()> callback)
{
    d->extraButtons.append({text, std::move(callback)});
}

void AspectList::addToLayoutImpl(Layouting::Layout &parent)
{
    d->addToLayoutImpl(parent, this);
}

} // namespace Utils
