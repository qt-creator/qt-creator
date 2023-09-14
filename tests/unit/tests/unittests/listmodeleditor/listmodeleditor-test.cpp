// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <mocks/abstractviewmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>

#include <qmldesigner/components/listmodeleditor/listmodeleditormodel.h>
#include <qmldesigner/designercore/include/abstractview.h>
#include <qmldesigner/designercore/include/bindingproperty.h>
#include <qmldesigner/designercore/include/model.h>
#include <qmldesigner/designercore/include/nodelistproperty.h>
#include <qmldesigner/designercore/include/nodeproperty.h>
#include <qmldesigner/designercore/include/variantproperty.h>

namespace {

using QmlDesigner::AbstractProperty;
using QmlDesigner::AbstractView;
using QmlDesigner::ListModelEditorModel;
using QmlDesigner::ModelNode;
using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::TypeId;
namespace Info = QmlDesigner::Storage::Info;

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
    using SourcePathCache = QmlDesigner::SourcePathCache<NiceMock<ProjectStorageMockWithQtQtuick>,
                                                         QmlDesigner::NonLockingMutex>;

public:
    ListModelEditor()
    {
        designerModel->attachView(&mockView);

        emptyListModelNode = mockView.createModelNode("ListModel");

        listViewNode = mockView.createModelNode("ListView");
        listModelNode = mockView.createModelNode("ListModel");
        mockView.rootModelNode().defaultNodeListProperty().reparentHere(listModelNode);
        element1 = createElement({{"name", "foo"}, {"value", 1}, {"value2", 42}},
                                 mockView,
                                 listModelNode);
        element2 = createElement({{"value", 4}, {"name", "bar"}, {"image", "pic.png"}},
                                 mockView,
                                 listModelNode);
        element3 = createElement({{"image", "pic.png"}, {"name", "poo"}, {"value", 111}},
                                 mockView,
                                 listModelNode);

        componentModel->attachView(&mockComponentView);
        mockComponentView.changeRootNodeType("ListModel", -1, -1);

        componentElement = createElement({{"name", "com"}, {"value", 11}, {"value2", 55}},
                                         mockComponentView,
                                         mockComponentView.rootModelNode());

        ON_CALL(goIntoComponentMock, Call(_)).WillByDefault([](ModelNode node) { return node; });
    }

    using Entry = std::pair<QmlDesigner::PropertyName, QVariant>;

    ModelNode createElement(std::initializer_list<Entry> entries, AbstractView &view, ModelNode listModel)
    {
        auto element = view.createModelNode("QtQml.Models/ListElement", 2, 15);
        listModel.defaultNodeListProperty().reparentHere(element);

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
                    model.data(model.index(rowIndex, columnIndex), Qt::BackgroundRole).value<QColor>());

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

    QModelIndex index(int row, int column) const { return model.index(row, column); }

    QList<ModelNode> elements(const ModelNode &node) const
    {
        return node.defaultNodeListProperty().toModelNodeList();
    }

protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCacheMock.sourceId};
    NiceMock<MockFunction<ModelNode(const ModelNode &)>> goIntoComponentMock;
    QmlDesigner::ModelPointer designerModel{
        QmlDesigner::Model::create(QmlDesigner::ProjectStorageDependencies{projectStorageMock,
                                                                           pathCacheMock},
                                   "Item",
                                   {QmlDesigner::Import::createLibraryImport("QtQml.Models"),
                                    QmlDesigner::Import::createLibraryImport("QtQuick")},
                                   pathCacheMock.path.toQString())};
    NiceMock<AbstractViewMock> mockView;
    QmlDesigner::ListModelEditorModel model{[&] { return mockView.createModelNode("ListModel"); },
                                            [&] { return mockView.createModelNode("ListElement"); },
                                            goIntoComponentMock.AsStdFunction()};
    ModelNode listViewNode;
    ModelNode listModelNode;
    ModelNode emptyListModelNode;
    ModelNode element1;
    ModelNode element2;
    ModelNode element3;
    QmlDesigner::ModelPointer componentModel{
        QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                   "ListModel",
                                   {QmlDesigner::Import::createLibraryImport("QtQml.Models"),
                                    QmlDesigner::Import::createLibraryImport("QtQuick")},
                                   pathCacheMock.path.toQString())};
    NiceMock<AbstractViewMock> mockComponentView;
    ModelNode componentElement;
};

