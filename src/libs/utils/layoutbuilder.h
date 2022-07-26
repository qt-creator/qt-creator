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

#include "optional.h"

#include <QList>
#include <QString>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QLayout;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

class BaseAspect;
class BoolAspect;

namespace Layouting {

enum AttachType {
    WithMargins,
    WithoutMargins,
};

} // Layouting

class QTCREATOR_UTILS_EXPORT LayoutBuilder
{
public:
    enum LayoutType {
        HBoxLayout,
        VBoxLayout,
        FormLayout,
        GridLayout,
    };

    enum class AlignmentType {
        DefaultAlignment,
        AlignAsFormLabel,
    };

    enum class SpecialType {
        NotSpecial,
        Space,
        Stretch,
        Break,
    };

    using Modifier = std::function<void(QLayout *)>;

    class QTCREATOR_UTILS_EXPORT LayoutItem
    {
    public:
        LayoutItem();
        LayoutItem(QLayout *layout);
        LayoutItem(QWidget *widget);
        LayoutItem(BaseAspect *aspect); // Remove
        LayoutItem(BaseAspect &aspect);
        LayoutItem(const QString &text);
        LayoutItem(const LayoutBuilder &builder);

        QLayout *layout = nullptr;
        QWidget *widget = nullptr;
        BaseAspect *aspect = nullptr;

        QString text; // FIXME: Use specialValue for that
        int span = 1;
        AlignmentType align = AlignmentType::DefaultAlignment;
        SpecialType specialType = SpecialType::NotSpecial;
        QVariant specialValue;
    };

    using LayoutItems = QList<LayoutItem>;

    explicit LayoutBuilder(LayoutType layoutType, const LayoutItems &items = {});

    LayoutBuilder(const LayoutBuilder &) = delete;
    LayoutBuilder(LayoutBuilder &&) = default;
    LayoutBuilder &operator=(const LayoutBuilder &) = delete;
    LayoutBuilder &operator=(LayoutBuilder &&) = default;

    ~LayoutBuilder();

    LayoutBuilder &setSpacing(int spacing);

    LayoutBuilder &addItem(const LayoutItem &item);
    LayoutBuilder &addItems(const LayoutItems &items);

    LayoutBuilder &finishRow();
    LayoutBuilder &addRow(const LayoutItem &item);
    LayoutBuilder &addRow(const LayoutItems &items);

    LayoutType layoutType() const { return m_layoutType; }

    void attachTo(QWidget *w, Layouting::AttachType attachType = Layouting::WithMargins) const;
    QWidget *emerge(Layouting::AttachType attachType = Layouting::WithMargins);

    class QTCREATOR_UTILS_EXPORT Space : public LayoutItem
    {
    public:
        explicit Space(int space);
    };

    class QTCREATOR_UTILS_EXPORT Span : public LayoutItem
    {
    public:
        Span(int span, const LayoutItem &item);
    };

    class QTCREATOR_UTILS_EXPORT AlignAsFormLabel : public LayoutItem
    {
    public:
        AlignAsFormLabel(const LayoutItem &item);
    };

    class QTCREATOR_UTILS_EXPORT Stretch : public LayoutItem
    {
    public:
        explicit Stretch(int stretch = 1);
    };

    class QTCREATOR_UTILS_EXPORT Break : public LayoutItem
    {
    public:
        Break();
    };

    using Setter = std::function<void(QObject *target)>;

    class QTCREATOR_UTILS_EXPORT Setters : public std::vector<Setter>
    {
    public:
        using std::vector<Setter>::vector;
        Setters(const Setter &setter) { push_back(setter); }
    };

protected:
    explicit LayoutBuilder(); // Adds to existing layout.

    QLayout *createLayout() const;
    void doLayout(QWidget *parent, Layouting::AttachType attachType) const;

    LayoutItems m_items;
    LayoutType m_layoutType;
    Utils::optional<int> m_spacing;
};

class QTCREATOR_UTILS_EXPORT LayoutExtender : public LayoutBuilder
{
public:
    explicit LayoutExtender(QLayout *layout);
    ~LayoutExtender();

private:
    QLayout *m_layout = nullptr;
};

namespace Layouting {

QTCREATOR_UTILS_EXPORT LayoutBuilder::Setter title(const QString &title,
                                                   BoolAspect *checker = nullptr);

class QTCREATOR_UTILS_EXPORT Group : public LayoutBuilder::LayoutItem
{
public:
    explicit Group(const LayoutBuilder &innerLayout);
    Group(const LayoutBuilder::Setters &setters, const LayoutBuilder &innerLayout);
};

class QTCREATOR_UTILS_EXPORT Column : public LayoutBuilder
{
public:
    Column() : LayoutBuilder(VBoxLayout) {}
    Column(std::initializer_list<LayoutItem> items) : LayoutBuilder(VBoxLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Row : public LayoutBuilder
{
public:
    Row() : LayoutBuilder(HBoxLayout) {}
    Row(std::initializer_list<LayoutItem> items) : LayoutBuilder(HBoxLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Grid : public LayoutBuilder
{
public:
    Grid() : LayoutBuilder(GridLayout) {}
    Grid(std::initializer_list<LayoutItem> items) : LayoutBuilder(GridLayout, items) {}
};

class QTCREATOR_UTILS_EXPORT Form : public LayoutBuilder
{
public:
    Form() : LayoutBuilder(FormLayout) {}
    Form(std::initializer_list<LayoutItem> items) : LayoutBuilder(FormLayout, items) {}
};

using Space = LayoutBuilder::Space;
using Span = LayoutBuilder::Span;
using AlignAsFormLabel = LayoutBuilder::AlignAsFormLabel;

using Stretch = LayoutBuilder::Stretch; // FIXME: Remove
using Break = LayoutBuilder::Break; // FIXME: Remove

QTCREATOR_UTILS_EXPORT extern LayoutBuilder::Break br;
QTCREATOR_UTILS_EXPORT extern LayoutBuilder::Stretch st;
QTCREATOR_UTILS_EXPORT extern LayoutBuilder::Space empty;

} // Layouting
} // Utils
