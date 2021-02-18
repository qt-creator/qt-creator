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
class QBoxLayout;
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
    enum LayoutType {
        Form,              // Plain QFormLayout, without contentMargins
        Grid,              // Plain QGridLayout, without contentMargins
        HBox,              // Plain QHBoxLayout, without contentMargins
        VBox,              // Plain QVBoxLayout, without contentMargins
        HBoxWithMargins,   // QHBoxLayout with margins
        VBoxWithMargins,   // QVBoxLayout with margins
        // Compat
        FormLayout = Form, // FIXME: Remove
        GridLayout = Grid, // FIXME: Remove
    };
    enum Alignment { DefaultAlignment, AlignAsFormLabel };

    class QTCREATOR_UTILS_EXPORT LayoutItem
    {
    public:
        LayoutItem();
        LayoutItem(QLayout *layout, int span = 1, Alignment align = {});
        LayoutItem(QWidget *widget, int span = 1, Alignment align = {});
        LayoutItem(BaseAspect *aspect, int span = 1, Alignment align = {}); // Remove
        LayoutItem(BaseAspect &aspect, int span = 1, Alignment align = {});
        LayoutItem(const QString &text, int span = 1, Alignment align = {});
        LayoutItem(const LayoutBuilder &builder, int span = 1, Alignment align = {});

        QLayout *layout = nullptr;
        QWidget *widget = nullptr;
        BaseAspect *aspect = nullptr;
        QString text;
        int span = 1;
        Alignment align;
        int space = 0;
        int stretch = 0;
        bool linebreak = false;
    };

    using LayoutItems = QList<LayoutItem>;

    class QTCREATOR_UTILS_EXPORT Space : public LayoutItem
    {
    public:
        explicit Space(int space_) { space = space_; }
    };

    class QTCREATOR_UTILS_EXPORT Stretch : public LayoutItem
    {
    public:
        explicit Stretch(int stretch_ = 1) { stretch = stretch_; }
    };

    class QTCREATOR_UTILS_EXPORT Break : public LayoutItem
    {
    public:
        Break() { linebreak = true; }
    };

    explicit LayoutBuilder(QWidget *parent, LayoutType layoutType = Form);
    explicit LayoutBuilder(QLayout *layout); // Adds to existing layout.
    explicit LayoutBuilder(LayoutType layoutType, const LayoutItems &items = {});

    LayoutBuilder(const LayoutBuilder &) = delete;
    LayoutBuilder(LayoutBuilder &&) = default;
    LayoutBuilder &operator=(const LayoutBuilder &) = delete;
    LayoutBuilder &operator=(LayoutBuilder &&) = default;

    ~LayoutBuilder();

    LayoutBuilder &addItem(const LayoutItem &item);
    LayoutBuilder &addItems(const LayoutItems &items);

    LayoutBuilder &finishRow();
    LayoutBuilder &addRow(const LayoutItem &item);
    LayoutBuilder &addRow(const LayoutItems &items);

    QLayout *layout() const;
    QWidget *parentWidget() const;

    void attachTo(QWidget *w, bool stretchAtBottom = true);

private:
    void flushPendingFormItems();
    void init(QWidget *parent, LayoutType layoutType);

    QFormLayout *m_formLayout = nullptr;
    QGridLayout *m_gridLayout = nullptr;
    QBoxLayout *m_boxLayout = nullptr;
    LayoutItems m_pendingFormItems;
    int m_currentGridRow = 0;
    int m_currentGridColumn = 0;
};

namespace Layouting {

class QTCREATOR_UTILS_EXPORT Group : public LayoutBuilder
{
public:
    Group(std::initializer_list<LayoutBuilder::LayoutItem> items);

    Group &withTitle(const QString &title);
};

class QTCREATOR_UTILS_EXPORT Box : public LayoutBuilder
{
public:
    Box(LayoutType type, const LayoutItems &items);
};

class QTCREATOR_UTILS_EXPORT Column : public Box
{
public:
    Column(std::initializer_list<LayoutItem> items)
        : Box(VBox, items)
    {}
};

class QTCREATOR_UTILS_EXPORT Row : public Box
{
public:
    Row(std::initializer_list<LayoutItem> items)
        : Box(HBox, items)
    {}
};

class QTCREATOR_UTILS_EXPORT Grid : public Box
{
public:
    Grid(std::initializer_list<LayoutItem> items)
        : Box(GridLayout, items)
    {}
};

using Stretch = LayoutBuilder::Stretch;
using Space = LayoutBuilder::Space;
using Break = LayoutBuilder::Break;

}
} // namespace Utils
