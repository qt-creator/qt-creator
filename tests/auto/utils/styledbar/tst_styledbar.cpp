// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/qtcsettings_p.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>
#include <utils/widgets.h>

#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QLayout>
#include <QListWidget>
#include <QMenu>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QToolButton>

#include <memory>

using namespace Utils;

class tst_StyledBar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void defaultOrderIsInsertionOrder();
    void orderIsMergedWithDefaults();
    void layoutFollowsOrder();
    void hiddenItemsAreHidden();
    void permanentItemsCannotBeHidden();
    void programmaticVisibilityKeepsUserHidden();
    void resetRestoresDefaults();
    void separatorsAreMovableAndHideable();
    void customizingPreviewsLive_data();
    void customizingPreviewsLive();
    void customizingKeepsPositionsOfHiddenItems();
    void dialogListsLayoutItems();
    void spacersAreFixedAndStretchesExpand();
    void userItemsFollowOrder();
    void availableListRejectsLockedItems();
    void listsRejectCopiesOfItems();
    void dialogAddsUserItems_data();
    void dialogAddsUserItems();
    void hidingRemovesUserItems_data();
    void hidingRemovesUserItems();
    void hideButtonMovesItemsToAvailableList();
    void hideButtonKeepsPositionsOfItems();
    void addButtonShowsHiddenItems();
    void templatesInstantiateOnInsertion();
    void templatesInstantiateOnDrop();
    void droppedItemsKeepIcons();
    void doubleClickMovesItems();
    void moveButtonsReorderItems_data();
    void moveButtonsReorderItems();
    void restoreDefaultsButtonResetsPreview_data();
    void restoreDefaultsButtonResetsPreview();
    void returnClicksFocusedButtonOrAccepts();
    void resetActionRestoresDefaults();
    void configurationIsStoredInSettings();
    void itemStretchFollowsItem();
    void overflowMovesTrailingItemsIntoPopup();
    void permanentItemsDoNotOverflow();
    void stretchesDoNotOverflow();
    void sizeHintsAllowShrinking();
    void addingToBarWithForeignLayoutFails();
    void barWithoutItemsHasNoContextMenu();

private:
    StyledBar *createBar(const KeyList &ids);
    KeyList layoutOrder() const;
    void triggerContextMenuAction(const QString &text);
    void customize(const std::function<void(QListWidget *)> &edit, bool accept);

    StyledBar *m_bar = nullptr;
    QHash<Key, QWidget *> m_widgets;
    QTemporaryDir m_settingsDir;
};

void tst_StyledBar::initTestCase()
{
    setCreatorTheme(new Theme("test"));
    QVERIFY(m_settingsDir.isValid());
    const QString fileName = m_settingsDir.filePath("settings.ini");
    Internal::SettingsSetup::setupSettings(new QtcSettings(fileName, QSettings::IniFormat),
                                           new QtcSettings(fileName, QSettings::IniFormat));
}

void tst_StyledBar::cleanupTestCase()
{
    Internal::SettingsSetup::destroySettings();
    setCreatorTheme(nullptr);
}

StyledBar *tst_StyledBar::createBar(const KeyList &ids)
{
    auto bar = new StyledBar;
    for (const Key &id : ids) {
        auto widget = new QWidget(bar);
        m_widgets.insert(id, widget);
        bar->addItem(id, stringFromKey(id), widget);
    }
    return bar;
}

void tst_StyledBar::init()
{
    m_bar = createBar({"A", "B", "C", "D"});
    m_bar->resize(1000, 30);
    m_bar->show();
}

void tst_StyledBar::cleanup()
{
    delete m_bar;
    m_bar = nullptr;
    m_widgets.clear();
}

KeyList tst_StyledBar::layoutOrder() const
{
    KeyList result;
    QLayout *layout = m_bar->layout();
    for (int i = 0; i < layout->count(); ++i)
        result.append(m_widgets.key(layout->itemAt(i)->widget()));
    return result;
}

void tst_StyledBar::defaultOrderIsInsertionOrder()
{
    QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
    QCOMPARE(layoutOrder(), KeyList({"A", "B", "C", "D"}));
}

void tst_StyledBar::orderIsMergedWithDefaults()
{
    m_bar->setItemOrder({"C", "Unknown", "A", "C"});
    QCOMPARE(m_bar->itemOrder(), KeyList({"C", "D", "A", "B"}));

    m_bar->setItemOrder({"D", "B"});
    QCOMPARE(m_bar->itemOrder(), KeyList({"A", "D", "B", "C"}));
}

