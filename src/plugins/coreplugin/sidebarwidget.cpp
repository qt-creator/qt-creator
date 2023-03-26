// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sidebarwidget.h"

#include "coreplugintr.h"
#include "sidebar.h"
#include "navigationsubwidget.h"

#include <utils/algorithm.h>
#include <utils/utilsicons.h>

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
    const Command *command(const QString &text) const override
        { return m_sideBarWidget->command(text); }

    SideBarWidget *m_sideBarWidget;
};

SideBarWidget::SideBarWidget(SideBar *sideBar, const QString &id)
    : m_sideBar(sideBar)
{
    m_comboBox = new SideBarComboBox(this);
    m_comboBox->setMinimumContentsLength(15);

    m_toolbar = new QToolBar(this);
    m_toolbar->setContentsMargins(0, 0, 0, 0);
    m_toolbar->addWidget(m_comboBox);

    QWidget *spacerItem = new QWidget(this);
    spacerItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_toolbar->addWidget(spacerItem);

    m_splitAction = new QAction(Tr::tr("Split"), m_toolbar);
    m_splitAction->setToolTip(Tr::tr("Split"));
    m_splitAction->setIcon(Utils::Icons::SPLIT_HORIZONTAL_TOOLBAR.icon());
    connect(m_splitAction, &QAction::triggered, this, &SideBarWidget::splitMe);
    m_toolbar->addAction(m_splitAction);

    m_closeAction = new QAction(Tr::tr("Close"), m_toolbar);
    m_closeAction->setToolTip(Tr::tr("Close"));
    m_closeAction->setIcon(Utils::Icons::CLOSE_SPLIT_BOTTOM.icon());
    connect(m_closeAction, &QAction::triggered, this, &SideBarWidget::closeMe);
    m_toolbar->addAction(m_closeAction);

    auto lay = new QVBoxLayout();
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    setLayout(lay);
    lay->addWidget(m_toolbar);

    const QStringList titleList = Utils::sorted(m_sideBar->availableItemTitles());
    QString t = id;
    if (!titleList.isEmpty()) {
        for (const QString &itemTitle : titleList)
            m_comboBox->addItem(itemTitle, m_sideBar->idForTitle(itemTitle));

        m_comboBox->setCurrentIndex(0);
        if (t.isEmpty())
            t = m_comboBox->itemData(0, SideBarComboBox::IdRole).toString();
    }
    setCurrentItem(t);

    connect(m_comboBox, &QComboBox::currentIndexChanged, this, &SideBarWidget::setCurrentIndex);
}

SideBarWidget::~SideBarWidget() = default;

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

        QSignalBlocker blocker(m_comboBox);
        m_comboBox->setCurrentIndex(idx);
    }

    SideBarItem *item = m_sideBar->item(id);
    if (!item)
        return;
    removeCurrentItem();
    m_currentItem = item;

    layout()->addWidget(m_currentItem->widget());
    m_currentItem->widget()->show();

    // Add buttons and remember their actions for later removal
    QList<QToolButton *> buttons = m_currentItem->createToolBarWidgets();
    for (QToolButton *b : buttons)
        m_addedToolBarActions.append(m_toolbar->insertWidget(m_splitAction, b));
}

void SideBarWidget::updateAvailableItems()
{
    QSignalBlocker blocker(m_comboBox);
    QString currentTitle = m_comboBox->currentText();
    m_comboBox->clear();
    QStringList titleList = m_sideBar->availableItemTitles();
    if (!currentTitle.isEmpty() && !titleList.contains(currentTitle))
        titleList.append(currentTitle);
    Utils::sort(titleList);

    for (const QString &itemTitle : std::as_const(titleList))
        m_comboBox->addItem(itemTitle, m_sideBar->idForTitle(itemTitle));

    int idx = m_comboBox->findText(currentTitle);

    if (idx < 0)
        idx = 0;

    m_comboBox->setCurrentIndex(idx);
    m_splitAction->setEnabled(titleList.count() > 1);
}

void SideBarWidget::removeCurrentItem()
{
    if (!m_currentItem)
        return;

    QWidget *w = m_currentItem->widget();
    w->hide();
    layout()->removeWidget(w);
    w->setParent(nullptr);
    m_sideBar->makeItemAvailable(m_currentItem);

    // Delete custom toolbar widgets
    qDeleteAll(m_addedToolBarActions);
    m_addedToolBarActions.clear();

    m_currentItem = nullptr;
}

void SideBarWidget::setCurrentIndex(int)
{
    setCurrentItem(m_comboBox->itemData(m_comboBox->currentIndex(),
                                        SideBarComboBox::IdRole).toString());
    emit currentWidgetChanged();
}

Command *SideBarWidget::command(const QString &title) const
{
    const QString id = m_sideBar->idForTitle(title);
    if (id.isEmpty())
        return nullptr;
    const QMap<QString, Command*> shortcutMap = m_sideBar->shortcutMap();
    QMap<QString, Command*>::const_iterator r = shortcutMap.find(id);
    if (r != shortcutMap.end())
        return r.value();
    return nullptr;
}

void SideBarWidget::setCloseIcon(const QIcon &icon)
{
    m_closeAction->setIcon(icon);
}

} // namespace Internal
} // namespace Core
