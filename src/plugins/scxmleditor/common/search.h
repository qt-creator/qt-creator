// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "outputpane.h"

#include <utils/utilsicons.h>

#include <QCoreApplication>
#include <QFrame>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QSortFilterProxyModel)

namespace Utils { class FancyLineEdit; }

namespace ScxmlEditor {

namespace PluginInterface {
class GraphicsScene;
class ScxmlDocument;
} // namespace PluginInterface

namespace OutputPane { class TableView; }

namespace Common {

class SearchModel;

/**
 * @brief The Search class provides the way to search/find items.
 */
class Search : public ScxmlEditor::OutputPane::OutputPane
{
    Q_OBJECT

public:
    explicit Search(QWidget *parent = nullptr);

    QString title() const override
    {
        return QCoreApplication::translate("QtC::ScxmlEditor", "Search");
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

    Utils::FancyLineEdit *m_searchEdit;
    ScxmlEditor::OutputPane::TableView *m_searchView;
};

} // namespace Common
} // namespace ScxmlEditor
