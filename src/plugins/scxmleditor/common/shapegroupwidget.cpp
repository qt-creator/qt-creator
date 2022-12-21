// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "shapegroupwidget.h"
#include "dragshapebutton.h"
#include "shapeprovider.h"

#include <utils/flowlayout.h>
#include <utils/utilsicons.h>

#include <QLabel>
#include <QToolBar>

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

ShapeGroupWidget::ShapeGroupWidget(ShapeProvider *shapeProvider, int groupIndex, QWidget *parent)
    : QWidget(parent)
{
    createUi();

    m_title->setText(shapeProvider->groupTitle(groupIndex));

    for (int i = 0; i < shapeProvider->shapeCount(groupIndex); ++i) {
        auto button = new DragShapeButton(this);
        button->setText(shapeProvider->shapeTitle(groupIndex, i));
        button->setIcon(shapeProvider->shapeIcon(groupIndex, i));
        button->setShapeInfo(groupIndex, i);

        m_content->layout()->addWidget(button);
    }

    connect(m_closeButton, &QToolButton::clicked, this, [this] {
        m_content->setVisible(!m_content->isVisible());
        m_closeButton->setIcon(m_content->isVisible()
                               ? Utils::Icons::COLLAPSE_TOOLBAR.icon()
                               : Utils::Icons::EXPAND_TOOLBAR.icon());
    });
}

void ShapeGroupWidget::createUi()
{
    m_title = new QLabel;
    m_title->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

    m_closeButton = new QToolButton;
    m_closeButton->setIcon(Utils::Icons::COLLAPSE_TOOLBAR.icon());

    auto toolBar = new QToolBar;
    toolBar->addWidget(m_title);
    toolBar->addWidget(m_closeButton);

    m_content = new QWidget;
    m_content->setLayout(new Utils::FlowLayout);

    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    layout()->setSpacing(0);
    layout()->addWidget(toolBar);
    layout()->addWidget(m_content);
}