TEST_F(ListModelEditor, create_property_name_set)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "value", "value2"));
}

TEST_F(ListModelEditor, create_property_name_set_for_empty_list)
{
    model.setListModel(emptyListModelNode);

    ASSERT_THAT(model.propertyNames(), IsEmpty());
}

TEST_F(ListModelEditor, horizontal_labels)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(headerLabels(model), ElementsAre(u"image", u"name", u"value", u"value2"));
}

TEST_F(ListModelEditor, horizontal_labels_for_empty_list)
{
    model.setListModel(emptyListModelNode);

    ASSERT_THAT(headerLabels(model), IsEmpty());
}

TEST_F(ListModelEditor, display_values)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, change_value_changes_display_values)
{
    model.setListModel(listModelNode);

    model.setValue(0, 1, "hello");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "hello", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, edit_value_call_variant_properties_changed)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element1, "name", "hello")),
                                         Eq(AbstractView::NoAdditionalChanges)));

    model.setValue(0, 1, "hello");
}

TEST_F(ListModelEditor, change_display_value_calls_variant_properties_changed)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element1, "name", "hello")),
                                         Eq(AbstractView::NoAdditionalChanges)))
        .Times(0);

    model.setValue(0, 1, "hello", Qt::DisplayRole);
}

TEST_F(ListModelEditor, add_row_added_invalid_row)
{
    model.setListModel(listModelNode);

    model.addRow();

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid()),
                            ElementsAre(IsInvalid(), IsInvalid(), IsInvalid(), IsInvalid())));
}

TEST_F(ListModelEditor, add_row_creates_new_model_node_and_reparents)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView, nodeCreated(Property(&ModelNode::type, Eq("ListElement"))));
    EXPECT_CALL(mockView,
                nodeReparented(Property(&ModelNode::type, Eq("ListElement")),
                               Property(&AbstractProperty::parentModelNode, Eq(listModelNode)),
                               _,
                               _));

    model.addRow();
}

TEST_F(ListModelEditor, change_added_row_propery)
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

TEST_F(ListModelEditor, change_added_row_propery_calls_variant_properties_changed)
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

TEST_F(ListModelEditor, add_column_inserts_property_name)
{
    model.setListModel(listModelNode);

    model.addColumn("other");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "other", "value", "value2"));
}

TEST_F(ListModelEditor, add_column_inserts_property_name_to_empty_model)
{
    model.setListModel(emptyListModelNode);

    model.addColumn("foo");

    ASSERT_THAT(model.propertyNames(), ElementsAre("foo"));
}

TEST_F(ListModelEditor, add_twice_column_inserts_property_name_to_empty_model)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo2");

    ASSERT_THAT(model.propertyNames(), ElementsAre("foo", "foo2"));
}

TEST_F(ListModelEditor, add_same_column_inserts_property_name)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo");

    ASSERT_THAT(model.propertyNames(), ElementsAre("foo"));
}

TEST_F(ListModelEditor, add_column_inserts_header_label)
{
    model.setListModel(listModelNode);

    model.addColumn("other");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"image", u"name", u"other", u"value", u"value2"));
}

TEST_F(ListModelEditor, add_column_inserts_header_label_to_empty_model)
{
    model.setListModel(emptyListModelNode);

    model.addColumn("foo");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"foo"));
}

TEST_F(ListModelEditor, add_twice_column_inserts_header_label_to_empty_model)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo2");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"foo", u"foo2"));
}

TEST_F(ListModelEditor, add_same_column_inserts_header_label)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("foo");

    model.addColumn("foo");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"foo"));
}

