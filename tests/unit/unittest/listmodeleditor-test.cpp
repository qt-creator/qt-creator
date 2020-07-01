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

#include "googletest.h"

#include "mocklistmodeleditorview.h"

#include <qmldesigner/components/listmodeleditor/listmodeleditormodel.h>
#include <qmldesigner/designercore/include/abstractview.h>
#include <qmldesigner/designercore/include/model.h>
#include <qmldesigner/designercore/include/nodelistproperty.h>
#include <qmldesigner/designercore/include/variantproperty.h>

namespace {

using QmlDesigner::AbstractProperty;
using QmlDesigner::AbstractView;
using QmlDesigner::ModelNode;

MATCHER_P2(HasItem,
           name,
           value,
           std::string(negation ? "hasn't " : "has ") + "(" + name + ", " + value + ")")
{
    QStandardItem *item = arg;

    return item->data(Qt::UserRole).toString() == name && item->data(Qt::UserRole).toDouble() == value;
}

MATCHER(IsInvalid, std::string(negation ? "isn't null" : "is null"))
{
    return !arg.isValid();
}

MATCHER_P3(IsVariantProperty,
           node,
           name,
           value,
           std::string(negation ? "isn't " : "is ") + "(" + name + ", " + PrintToString(value) + ")")
{
    const QmlDesigner::VariantProperty &property = arg;

    return property.parentModelNode() == node && property.name() == name && property.value() == value;
}

MATCHER_P2(IsVariantProperty,
           name,
           value,
           std::string(negation ? "isn't " : "is ") + "(" + name + ", " + PrintToString(value) + ")")
{
    const QmlDesigner::VariantProperty &property = arg;

    return property.name() == name && property.value() == value;
}

MATCHER_P2(IsAbstractProperty, node, name, std::string(negation ? "isn't " : "is ") + "(" + name + ")")
{
    const QmlDesigner::AbstractProperty &property = arg;

    return property.parentModelNode() == node && property.name() == name;
}

class ListModelEditor : public testing::Test
{
public:
    ListModelEditor()
    {
        designerModel->attachView(&mockView);

        emptyListModelNode = mockView.createModelNode("QtQml.Models.ListModel", 2, 15);

        listModelNode = mockView.createModelNode("QtQml.Models.ListModel", 2, 15);
        mockView.rootModelNode().defaultNodeListProperty().reparentHere(listModelNode);
        element1 = createElement({{"name", "foo"}, {"value", 1}, {"value2", 42}});
        element2 = createElement({{"value", 4}, {"name", "bar"}, {"image", "pic.png"}});
        element3 = createElement({{"image", "pic.png"}, {"name", "poo"}, {"value", 111}});
    }

    using Entry = std::pair<QmlDesigner::PropertyName, QVariant>;

    ModelNode createElement(std::initializer_list<Entry> entries)
    {
        auto element = mockView.createModelNode("QtQml.Models/ListElement", 2, 15);
        listModelNode.defaultNodeListProperty().reparentHere(element);

        for (const auto &entry : entries) {
            element.variantProperty(entry.first).setValue(entry.second);
        }

        return element;
    }

    QList<QString> headerLabels(const QmlDesigner::ListModelEditorModel &model) const
    {
        QList<QString> labels;
        labels.reserve(model.columnCount());

        for (int i = 0; i < model.columnCount(); ++i)
            labels.push_back(model.headerData(i, Qt::Horizontal).toString());

        return labels;
    }

    QList<QList<QVariant>> displayValues() const
    {
        QList<QList<QVariant>> rows;

        for (int rowIndex = 0; rowIndex < model.rowCount(); ++rowIndex) {
            QList<QVariant> row;

            for (int columnIndex = 0; columnIndex < model.columnCount(); ++columnIndex)
                row.push_back(model.data(model.index(rowIndex, columnIndex), Qt::DisplayRole));

            rows.push_back(row);
        }

        return rows;
    }

