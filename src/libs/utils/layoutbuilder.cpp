/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "layoutbuilder.h"

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QGridLayout>
#include <QWidget>

namespace Utils {

LayoutBuilder::LayoutBuilder(QWidget *parent, LayoutType layoutType)
{
    if (layoutType == FormLayout) {
        m_formLayout = new QFormLayout(parent);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    } else {
        m_gridLayout = new QGridLayout(parent);
        m_gridLayout->setContentsMargins(0, 0, 0, 0);
    }
}

LayoutBuilder::LayoutBuilder(QLayout *layout)
{
    if (auto fl = qobject_cast<QFormLayout *>(layout)) {
        m_formLayout = fl;
    } else if (auto grid = qobject_cast<QGridLayout *>(layout)) {
        m_gridLayout = grid;
        m_currentGridRow = grid->rowCount();
        m_currentGridColumn = 0;
    }
}

LayoutBuilder::~LayoutBuilder()
{
    flushPendingFormItems();
}

LayoutBuilder &LayoutBuilder::startNewRow()
{
    if (m_formLayout)
        flushPendingFormItems();
    if (m_gridLayout) {
        if (m_currentGridColumn != 0) {
            ++m_currentGridRow;
            m_currentGridColumn = 0;
        }
    }
    return *this;
}

LayoutBuilder &LayoutBuilder::addRow(const LayoutItem &item)
{
    startNewRow();
    addItem(item);
    return *this;
}

void LayoutBuilder::flushPendingFormItems()
{
    QTC_ASSERT(m_formLayout, return);

    if (m_pendingFormItems.isEmpty())
        return;

    // If there are more than two items, we cram the last ones in one hbox.
    if (m_pendingFormItems.size() > 2) {
        auto hbox = new QHBoxLayout;
        hbox->setContentsMargins(0, 0, 0, 0);
        for (int i = 1; i < m_pendingFormItems.size(); ++i) {
            if (QWidget *w = m_pendingFormItems.at(i).widget)
                hbox->addWidget(w);
            else if (QLayout *l = m_pendingFormItems.at(i).layout)
                hbox->addItem(l);
            else
                QTC_CHECK(false);
        }
        while (m_pendingFormItems.size() >= 2)
            m_pendingFormItems.takeLast();
        m_pendingFormItems.append(LayoutItem(hbox));
    }

    if (m_pendingFormItems.size() == 1) { // One one item given, so this spans both columns.
        if (auto layout = m_pendingFormItems.at(0).layout)
            m_formLayout->addRow(layout);
        else if (auto widget = m_pendingFormItems.at(0).widget)
            m_formLayout->addRow(widget);
    } else if (m_pendingFormItems.size() == 2) { // Normal case, both columns used.
        if (auto label = m_pendingFormItems.at(0).widget) {
            if (auto layout = m_pendingFormItems.at(1).layout)
                m_formLayout->addRow(label, layout);
            else if (auto widget = m_pendingFormItems.at(1).widget)
                m_formLayout->addRow(label, widget);
        } else  {
            if (auto layout = m_pendingFormItems.at(1).layout)
                m_formLayout->addRow(m_pendingFormItems.at(0).text, layout);
            else if (auto widget = m_pendingFormItems.at(1).widget)
                m_formLayout->addRow(m_pendingFormItems.at(0).text, widget);
        }
    } else {
        QTC_CHECK(false);
    }

    m_pendingFormItems.clear();
}

QLayout *LayoutBuilder::layout() const
{
    if (m_formLayout)
        return m_formLayout;
    return m_gridLayout;
}

LayoutBuilder &LayoutBuilder::addItem(LayoutItem item)
{
    if (item.widget && !item.widget->parent())
        item.widget->setParent(layout()->parentWidget());

    if (item.aspect) {
        item.aspect->addToLayout(*this);
    } else {
        if (m_gridLayout) {
            if (auto widget = item.widget)
                m_gridLayout->addWidget(widget, m_currentGridRow, m_currentGridColumn, 1, item.span, item.align);
            m_currentGridColumn += item.span;
        } else {
            m_pendingFormItems.append(item);
        }
    }
    return *this;
}

} // Utils
