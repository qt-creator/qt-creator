/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "sidebar.h"
#include "imode.h"

#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"

#include <QtCore/QEvent>
#include <QtCore/QSettings>
#include <QtGui/QLayout>
#include <QtGui/QToolBar>
#include <QtGui/QAction>
#include <QtGui/QToolButton>

using namespace Core;
using namespace Core::Internal;

SideBarItem::~SideBarItem()
{
    delete m_widget;
}

SideBar::SideBar(QList<SideBarItem*> itemList,
                 QList<SideBarItem*> defaultVisible) :
    m_closeWhenEmpty(false)
{
    setOrientation(Qt::Vertical);
    foreach (SideBarItem *item, itemList) {
        m_itemMap.insert(item->id(), item);
        m_availableItemIds.append(item->id());
        m_availableItemTitles.append(item->title());
    }

    foreach (SideBarItem *item, defaultVisible) {
        if (!itemList.contains(item))
            continue;
        m_defaultVisible.append(item->id());
    }
}

SideBar::~SideBar()
{
    QMutableMapIterator<QString, QWeakPointer<SideBarItem> > iter(m_itemMap);
    while(iter.hasNext()) {
        iter.next();
        if (!iter.value().isNull())
            delete iter.value().data();
    }
}

QString SideBar::idForTitle(const QString &title) const
{
    QMapIterator<QString, QWeakPointer<SideBarItem> > iter(m_itemMap);
    while(iter.hasNext()) {
        iter.next();
        if (iter.value().data()->title() == title)
            return iter.key();
    }
    return QString();
}

QStringList SideBar::availableItemIds() const
{
    return m_availableItemIds;
}

QStringList SideBar::availableItemTitles() const
{
    return m_availableItemTitles;
}

QStringList SideBar::unavailableItemIds() const
{
    return m_unavailableItemIds;
}

bool SideBar::closeWhenEmpty() const
{
    return m_closeWhenEmpty;
}
void SideBar::setCloseWhenEmpty(bool value)
{
    m_closeWhenEmpty = value;
}

void SideBar::makeItemAvailable(SideBarItem *item)
{
    QMap<QString, QWeakPointer<SideBarItem> >::const_iterator it = m_itemMap.constBegin();
    while (it != m_itemMap.constEnd()) {
        if (it.value().data() == item) {
            m_availableItemIds.append(it.key());
            m_availableItemTitles.append(it.value().data()->title());
            m_unavailableItemIds.removeAll(it.key());
            qSort(m_availableItemTitles);
            emit availableItemsChanged();
            //updateWidgets();
            break;
        }
        ++it;
    }
}

// sets a list of externally used, unavailable items. For example,
// another sidebar could set
void SideBar::setUnavailableItemIds(const QStringList &itemIds)
{
    // re-enable previous items
    foreach(const QString &id, m_unavailableItemIds) {
        m_availableItemIds.append(id);
        m_availableItemTitles.append(m_itemMap.value(id).data()->title());
    }

    m_unavailableItemIds.clear();

    foreach (const QString &id, itemIds) {
        if (!m_unavailableItemIds.contains(id))
            m_unavailableItemIds.append(id);
        m_availableItemIds.removeAll(id);
        m_availableItemTitles.removeAll(m_itemMap.value(id).data()->title());
    }
    qSort(m_availableItemTitles);
    updateWidgets();
}

SideBarItem *SideBar::item(const QString &id)
{
    if (m_itemMap.contains(id)) {
        m_availableItemIds.removeAll(id);
        m_availableItemTitles.removeAll(m_itemMap.value(id).data()->title());

        if (!m_unavailableItemIds.contains(id))
            m_unavailableItemIds.append(id);

        emit availableItemsChanged();
        return m_itemMap.value(id).data();
    }
    return 0;
}

SideBarWidget *SideBar::insertSideBarWidget(int position, const QString &id)
{
    SideBarWidget *item = new SideBarWidget(this, id);
    connect(item, SIGNAL(splitMe()), this, SLOT(splitSubWidget()));
    connect(item, SIGNAL(closeMe()), this, SLOT(closeSubWidget()));
    connect(item, SIGNAL(currentWidgetChanged()), this, SLOT(updateWidgets()));
    insertWidget(position, item);
    m_widgets.insert(position, item);
    updateWidgets();
    return item;
}

void SideBar::removeSideBarWidget(SideBarWidget *widget)
{
    widget->removeCurrentItem();
    m_widgets.removeOne(widget);
    widget->hide();
    widget->deleteLater();
}