    QList<QList<QColor>> backgroundColors() const
    {
        QList<QList<QColor>> rows;

        for (int rowIndex = 0; rowIndex < model.rowCount(); ++rowIndex) {
            QList<QColor> row;

            for (int columnIndex = 0; columnIndex < model.columnCount(); ++columnIndex)
                row.push_back(
                    model.data(model.index(rowIndex, columnIndex), Qt::BackgroundColorRole)
                        .value<QColor>());

            rows.push_back(row);
        }

        return rows;
    }

    QList<QList<QmlDesigner::VariantProperty>> properties() const
    {
        QList<QList<QmlDesigner::VariantProperty>> properties;
        properties.reserve(10);

        auto nodes = listModelNode.defaultNodeListProperty().toModelNodeList();

        for (const ModelNode &node : nodes)
            properties.push_back(node.variantProperties());

        return properties;
    }

protected:
    std::unique_ptr<QmlDesigner::Model> designerModel{QmlDesigner::Model::create("QtQuick.Item", 1, 1)};
    NiceMock<MockListModelEditorView> mockView;
    QmlDesigner::ListModelEditorModel model;
    ModelNode listModelNode;
    ModelNode emptyListModelNode;
    ModelNode element1;
    ModelNode element2;
    ModelNode element3;
};

TEST_F(ListModelEditor, CreatePropertyNameSet)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "value", "value2"));
}

TEST_F(ListModelEditor, CreatePropertyNameSetForEmptyList)
{
    model.setListModel(emptyListModelNode);

    ASSERT_THAT(model.propertyNames(), IsEmpty());
}

TEST_F(ListModelEditor, HorizontalLabels)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(headerLabels(model), ElementsAre("image", "name", "value", "value2"));
}

TEST_F(ListModelEditor, HorizontalLabelsForEmptyList)
{
    model.setListModel(emptyListModelNode);

    ASSERT_THAT(headerLabels(model), IsEmpty());
}

TEST_F(ListModelEditor, DisplayValues)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, ChangeValueChangesDisplayValues)
{
    model.setListModel(listModelNode);

    model.setValue(0, 1, "hello");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "hello", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, EditValueCallVariantPropertiesChanged)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element1, "name", "hello")),
                                         Eq(AbstractView::NoAdditionalChanges)));

    model.setValue(0, 1, "hello");
}

TEST_F(ListModelEditor, ChangeDisplayValueCallsVariantPropertiesChanged)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element1, "name", "hello")),
                                         Eq(AbstractView::NoAdditionalChanges)))
        .Times(0);

    model.setValue(0, 1, "hello", Qt::DisplayRole);
}

TEST_F(ListModelEditor, AddRowAddedInvalidRow)
{
    model.setListModel(listModelNode);

    model.addRow();

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid()),
                            ElementsAre(IsInvalid(), IsInvalid(), IsInvalid(), IsInvalid())));
}

TEST_F(ListModelEditor, AddRowCreatesNewModelNodeAndReparents)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView, nodeCreated(Property(&ModelNode::type, Eq("QtQml.Models.ListElement"))));
    EXPECT_CALL(mockView,
                nodeReparented(Property(&ModelNode::type, Eq("QtQml.Models.ListElement")),
                               Property(&AbstractProperty::parentModelNode, Eq(listModelNode)),
                               _,
                               _));

    model.addRow();
}

TEST_F(ListModelEditor, ChangeAddedRowPropery)
{
    model.setListModel(listModelNode);
    model.addRow();

    model.setValue(3, 2, 22);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid()),
                            ElementsAre(IsInvalid(), IsInvalid(), 22, IsInvalid())));
}

TEST_F(ListModelEditor, ChangeAddedRowProperyCallsVariantPropertiesChanged)
{
    model.setListModel(listModelNode);
    ModelNode element4;
    ON_CALL(mockView, nodeReparented(_, _, _, _)).WillByDefault(SaveArg<0>(&element4));
    model.addRow();

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element4, "value", 22)),
                                         Eq(AbstractView::PropertiesAdded)));

    model.setValue(3, 2, 22);
}