TEST_F(ListModelEditor, add_column_inserts_display_values)
{
    model.setListModel(listModelNode);

    model.addColumn("other");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", IsInvalid(), 1, 42),
                            ElementsAre("pic.png", "bar", IsInvalid(), 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", IsInvalid(), 111, IsInvalid())));
}

TEST_F(ListModelEditor, change_add_column_property_display_value)
{
    model.setListModel(listModelNode);
    model.addColumn("other");

    model.setValue(1, 2, 22);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", IsInvalid(), 1, 42),
                            ElementsAre("pic.png", "bar", 22, 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", IsInvalid(), 111, IsInvalid())));
}

TEST_F(ListModelEditor, change_add_column_property_calls_variant_properties_changed)
{
    model.setListModel(listModelNode);
    model.addColumn("other");

    EXPECT_CALL(mockView,
                variantPropertiesChanged(ElementsAre(IsVariantProperty(element2, "other", 434)), _));

    model.setValue(1, 2, 434);
}

TEST_F(ListModelEditor, remove_column_removes_display_values)
{
    model.setListModel(listModelNode);

    model.removeColumns({index(0, 2)});

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 42),
                            ElementsAre("pic.png", "bar", IsInvalid()),
                            ElementsAre("pic.png", "poo", IsInvalid())));
}

TEST_F(ListModelEditor, remove_column_removes_properties)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView, propertiesRemoved(ElementsAre(IsAbstractProperty(element2, "image"))));
    EXPECT_CALL(mockView, propertiesRemoved(ElementsAre(IsAbstractProperty(element3, "image"))));

    model.removeColumns({index(0, 0)});
}

TEST_F(ListModelEditor, remove_column_removes_property_name)
{
    model.setListModel(listModelNode);

    model.removeColumns({index(0, 1)});

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "value", "value2"));
}

TEST_F(ListModelEditor, remove_row_removes_display_values)
{
    model.setListModel(listModelNode);

    model.removeRows({index(1, 0)});

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, remove_row_removes_element_in_list_model)
{
    model.setListModel(listModelNode);

    EXPECT_CALL(mockView, nodeRemoved(Eq(element2), _, _));

    model.removeRows({index(1, 0)});
}

TEST_F(ListModelEditor, convert_string_float_to_float)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25.5");

    ASSERT_THAT(element2.variantProperty("name").value().value<double>(), 25.5);
    ASSERT_THAT(element2.variantProperty("name").value().type(), QVariant::Double);
}

TEST_F(ListModelEditor, convert_string_integer_to_double)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25");

    ASSERT_THAT(element2.variantProperty("name").value().value<double>(), 25);
    ASSERT_THAT(element2.variantProperty("name").value().type(), QVariant::Double);
}

TEST_F(ListModelEditor, dont_convert_string_to_number)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "hello");

    ASSERT_THAT(element2.variantProperty("name").value().value<QString>(), u"hello");
    ASSERT_THAT(element2.variantProperty("name").value().type(), QVariant::String);
}

TEST_F(ListModelEditor, empty_strings_removes_property)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "");

    ASSERT_THAT(element2.variantProperty("name").value().value<QString>(), Eq(u""));
}

TEST_F(ListModelEditor, invalid_variant_removes_property)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, QVariant());

    ASSERT_FALSE(element2.hasProperty("name"));
}

TEST_F(ListModelEditor, dispay_value_is_changed_to_double)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25.5");

    ASSERT_THAT(displayValues()[1][1].type(), QVariant::Double);
}

TEST_F(ListModelEditor, string_dispay_value_is_not_changed)
{
    model.setListModel(listModelNode);

    model.setValue(1, 1, "25.5a");

    ASSERT_THAT(displayValues()[1][1].type(), QVariant::String);
}

TEST_F(ListModelEditor, set_invalid_to_dark_yellow_background_color)
{
    model.setListModel(listModelNode);

    ASSERT_THAT(
        backgroundColors(),
        ElementsAre(
            ElementsAre(Qt::darkYellow, Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow)),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow),
            ElementsAre(Not(Qt::darkYellow), Not(Qt::darkYellow), Not(Qt::darkYellow), Qt::darkYellow)));
}

TEST_F(ListModelEditor, setting_value_changes_background_color)
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

TEST_F(ListModelEditor, setting_value_changes_by_display_role_background_color)
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

TEST_F(ListModelEditor, resetting_value_changes_background_color)
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

TEST_F(ListModelEditor, resetting_value_changes_by_display_role_background_color)
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

TEST_F(ListModelEditor, setting_null_value_changes_background_color)
{
    model.setListModel(listModelNode);

    model.setValue(0, 0, 0);

    ASSERT_THAT(backgroundColors(),
                ElementsAre(ElementsAre(_, _, _, _),
                            ElementsAre(_, _, _, Qt::darkYellow),
                            ElementsAre(_, _, _, Qt::darkYellow)));
}