void tst_StyledBar::layoutFollowsOrder()
{
    m_bar->setItemOrder({"D", "C", "B", "A"});
    QCOMPARE(layoutOrder(), KeyList({"D", "C", "B", "A"}));
}

void tst_StyledBar::hiddenItemsAreHidden()
{
    m_bar->setHiddenItems({"B", "Unknown"});
    QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
    QVERIFY(!m_widgets.value("B")->isVisible());
    QVERIFY(m_widgets.value("A")->isVisible());

    m_bar->setHiddenItems({});
    QVERIFY(m_widgets.value("B")->isVisible());
}

void tst_StyledBar::permanentItemsCannotBeHidden()
{
    m_bar->setHiddenItems({"A", "B"});
    m_bar->setPermanentItems({"A"});
    QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
    QVERIFY(m_widgets.value("A")->isVisible());

    m_bar->setHiddenItems({"A", "C"});
    QCOMPARE(m_bar->hiddenItems(), KeyList({"C"}));
    QVERIFY(m_widgets.value("A")->isVisible());
}

void tst_StyledBar::programmaticVisibilityKeepsUserHidden()
{
    m_bar->setItemVisible("A", false);
    QVERIFY(!m_widgets.value("A")->isVisible());
    m_bar->setItemVisible("A", true);
    QVERIFY(m_widgets.value("A")->isVisible());

    m_bar->setHiddenItems({"B"});
    m_bar->setItemVisible("B", false);
    m_bar->setItemVisible("B", true);
    QVERIFY(!m_widgets.value("B")->isVisible());
}

void tst_StyledBar::resetRestoresDefaults()
{
    m_bar->setItemOrder({"D", "C", "B", "A"});
    m_bar->setHiddenItems({"B"});
    m_bar->resetItemConfiguration();
    QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
    QCOMPARE(m_bar->hiddenItems(), KeyList());
    QCOMPARE(layoutOrder(), KeyList({"A", "B", "C", "D"}));
    QVERIFY(m_widgets.value("B")->isVisible());
}

void tst_StyledBar::separatorsAreMovableAndHideable()
{
    m_bar->addSeparator("Separator1");
    QWidget *separator = m_bar->layout()->itemAt(4)->widget();
    QVERIFY(separator);
    m_widgets.insert("Separator1", separator);
    QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D", "Separator1"}));

    m_bar->setItemOrder({"A", "Separator1", "B", "C", "D"});
    QCOMPARE(layoutOrder(), KeyList({"A", "Separator1", "B", "C", "D"}));

    m_bar->setHiddenItems({"Separator1"});
    QCOMPARE(m_bar->hiddenItems(), KeyList({"Separator1"}));
    QVERIFY(!separator->isVisible());
}

void tst_StyledBar::triggerContextMenuAction(const QString &text)
{
    bool triggered = false;
    QObject guard;
    QTimer::singleShot(0, &guard, [&] {
        auto menu = m_bar->findChild<QMenu *>();
        if (!menu)
            return;
        const QList<QAction *> actions = menu->actions();
        for (QAction *action : actions) {
            if (action->text() == text) {
                action->trigger();
                triggered = true;
            }
        }
        menu->close();
    });
    QContextMenuEvent event(QContextMenuEvent::Mouse, {}, m_bar->mapToGlobal(QPoint()));
    QApplication::sendEvent(m_bar, &event);
    QVERIFY(triggered);
}

void tst_StyledBar::customize(const std::function<void(QListWidget *)> &edit, bool accept)
{
    triggerContextMenuAction("Customize Toolbar...");
    if (QTest::currentTestFailed())
        return;

    QPointer<QDialog> dialog = m_bar->findChild<QDialog *>(QString(), Qt::FindDirectChildrenOnly);
    QVERIFY(dialog);
    QCOMPARE(QApplication::activeModalWidget(), dialog.data());
    auto list = dialog->findChild<QListWidget *>("ShownItems");
    QVERIFY(list);
    edit(list);
    if (dialog->isVisible()) {
        if (accept)
            dialog->accept();
        else
            dialog->reject();
    }
    QTRY_VERIFY(!dialog);
}

static void addAcceptColumn()
{
    QTest::addColumn<bool>("accept");
    QTest::newRow("cancel") << false;
    QTest::newRow("accept") << true;
}