TEST_F(ListModelEditor, AddColumnInsertsPropertyName)
{
    model.setListModel(listModelNode);

    model.addColumn("other");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "other", "value", "value2"));
}

TEST_F(ListModelEditor, AddColumnInsertsPropertyNameToEmptyModel)
{
    model.setListModel(emptyListModelNode);

    model.addColumn("foo");

    ASSERT_THAT(model.propertyNames(), ElementsAre("foo"));
}

TEST_F(ListModelEditor, AddTwiceColumnInsertsPropertyNameToEmptyModel)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo2");

    ASSERT_THAT(model.propertyNames(), ElementsAre("foo", "foo2"));
}

TEST_F(ListModelEditor, AddSameColumnInsertsPropertyName)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo");

    ASSERT_THAT(model.propertyNames(), ElementsAre("foo"));
}

TEST_F(ListModelEditor, AddColumnInsertsHeaderLabel)
{
    model.setListModel(listModelNode);

    model.addColumn("other");

    ASSERT_THAT(headerLabels(model), ElementsAre("image", "name", "other", "value", "value2"));
}

TEST_F(ListModelEditor, AddColumnInsertsHeaderLabelToEmptyModel)
{
    model.setListModel(emptyListModelNode);

    model.addColumn("foo");

    ASSERT_THAT(headerLabels(model), ElementsAre("foo"));
}

TEST_F(ListModelEditor, AddTwiceColumnInsertsHeaderLabelToEmptyModel)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo2");

    ASSERT_THAT(headerLabels(model), ElementsAre("foo", "foo2"));
}

TEST_F(ListModelEditor, AddSameColumnInsertsHeaderLabel)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo");

    ASSERT_THAT(headerLabels(model), ElementsAre("foo"));
}

TEST_F(ListModelEditor, AddColumnInsertsDisplayValues)
{
    model.setListModel(listModelNode);

    model.addColumn("other");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", IsInvalid(), 1, 42),
                            ElementsAre("pic.png", "bar", IsInvalid(), 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", IsInvalid(), 111, IsInvalid())));
}

TEST_F(ListModelEditor, ChangeAddColumnPropertyDisplayValue)
{
    model.setListModel(listModelNode);
    model.addColumn("other");

    model.setValue(1, 2, 22);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", IsInvalid(), 1, 42),
                            ElementsAre("pic.png", "bar", 22, 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", IsInvalid(), 111, IsInvalid())));
}

TEST_F(ListModelEditor, ChangeAddColumnPropertyCallsVariantPropertiesChanged)
{
    model.setListModel(listModelNode);
    model.addColumn("other");

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element2, "other", 434)), _));

    model.setValue(1, 2, 434);
}

TEST_F(ListModelEditor, RemoveColumnRemovesDisplayValues)
{
    model.setListModel(listModelNode);

    model.removeColumn(2);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 42),
                            ElementsAre("pic.png", "bar", IsInvalid()),
                            ElementsAre("pic.png", "poo", IsInvalid())));
}

TEST_F(ListModelEditor, RemoveColumnRemovesProperties)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView, propertiesRemoved(ElementsAre(IsAbstractProperty(element2, "image"))));
    EXPECT_CALL(mockView, propertiesRemoved(ElementsAre(IsAbstractProperty(element3, "image"))));

    model.removeColumn(0);
}

TEST_F(ListModelEditor, RemoveColumnRemovesPropertyName)
{
    model.setListModel(listModelNode);

    model.removeColumn(1);

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "value", "value2"));
}

TEST_F(ListModelEditor, RemoveRowRemovesDisplayValues)
{
    model.setListModel(listModelNode);

    model.removeRow(1);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, RemoveRowRemovesElementInListModel)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView, nodeRemoved(Eq(element2), _, _));

    model.removeRow(1);
}

