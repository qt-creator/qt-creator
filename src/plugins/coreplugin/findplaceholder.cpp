// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findplaceholder.h"
#include "find/findtoolbar.h"

#include <QVBoxLayout>

using namespace Core;

FindToolBarPlaceHolder *FindToolBarPlaceHolder::m_current = nullptr;

static QList<FindToolBarPlaceHolder *> g_findToolBarPlaceHolders;

FindToolBarPlaceHolder::FindToolBarPlaceHolder(QWidget *owner, QWidget *parent)
    : QWidget(parent), m_owner(owner), m_subWidget(nullptr)
{
    g_findToolBarPlaceHolders.append(this);
    setLayout(new QVBoxLayout);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    layout()->setContentsMargins(0, 0, 0, 0);
}

FindToolBarPlaceHolder::~FindToolBarPlaceHolder()
{
    g_findToolBarPlaceHolders.removeOne(this);
    if (m_subWidget) {
        m_subWidget->setVisible(false);
        m_subWidget->setParent(nullptr);
    }
    if (m_current == this)
        m_current = nullptr;
}

const QList<FindToolBarPlaceHolder *> FindToolBarPlaceHolder::allFindToolbarPlaceHolders()
{
    return g_findToolBarPlaceHolders;
}

QWidget *FindToolBarPlaceHolder::owner() const
{
    return m_owner;
}

/*!
 * Returns if \a widget is a subwidget of the place holder's owner
 */
bool FindToolBarPlaceHolder::isUsedByWidget(QWidget *widget)
{
    QWidget *current = widget;
    while (current) {
        if (current == m_owner)
            return true;
        current = current->parentWidget();
    }
    return false;
}

void FindToolBarPlaceHolder::setWidget(Internal::FindToolBar *widget)
{
    if (m_subWidget) {
        m_subWidget->setVisible(false);
        m_subWidget->setParent(nullptr);
    }
    m_subWidget = widget;
    if (m_subWidget) {
        m_subWidget->setLightColored(m_lightColored);
        m_subWidget->setLightColoredIcon(m_lightColored);
        layout()->addWidget(m_subWidget);
    }
}

FindToolBarPlaceHolder *FindToolBarPlaceHolder::getCurrent()
{
    return m_current;
}

void FindToolBarPlaceHolder::setCurrent(FindToolBarPlaceHolder *placeHolder)
{
    m_current = placeHolder;
}

void FindToolBarPlaceHolder::setLightColored(bool lightColored)
{
    m_lightColored = lightColored;
}

bool FindToolBarPlaceHolder::isLightColored() const
{
    return m_lightColored;
}