static QAbstractButton *dialogButton(QListWidget *list, const QString &objectName)
{
    return list->window()->findChild<QAbstractButton *>(objectName);
}

static QListWidget *availableItems(QListWidget *list)
{
    return list->window()->findChild<QListWidget *>("AvailableItems");
}

static bool acceptsDrop(QListWidget *source, int row, QListWidget *target, Qt::DropAction action)
{
    QAbstractItemModel *model = source->model();
    std::unique_ptr<QMimeData> mimeData(model->mimeData({model->index(row, 0)}));
    QDragEnterEvent event(QPoint(1, 1), action, mimeData.get(), Qt::LeftButton, {});
    QApplication::sendEvent(target->viewport(), &event);
    return event.isAccepted();
}

void tst_StyledBar::customizingPreviewsLive_data()
{
    addAcceptColumn();
}

void tst_StyledBar::customizingPreviewsLive()
{
    QFETCH(bool, accept);
    m_bar->setHiddenItems({"C"});

    bool edited = false;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        QCOMPARE(list->count(), 3);
        QCOMPARE(availableList->count(), 4);
        QCOMPARE(availableList->item(0)->text(), "Separator");
        QCOMPARE(availableList->item(1)->text(), "Spacer");
        QCOMPARE(availableList->item(2)->text(), "Stretch");
        QCOMPARE(availableList->item(3)->text(), "C");
        list->insertItem(0, list->takeItem(2));
        availableList->addItem(list->takeItem(2));
        list->addItem(availableList->takeItem(3));
        QCOMPARE(layoutOrder(), KeyList({"D", "A", "B", "C"}));
        QVERIFY(!m_widgets.value("B")->isVisible());
        QVERIFY(m_widgets.value("C")->isVisible());
        edited = true;
    }, accept);
    QVERIFY(edited);

    if (accept) {
        QCOMPARE(m_bar->itemOrder(), KeyList({"D", "A", "B", "C"}));
        QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
        QCOMPARE(layoutOrder(), KeyList({"D", "A", "B", "C"}));
        QVERIFY(!m_widgets.value("B")->isVisible());
        QVERIFY(m_widgets.value("C")->isVisible());
    } else {
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        QCOMPARE(m_bar->hiddenItems(), KeyList({"C"}));
        QCOMPARE(layoutOrder(), KeyList({"A", "B", "C", "D"}));
        QVERIFY(m_widgets.value("B")->isVisible());
        QVERIFY(!m_widgets.value("C")->isVisible());
    }
}

void tst_StyledBar::customizingKeepsPositionsOfHiddenItems()
{
    m_bar->setHiddenItems({"B"});

    bool edited = false;
    customize([&](QListWidget *list) {
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        list->insertItem(0, list->takeItem(2));
        QCOMPARE(m_bar->itemOrder(), KeyList({"D", "A", "B", "C"}));
        edited = true;
    }, true);
    QVERIFY(edited);
    QCOMPARE(m_bar->itemOrder(), KeyList({"D", "A", "B", "C"}));
    QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));

    customize([](QListWidget *) {}, true);
    QCOMPARE(m_bar->itemOrder(), KeyList({"D", "A", "B", "C"}));

    m_bar->setHiddenItems({});
    QCOMPARE(layoutOrder(), KeyList({"D", "A", "B", "C"}));
}

void tst_StyledBar::dialogListsLayoutItems()
{
    m_bar->addStretch("Stretch");
    m_bar->addSpacer("Spacer1");
    m_bar->addSeparator("Separator1");

    QStringList texts;
    customize([&](QListWidget *list) {
        for (int row = 0; row < list->count(); ++row)
            texts.append(list->item(row)->text());
    }, false);
    QCOMPARE(texts, QStringList({"A", "B", "C", "D", "Stretch", "Spacer", "Separator"}));
}

void tst_StyledBar::spacersAreFixedAndStretchesExpand()
{
    m_bar->setItemOrder({"A", "StyledBar.UserSpacer1", "B", "StyledBar.UserStretch1"});
    QLayout *layout = m_bar->layout();
    QCOMPARE(layout->count(), 6);
    QWidget *spacer = layout->itemAt(1)->widget();
    QWidget *stretch = layout->itemAt(5)->widget();
    QVERIFY(spacer && stretch);
    QVERIFY(spacer->minimumWidth() > 0);
    QCOMPARE(spacer->maximumWidth(), spacer->minimumWidth());
    QCOMPARE(stretch->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);
}