TEST_F(ListModelEditor, ConvertStringFloatToFloat)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25.5");

    ASSERT_THAT(element2.variantProperty("name").value().value<double>(), 25.5);
    ASSERT_THAT(element2.variantProperty("name").value().type(), QVariant::Double);
}

TEST_F(ListModelEditor, ConvertStringIntegerToDouble)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25");

    ASSERT_THAT(element2.variantProperty("name").value().value<double>(), 25);
    ASSERT_THAT(element2.variantProperty("name").value().type(), QVariant::Double);
}

TEST_F(ListModelEditor, DontConvertStringToNumber)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "hello");

    ASSERT_THAT(element2.variantProperty("name").value().value<QString>(), "hello");
    ASSERT_THAT(element2.variantProperty("name").value().type(), QVariant::String);
}

TEST_F(ListModelEditor, EmptyStringsRemovesProperty)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "");

    ASSERT_THAT(element2.variantProperty("name").value().value<QString>(), Eq(""));
}

TEST_F(ListModelEditor, InvalidVariantRemovesProperty)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, QVariant());

    ASSERT_FALSE(element2.hasProperty("name"));
}

TEST_F(ListModelEditor, DispayValueIsChangedToDouble)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25.5");

    ASSERT_THAT(displayValues()[1][1].type(), QVariant::Double);
}

TEST_F(ListModelEditor, StringDispayValueIsNotChanged)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25.5a");

    ASSERT_THAT(displayValues()[1][1].type(), QVariant::String);
}

TEST_F(ListModelEditor, SetInvalidToDarkYellowBackgroundColor)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(
        backgroundColors(),
        ElementsAre(
            ElementsAre(Qt::darkYellow, Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow)),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow)));
}

TEST_F(ListModelEditor, SettingValueChangesBackgroundColor)
{
    model.setListModel(listModelNode);

    model.setValue(0, 0, "foo");

    ASSERT_THAT(
        backgroundColors(),
        ElementsAre(
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow)),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow)));
}

TEST_F(ListModelEditor, SettingValueChangesByDisplayRoleBackgroundColor)
{
    model.setListModel(listModelNode);

    model.setValue(0, 0, "foo", Qt::DisplayRole);

    ASSERT_THAT(
        backgroundColors(),
        ElementsAre(
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow)),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow)));
}

TEST_F(ListModelEditor, ResettingValueChangesBackgroundColor)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, QVariant{});

    ASSERT_THAT(
        backgroundColors(),
        ElementsAre(
            ElementsAre(Qt::darkYellow, Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow)),
            ElementsAre(Not(Qt::darkYellow), Qt::darkYellow, Not(Qt::darkYellow), Qt::darkYellow),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow)));
}

TEST_F(ListModelEditor, ResettingValueChangesByDisplayRoleBackgroundColor)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, QVariant{}, Qt::DisplayRole);

    ASSERT_THAT(
        backgroundColors(),
        ElementsAre(
            ElementsAre(Qt::darkYellow, Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow)),
            ElementsAre(Not(Qt::darkYellow), Qt::darkYellow, Not(Qt::darkYellow), Qt::darkYellow),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow)));
}

TEST_F(ListModelEditor, SettingNullValueChangesBackgroundColor)
{
    model.setListModel(listModelNode);

    model.setValue(0, 0, 0);

    ASSERT_THAT(backgroundColors(),
                ElementsAre(ElementsAre(_, _, _, _),
                            ElementsAre(_, _, _, Qt::darkYellow),
                            ElementsAre(_, _, _, Qt::darkYellow)));
}

TEST_F(ListModelEditor, DontRenamePropertyIfColumnNameExists)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "value", "value2"));
}