TEST_F(ListModelEditor, dont_rename_property_if_column_name_exists)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "value", "value2"));
}

TEST_F(ListModelEditor, dont_rename_column_if_column_name_exists)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"image", u"name", u"value", u"value2"));
}

TEST_F(ListModelEditor, dont_rename_column_if_column_name_exists_does_not_change_display_values)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "value2");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, dont_rename_column_if_column_name_exists_does_not_change_properties)
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

TEST_F(ListModelEditor, rename_property_but_dont_change_order)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "mood", "value", "value2"));
}

TEST_F(ListModelEditor, rename_column_but_dont_change_order)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"image", u"mood", u"value", u"value2"));
}

TEST_F(ListModelEditor, rename_column_but_dont_change_order_display_values)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "mood");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, rename_column_but_dont_change_order_properies)
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

TEST_F(ListModelEditor, remove_column_after_rename_column)
{
    model.setListModel(listModelNode);
    model.renameColumn(1, "mood");

    model.removeColumns({index(0, 1)});

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, change_value_after_rename_column)
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

TEST_F(ListModelEditor, remove_property_after_rename_column)
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

TEST_F(ListModelEditor, rename_to_preceding_property)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(model.propertyNames(), ElementsAre("alpha", "image", "value", "value2"));
}

TEST_F(ListModelEditor, rename_to_preceding_column)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"alpha", u"image", u"value", u"value2"));
}

TEST_F(ListModelEditor, rename_to_preceding_column_display_values)
{
    model.setListModel(listModelNode);

    model.renameColumn(1, "alpha");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre("foo", IsInvalid(), 1, 42),
                            ElementsAre("bar", "pic.png", 4, IsInvalid()),
                            ElementsAre("poo", "pic.png", 111, IsInvalid())));
}

TEST_F(ListModelEditor, rename_to_preceding_column_properties)
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

TEST_F(ListModelEditor, rename_to_following_property)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(model.propertyNames(), ElementsAre("image", "name", "value2", "zoo"));
}

TEST_F(ListModelEditor, rename_to_following_column)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(headerLabels(model), ElementsAre(u"image", u"name", u"value2", u"zoo"));
}

TEST_F(ListModelEditor, rename_to_following_column_display_values)
{
    model.setListModel(listModelNode);

    model.renameColumn(2, "zoo");

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 42, 1),
                            ElementsAre("pic.png", "bar", IsInvalid(), 4),
                            ElementsAre("pic.png", "poo", IsInvalid(), 111)));
}

TEST_F(ListModelEditor, rename_to_following_column_properties)
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

TEST_F(ListModelEditor, rename_properties_with_invalid_value)
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

TEST_F(ListModelEditor, change_value_after_rename_properties_with_invalid_value)
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

TEST_F(ListModelEditor, remove_last_row)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();

    model.removeRows({index(0, 0)});

    ASSERT_THAT(displayValues(), IsEmpty());
}

TEST_F(ListModelEditor, remove_last_empty_row)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();
    model.removeColumns({index(0, 0)});

    model.removeRows({index(0, 0)});

    ASSERT_THAT(displayValues(), ElementsAre(IsEmpty()));
}

TEST_F(ListModelEditor, remove_last_column)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();

    model.removeColumns({index(0, 0)});

    ASSERT_THAT(displayValues(), ElementsAre(IsEmpty()));
}

TEST_F(ListModelEditor, remove_last_empty_column)
{
    model.setListModel(emptyListModelNode);
    model.addColumn("mood");
    model.addRow();
    model.removeRows({index(0, 0)});

    model.removeColumns({index(0, 0)});

    ASSERT_THAT(displayValues(), IsEmpty());
}

TEST_F(ListModelEditor, remove_columns)
{
    model.setListModel(listModelNode);
    model.removeColumns({index(0, 1), index(0, 3), index(1, 1), index(0, 4)});

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("value", 1)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, remove_rows)
{
    model.setListModel(listModelNode);

    model.removeRows({index(1, 0), index(2, 0), index(3, 0), index(2, 0)});

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", "foo"),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42))));
}

TEST_F(ListModelEditor, filter_columns)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(0, 0), index(1, 1), index(0, 2), index(0, 1)};

    auto columns = ListModelEditorModel::filterColumns(indices);

    ASSERT_THAT(columns, ElementsAre(0, 1, 2));
}