void tst_StyledBar::availableListRejectsLockedItems()
{
    m_bar->setPermanentItems({"A"});

    QList<bool> accepted;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        for (int row : {0, 1})
            accepted.append(acceptsDrop(list, row, availableList, Qt::MoveAction));
    }, false);
    QCOMPARE(accepted, QList<bool>({false, true}));
}

void tst_StyledBar::listsRejectCopiesOfItems()
{
    QList<bool> accepted;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        accepted.append(acceptsDrop(list, 1, availableList, Qt::CopyAction));
        accepted.append(acceptsDrop(list, 1, list, Qt::CopyAction));
        accepted.append(acceptsDrop(availableList, 0, list, Qt::CopyAction));
        accepted.append(acceptsDrop(list, 1, availableList, Qt::MoveAction));
    }, false);
    QCOMPARE(accepted, QList<bool>({false, false, true, true}));
}

void tst_StyledBar::userItemsFollowOrder()
{
    m_bar->setItemOrder({"A", "StyledBar.UserSeparator1", "B", "StyledBar.UserSpacer1",
                         "StyledBar.UserStretch1"});
    QCOMPARE(m_bar->itemOrder(),
             KeyList({"A", "StyledBar.UserSeparator1", "B", "C", "D", "StyledBar.UserSpacer1",
                      "StyledBar.UserStretch1"}));
    QCOMPARE(m_bar->layout()->count(), 7);

    m_bar->setItemOrder({"A", "B", "C", "D"});
    QCOMPARE(m_bar->layout()->count(), 4);

    m_bar->setItemOrder({"StyledBar.UserSeparator1"});
    QCOMPARE(m_bar->layout()->count(), 5);
    m_bar->resetItemConfiguration();
    QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
    QCOMPARE(m_bar->layout()->count(), 4);
}

void tst_StyledBar::dialogAddsUserItems_data()
{
    addAcceptColumn();
}

void tst_StyledBar::dialogAddsUserItems()
{
    QFETCH(bool, accept);

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *add = dialogButton(list, "AddButton");
        QListWidget *availableList = availableItems(list);
        QVERIFY(add && availableList);
        list->setCurrentRow(0);
        availableList->setCurrentRow(0);
        add->click();
        availableList->setCurrentRow(1);
        add->click();
        availableList->setCurrentRow(2);
        add->click();
        QCOMPARE(availableList->count(), 3);
        QCOMPARE(list->count(), 7);
        QCOMPARE(list->item(1)->text(), "Separator");
        QCOMPARE(list->item(2)->text(), "Spacer");
        QCOMPARE(list->item(3)->text(), "Stretch");
        QCOMPARE(m_bar->layout()->count(), 7);
        QVERIFY(m_bar->layout()->itemAt(1)->widget()->isVisible());
        QCOMPARE(m_bar->itemOrder().at(1), Key("StyledBar.UserSeparator1"));
        QCOMPARE(m_bar->itemOrder().at(2), Key("StyledBar.UserSpacer1"));
        QCOMPARE(m_bar->itemOrder().at(3), Key("StyledBar.UserStretch1"));
        edited = true;
    }, accept);
    QVERIFY(edited);

    if (accept) {
        QCOMPARE(m_bar->itemOrder(),
                 KeyList({"A", "StyledBar.UserSeparator1", "StyledBar.UserSpacer1",
                          "StyledBar.UserStretch1", "B", "C", "D"}));
        QCOMPARE(m_bar->layout()->count(), 7);
    } else {
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        QCOMPARE(m_bar->layout()->count(), 4);
    }
}

void tst_StyledBar::hidingRemovesUserItems_data()
{
    addAcceptColumn();
}

void tst_StyledBar::hidingRemovesUserItems()
{
    QFETCH(bool, accept);
    m_bar->setItemOrder({"A", "StyledBar.UserSeparator1", "B", "C", "D"});

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *hide = dialogButton(list, "HideButton");
        QListWidget *availableList = availableItems(list);
        QVERIFY(hide && availableList);
        list->setCurrentRow(1);
        QVERIFY(hide->isEnabled());
        hide->click();
        QCOMPARE(list->count(), 4);
        QCOMPARE(availableList->count(), 3);
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        QCOMPARE(m_bar->layout()->count(), 4);
        edited = true;
    }, accept);
    QVERIFY(edited);

    if (accept) {
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        QCOMPARE(m_bar->layout()->count(), 4);
    } else {
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "StyledBar.UserSeparator1", "B", "C", "D"}));
        QCOMPARE(m_bar->layout()->count(), 5);
    }
}

