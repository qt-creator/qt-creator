// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "stateview.h"
#include "graphicsscene.h"
#include "scxmldocument.h"
#include "scxmluifactory.h"
#include "stateitem.h"

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

StateView::StateView(StateItem *state, QWidget *parent)
    : QWidget(parent)
    , m_parentState(state)
{
    m_ui.setupUi(this);

    m_isMainView = !m_parentState;

    connect(m_ui.m_btnClose, &QPushButton::clicked, this, &StateView::closeView);
    if (!m_isMainView)
        m_ui.m_stateName->setText(m_parentState->itemId());
    m_ui.m_titleFrame->setVisible(!m_isMainView);

    initScene();
}

StateView::StateView(QWidget *parent)
    : StateView(nullptr, parent)
{
}

StateView::~StateView()
{
    clear();
}

void StateView::clear()
{
    m_scene->clearAllTags();
    m_scene->setBlockUpdates(true);
    m_scene->clear();
}

void StateView::initScene()
{
    // Init scene
    m_scene = new GraphicsScene(this);
    m_ui.m_graphicsView->setGraphicsScene(m_scene);
}

void StateView::closeView()
{
    deleteLater();
}

void StateView::setUiFactory(ScxmlUiFactory *factory)
{
    m_scene->setUiFactory(factory);
    m_ui.m_graphicsView->setUiFactory(factory);
}

void StateView::setDocument(ScxmlDocument *doc)
{
    // Set document to scene
    m_scene->setDocument(doc);
    m_ui.m_graphicsView->setDocument(doc);
    if (doc)
        connect(doc, &ScxmlDocument::colorThemeChanged, m_scene, [this] { m_scene->invalidate(); });
}

StateItem *StateView::parentState() const
{
    return m_parentState;
}

GraphicsScene *StateView::scene() const
{
    return m_scene;
}

GraphicsView *StateView::view() const
{
    return m_ui.m_graphicsView;
}
