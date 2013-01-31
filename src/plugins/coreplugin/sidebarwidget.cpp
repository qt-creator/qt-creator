/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "sidebarwidget.h"
#include "sidebar.h"
#include "navigationsubwidget.h"

#include <coreplugin/coreconstants.h>

#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QVBoxLayout>

namespace Core {
namespace Internal {

class SideBarComboBox : public CommandComboBox
{
public:
    enum DataRoles {
        IdRole = Qt::UserRole
    };

    explicit SideBarComboBox(SideBarWidget *sideBarWidget) : m_sideBarWidget(sideBarWidget) {}

private:
    virtual const Core::Command *command(const QString &text) const
        { return m_sideBarWidget->command(text); }

    SideBarWidget *m_sideBarWidget;
};

SideBarWidget::SideBarWidget(SideBar *sideBar, const QString &id)
    : m_currentItem(0)
    , m_sideBar(sideBar)
{
    m_comboBox = new SideBarComboBox(this);
    m_comboBox->setMinimumContentsLength(15);

    m_toolbar = new QToolBar(this);
    m_toolbar->setContentsMargins(0, 0, 0, 0);
    m_toolbar->addWidget(m_comboBox);

    QWidget *spacerItem = new QWidget(this);
    spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_toolbar->addWidget(spacerItem);

    m_splitAction = new QAction(tr("Split"), m_toolbar);
    m_splitAction->setToolTip(tr("Split"));
    m_splitAction->setIcon(QIcon(QLatin1String(Constants::ICON_SPLIT_HORIZONTAL)));
    connect(m_splitAction, SIGNAL(triggered()), this, SIGNAL(splitMe()));
    m_toolbar->addAction(m_splitAction);

    QAction *closeAction = new QAction(tr("Close"), m_toolbar);
    closeAction->setToolTip(tr("Close"));
    closeAction->setIcon(QIcon(QLatin1String(Constants::ICON_CLOSE_DOCUMENT)));
    connect(closeAction, SIGNAL(triggered()), this, SIGNAL(closeMe()));
    m_toolbar->addAction(closeAction);

    QVBoxLayout *lay = new QVBoxLayout();
    lay->setMargin(0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolbar);

    QStringList titleList = m_sideBar->availableItemTitles();
    qSort(titleList);
    QString t = id;
    if (titleList.count()) {
        foreach (const QString &itemTitle, titleList)
            m_comboBox->addItem(itemTitle, m_sideBar->idForTitle(itemTitle));

        m_comboBox->setCurrentIndex(0);
        if (t.isEmpty())
            t = m_comboBox->itemData(0, SideBarComboBox::IdRole).toString();
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
        int idx = m_comboBox->findData(QVariant(id), SideBarComboBox::IdRole);

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
    m_currentItem->widget()->show();

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

    foreach (const QString &itemTitle, titleList)
        m_comboBox->addItem(itemTitle, m_sideBar->idForTitle(itemTitle));

    int idx = m_comboBox->findText(currentTitle);

    if (idx < 0)
        idx = 0;

    m_comboBox->setCurrentIndex(idx);
    m_splitAction->setEnabled(titleList.count() > 1);
    m_comboBox->blockSignals(blocked);
}

void SideBarWidget::removeCurrentItem()
{
    if (!m_currentItem)
        return;

    QWidget *w = m_currentItem->widget();
    w->hide();
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
                                        SideBarComboBox::IdRole).toString());
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

} // namespace Internal
} // namespace Core
