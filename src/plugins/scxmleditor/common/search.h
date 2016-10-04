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

#pragma once

#include "outputpane.h"
#include "ui_search.h"

#include <utils/utilsicons.h>

#include <QFrame>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)

namespace ScxmlEditor {

namespace PluginInterface {
class GraphicsScene;
class ScxmlDocument;
} // namespace PluginInterface

namespace Common {

class SearchModel;

/**
 * @brief The Search class provides the way to search/find items.
 */
class Search : public OutputPane::OutputPane
{
    Q_OBJECT

public:
    explicit Search(QWidget *parent = nullptr);

    QString title() const override
    {
        return tr("Search");
    }

    QIcon icon() const override
    {
        return Utils::Icons::ZOOM_TOOLBAR.icon();
    }

    void setPaneFocus() override;
    void setDocument(PluginInterface::ScxmlDocument *document);
    void setGraphicsScene(PluginInterface::GraphicsScene *scene);

private:
    void setSearchText(const QString &text);
    void rowEntered(const QModelIndex &index);
    void rowActivated(const QModelIndex &index);

    QPointer<PluginInterface::GraphicsScene> m_scene;
    SearchModel *m_model;
    QSortFilterProxyModel *m_proxyModel;
    QPointer<PluginInterface::ScxmlDocument> m_document;
    Ui::Search m_ui;
};

} // namespace Common
} // namespace ScxmlEditor