TEST_F(ListModelEditor, filter_columns_invalid_columns)
{
    QList<QModelIndex> indices = {index(0, 0), index(1, 1), index(0, 2), index(0, 1)};

    auto columns = ListModelEditorModel::filterColumns(indices);

    ASSERT_THAT(columns, IsEmpty());
}

TEST_F(ListModelEditor, filter_columns_empty_input)
{
    QList<QModelIndex> indices;

    auto columns = ListModelEditorModel::filterColumns(indices);

    ASSERT_THAT(columns, IsEmpty());
}

TEST_F(ListModelEditor, filter_rows)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(0, 0), index(1, 1), index(2, 2), index(0, 1)};

    auto rows = ListModelEditorModel::filterRows(indices);

    ASSERT_THAT(rows, ElementsAre(0, 1, 2));
}

TEST_F(ListModelEditor, filter_rows_invalid_columns)
{
    QList<QModelIndex> indices = {index(0, 0), index(1, 1), index(2, 2), index(0, 1)};

    auto rows = ListModelEditorModel::filterRows(indices);

    ASSERT_THAT(rows, IsEmpty());
}

TEST_F(ListModelEditor, filter_rows_empty_input)
{
    QList<QModelIndex> indices;

    auto rows = ListModelEditorModel::filterRows(indices);

    ASSERT_THAT(rows, IsEmpty());
}

TEST_F(ListModelEditor, cannot_move_empty_rows_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(-1, 1)};

    model.moveRowsUp(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element1, element2, element3));
}

TEST_F(ListModelEditor, move_row_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(1, 2), index(1, 0)};

    model.moveRowsUp(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element2, element1, element3));
}

TEST_F(ListModelEditor, move_rows_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(2, 2), index(1, 0)};

    model.moveRowsUp(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element2, element3, element1));
}

TEST_F(ListModelEditor, cannot_move_first_rows_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(0, 1), index(1, 2), index(0, 0)};

    model.moveRowsUp(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element1, element2, element3));
}

TEST_F(ListModelEditor, cannot_move_empty_rows_up_display_values)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(-1, 1)};

    model.moveRowsUp(indices);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, cannot_move_first_row_up_display_values)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(0, 1), index(1, 2), index(0, 0)};

    model.moveRowsUp(indices);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, move_rows_up_display_values)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(2, 2), index(1, 0)};

    model.moveRowsUp(indices);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid()),
                            ElementsAre(IsInvalid(), "foo", 1, 42)));
}

TEST_F(ListModelEditor, no_selection_after_cannot_move_last_rows_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(0, 1), index(1, 2), index(0, 0)};

    auto selection = model.moveRowsUp(indices);

    ASSERT_THAT(selection.indexes(), IsEmpty());
}

TEST_F(ListModelEditor, no_selection_after_move_empty_rows_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(-1, 1)};

    auto selection = model.moveRowsUp(indices);

    ASSERT_THAT(selection.indexes(), IsEmpty());
}

TEST_F(ListModelEditor, selection_after_move_rows_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(2, 2), index(1, 0)};

    auto selection = model.moveRowsUp(indices);

    ASSERT_THAT(selection.indexes(),
                ElementsAre(index(0, 0),
                            index(0, 1),
                            index(0, 2),
                            index(0, 3),
                            index(1, 0),
                            index(1, 1),
                            index(1, 2),
                            index(1, 3)));
}

TEST_F(ListModelEditor, cannot_move_empty_rows_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(-1, 1)};

    model.moveRowsDown(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element1, element2, element3));
}

TEST_F(ListModelEditor, move_row_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(1, 2), index(1, 0)};

    model.moveRowsDown(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element1, element3, element2));
}

TEST_F(ListModelEditor, move_rows_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(0, 2), index(1, 0)};

    model.moveRowsDown(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element3, element1, element2));
}

TEST_F(ListModelEditor, cannot_move_last_rows_down)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(2, 1), index(1, 2), index(2, 0)};

    model.moveRowsDown(indices);

    ASSERT_THAT(elements(listModelNode), ElementsAre(element1, element2, element3));
}