void tst_StyledBar::hideButtonMovesItemsToAvailableList()
{
    m_bar->setPermanentItems({"A"});
    m_bar->addStretch("Stretch");

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *hide = dialogButton(list, "HideButton");
        QListWidget *availableList = availableItems(list);
        QVERIFY(hide && availableList);
        list->setCurrentRow(0);
        QVERIFY(!hide->isEnabled());
        list->setCurrentRow(4);
        QVERIFY(!hide->isEnabled());
        list->setCurrentRow(1);
        QVERIFY(hide->isEnabled());
        hide->click();
        QCOMPARE(list->count(), 4);
        QCOMPARE(availableList->count(), 4);
        QCOMPARE(availableList->item(3)->text(), "B");
        QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
        QVERIFY(!m_widgets.value("B")->isVisible());
        edited = true;
    }, true);
    QVERIFY(edited);
    QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
}

void tst_StyledBar::hideButtonKeepsPositionsOfItems()
{
    m_bar->setItemOrder({"B", "A", "C", "D"});

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *hide = dialogButton(list, "HideButton");
        QVERIFY(hide);
        list->setCurrentRow(0);
        hide->click();
        QCOMPARE(m_bar->itemOrder(), KeyList({"B", "A", "C", "D"}));
        edited = true;
    }, true);
    QVERIFY(edited);
    QCOMPARE(m_bar->itemOrder(), KeyList({"B", "A", "C", "D"}));
    QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
}

void tst_StyledBar::addButtonShowsHiddenItems()
{
    m_bar->setHiddenItems({"B"});

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *add = dialogButton(list, "AddButton");
        QListWidget *availableList = availableItems(list);
        QVERIFY(add && availableList);
        availableList->setCurrentRow(-1);
        QVERIFY(!add->isEnabled());
        list->setCurrentRow(0);
        availableList->setCurrentRow(3);
        QVERIFY(add->isEnabled());
        add->click();
        QCOMPARE(availableList->count(), 3);
        QCOMPARE(list->item(1)->text(), "B");
        QCOMPARE(m_bar->hiddenItems(), KeyList());
        QCOMPARE(layoutOrder(), KeyList({"A", "B", "C", "D"}));
        QVERIFY(m_widgets.value("B")->isVisible());
        edited = true;
    }, true);
    QVERIFY(edited);
    QCOMPARE(m_bar->hiddenItems(), KeyList());
}

void tst_StyledBar::templatesInstantiateOnInsertion()
{
    bool edited = false;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        list->insertItem(1, availableList->item(0)->clone());
        list->insertItem(2, availableList->item(0)->clone());
        QCOMPARE(availableList->count(), 3);
        QCOMPARE(m_bar->itemOrder(),
                 KeyList({"A", "StyledBar.UserSeparator1", "StyledBar.UserSeparator2",
                          "B", "C", "D"}));
        QCOMPARE(m_bar->layout()->count(), 6);
        edited = true;
    }, true);
    QVERIFY(edited);
}

void tst_StyledBar::templatesInstantiateOnDrop()
{
    bool edited = false;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        for (int row : {0, 1, 2}) {
            std::unique_ptr<QMimeData> mimeData(
                availableList->model()->mimeData({availableList->model()->index(row, 0)}));
            QVERIFY(list->model()->dropMimeData(mimeData.get(), Qt::CopyAction, -1, -1, {}));
        }
        QCOMPARE(availableList->count(), 3);
        QCOMPARE(m_bar->itemOrder(),
                 KeyList({"A", "B", "C", "D", "StyledBar.UserSeparator1",
                          "StyledBar.UserSpacer1", "StyledBar.UserStretch1"}));
        QCOMPARE(m_bar->layout()->count(), 7);
        edited = true;
    }, true);
    QVERIFY(edited);
    QCOMPARE(m_bar->layout()->count(), 7);
}

