// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <abstractviewmock.h>
#include <googletest.h>
#include <projectstoragemock.h>
#include <sourcepathcachemock.h>

#include <model.h>
#include <modelnode.h>
#include <nodelistproperty.h>
#include <projectstorage/projectstorage.h>
#include <sqlitedatabase.h>

namespace {

using QmlDesigner::ModelNode;
using QmlDesigner::ModuleId;
using QmlDesigner::PropertyDeclarationId;
using QmlDesigner::TypeId;
namespace Info = QmlDesigner::Storage::Info;

class NodeListProperty : public testing::Test
{
protected:
    using iterator = QmlDesigner::NodeListProperty::iterator;
    NodeListProperty()
    {
        model->attachView(&abstractViewMock);
        nodeListProperty = abstractViewMock.rootModelNode().nodeListProperty("foo");

        node1 = abstractViewMock.createModelNode("QtQick.Item1", -1, -1);
        node2 = abstractViewMock.createModelNode("QtQick.Item2", -1, -1);
        node3 = abstractViewMock.createModelNode("QtQick.Item3", -1, -1);
        node4 = abstractViewMock.createModelNode("QtQick.Item4", -1, -1);
        node5 = abstractViewMock.createModelNode("QtQick.Item5", -1, -1);

        nodeListProperty.reparentHere(node1);
        nodeListProperty.reparentHere(node2);
        nodeListProperty.reparentHere(node3);
        nodeListProperty.reparentHere(node4);
        nodeListProperty.reparentHere(node5);
    }

    ~NodeListProperty() { model->detachView(&abstractViewMock); }

    void setModuleId(Utils::SmallStringView moduleName, ModuleId moduleId)
    {
        ON_CALL(projectStorageMock, moduleId(Eq(moduleName))).WillByDefault(Return(moduleId));
    }

    void setType(ModuleId moduleId,
                 Utils::SmallStringView typeName,
                 Utils::SmallString defaultPeopertyName)
    {
        static int typeIdNumber = 0;
        TypeId typeId = TypeId::create(++typeIdNumber);

        static int defaultPropertyIdNumber = 0;
        PropertyDeclarationId defaultPropertyId = PropertyDeclarationId::create(
            ++defaultPropertyIdNumber);

        ON_CALL(projectStorageMock, typeId(Eq(moduleId), Eq(typeName), _)).WillByDefault(Return(typeId));
        ON_CALL(projectStorageMock, type(Eq(typeId)))
            .WillByDefault(Return(Info::Type{defaultPropertyId, QmlDesigner::SourceId{}, {}}));
        ON_CALL(projectStorageMock, propertyName(Eq(defaultPropertyId)))
            .WillByDefault(Return(defaultPeopertyName));
    }

    std::vector<ModelNode> nodes() const
    {
        return std::vector<ModelNode>{nodeListProperty.begin(), nodeListProperty.end()};
    }