TEST_F(ListModelEditor, DontRenameColumnIfColumnNameExists)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(headerLabels(model), ElementsAre("image", "name", "value", "value2"));
}

TEST_F(ListModelEditor, DontRenameColumnIfColumnNameExistsDoesNotChangeDisplayValues)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, DontRenameColumnIfColumnNameExistsDoesNotChangeProperties)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, RenamePropertyButDontChangeOrder)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "mood", "value", "value2"));
}

TEST_F(ListModelEditor, RenameColumnButDontChangeOrder)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(headerLabels(model), ElementsAre("image", "mood", "value", "value2"));
}

TEST_F(ListModelEditor, RenameColumnButDontChangeOrderDisplayValues)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, RenameColumnButDontChangeOrderProperies)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("mood", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("mood", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("mood", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, RemoveColumnAfterRenameColumn)
{
    model.setListModel(listModelNode);
    model.renameColumn(1, "mood");

    model.removeColumn(1);

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, ChangeValueAfterRenameColumn)
{
    model.setListModel(listModelNode);
    model.renameColumn(1, "mood");

    model.setValue(1, 1, "taaa");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("mood", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("mood", "taaa"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("mood", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, RemovePropertyAfterRenameColumn)
{
    model.setListModel(listModelNode);
    model.renameColumn(1, "mood");

    model.setValue(1, 1, {});

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("mood", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("mood", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, RenameToPrecedingProperty)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(model.propertyNames(), ElementsAre("alpha", "image", "value", "value2"));
}

TEST_F(ListModelEditor, RenameToPrecedingColumn)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(headerLabels(model), ElementsAre("alpha", "image", "value", "value2"));
}

TEST_F(ListModelEditor, RenameToPrecedingColumnDisplayValues)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre("foo", IsInvalid(), 1, 42),
                            ElementsAre("bar", "pic.png", 4, IsInvalid()),
                            ElementsAre("poo", "pic.png", 111, IsInvalid())));
}

TEST_F(ListModelEditor, RenameToPrecedingColumnProperties)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("alpha", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("alpha", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("alpha", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, RenameToFollowingProperty)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "value2", "zoo"));
}

TEST_F(ListModelEditor, RenameToFollowingColumn)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(headerLabels(model), ElementsAre("image", "name", "value2", "zoo"));
}

TEST_F(ListModelEditor, RenameToFollowingColumnDisplayValues)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 42, 1),
                            ElementsAre("pic.png", "bar", IsInvalid(), 4),
                            ElementsAre("pic.png", "poo", IsInvalid(), 111)));
}

TEST_F(ListModelEditor, RenameToFollowingColumnProperties)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", "foo"),
                                                 IsVariantProperty("zoo", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("zoo", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("zoo", 111))));
}

TEST_F(ListModelEditor, RenamePropertiesWithInvalidValue)
{
    model.setListModel(listModelNode);

    model.renameColumn(0, "mood");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("mood", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("mood", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, ChangeValueAfterRenamePropertiesWithInvalidValue)
{
    model.setListModel(listModelNode);
    model.renameColumn(0, "mood");

    model.setValue(0, 0, "haaa");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("mood", "haaa"),
                                                 IsVariantProperty("name", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("mood", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("mood", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, RemoveLastRow)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();

    model.removeRow(0);

    ASSERT_THAT(displayValues(), IsEmpty());
}

TEST_F(ListModelEditor, RemoveLastColumn)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();

    model.removeColumn(0);

    ASSERT_THAT(displayValues(), ElementsAre(IsEmpty()));
}

TEST_F(ListModelEditor, RemoveLastEmptyColumn)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();
    model.removeRow(0);

    model.removeColumn(0);

    ASSERT_THAT(displayValues(), IsEmpty());
}

TEST_F(ListModelEditor, RemoveLastEmptyRow)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();
    model.removeColumn(0);

    model.removeRow(0);

    ASSERT_THAT(displayValues(), IsEmpty());
}

} // namespace
