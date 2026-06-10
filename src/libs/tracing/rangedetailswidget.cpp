// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rangedetailswidget.h"

#include "tracingtr.h"

#include <utils/elidinglabel.h>
#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>

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

static QColor themeColor(Utils::Theme::Color role)
{
    if (Utils::creatorTheme())
        return Utils::creatorTheme()->color(role);
    return QColor();
}

RangeDetailsWidget::RangeDetailsWidget(QWidget *parent)
    : QWidget(parent)
{
    using namespace Layouting;

    setObjectName("RangeDetails");
    setWindowTitle(Tr::tr("Details"));
    setAutoFillBackground(true);
    setMinimumWidth(150);

    m_titleLabel = new Utils::ElidingLabel;
    QFont boldFont = m_titleLabel->font();
    boldFont.setBold(true);
    m_titleLabel->setFont(boldFont);
    m_titleLabel->setTextFormat(Qt::PlainText);

    auto separator = new QWidget;
    separator->setFixedHeight(1);
    separator->setAutoFillBackground(true);
    {
        auto pal = separator->palette();
        pal.setColor(QPalette::Window, themeColor(Utils::Theme::PanelTextColorMid));
        separator->setPalette(pal);
    }

    m_model = new QStandardItemModel(this);
    m_model->setColumnCount(2);

    m_treeView = new QTreeView;
    m_treeView->setModel(m_model);
    m_treeView->setHeaderHidden(true);
    m_treeView->setRootIsDecorated(true);
    m_treeView->setFrameShape(QFrame::NoFrame);
    m_treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setUniformRowHeights(true);
    m_treeView->setWordWrap(true);
    m_treeView->setItemsExpandable(true);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    Column{
        noMargin,
        spacing(0),
        Widget{
            bindTo(&m_titleBar),
            Layouting::autoFillBackground(true),
            Row{customMargins(8, 0, 4, 0), spacing(2), m_titleLabel},
        },
        separator,
        m_treeView,
        stretch(2, 1),
    }
        .attachTo(this);

    m_titleBar->setFixedHeight(24);

    // Apply theme colors
    //auto pal = palette();
    //pal.setColor(QPalette::Window, themeColor(Utils::Theme::Timeline_PanelBackgroundColor));
    //pal.setColor(QPalette::WindowText, themeColor(Utils::Theme::Timeline_TextColor));
    //setPalette(pal);
    //
    //auto titlePal = m_titleBar->palette();
    //titlePal.setColor(QPalette::Window, themeColor(Utils::Theme::Timeline_PanelHeaderColor));
    //titlePal.setColor(QPalette::WindowText, themeColor(Utils::Theme::PanelTextColorLight));
    //m_titleBar->setPalette(titlePal);
    //m_titleLabel->setPalette(titlePal);

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

    for (const auto &[key, val] : content) {
        auto keyItem = new QStandardItem(key);

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
