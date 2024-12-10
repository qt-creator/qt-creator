/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// This file is a part of the QuickQanava software library.
//
// \file	qanTableGroupItem.cpp
// \author	benoit@destrat.io
// \date	2023 01 26
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanGraph.h"
#include "./qanGroupItem.h"
#include "./qanTableGroupItem.h"
#include "./qanTableCell.h"

namespace qan { // ::qan

/* TableGroupItem Object Management *///---------------------------------------
TableGroupItem::TableGroupItem(QQuickItem* parent) :
    qan::GroupItem{parent}
{
    setObjectName(QStringLiteral("qan::TableGroupItem"));
    setItemStyle(qan::TableGroup::style(parent));
    setStrictDrop(false);  // Top left corner of a node is enought to allow a drop
}

TableGroupItem::~TableGroupItem()
{
    clearLayout();
}

bool    TableGroupItem::setContainer(QQuickItem* container) noexcept
{
    //qWarning() << "qan::TableGroupItem::setContainer(): container=" << container;
    if (qan::GroupItem::setContainer(container)) {
        // Note: Force reparenting all borders and cell to container, it might be nullptr
        // at initialization.
        for (const auto verticalBorder: _verticalBorders)
            if (verticalBorder != nullptr)
                verticalBorder->setParentItem(container);
        for (const auto horizontalBorder: _horizontalBorders)
            if (horizontalBorder != nullptr)
                horizontalBorder->setParentItem(container);
        for (const auto cell: _cells)
            if (cell != nullptr)
                cell->setParentItem(container);

        // Note 20240830: React to size modifications, usually table size
        // is fully initialized at this point, to prevent spurious reaction
        // set setEnabled(false).
        const auto container = getContainer();
        if (container != nullptr) {
            connect(container,  &QQuickItem::widthChanged,
                    this,       &TableGroupItem::layoutTable);
            connect(container,  &QQuickItem::heightChanged,
                    this,       &TableGroupItem::layoutTable);
        }

        return true;
    }
    return false;
}
//-----------------------------------------------------------------------------


/* Borders and Cells Management *///-------------------------------------------
void    TableGroupItem::clearLayout()
{
    //Warning() << "qan::TableGroupItem::clearLayout()";
    for (const auto verticalBorder: _verticalBorders)
        if (verticalBorder != nullptr)
            verticalBorder->deleteLater();
    _verticalBorders.clear();
    for (const auto horizontalBorder: _horizontalBorders)
        if (horizontalBorder != nullptr)
            horizontalBorder->deleteLater();
    _horizontalBorders.clear();

    for (const auto cell: _cells)
        if (cell != nullptr)
            cell->deleteLater();
    _cells.clear();
}

void    TableGroupItem::initialize(int cols, int rows)
{
    //qWarning() << "qan::TableGroupItem::initialize(): rows=" << rows << "  cols=" << cols;
    //qWarning() << "  container=" << getContainer();
    if (rows <= 0 || cols <= 0) {
        qWarning() << "TableGroupItem::initialize(): Error, invalid rows or cols count.";
        return;
    }

    auto engine = qmlEngine(this);
    if (engine == nullptr) {
        qWarning() << "qan::TableGroupItem::initialize(): Error, no QML engine.";
        return;
    }
    clearLayout();
    createCells(rows * cols);  // Create cells

    auto borderComponent = new QQmlComponent(engine, "qrc:/QuickQanava/TableBorder.qml",
                                             QQmlComponent::PreferSynchronous, nullptr);

    // Notes:
    // - There is no "exterior" borders:
    //    - So there is  cols-1 vertical borders
    //    - And there is rows-1 horizontal borders
    //    - For exemple 6 cells == 4 borders
    // - There is rows*cols cells for (rows-1) + (cols-1) borders.
    //
    // Internal cells vector is indexed row major:
    //   cell1 | cell2 | cell3
    //   ------+-------+------
    //   cell4 | cell5 | cell6
    //   ------+-------+------
    //   cell7 | cell8 | cell9
    //
    // So cell index in _cells at (col=c, row=r) is _cells[(r * cols) + c]

    createBorders(cols - 1, rows - 1);
    int c = 1;
    for (auto verticalBorder: _verticalBorders) {
        if (verticalBorder == nullptr)
            continue;
        for (int r = 0; r < rows; r++) {
            verticalBorder->addPrevCell(_cells[(r * cols) + c - 1]);
            verticalBorder->addNextCell(_cells[(r * cols) + c]);
        }
        c++;
    }

    int r = 1;
    for (auto horizontalBorder: _horizontalBorders) {
        if (horizontalBorder == nullptr)
            continue;
        for (int c = 0; c < cols; c++) {
            horizontalBorder->addPrevCell(_cells[((r-1) * cols) + c]);
            horizontalBorder->addNextCell(_cells[(r * cols)     + c]);
        }
        r++;
    }

    borderComponent->deleteLater();

    // Note 20240830: Do not call initializeTableLayout(),
    // it is up to the user to do that, it might not be
    // desirable in vertain serialization use cases.
}

void    TableGroupItem::createCells(int cellsCount)
{
    if (cellsCount <= 0) {
        qWarning() << "TableGroupItem::createCells(): Error, invalid rows or cols count.";
        return;
    }
    if (cellsCount == static_cast<int>(_cells.size()))
        return;

    auto engine = qmlEngine(this);
    if (engine == nullptr) {
        qWarning() << "qan::TableGroupItem::createCells(): Error, no QML engine.";
        return;
    }

    // Create cells
    auto cellComponent = new QQmlComponent(engine, "qrc:/QuickQanava/TableCell.qml",
                                           QQmlComponent::PreferSynchronous, nullptr);
    for (auto c = 0; c < cellsCount; c++) {
        auto cell = qobject_cast<qan::TableCell*>(createFromComponent(*cellComponent));
        if (cell != nullptr) {
            _cells.push_back(cell);
            cell->setParentItem(getContainer() != nullptr ? getContainer() : this);
            cell->setVisible(true);
            cell->setTable(getTableGroup());
        }
    }

    cellComponent->deleteLater();
}

void    TableGroupItem::createBorders(int verticalBordersCount, int horizontalBordersCount)
{
    //qWarning() << "qan::TableGroupItem::createBorders(): verticalBordersCount=" << verticalBordersCount << "  horizontalBordersCount=" << horizontalBordersCount;
    if (verticalBordersCount < 0 ||     // Might be 0 for 1x1 tables
        horizontalBordersCount < 0) {
        qWarning() << "TableGroupItem::createBorders(): Error, invalid vertical or horizontal borders count.";
        return;
    }
    auto engine = qmlEngine(this);
    if (engine == nullptr) {
        qWarning() << "qan::TableGroupItem::createBorders(): Error, no QML engine.";
        return;
    }

    const auto borderComponent = new QQmlComponent(engine, "qrc:/QuickQanava/TableBorder.qml",
                                                   QQmlComponent::PreferSynchronous, nullptr);

    qan::TableBorder* prevBorder = nullptr;
    if (verticalBordersCount != static_cast<int>(_verticalBorders.size())) {
        for (auto v = 0; v < verticalBordersCount; v++) {
            auto border = qobject_cast<qan::TableBorder*>(createFromComponent(*borderComponent));
            if (border != nullptr) {
                border->setTableGroup(getTableGroup());
                border->setOrientation(Qt::Vertical);
                border->setParentItem(getContainer() != nullptr ? getContainer() : this);
                border->setVisible(true);
                border->setPrevBorder(prevBorder);
                connect(border, &qan::TableBorder::modified,
                        this,   [this]() {
                    const auto graph = this->getGraph();
                    const auto tableGroup = this->getTableGroup();
                    if (graph != nullptr &&
                        tableGroup != nullptr)
                    emit graph->tableModified(tableGroup);
                });
                _verticalBorders.push_back(border);

                if (prevBorder != nullptr)  // Audacious initialization of prevBorder nextBorder
                    prevBorder->setNextBorder(border);  // with this border
                prevBorder = border;
            }
        }
    }
    prevBorder = nullptr;
    if (horizontalBordersCount != static_cast<int>(_horizontalBorders.size())) {
        for (auto h = 0; h < horizontalBordersCount; h++) {
            auto border = qobject_cast<qan::TableBorder*>(createFromComponent(*borderComponent));
            if (border != nullptr) {
                border->setTableGroup(getTableGroup());
                border->setOrientation(Qt::Horizontal);
                border->setParentItem(getContainer() != nullptr ? getContainer() : this);
                border->setVisible(true);
                border->setPrevBorder(prevBorder);
                connect(border, &qan::TableBorder::modified,
                        this,   [this]() {
                    const auto graph = this->getGraph();
                    const auto tableGroup = this->getTableGroup();
                    if (graph != nullptr &&
                        tableGroup != nullptr)
                    emit graph->tableModified(tableGroup);
                });
                _horizontalBorders.push_back(border);

                if (prevBorder != nullptr)  // Audacious initialization of prevBorder nextBorder
                    prevBorder->setNextBorder(border);  // with this border
                prevBorder = border;
            }
        }
    }

    borderComponent->deleteLater();
}

auto TableGroupItem::createFromComponent(QQmlComponent& component) -> QQuickItem*
{
    if (!component.isReady()) {
        qWarning() << "qan::TableGroupItem::classBegin(): createTableCell(): Error table cell component is not ready.";
        qWarning() << component.errorString();
        return nullptr;
    }
    const auto rootContext = qmlContext(this);
    if (rootContext == nullptr) {
        qWarning() << "qan::TableGroupItem::classBegin(): createTableCell(): Error, no QML context.";
        return nullptr;
    }
    QQuickItem* item = nullptr;
    QObject* object = component.beginCreate(rootContext);
    if (object == nullptr ||
        component.isError()) {
        if (object != nullptr)
            object->deleteLater();
        return nullptr;
    }
    component.completeCreate();
    if (!component.isError()) {
        QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
        item = qobject_cast<QQuickItem*>(object);
    } // Note: There is no leak until cpp ownership is set
    if (item != nullptr)
        item->setVisible(true);
    return item;
};

void    TableGroupItem::initializeTableLayout()
{
    const auto tableGroup = getTableGroup();
    if (tableGroup == nullptr)
        return;
    const auto tableContainer = getContainer();
    if (tableContainer == nullptr)
        return;
    const auto tableWidth = tableContainer->width();
    const auto tableHeight = tableContainer->height();
    const auto tableSize = tableContainer->size();
    qWarning() << "qan::TableGroupItem::initializeTableLayout(): tableSize=" << tableSize;
    if (qRound(tableWidth) <= 0 || qRound(tableHeight) <= 0)
        return;

    const int cols = tableGroup->getCols();
    const int rows = tableGroup->getRows();
    const auto spacing = tableGroup != nullptr ? tableGroup->getCellSpacing() :
                                                 5.;
    const auto padding = tableGroup != nullptr ? tableGroup->getTablePadding() :
                                                 2.;

    if (cols <= 0 || rows <= 0) {
        qWarning() << "qan::TableGroupItem::initializeTableLayout(): Error, rows and columns count can't be <= 0.";
        return;
    }
    if (spacing < 0 || padding < 0) {
        qWarning() << "qan::TableGroupItem::initializeTableLayout(): Error, padding and spacing can't be < 0.";
        return;
    }

    const auto cellWidth = tableWidth > 0. ? (tableWidth
                                           - (2 * padding)
                                           - ((cols - 1) * spacing)) / cols :
                                          0.;
    const auto cellHeight = tableHeight > 0. ? (tableHeight
                                             - (2 * padding)
                                             - ((rows - 1) * spacing)) / rows :
                                            0.;

    //qWarning() << "  cellWidth=" << cellWidth << " cellHeight=" << cellHeight;

    if (cellWidth < 0. || cellHeight < 0.) {
        qWarning() << "qan::TableGroupItem::initializeTableLayout(): Error, invalid cell width/height.";
        return;
    }
    // Note: cells are laid out by their borders, do not set their geometry
    // Layout in space

    // Layout vertical borders:
    // |             cell         |         cell         |         cell             |
    // | padding |   cell   |   border  |   cell   |   border  |   cell   | padding |
    //                       <-spacing->            <-spacing->
    if (static_cast<int>(_verticalBorders.size()) == cols - 1) {
        const auto borderWidth = 3.;    // All easy mouse resize handling
        const auto borderWidth2 = borderWidth / 2.;
        for (auto c = 1; c <= (cols - 1); c++) {
            auto* verticalBorder = _verticalBorders[c-1];
            const auto x = padding +
                           ((c - 1) * spacing) +
                           (c * cellWidth) +
                           (spacing / 2.);
            const auto borderX = x - borderWidth2;
            verticalBorder->setSx(borderX / tableWidth);
            verticalBorder->setY(0.);
            verticalBorder->setWidth(borderWidth);
            verticalBorder->setHeight(tableHeight);
        }
    } else
        qWarning() << "qan::TableGoupItem::initializeTableLayout(): Invalid vertical border count.";

    // Layout horizontal borders
    if (static_cast<int>(_horizontalBorders.size()) == rows - 1) {
        const auto borderHeight = 3.;    // All easy mouse resize handling
        const auto borderHeight2 = borderHeight / 2.;
        for (auto r = 1; r <= (rows - 1); r++) {
            auto* horizontalBorder = _horizontalBorders[r-1];
            const auto y = padding +
                           ((r - 1) * spacing) +
                           (r * cellHeight) +
                           (spacing / 2.);
            horizontalBorder->setX(0.);
            // FIXME #1756 BTW, ce serait peut-être bien aussi de normaliser
            // width et heght en prevision merge...
            const auto borderY = y - borderHeight2;
            horizontalBorder->setSy(borderY / tableHeight);
            horizontalBorder->setWidth(tableWidth);
            horizontalBorder->setHeight(borderHeight);
        }
    } else
        qWarning() << "qan::TableGoupItem::initializeTableLayout(): Invalid horizontal border count.";

    // Note: There is no need to manually call borders layoutCells() method
    // it will be called automatically when border are moved.
    // Note 20230406: In fact calling layout cell is necessary for rows==1, cols==1
    // table that need special handling to dimension cells since there is no horiz/vert borders.
    // FIXME #1756
    layoutTable();
    //layoutCells();
}

void    TableGroupItem::layoutTable()
{
    if (!isEnabled())
        return; // Note 20240830: prevent spurious layouts during serialization (see hlg::TableGroup::serializeFromJson()).
    const auto tableContainer = getContainer();
    if (tableContainer == nullptr)
        return;

    const auto tableSize = tableContainer->size();
    const auto tableWidth = tableContainer->width();
    const auto tableHeight = tableContainer->height();

    // During initial polish loop and since we are binded directly on width/
    // height change, table container size might be empty.
    if (tableSize.isEmpty() || tableSize.isNull())
        return;

    qWarning() << "TableGroupItem::layoutTable(): " << getGroup()->getLabel() <<
        "  tableWidth=" << tableWidth << "tableHeight=" << tableHeight;

    // Project normalized sx/sy coordinates to table item container CS.
    for (const auto verticalBorder: _verticalBorders) {
        if (verticalBorder == nullptr)
            continue;
        verticalBorder->setX(verticalBorder->getSx() * tableWidth);
        verticalBorder->setY(0.);
        verticalBorder->setHeight(tableHeight);
    }

    for (const auto horizontalBorder: _horizontalBorders) {
        if (horizontalBorder == nullptr)
            continue;
        horizontalBorder->setX(0.);
        horizontalBorder->setY(horizontalBorder->getSy() * tableHeight);
        horizontalBorder->setWidth(tableWidth);
    }

    layoutCells();
}

void    TableGroupItem::layoutCells()
{
    //qWarning() << "qan::TableGroupItem::layoutCells()";
    for (const auto verticalBorder: _verticalBorders) {
        if (verticalBorder != nullptr)
            verticalBorder->layoutCells();
    }
    for (const auto horizontalBorder: _horizontalBorders) {
        if (horizontalBorder != nullptr)
            horizontalBorder->layoutCells();
    }

    const auto tableContainer = getContainer();
    if (tableContainer == nullptr)
        return;
    const auto tableWidth = tableContainer->width();
    const auto tableHeight = tableContainer->height();

    // Special handling for 1 row or 1 column table: since there is
    // no "moveable" border, calls to layoutCells() do not set either width/height
    // of cells. Manually set correct width / height:
    const auto tableGroup = getTableGroup();
    if (tableGroup == nullptr)
        return;
    //qWarning() << "  tableGroup->getRows()=" << tableGroup->getRows();
    //qWarning() << "  tableGroup->getCols()=" << tableGroup->getCols();
    const auto padding = getTableGroup() != nullptr ? getTableGroup()->getTablePadding() : 2.;
    const auto padding2 = padding * 2.;
    if (tableGroup->getCols() == 1) {
        for (const auto cell: _cells)
            if (cell != nullptr) {
                cell->setX(padding);
                cell->setWidth(tableWidth - padding2);
            }
    }
    if (tableGroup->getRows() == 1) {
        for (const auto cell: _cells)
            if (cell != nullptr) {
                cell->setHeight(tableHeight - padding2);
                cell->setY(padding);
            }
    }
}

bool    TableGroupItem::setGroup(qan::Group* group) noexcept
{
    if (qan::GroupItem::setGroup(group)) {
        auto tableGroup = qobject_cast<qan::TableGroup*>(group);
        if (tableGroup != nullptr) {
            initialize(tableGroup->getCols(),
                       tableGroup->getRows());

            // Set borders reference to group
            for (auto border: _horizontalBorders)
                if (border)
                    border->setTableGroup(tableGroup);
            for (auto border: _verticalBorders)
                if (border)
                    border->setTableGroup(tableGroup);
            connect(tableGroup, &qan::TableGroup::cellSpacingChanged,
                    this,       &qan::TableGroupItem::layoutCells);
            connect(tableGroup, &qan::TableGroup::cellMinimumSizeChanged,
                    this,       &qan::TableGroupItem::layoutCells);
            connect(tableGroup, &qan::TableGroup::tablePaddingChanged,
                    this,       &qan::TableGroupItem::layoutCells);

            // Set cell reference to group
            for (auto cell: _cells)
                if (cell != nullptr)
                    cell->setTable(tableGroup);

            // Note 20240831: Do not layout it is up to the user to call qan::TableGroup::initializeLayout()
            return true;
        }
    }
    return false;
}

const qan::TableGroup*  TableGroupItem::getTableGroup() const { return qobject_cast<const qan::TableGroup*>(getGroup()); }
qan::TableGroup*        TableGroupItem::getTableGroup() { return qobject_cast<qan::TableGroup*>(getGroup()); };
//-----------------------------------------------------------------------------


/* TableGroupItem DnD Management *///------------------------------------------
void    TableGroupItem::groupNodeItem(qan::NodeItem* nodeItem, qan::TableCell* groupCell, bool transform)
{
    //qWarning() << "qan::TableGroupItem::groupNodeItem(): nodeItem=" << nodeItem << "  groupCell=" << groupCell;
    // PRECONDITIONS:
        // nodeItem can't be nullptr
        // A 'container' must have been configured
    Q_UNUSED(groupCell)
    Q_UNUSED(transform)
    if (nodeItem == nullptr ||
        getContainer() == nullptr)   // A container must have configured in concrete QML group component
        return;

    // Note: no need for the container to be visible or open.
    const auto globalPos = nodeItem->mapToGlobal(QPointF{0., 0.});
    const auto groupPos = getContainer()->mapFromGlobal(globalPos);
    if (groupCell != nullptr) {
        if (std::find(_cells.cbegin(), _cells.cend(), groupCell) != _cells.cend()) {
            groupCell->setItem(nodeItem);
            nodeItem->getNode()->setCell(groupCell);
        } else
            qWarning() << "qan::TableGroupItem::groupNodeItem(): Internal error, groupCell could not be found in internal cells.";
    } else {
        // Find cell at groupPos and attach node to cell
        for (const auto& cell: _cells) {
            if (cell == nullptr)
                continue;
            const auto cellBr = cell->boundingRect().translated(cell->position());
            if (cellBr.contains(groupPos)) {
                cell->setItem(nodeItem);
                nodeItem->getNode()->setCell(cell);
                break;
            }
        }
    }

    groupMoved();           // Force call to groupMoved() to update group adjacent edges

    // Notify that the table has changed
    const auto graph = this->getGraph();
    const auto tableGroup = this->getTableGroup();
    if (graph != nullptr &&
        tableGroup != nullptr)
        emit graph->tableModified(tableGroup);

    endProposeNodeDrop();
}

void    TableGroupItem::ungroupNodeItem(qan::NodeItem* nodeItem, bool transform)
{
    if (nodeItem == nullptr)   // A container must have configured in concrete QML group component
        return;
    if (getGraph() != nullptr &&
        getGraph()->getContainerItem() != nullptr) {
        auto nodeCell = nodeItem->getNode()->getCell();
        if (nodeCell != nullptr) {
            const QPointF nodeGlobalPos = nodeItem->mapToItem(getGraph()->getContainerItem(),
                                                              QPointF{0., 0.});

            nodeCell->restoreCache(nodeItem);
            nodeCell->setItem(nullptr);
            nodeCell->setUserProp(QVariant{});
            nodeItem->setParentItem(getGraph()->getContainerItem());
            if (transform)
                nodeItem->setPosition(nodeGlobalPos + QPointF{10., 10.});  // A delta to visualize ungroup
            nodeItem->setZ(z() + 1.);
            nodeItem->getNode()->setCell(nullptr);
        }
    }
}

void    TableGroupItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    qan::NodeItem::mouseDoubleClickEvent(event);
    if (event->button() == Qt::LeftButton &&
        (getNode() != nullptr &&
         !getNode()->getLocked()))
        emit groupDoubleClicked(this, event->localPos());
}

void    TableGroupItem::mousePressEvent(QMouseEvent* event)
{
    qan::NodeItem::mousePressEvent(event);

    if (event->button() == Qt::LeftButton &&    // Selection management
         getGroup() &&
         isSelectable() &&
         !getCollapsed() &&         // Locked/Collapsed group is not selectable
         !getNode()->getLocked()) {
        if (getGraph())
            getGraph()->selectGroup(*getGroup(), event->modifiers());
    }

    if (event->button() == Qt::LeftButton)
        emit groupClicked(this, event->localPos());
    else if (event->button() == Qt::RightButton)
        emit groupRightClicked(this, event->localPos());
}
//-----------------------------------------------------------------------------

} // ::qan