void SideBar::splitSubWidget()
{
    SideBarWidget *original = qobject_cast<SideBarWidget*>(sender());
    int pos = indexOf(original) + 1;
    insertSideBarWidget(pos);
    updateWidgets();
}

void SideBar::closeSubWidget()
{
    if (m_widgets.count() != 1) {
        SideBarWidget *widget = qobject_cast<SideBarWidget*>(sender());
        if (!widget)
            return;
        removeSideBarWidget(widget);
        updateWidgets();
    } else {
        if (m_closeWhenEmpty)
            setVisible(false);
    }
}

void SideBar::updateWidgets()
{
    foreach (SideBarWidget *i, m_widgets)
        i->updateAvailableItems();
}

void SideBar::saveSettings(QSettings *settings, const QString &name)
{
    const QString prefix = name.isEmpty() ? name : (name + QLatin1Char('/'));

    QStringList views;
    for (int i = 0; i < m_widgets.count(); ++i) {
        QString currentItemId = m_widgets.at(i)->currentItemId();
        if (!currentItemId.isEmpty())
            views.append(currentItemId);
    }
    if (views.isEmpty() && m_itemMap.size()) {
        QMapIterator<QString, QWeakPointer<SideBarItem> > iter(m_itemMap);
        iter.next();
        views.append(iter.key());
    }

    settings->setValue(prefix + "Views", views);
    settings->setValue(prefix + "Visible", true);
    settings->setValue(prefix + "VerticalPosition", saveState());
    settings->setValue(prefix + "Width", width());
}

void SideBar::closeAllWidgets()
{
    foreach (SideBarWidget *widget, m_widgets)
        removeSideBarWidget(widget);
}

void SideBar::readSettings(QSettings *settings, const QString &name)
{
    const QString prefix = name.isEmpty() ? name : (name + QLatin1Char('/'));

    closeAllWidgets();

    if (settings->contains(prefix + "Views")) {
        QStringList views = settings->value(prefix + "Views").toStringList();
        if (views.count()) {
            foreach (const QString &id, views)
                insertSideBarWidget(m_widgets.count(), id);

        } else {
            insertSideBarWidget(0);
        }
    } else {
        foreach (const QString &id, m_defaultVisible)
            insertSideBarWidget(m_widgets.count(), id);
    }

    if (settings->contains(prefix + "Visible"))
        setVisible(settings->value(prefix + "Visible").toBool());

    if (settings->contains(prefix + "VerticalPosition"))
        restoreState(settings->value(prefix + "VerticalPosition").toByteArray());

    if (settings->contains(prefix + "Width")) {
        QSize s = size();
        s.setWidth(settings->value(prefix + "Width").toInt());
        resize(s);
    }
}

void SideBar::activateItem(SideBarItem *item)
{
    QMap<QString, QWeakPointer<SideBarItem> >::const_iterator it = m_itemMap.constBegin();
    QString id;
    while (it != m_itemMap.constEnd()) {
        if (it.value().data() == item) {
            id = it.key();
            break;
        }
        ++it;
    }

    if (id.isEmpty())
        return;

    for (int i = 0; i < m_widgets.count(); ++i) {
        if (m_widgets.at(i)->currentItemId() == id) {
            item->widget()->setFocus();
            return;
        }
    }

    SideBarWidget *widget = m_widgets.first();
    widget->setCurrentItem(id);
    updateWidgets();
    item->widget()->setFocus();
}

void SideBar::setShortcutMap(const QMap<QString, Core::Command*> &shortcutMap)
{
    m_shortcutMap = shortcutMap;
}

QMap<QString, Core::Command*> SideBar::shortcutMap() const
{
    return m_shortcutMap;
}