void tst_StyledBar::droppedItemsKeepIcons()
{
    auto button = new QToolButton(m_bar);
    button->setIcon(Icons::SLASH.icon());
    m_bar->addItem("E", "E", button);
    bool edited = false;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        const auto drop = [](QListWidget *source, int row, QListWidget *target,
                             Qt::DropAction action) {
            QAbstractItemModel *model = source->model();
            std::unique_ptr<QMimeData> mimeData(model->mimeData({model->index(row, 0)}));
            return target->model()->dropMimeData(mimeData.get(), action, -1, -1, {});
        };
        QVERIFY(drop(availableList, 0, list, Qt::CopyAction));
        QCOMPARE(list->count(), 6);
        QVERIFY(!list->item(5)->icon().isNull());
        QVERIFY(drop(list, 4, availableList, Qt::MoveAction));
        QCOMPARE(availableList->count(), 4);
        QVERIFY(!availableList->item(3)->icon().isNull());
        edited = true;
    }, false);
    QVERIFY(edited);
}

static void doubleClick(QListWidget *list, int row)
{
    const QPoint center = list->visualItemRect(list->item(row)).center();
    QTest::mouseClick(list->viewport(), Qt::LeftButton, {}, center);
    QTest::mouseDClick(list->viewport(), Qt::LeftButton, {}, center);
}

void tst_StyledBar::doubleClickMovesItems()
{
    bool edited = false;
    customize([&](QListWidget *list) {
        QListWidget *availableList = availableItems(list);
        QVERIFY(availableList);
        doubleClick(list, 1);
        QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
        QCOMPARE(availableList->count(), 4);
        doubleClick(availableList, 0);
        QCOMPARE(availableList->count(), 4);
        QCOMPARE(m_bar->layout()->count(), 5);
        edited = true;
    }, false);
    QVERIFY(edited);
}

void tst_StyledBar::moveButtonsReorderItems_data()
{
    addAcceptColumn();
}

void tst_StyledBar::moveButtonsReorderItems()
{
    QFETCH(bool, accept);

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *up = dialogButton(list, "MoveUpButton");
        QAbstractButton *down = dialogButton(list, "MoveDownButton");
        QVERIFY(up && down);
        list->setCurrentRow(0);
        QVERIFY(!up->isEnabled());
        QVERIFY(down->isEnabled());
        down->click();
        QCOMPARE(list->currentRow(), 1);
        QCOMPARE(m_bar->itemOrder(), KeyList({"B", "A", "C", "D"}));
        list->setCurrentRow(3);
        QVERIFY(up->isEnabled());
        QVERIFY(!down->isEnabled());
        up->click();
        QCOMPARE(list->currentRow(), 2);
        QCOMPARE(m_bar->itemOrder(), KeyList({"B", "A", "D", "C"}));
        QCOMPARE(layoutOrder(), KeyList({"B", "A", "D", "C"}));
        edited = true;
    }, accept);
    QVERIFY(edited);

    const KeyList expected = accept ? KeyList({"B", "A", "D", "C"})
                                    : KeyList({"A", "B", "C", "D"});
    QCOMPARE(m_bar->itemOrder(), expected);
    QCOMPARE(layoutOrder(), expected);
}

void tst_StyledBar::restoreDefaultsButtonResetsPreview_data()
{
    addAcceptColumn();
}

void tst_StyledBar::restoreDefaultsButtonResetsPreview()
{
    QFETCH(bool, accept);
    m_bar->setItemOrder({"D", "StyledBar.UserSeparator1", "C", "B", "A"});
    m_bar->setHiddenItems({"B"});

    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *restore = dialogButton(list, "RestoreDefaultsButton");
        QListWidget *availableList = availableItems(list);
        QVERIFY(restore && availableList);
        restore->click();
        QCOMPARE(list->count(), 4);
        QCOMPARE(availableList->count(), 3);
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        QCOMPARE(m_bar->hiddenItems(), KeyList());
        QCOMPARE(m_bar->layout()->count(), 4);
        QVERIFY(m_widgets.value("B")->isVisible());
        edited = true;
    }, accept);
    QVERIFY(edited);

    if (accept) {
        QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
        QCOMPARE(m_bar->hiddenItems(), KeyList());
        QCOMPARE(m_bar->layout()->count(), 4);
        QVERIFY(m_widgets.value("B")->isVisible());
    } else {
        QCOMPARE(m_bar->itemOrder(), KeyList({"D", "StyledBar.UserSeparator1", "C", "B", "A"}));
        QCOMPARE(m_bar->hiddenItems(), KeyList({"B"}));
        QCOMPARE(m_bar->layout()->count(), 5);
        QVERIFY(m_bar->layout()->itemAt(1)->widget()->isVisible());
        QVERIFY(!m_widgets.value("B")->isVisible());
    }
}

