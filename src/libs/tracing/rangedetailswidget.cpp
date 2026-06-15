// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rangedetailswidget.h"

#include "tracingtr.h"

#include <utils/elidinglabel.h>
#include <utils/itemviews.h>
#include <utils/layoutbuilder.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>
#include <utils/widgets.h>

#include <QFont>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPalette>
#include <QStandardItemModel>
#include <QTreeView>

namespace Timeline {

RangeDetailsWidget::RangeDetailsWidget(QWidget *parent)
    : QWidget(parent)
{
    using namespace Layouting;

    setObjectName("RangeDetails");
    setWindowTitle(Tr::tr("Details"));
    setMinimumWidth(150);

    m_titleBar = new Utils::StyledBar;

    m_titleLabel = new Utils::ElidingLabel;
    m_titleLabel->setFont(Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementH6));
    m_titleLabel->setTextFormat(Qt::PlainText);

    m_model = new QStandardItemModel(this);
    m_model->setColumnCount(2);

    m_treeView = new Utils::TreeView;
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_treeView->setWordWrap(true);
    m_treeView->setItemsExpandable(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

    Column {
        customMargins(Utils::StyleHelper::SpacingTokens::PaddingVS, 0, 0, 0),
        spacing(0),
        m_titleLabel,
    }.attachTo(m_titleBar);

    connect(m_treeView, &QAbstractItemView::doubleClicked, this,
            [this](const QModelIndex &index) {
                // Only top-level rows map 1:1 to orderedDetails content rows;
                // child rows (the "Arguments" JSON expansion) are not navigable.
                if (!index.parent().isValid())
                    emit rowDoubleClicked(index.row());
            });

    Column{
        noMargin,
        spacing(0),
        m_titleBar,
        m_treeView,
    }.attachTo(this);

    clear();
}

void RangeDetailsWidget::setData(const QString &title, const QList<QPair<QString, QString>> &content)
{
    m_hasData = true;
    m_titleLabel->setText(title);
    rebuildRows(content);
}

void RangeDetailsWidget::clear()
{
    m_hasData = false;
    m_titleLabel->setText(Tr::tr("No item selected"));
    rebuildRows({});
}

// Fills the row formed by (keyItem, valueItem) from a JSON value. Containers are
// expanded into child rows of keyItem -- children must hang off the column-0 item,
// otherwise the tree cannot navigate or expand them. Scalars go into valueItem.
static void fillJson(QStandardItem *keyItem, QStandardItem *valueItem, const QJsonValue &value)
{
    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            auto childKey = new QStandardItem(it.key());
            auto childVal = new QStandardItem;
            fillJson(childKey, childVal, it.value());
            keyItem->appendRow({childKey, childVal});
        }
    } else if (value.isArray()) {
        const QJsonArray arr = value.toArray();
        int i = 0;
        for (const auto &elem : arr) {
            auto childKey = new QStandardItem(QString::number(i++));
            auto childVal = new QStandardItem;
            fillJson(childKey, childVal, elem);
            keyItem->appendRow({childKey, childVal});
        }
    } else {
        valueItem->setText(value.toVariant().toString());
    }
}

void RangeDetailsWidget::rebuildRows(const QList<QPair<QString, QString>> &content)
{
    m_model->removeRows(0, m_model->rowCount());

    constexpr Utils::StyleHelper::TextFormat keyFormat {
        .themeColor = Utils::Theme::Token_Text_Muted,
        .uiElement = Utils::StyleHelper::UiElementCaptionStrong,
    };
    const QFont keyFont = keyFormat.font();
    const int lineHeight = int(keyFormat.lineHeight() * 1.8);
    for (const auto &[key, val] : content) {
        auto keyItem = new QStandardItem(key);
        keyItem->setFont(keyFont);
        keyItem->setForeground(keyFormat.color());

        if (key == "Arguments") {
            // Assume the value is a JSON object. Parse it and display each key-value pair on a separate line:
            QJsonParseError error;
            QJsonDocument jsonDoc
                = QJsonDocument::fromJson(QString("{%1}").arg(val).toUtf8(), &error);
            if (error.error == QJsonParseError::NoError) {
                auto valItem = new QStandardItem;
                fillJson(keyItem,
                         valItem,
                         jsonDoc.isObject() ? QJsonValue(jsonDoc.object())
                                            : QJsonValue(jsonDoc.array()));
                m_model->appendRow({keyItem, valItem});
                continue;
            }
        }

        auto valItem = new QStandardItem(val);
        valItem->setSizeHint({-1, lineHeight});
        m_model->appendRow({keyItem, valItem});
    }

    m_treeView->expandAll();
}

void RangeDetailsWidget::mousePressEvent(QMouseEvent *event)
{
    event->accept();
}

void RangeDetailsWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_hasData
            && m_titleBar->geometry().contains(event->pos())) {
        emit recenterOnItem();
    }
    event->accept();
}

} // namespace Timeline
