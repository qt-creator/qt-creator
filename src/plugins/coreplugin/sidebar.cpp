/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "sidebar.h"
#include "imode.h"
#include "modemanager.h"

#include "actionmanager/actionmanager.h"

#include <QtCore/QDebug>
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
                 QList<SideBarItem*> defaultVisible)
{
    setOrientation(Qt::Vertical);
    foreach (SideBarItem *item, itemList) {
        const QString title = item->widget()->windowTitle();
        m_itemMap.insert(title, item);
        if (defaultVisible.contains(item))
            m_defaultVisible.append(title);
    }

    m_availableItems = m_itemMap.keys();
}

SideBar::~SideBar()
{
    qDeleteAll(m_itemMap.values());
}

QStringList SideBar::availableItems() const
{
    return m_availableItems;
}

void SideBar::makeItemAvailable(SideBarItem *item)
{
    QMap<QString, SideBarItem*>::const_iterator it = m_itemMap.constBegin();
    while (it != m_itemMap.constEnd()) {
        if (it.value() == item) {
            m_availableItems.append(it.key());
            qSort(m_availableItems);
            break;
        }
        ++it;
    }
}

SideBarItem *SideBar::item(const QString &title)
{
    if (m_itemMap.contains(title)) {
        m_availableItems.removeAll(title);
        return m_itemMap.value(title);
    }
    return 0;
}

SideBarWidget *SideBar::insertSideBarWidget(int position, const QString &title)
{
    SideBarWidget *item = new SideBarWidget(this, title);
    connect(item, SIGNAL(split()), this, SLOT(split()));
    connect(item, SIGNAL(close()), this, SLOT(close()));
    connect(item, SIGNAL(currentWidgetChanged()), this, SLOT(updateWidgets()));
    insertWidget(position, item);
    m_widgets.insert(position, item);
    updateWidgets();
    return item;
}

void SideBar::split()
{
    SideBarWidget *original = qobject_cast<SideBarWidget*>(sender());
    int pos = indexOf(original) + 1;
    insertSideBarWidget(pos);
    updateWidgets();
}

void SideBar::close()
{
    if (m_widgets.count() != 1) {
        SideBarWidget *widget = qobject_cast<SideBarWidget*>(sender());
        if (!widget)
            return;
        widget->removeCurrentItem();
        m_widgets.removeOne(widget);
        widget->hide();
        widget->deleteLater();
        updateWidgets();
    }
}

void SideBar::updateWidgets()
{
    foreach (SideBarWidget *i, m_widgets)
        i->updateAvailableItems();
}

void SideBar::saveSettings(QSettings *settings)
{
    QStringList views;
    for (int i = 0; i < m_widgets.count(); ++i)
        views.append(m_widgets.at(i)->currentItemTitle());
    settings->setValue("HelpSideBar/Views", views);
    settings->setValue("HelpSideBar/Visible", true);//isVisible());
    settings->setValue("HelpSideBar/VerticalPosition", saveState());
    settings->setValue("HelpSideBar/Width", width());
}

void SideBar::readSettings(QSettings *settings)
{
    if (settings->contains("HelpSideBar/Views")) {
        QStringList views = settings->value("HelpSideBar/Views").toStringList();
        if (views.count()) {
            foreach (const QString &title, views)
                insertSideBarWidget(m_widgets.count(), title);
        } else {
            insertSideBarWidget(0);
        }
    } else {
        foreach (const QString &title, m_defaultVisible)
            insertSideBarWidget(0, title);
    }

    if (settings->contains("HelpSideBar/Visible"))
        setVisible(settings->value("HelpSideBar/Visible").toBool());

    if (settings->contains("HelpSideBar/VerticalPosition"))
        restoreState(settings->value("HelpSideBar/VerticalPosition").toByteArray());

    if (settings->contains("HelpSideBar/Width")) {
        QSize s = size();
        s.setWidth(settings->value("HelpSideBar/Width").toInt());
        resize(s);
    }
}

void SideBar::activateItem(SideBarItem *item)
{
    QMap<QString, SideBarItem*>::const_iterator it = m_itemMap.constBegin();
    QString title;
    while (it != m_itemMap.constEnd()) {
        if (it.value() == item) {
            title = it.key();
            break;
        }
        ++it;
    }

    if (title.isEmpty())
        return;

    for (int i = 0; i < m_widgets.count(); ++i) {
        if (m_widgets.at(i)->currentItemTitle() == title) {
            item->widget()->setFocus();
            return;
        }
    }

    SideBarWidget *widget = m_widgets.first();
    widget->setCurrentItem(title);
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



SideBarWidget::SideBarWidget(SideBar *sideBar, const QString &title)
    : m_currentItem(0)
    , m_sideBar(sideBar)
{
    m_comboBox = new ComboBox(this);
    m_comboBox->setMinimumContentsLength(15);

    m_toolbar = new QToolBar(this);
    m_toolbar->setContentsMargins(0, 0, 0, 0);
    m_toolbar->addWidget(m_comboBox);

    m_splitButton = new QToolButton;
    m_splitButton->setProperty("type", QLatin1String("dockbutton"));
    m_splitButton->setIcon(QIcon(":/core/images/splitbutton_horizontal.png"));
    m_splitButton->setToolTip(tr("Split"));
    connect(m_splitButton, SIGNAL(clicked(bool)), this, SIGNAL(split()));

    m_closeButton = new QToolButton;
    m_closeButton->setProperty("type", QLatin1String("dockbutton"));
    m_closeButton->setIcon(QIcon(":/core/images/closebutton.png"));
    m_closeButton->setToolTip(tr("Close"));

    connect(m_closeButton, SIGNAL(clicked(bool)), this, SIGNAL(close()));

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

    const QStringList lst = m_sideBar->availableItems();
    QString t = title;
    if (lst.count()) {
        m_comboBox->addItems(lst);
        m_comboBox->setCurrentIndex(0);
        if (t.isEmpty())
            t = m_comboBox->currentText();
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

void SideBarWidget::setCurrentItem(const QString &title)
{
    if (!title.isEmpty()) {
        int idx = m_comboBox->findText(title);
        if (idx < 0)
            idx = 0;
        m_comboBox->blockSignals(true);
        m_comboBox->setCurrentIndex(idx);
        m_comboBox->blockSignals(false);
    }

    SideBarItem *item = m_sideBar->item(title);
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
    m_comboBox->blockSignals(true);
    QString current = m_comboBox->currentText();
    m_comboBox->clear();
    QStringList itms = m_sideBar->availableItems();
    if (!current.isEmpty() && !itms.contains(current))
        itms.append(current);
    qSort(itms);
    m_comboBox->addItems(itms);
    int idx = m_comboBox->findText(current);
    if (idx < 0)
        idx = 0;
    m_comboBox->setCurrentIndex(idx);
    m_splitButton->setEnabled(itms.count() > 1);
    m_comboBox->blockSignals(false);
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
    setCurrentItem(m_comboBox->currentText());
    emit currentWidgetChanged();
}

Core::Command *SideBarWidget::command(const QString &title) const
{
    const QMap<QString, Core::Command*> shortcutMap = m_sideBar->shortcutMap();
    QMap<QString, Core::Command*>::const_iterator r = shortcutMap.find(title);
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