    static std::vector<ModelNode> nodes(iterator begin, iterator end)
    {
        return std::vector<ModelNode>{begin, end};
    }

protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCache{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCache.sourceId};
    QmlDesigner::ModelPointer model{
        QmlDesigner::Model::create(QmlDesigner::ProjectStorageDependencies{projectStorageMock,
                                                                           pathCache},
                                   "Item",
                                   {QmlDesigner::Import::createLibraryImport("QtQuick")},
                                   QUrl::fromLocalFile(pathCache.path.toQString()))};
    NiceMock<AbstractViewMock> abstractViewMock;
    QmlDesigner::NodeListProperty nodeListProperty;
    ModelNode node1;
    ModelNode node2;
    ModelNode node3;
    ModelNode node4;
    ModelNode node5;
};

TEST_F(NodeListProperty, begin_and_end_itertors)
{
    std::vector<ModelNode> nodes{nodeListProperty.begin(), nodeListProperty.end()};

    ASSERT_THAT(nodes, ElementsAre(node1, node2, node3, node4, node5));
}

TEST_F(NodeListProperty, loop_over_range)
{
    std::vector<ModelNode> nodes;

    for (const ModelNode &node : nodeListProperty)
        nodes.push_back(node);

    ASSERT_THAT(nodes, ElementsAre(node1, node2, node3, node4, node5));
}

TEST_F(NodeListProperty, next_iterator)
{
    auto begin = nodeListProperty.begin();

    auto nextIterator = std::next(begin);

    ASSERT_THAT(nodes(nextIterator, nodeListProperty.end()), ElementsAre(node2, node3, node4, node5));
}

TEST_F(NodeListProperty, previous_iterator)
{
    auto end = nodeListProperty.end();

    auto previousIterator = std::prev(end);

    ASSERT_THAT(nodes(nodeListProperty.begin(), previousIterator),
                ElementsAre(node1, node2, node3, node4));
}

TEST_F(NodeListProperty, increment_iterator)
{
    auto incrementIterator = nodeListProperty.begin();

    ++incrementIterator;

    ASSERT_THAT(nodes(incrementIterator, nodeListProperty.end()),
                ElementsAre(node2, node3, node4, node5));
}

TEST_F(NodeListProperty, increment_iterator_returns_iterator)
{
    auto begin = nodeListProperty.begin();

    auto incrementIterator = ++begin;

    ASSERT_THAT(nodes(incrementIterator, nodeListProperty.end()),
                ElementsAre(node2, node3, node4, node5));
}

TEST_F(NodeListProperty, post_increment_iterator)
{
    auto postIncrementIterator = nodeListProperty.begin();

    postIncrementIterator++;

    ASSERT_THAT(nodes(postIncrementIterator, nodeListProperty.end()),
                ElementsAre(node2, node3, node4, node5));
}

TEST_F(NodeListProperty, post_increment_iterator_returns_preincrement_iterator)
{
    auto begin = nodeListProperty.begin();

    auto postIncrementIterator = begin++;

    ASSERT_THAT(nodes(postIncrementIterator, nodeListProperty.end()),
                ElementsAre(node1, node2, node3, node4, node5));
}

TEST_F(NodeListProperty, decrement_iterator)
{
    auto decrementIterator = nodeListProperty.end();

    --decrementIterator;

    ASSERT_THAT(nodes(nodeListProperty.begin(), decrementIterator),
                ElementsAre(node1, node2, node3, node4));
}

TEST_F(NodeListProperty, derement_iterator_returns_iterator)
{
    auto end = nodeListProperty.end();

    auto decrementIterator = --end;

    ASSERT_THAT(nodes(nodeListProperty.begin(), decrementIterator),
                ElementsAre(node1, node2, node3, node4));
}

TEST_F(NodeListProperty, post_decrement_iterator)
{
    auto postDecrementIterator = nodeListProperty.end();

    postDecrementIterator--;

    ASSERT_THAT(nodes(nodeListProperty.begin(), postDecrementIterator),
                ElementsAre(node1, node2, node3, node4));
}

TEST_F(NodeListProperty, post_decrement_iterator_returns_predecrement_iterator)
{
    auto end = nodeListProperty.end();

    auto postDecrementIterator = end--;

    ASSERT_THAT(nodes(nodeListProperty.begin(), postDecrementIterator),
                ElementsAre(node1, node2, node3, node4, node5));
}

TEST_F(NodeListProperty, increment_iterator_by_two)
{
    auto incrementIterator = nodeListProperty.begin();

    incrementIterator += 2;

    ASSERT_THAT(nodes(incrementIterator, nodeListProperty.end()), ElementsAre(node3, node4, node5));
}

TEST_F(NodeListProperty, increment_iterator_by_two_returns_iterator)
{
    auto begin = nodeListProperty.begin();

    auto incrementIterator = begin += 2;

    ASSERT_THAT(nodes(incrementIterator, nodeListProperty.end()), ElementsAre(node3, node4, node5));
}

TEST_F(NodeListProperty, decrement_iterator_by_two)
{
    auto decrementIterator = nodeListProperty.end();

    decrementIterator -= 2;

    ASSERT_THAT(nodes(nodeListProperty.begin(), decrementIterator), ElementsAre(node1, node2, node3));
}

TEST_F(NodeListProperty, decrement_iterator_by_two_returns_iterator)
{
    auto end = nodeListProperty.end();

    auto decrementIterator = end -= 2;

    ASSERT_THAT(nodes(nodeListProperty.begin(), decrementIterator), ElementsAre(node1, node2, node3));
}

TEST_F(NodeListProperty, access_iterator)
{
    auto iterator = std::next(nodeListProperty.begin(), 3);

    auto accessIterator = iterator[2];

    ASSERT_THAT(nodes(accessIterator, nodeListProperty.end()), ElementsAre(node3, node4, node5));
}

TEST_F(NodeListProperty, add_iterator_by_index_second_operand)
{
    auto begin = nodeListProperty.begin();

    auto addedIterator = begin + 2;

    ASSERT_THAT(nodes(addedIterator, nodeListProperty.end()), ElementsAre(node3, node4, node5));
}

TEST_F(NodeListProperty, add_iterator_by_index_first_operand)
{
    auto begin = nodeListProperty.begin();

    auto addedIterator = 2 + begin;

    ASSERT_THAT(nodes(addedIterator, nodeListProperty.end()), ElementsAre(node3, node4, node5));
}

TEST_F(NodeListProperty, subtract_iterator)
{
    auto end = nodeListProperty.end();

    auto subtractedIterator = end - 3;

    ASSERT_THAT(nodes(subtractedIterator, nodeListProperty.end()), ElementsAre(node3, node4, node5));
}

TEST_F(NodeListProperty, compare_equal_iterator_are_equal)
{
    auto first = nodeListProperty.begin();
    auto second = nodeListProperty.begin();

    auto isEqual = first == second;

    ASSERT_TRUE(isEqual);
}

TEST_F(NodeListProperty, compare_equal_iterator_are_not_equal)
{
    auto first = nodeListProperty.begin();
    auto second = std::next(nodeListProperty.begin());

    auto isEqual = first == second;

    ASSERT_FALSE(isEqual);
}

TEST_F(NodeListProperty, compare_unqual_iterator_are_equal)
{
    auto first = nodeListProperty.begin();
    auto second = nodeListProperty.begin();

    auto isUnequal = first != second;

    ASSERT_FALSE(isUnequal);
}

TEST_F(NodeListProperty, compare_unequal_iterator_are_not_equal)
{
    auto first = nodeListProperty.begin();
    auto second = std::next(nodeListProperty.begin());

    auto isUnequal = first != second;

    ASSERT_TRUE(isUnequal);
}

TEST_F(NodeListProperty, compare_less_iterator_are_not_less_if_equal)
{
    auto first = nodeListProperty.begin();
    auto second = nodeListProperty.begin();

    auto isLess = first < second;

    ASSERT_FALSE(isLess);
}

TEST_F(NodeListProperty, compare_less_iterator_are_not_less_if_greater)
{
    auto first = std::next(nodeListProperty.begin());
    auto second = nodeListProperty.begin();

    auto isLess = first < second;

    ASSERT_FALSE(isLess);
}

TEST_F(NodeListProperty, compare_less_iterator_are_less_if_less)
{
    auto first = nodeListProperty.begin();
    auto second = std::next(nodeListProperty.begin());

    auto isLess = first < second;

    ASSERT_TRUE(isLess);
}

TEST_F(NodeListProperty, compare_less_equal_iterator_are_less_equal_if_equal)
{
    auto first = nodeListProperty.begin();
    auto second = nodeListProperty.begin();

    auto isLessEqual = first <= second;

    ASSERT_TRUE(isLessEqual);
}

TEST_F(NodeListProperty, compare_less_equal_iterator_are_not_less_equal_if_greater)
{
    auto first = std::next(nodeListProperty.begin());
    auto second = nodeListProperty.begin();

    auto isLessEqual = first <= second;

    ASSERT_FALSE(isLessEqual);
}

TEST_F(NodeListProperty, compare_less_equal_iterator_are_less_equal_if_less)
{
    auto first = nodeListProperty.begin();
    auto second = std::next(nodeListProperty.begin());

    auto isLessEqual = first <= second;

    ASSERT_TRUE(isLessEqual);
}

TEST_F(NodeListProperty, compare_greater_iterator_are_greater_if_equal)
{
    auto first = nodeListProperty.begin();
    auto second = nodeListProperty.begin();

    auto isGreater = first > second;

    ASSERT_FALSE(isGreater);
}

TEST_F(NodeListProperty, compare_greater_iterator_are_greater_if_greater)
{
    auto first = std::next(nodeListProperty.begin());
    auto second = nodeListProperty.begin();

    auto isGreater = first > second;

    ASSERT_TRUE(isGreater);
}

TEST_F(NodeListProperty, compare_greater_iterator_are_not_greater_if_less)
{
    auto first = nodeListProperty.begin();
    auto second = std::next(nodeListProperty.begin());

    auto isGreater = first > second;

    ASSERT_FALSE(isGreater);
}

TEST_F(NodeListProperty, compare_greater_equal_iterator_are_greater_equal_if_equal)
{
    auto first = nodeListProperty.begin();
    auto second = nodeListProperty.begin();

    auto isGreaterEqual = first >= second;

    ASSERT_TRUE(isGreaterEqual);
}

TEST_F(NodeListProperty, compare_greater_equal_iterator_are_greater_equal_if_greater)
{
    auto first = std::next(nodeListProperty.begin());
    auto second = nodeListProperty.begin();

    auto isGreaterEqual = first >= second;

    ASSERT_TRUE(isGreaterEqual);
}

TEST_F(NodeListProperty, compare_greater_equal_iterator_are_not_greater_equal_if_less)
{
    auto first = nodeListProperty.begin();
    auto second = std::next(nodeListProperty.begin());

    auto isGreaterEqual = first >= second;

    ASSERT_FALSE(isGreaterEqual);
}

TEST_F(NodeListProperty, dereference_iterator)
{
    auto iterator = std::next(nodeListProperty.begin());

    auto node = *iterator;

    ASSERT_THAT(node, Eq(node2));
}

TEST_F(NodeListProperty, iter_swap)
{
    auto first = std::next(nodeListProperty.begin(), 2);
    auto second = nodeListProperty.begin();

    nodeListProperty.iterSwap(first, second);

    ASSERT_THAT(nodes(), ElementsAre(node3, node2, node1, node4, node5));
}

TEST_F(NodeListProperty, rotate)
{
    auto first = std::next(nodeListProperty.begin());
    auto newFirst = std::next(nodeListProperty.begin(), 2);
    auto last = std::prev(nodeListProperty.end());

    nodeListProperty.rotate(first, newFirst, last);

    ASSERT_THAT(nodes(), ElementsAre(node1, node3, node4, node2, node5));
}

TEST_F(NodeListProperty, rotate_calls_node_ordered_changed)
{
    auto first = std::next(nodeListProperty.begin());
    auto newFirst = std::next(nodeListProperty.begin(), 2);
    auto last = std::prev(nodeListProperty.end());

    EXPECT_CALL(abstractViewMock, nodeOrderChanged(ElementsAre(node1, node3, node4, node2, node5)));

    nodeListProperty.rotate(first, newFirst, last);
}

TEST_F(NodeListProperty, rotate_range)
{
    auto newFirst = std::prev(nodeListProperty.end(), 2);

    nodeListProperty.rotate(nodeListProperty, newFirst);

    ASSERT_THAT(nodes(), ElementsAre(node4, node5, node1, node2, node3));
}

TEST_F(NodeListProperty, rotate_returns_iterator)
{
    auto first = std::next(nodeListProperty.begin());
    auto newFirst = std::next(nodeListProperty.begin(), 2);
    auto last = std::prev(nodeListProperty.end());

    auto iterator = nodeListProperty.rotate(first, newFirst, last);

    ASSERT_THAT(iterator, Eq(first + (last - newFirst)));
}

TEST_F(NodeListProperty, rotate_range_returns_iterator)
{
    auto newFirst = std::prev(nodeListProperty.end(), 2);

    auto iterator = nodeListProperty.rotate(nodeListProperty, newFirst);

    ASSERT_THAT(iterator, Eq(nodeListProperty.begin() + (nodeListProperty.end() - newFirst)));
}

TEST_F(NodeListProperty, reverse)
{
    auto first = std::next(nodeListProperty.begin());
    auto last = std::prev(nodeListProperty.end());

    nodeListProperty.reverse(first, last);

    ASSERT_THAT(nodes(), ElementsAre(node1, node4, node3, node2, node5));
}

TEST_F(NodeListProperty, reverse_calls_node_ordered_changed)
{
    auto first = std::next(nodeListProperty.begin());
    auto last = std::prev(nodeListProperty.end());

    EXPECT_CALL(abstractViewMock, nodeOrderChanged(ElementsAre(node1, node4, node3, node2, node5)));

    nodeListProperty.reverse(first, last);
}

} // namespace
