/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "search.h"
#include "graphicsscene.h"
#include "scxmldocument.h"
#include "searchmodel.h"

#include <QSortFilterProxyModel>

using namespace ScxmlEditor::PluginInterface;
using namespace ScxmlEditor::Common;

Search::Search(QWidget *parent)
    : OutputPane(parent)
{
    m_ui.setupUi(this);

    m_model = new SearchModel(this);
    m_proxyModel = new QSortFilterProxyModel(this);

    m_proxyModel->setFilterKeyColumn(-1);
    m_proxyModel->setFilterRole(SearchModel::FilterRole);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setDynamicSortFilter(false);
    m_proxyModel->setFilterWildcard("xxxxxxxx");

    m_ui.m_searchView->setModel(m_proxyModel);

    connect(m_ui.m_searchEdit, &QLineEdit::textChanged, this, &Search::setSearchText);
    connect(m_ui.m_searchView, &ScxmlEditor::OutputPane::TableView::pressed, this, &Search::rowActivated);
    connect(m_ui.m_searchView, &ScxmlEditor::OutputPane::TableView::entered, this, &Search::rowEntered);
}

void Search::setPaneFocus()
{
    m_ui.m_searchEdit->setFocus();
}

void Search::setSearchText(const QString &text)
{
    m_model->setFilter(text);
    m_proxyModel->setFilterWildcard(text.isEmpty() ? "xxxxxxxx" : text);
}

void Search::setDocument(ScxmlDocument *document)
{
    m_document = document;
    m_model->setDocument(document);
}

void Search::setGraphicsScene(GraphicsScene *scene)
{
    m_scene = scene;
    connect(m_ui.m_searchView, &ScxmlEditor::OutputPane::TableView::mouseExited, m_scene.data(), &GraphicsScene::unhighlightAll);
}

void Search::rowEntered(const QModelIndex &index)
{
    if (m_scene) {
        ScxmlTag *tag = m_model->tag(m_proxyModel->mapToSource(index));
        if (tag)
            m_scene->highlightItems(QVector<ScxmlTag*>() << tag);
        else
            m_scene->unhighlightAll();
    }
}

void Search::rowActivated(const QModelIndex &index)
{
    if (m_scene)
        m_scene->unselectAll();

    if (m_document)
        m_document->setCurrentTag(m_model->tag(m_proxyModel->mapToSource(index)));
}