void tst_StyledBar::returnClicksFocusedButtonOrAccepts()
{
    bool edited = false;
    customize([&](QListWidget *list) {
        QAbstractButton *down = dialogButton(list, "MoveDownButton");
        QVERIFY(down);
        QWidget *dialog = list->window();
        list->setCurrentRow(0);
        down->setFocus();
        QCOMPARE(dialog->focusWidget(), down);
        QTest::keyClick(dialog, Qt::Key_Return);
        QCOMPARE(m_bar->itemOrder(), KeyList({"B", "A", "C", "D"}));
        QVERIFY(dialog->isVisible());

        list->setFocus();
        QCOMPARE(dialog->focusWidget(), list);
        QTest::keyClick(dialog, Qt::Key_Return);
        QVERIFY(!dialog->isVisible());
        edited = true;
    }, false);
    QVERIFY(edited);
    QCOMPARE(m_bar->itemOrder(), KeyList({"B", "A", "C", "D"}));
}

void tst_StyledBar::resetActionRestoresDefaults()
{
    m_bar->setItemOrder({"D", "StyledBar.UserSeparator1", "C", "B", "A"});
    m_bar->setHiddenItems({"B"});

    triggerContextMenuAction("Reset Toolbar");
    QCOMPARE(m_bar->itemOrder(), KeyList({"A", "B", "C", "D"}));
    QCOMPARE(m_bar->hiddenItems(), KeyList());
    QCOMPARE(layoutOrder(), KeyList({"A", "B", "C", "D"}));
    QVERIFY(m_widgets.value("B")->isVisible());
}

void tst_StyledBar::configurationIsStoredInSettings()
{
    const Key key = "StyledBarTest";
    m_bar->addItem("Removed", "Removed", new QWidget(m_bar));
    m_bar->setSettingsKey(key);
    m_bar->setItemOrder({"A", "StyledBar.UserSeparator1", "Removed", "C", "B", "D"});
    m_bar->setHiddenItems({"B"});
    QVERIFY(userSettings().contains(key + "/Order"));
    QVERIFY(userSettings().contains(key + "/Hidden"));

    std::unique_ptr<StyledBar> bar(createBar({"A", "B", "C", "D", "Added"}));
    bar->setSettingsKey(key);
    QCOMPARE(bar->itemOrder(),
             KeyList({"A", "StyledBar.UserSeparator1", "C", "B", "D", "Added"}));
    QCOMPARE(bar->hiddenItems(), KeyList({"B"}));
    QCOMPARE(bar->layout()->count(), 6);

    bar->resetItemConfiguration();
    QVERIFY(!userSettings().contains(key + "/Order"));
    QVERIFY(!userSettings().contains(key + "/Hidden"));
}

void tst_StyledBar::itemStretchFollowsItem()
{
    auto layout = qobject_cast<QBoxLayout *>(m_bar->layout());
    QVERIFY(layout);
    m_bar->setItemStretch("B", 2);
    QCOMPARE(layout->stretch(1), 2);
    QCOMPARE(layout->stretch(0), 0);

    m_bar->setItemOrder({"B", "A", "C", "D"});
    QCOMPARE(layout->stretch(0), 2);
    QCOMPARE(layout->stretch(1), 0);
}

class OverflowBar
{
public:
    OverflowBar()
    {
        bar = new StyledBar(&container);
        for (const Key &id : KeyList({"A", "B", "C", "D"})) {
            auto widget = new QWidget;
            widget->setFixedWidth(itemWidth);
            widgets.insert(id, widget);
            bar->addItem(id, stringFromKey(id), widget);
        }
        extension = bar->findChild<QToolButton *>("StyledBarExtensionButton");
        container.resize(1000, 100);
        container.show();
    }

    int extensionWidth() const { return extension->sizeHint().width(); }
    void resizeTo(int width) { bar->resize(width, bar->sizeHint().height()); }

    QStringList visibleItems() const
    {
        QStringList result;
        for (const Key &id : KeyList({"A", "B", "C", "D"})) {
            QWidget *widget = widgets.value(id);
            if (widget->isVisible() && widget->parentWidget() == bar)
                result.append(stringFromKey(id));
        }
        return result;
    }

    static constexpr int itemWidth = 50;
    QWidget container;
    StyledBar *bar = nullptr;
    QToolButton *extension = nullptr;
    QHash<Key, QWidget *> widgets;
};