TEST_F(ListModelEditor, cannot_move_empty_rows_down_display_values)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(-1, 1)};

    model.moveRowsDown(indices);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, cannot_move_last_row_down_display_values)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(2, 1), index(1, 2), index(2, 0)};

    model.moveRowsDown(indices);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, move_rows_down_display_values)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(0, 2), index(1, 0)};

    model.moveRowsDown(indices);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre("pic.png", "poo", 111, IsInvalid()),
                            ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid())));
}

TEST_F(ListModelEditor, no_selection_after_cannot_move_last_rows_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(2, 1), index(1, 2), index(2, 0)};

    auto selection = model.moveRowsDown(indices);

    ASSERT_THAT(selection.indexes(), IsEmpty());
}

TEST_F(ListModelEditor, no_selection_after_move_empty_rows_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(-1, 1)};

    auto selection = model.moveRowsDown(indices);

    ASSERT_THAT(selection.indexes(), IsEmpty());
}

TEST_F(ListModelEditor, selection_after_move_rows_up)
{
    model.setListModel(listModelNode);
    QList<QModelIndex> indices = {index(1, 1), index(0, 2), index(1, 0)};

    auto selection = model.moveRowsDown(indices);

    ASSERT_THAT(selection.indexes(),
                ElementsAre(index(1, 0),
                            index(1, 1),
                            index(1, 2),
                            index(1, 3),
                            index(2, 0),
                            index(2, 1),
                            index(2, 2),
                            index(2, 3)));
}

TEST_F(ListModelEditor, list_view_has_no_model)
{
    model.setListView(listViewNode);

    ASSERT_THAT(listViewNode.nodeProperty("model").modelNode().type(), Eq("ListModel"));
}

TEST_F(ListModelEditor, list_view_has_model_inside)
{
    listViewNode.nodeProperty("model").reparentHere(listModelNode);

    model.setListView(listViewNode);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, list_view_has_model_binding)
{
    listModelNode.setIdWithoutRefactoring("listModel");
    listViewNode.bindingProperty("model").setExpression("listModel");

    model.setListView(listViewNode);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), "foo", 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, add_boolean_display_values)
{
    model.setListModel(listModelNode);

    model.setValue(0, 1, true);

    ASSERT_THAT(displayValues(),
                ElementsAre(ElementsAre(IsInvalid(), true, 1, 42),
                            ElementsAre("pic.png", "bar", 4, IsInvalid()),
                            ElementsAre("pic.png", "poo", 111, IsInvalid())));
}

TEST_F(ListModelEditor, add_boolean_properties)
{
    model.setListModel(listModelNode);

    model.setValue(0, 1, true);

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", true),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, add_true_as_string_properties)
{
    model.setListModel(listModelNode);

    model.setValue(0, 1, "true");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", true),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, add_false_as_string_properties)
{
    model.setListModel(listModelNode);

    model.setValue(0, 1, "false");

    ASSERT_THAT(properties(),
                ElementsAre(UnorderedElementsAre(IsVariantProperty("name", false),
                                                 IsVariantProperty("value", 1),
                                                 IsVariantProperty("value2", 42)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "bar"),
                                                 IsVariantProperty("value", 4)),
                            UnorderedElementsAre(IsVariantProperty("image", "pic.png"),
                                                 IsVariantProperty("name", "poo"),
                                                 IsVariantProperty("value", 111))));
}

TEST_F(ListModelEditor, go_into_component_for_binding)
{
    EXPECT_CALL(goIntoComponentMock, Call(Eq(listModelNode)))
        .WillRepeatedly(Return(mockComponentView.rootModelNode()));
    listModelNode.setIdWithoutRefactoring("listModel");
    listViewNode.bindingProperty("model").setExpression("listModel");

    model.setListView(listViewNode);

    ASSERT_THAT(displayValues(), ElementsAre(ElementsAre("com", 11, 55)));
}

TEST_F(ListModelEditor, go_into_component_for_model_node)
{
    EXPECT_CALL(goIntoComponentMock, Call(Eq(listModelNode)))
        .WillRepeatedly(Return(mockComponentView.rootModelNode()));
    listViewNode.nodeProperty("model").reparentHere(listModelNode);

    model.setListView(listViewNode);

    ASSERT_THAT(displayValues(), ElementsAre(ElementsAre("com", 11, 55)));
}

} // namespace
