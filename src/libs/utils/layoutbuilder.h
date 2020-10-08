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

#pragma once

#include "utils_global.h"

#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QGridLayout;
class QLayout;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

class BaseAspect;

class QTCREATOR_UTILS_EXPORT LayoutBuilder
{
public:
    enum LayoutType { GridLayout, FormLayout };
    enum Alignment { DefaultAlignment, AlignAsFormLabel };

    explicit LayoutBuilder(QWidget *parent, LayoutType layoutType = FormLayout);
    explicit LayoutBuilder(QLayout *layout); // Adds to existing layout.

    ~LayoutBuilder();

    class QTCREATOR_UTILS_EXPORT LayoutItem
    {
    public:
        LayoutItem();
        LayoutItem(QLayout *layout, int span = 1, Alignment align = {});
        LayoutItem(QWidget *widget, int span = 1, Alignment align = {});
        LayoutItem(BaseAspect *aspect);
        LayoutItem(const QString &text);

        QLayout *layout = nullptr;
        QWidget *widget = nullptr;
        BaseAspect *aspect = nullptr;
        QString text;
        int span = 1;
        Alignment align;
    };

    LayoutBuilder &addItem(const LayoutItem &item);
    LayoutBuilder &addItems(const QList<LayoutItem> &items);

    LayoutBuilder &finishRow();
    LayoutBuilder &addRow(const LayoutItem &item);
    LayoutBuilder &addRow(const QList<LayoutItem> &items);

    QLayout *layout() const;

private:
    void flushPendingFormItems();

    QFormLayout *m_formLayout = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QList<LayoutItem> m_pendingFormItems;
    int m_currentGridRow = 0;
    int m_currentGridColumn = 0;
};

} // namespace Utils