SideBarWidget::SideBarWidget(SideBar *sideBar, const QString &id)
    : m_currentItem(0)
    , m_sideBar(sideBar)
{
    m_comboBox = new ComboBox(this);
    m_comboBox->setMinimumContentsLength(15);

    m_toolbar = new QToolBar(this);
    m_toolbar->setContentsMargins(0, 0, 0, 0);
    m_toolbar->addWidget(m_comboBox);

    m_splitButton = new QToolButton;
    m_splitButton->setIcon(QIcon(":/core/images/splitbutton_horizontal.png"));
    m_splitButton->setToolTip(tr("Split"));
    connect(m_splitButton, SIGNAL(clicked(bool)), this, SIGNAL(splitMe()));

    m_closeButton = new QToolButton;
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setToolTip(tr("Close"));

    connect(m_closeButton, SIGNAL(clicked(bool)), this, SIGNAL(closeMe()));

    QWidget *spacerItem = new QWidget(this);
    spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_toolbar->addWidget(spacerItem);
    m_splitAction = m_toolbar->addWidget(m_splitButton);
    m_toolbar->addWidget(m_closeButton);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setMargin(0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolbar);

    QStringList titleList = m_sideBar->availableItemTitles();
    qSort(titleList);
    QString t = id;
    if (titleList.count()) {
        foreach(const QString &itemTitle, titleList)
            m_comboBox->addItem(itemTitle, m_sideBar->idForTitle(itemTitle));

        m_comboBox->setCurrentIndex(0);
        if (t.isEmpty())
            t = m_comboBox->itemData(0, ComboBox::IdRole).toString();
    }
    setCurrentItem(t);

    connect(m_comboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(setCurrentIndex(int)));
}

SideBarWidget::~SideBarWidget()
{
}

QString SideBarWidget::currentItemTitle() const
{
    return m_comboBox->currentText();
}

QString SideBarWidget::currentItemId() const
{
    if (m_currentItem)
        return m_currentItem->id();
    return QString();
}

void SideBarWidget::setCurrentItem(const QString &id)
{
    if (!id.isEmpty()) {
        int idx = m_comboBox->findData(QVariant(id), ComboBox::IdRole);

        if (idx < 0)
            idx = 0;

        bool blocked = m_comboBox->blockSignals(true);
        m_comboBox->setCurrentIndex(idx);
        m_comboBox->blockSignals(blocked);
    }

    SideBarItem *item = m_sideBar->item(id);
    if (!item)
        return;
    removeCurrentItem();
    m_currentItem = item;

    layout()->addWidget(m_currentItem->widget());

    // Add buttons and remember their actions for later removal
    foreach (QToolButton *b, m_currentItem->createToolBarWidgets())
        m_addedToolBarActions.append(m_toolbar->insertWidget(m_splitAction, b));
}

void SideBarWidget::updateAvailableItems()
{
    bool blocked = m_comboBox->blockSignals(true);
    QString currentTitle = m_comboBox->currentText();
    m_comboBox->clear();
    QStringList titleList = m_sideBar->availableItemTitles();
    if (!currentTitle.isEmpty() && !titleList.contains(currentTitle))
        titleList.append(currentTitle);
    qSort(titleList);

    foreach(const QString &itemTitle, titleList)
        m_comboBox->addItem(itemTitle, m_sideBar->idForTitle(itemTitle));

    int idx = m_comboBox->findText(currentTitle);

    if (idx < 0)
        idx = 0;

    m_comboBox->setCurrentIndex(idx);
    m_splitButton->setEnabled(titleList.count() > 1);
    m_comboBox->blockSignals(blocked);
}

void SideBarWidget::removeCurrentItem()
{
    if (!m_currentItem)
        return;

    QWidget *w = m_currentItem->widget();
    layout()->removeWidget(w);
    w->setParent(0);
    m_sideBar->makeItemAvailable(m_currentItem);

    // Delete custom toolbar widgets
    qDeleteAll(m_addedToolBarActions);
    m_addedToolBarActions.clear();

    m_currentItem = 0;
}

void SideBarWidget::setCurrentIndex(int)
{
    setCurrentItem(m_comboBox->itemData(m_comboBox->currentIndex(),
                                        ComboBox::IdRole).toString());
    emit currentWidgetChanged();
}

Core::Command *SideBarWidget::command(const QString &id) const
{
    const QMap<QString, Core::Command*> shortcutMap = m_sideBar->shortcutMap();
    QMap<QString, Core::Command*>::const_iterator r = shortcutMap.find(id);
    if (r != shortcutMap.end())
        return r.value();
    return 0;
}

ComboBox::ComboBox(SideBarWidget *sideBarWidget)
    : m_sideBarWidget(sideBarWidget)
{
}

bool ComboBox::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        QString txt = currentText();
        Core::Command *cmd = m_sideBarWidget->command(txt);
        if (cmd) {
            txt = tr("Activate %1").arg(txt);
            setToolTip(cmd->stringWithAppendedShortcut(txt));
        } else {
            setToolTip(txt);
        }
    }
    return QComboBox::event(e);
}