void tst_StyledBar::overflowMovesTrailingItemsIntoPopup()
{
    OverflowBar overflow;
    QVERIFY(overflow.extension);
    overflow.resizeTo(4 * OverflowBar::itemWidth);
    QCOMPARE(overflow.visibleItems(), QStringList({"A", "B", "C", "D"}));
    QVERIFY(!overflow.extension->isVisible());

    overflow.resizeTo(2 * OverflowBar::itemWidth + overflow.extensionWidth());
    QCOMPARE(overflow.visibleItems(), QStringList({"A", "B"}));
    QVERIFY(overflow.extension->isVisible());

    overflow.extension->click();
    QWidget *itemC = overflow.widgets.value("C");
    QPointer<QWidget> popup = itemC->parentWidget();
    QVERIFY(popup && popup != overflow.bar && popup->isWindow());
    QVERIFY(popup->isVisible());
    QCOMPARE(overflow.widgets.value("D")->parentWidget(), popup.data());
    QVERIFY(itemC->isVisible());

    popup->close();
    QTRY_VERIFY(!popup);
    QCOMPARE(itemC->parentWidget(), overflow.bar);
    QCOMPARE(overflow.bar->layout()->count(), 4);
    QCOMPARE(overflow.visibleItems(), QStringList({"A", "B"}));

    overflow.resizeTo(4 * OverflowBar::itemWidth);
    QCOMPARE(overflow.visibleItems(), QStringList({"A", "B", "C", "D"}));
    QVERIFY(!overflow.extension->isVisible());
}

void tst_StyledBar::permanentItemsDoNotOverflow()
{
    OverflowBar overflow;
    overflow.bar->setPermanentItems({"D"});
    overflow.resizeTo(2 * OverflowBar::itemWidth + overflow.extensionWidth());
    QCOMPARE(overflow.visibleItems(), QStringList({"A", "D"}));
}

void tst_StyledBar::stretchesDoNotOverflow()
{
    OverflowBar overflow;
    overflow.bar->addStretch("Stretch");
    QLayout *layout = overflow.bar->layout();
    QWidget *stretch = layout->itemAt(layout->count() - 1)->widget();
    QVERIFY(stretch);
    overflow.resizeTo(2 * OverflowBar::itemWidth + overflow.extensionWidth());
    QCOMPARE(overflow.visibleItems(), QStringList({"A", "B"}));
    QVERIFY(stretch->isVisible());
}

void tst_StyledBar::sizeHintsAllowShrinking()
{
    OverflowBar overflow;
    QCOMPARE(overflow.bar->sizeHint().width(), 4 * OverflowBar::itemWidth);
    QCOMPARE(overflow.bar->minimumSizeHint().width(), overflow.extensionWidth());

    overflow.bar->setPermanentItems({"D"});
    QCOMPARE(overflow.bar->minimumSizeHint().width(),
             OverflowBar::itemWidth + overflow.extensionWidth());

    overflow.resizeTo(OverflowBar::itemWidth + overflow.extensionWidth());
    QCOMPARE(overflow.bar->sizeHint().width(), 4 * OverflowBar::itemWidth);
}

void tst_StyledBar::addingToBarWithForeignLayoutFails()
{
    if (qEnvironmentVariableIsSet("QTC_FATAL_ASSERTS"))
        QSKIP("The test triggers soft asserts, which QTC_FATAL_ASSERTS makes fatal.");
    StyledBar bar;
    new QHBoxLayout(&bar);
    const QRegularExpression softAssert("^SOFT ASSERT");
    QTest::ignoreMessage(QtDebugMsg, softAssert);
    QTest::ignoreMessage(QtDebugMsg, softAssert);
    bar.addStretch("Stretch");
    bar.addSeparator("Separator");
    QVERIFY(bar.itemOrder().isEmpty());
    QVERIFY(bar.findChildren<QWidget *>(Qt::FindDirectChildrenOnly).isEmpty());
}

void tst_StyledBar::barWithoutItemsHasNoContextMenu()
{
    StyledBar bar;
    new QHBoxLayout(&bar);
    bool menuShown = false;
    QObject guard;
    QTimer::singleShot(0, &guard, [&] {
        if (auto menu = bar.findChild<QMenu *>()) {
            menuShown = true;
            menu->close();
        }
    });
    QContextMenuEvent event(QContextMenuEvent::Mouse, {}, bar.mapToGlobal(QPoint()));
    QApplication::sendEvent(&bar, &event);
    QVERIFY(!event.isAccepted());
    QVERIFY(!menuShown);
}

QTEST_MAIN(tst_StyledBar)

#include "tst_styledbar.moc"
