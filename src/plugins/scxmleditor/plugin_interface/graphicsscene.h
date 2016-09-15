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

#include "baseitem.h"
#include "scxmldocument.h"

#include <QGraphicsLineItem>
#include <QGraphicsScene>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QKeyEvent)

namespace ScxmlEditor {

namespace OutputPane {
class Warning;
class WarningModel;
} // namespace OutputPane

namespace PluginInterface {

class ActionHandler;
class ScxmlUiFactory;
class SnapLine;
class WarningItem;

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit GraphicsScene(QObject *parent = nullptr);
    ~GraphicsScene() override;

    QPair<bool, bool> checkSnapToItem(BaseItem *item, const QPointF &p, QPointF &pp);
    void checkItemsVisibility(double scaleFactor);
    void checkInitialState();
    void clearAllTags();
    void setBlockUpdates(bool block);

    QList<QGraphicsItem*> sceneItems(Qt::SortOrder order) const;
    ScxmlTag *tagByWarning(const OutputPane::Warning *w) const;
    void highlightWarningItem(const OutputPane::Warning *w);
    void selectWarningItem(const OutputPane::Warning *w);

    QRectF selectedBoundingRect() const;
    BaseItem *findItem(const ScxmlTag *tag) const;
    bool topMostScene() const;

    void setActionHandler(ActionHandler *mgr);
    void setWarningModel(OutputPane::WarningModel *model);
    void setUiFactory(ScxmlUiFactory *uifactory);

    void setEditorInfo(const QString &key, const QString &value);
    void setDocument(ScxmlDocument *document);
    void unselectAll();
    void unhighlightAll();
    void highlightItems(const QVector<ScxmlTag*> &lstIds);
    void addConnectableItem(ItemType type, const QPointF &pos, BaseItem *parentItem);
    void runAutomaticLayout();
    void runLayoutToSelectedStates();
    void alignStates(int alignType);
    void adjustStates(int adjustType);
    void copy();
    void cut();
    void removeSelectedItems();
    void checkPaste();
    void paste(const QPointF &targetPos);
    void setTopMostScene(bool topmost);

    ActionHandler *actionHandler() const;
    OutputPane::WarningModel *warningModel() const;
    ScxmlUiFactory *uiFactory() const;

signals:
    void openStateView(BaseItem *item);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void selectedStateCountChanged(int count);
    void selectedBaseItemCountChanged(int count);
    void pasteAvailable(bool para);

private slots:
    void warningVisibilityChanged(int type, WarningItem *item = nullptr);

private:
    void beginTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value);
    void endTagChange(ScxmlDocument::TagChange change, ScxmlTag *tag, const QVariant &value);
    void selectionChanged(bool para);

    friend class BaseItem;
    friend class WarningItem;

    void init();
    qreal selectedMaxWidth() const;
    qreal selectedMaxHeight() const;
    void removeItems(const ScxmlTag *tag);
    void addChild(BaseItem *item);
    void removeChild(BaseItem *item);
    void addWarningItem(WarningItem *item);
    void removeWarningItem(WarningItem *item);
    void connectDocument();
    void disconnectDocument();

    QPointer<ActionHandler> m_actionHandler;
    QPointer<OutputPane::WarningModel> m_warningModel;
    QPointer<ScxmlUiFactory> m_uiFactory;
    QPointer<ScxmlDocument> m_document;
    QVector<BaseItem*> m_baseItems;
    QVector<WarningItem*> m_allWarnings;
    int m_pasteCounter = 0;
    QPointer<BaseItem> m_lastPasteTargetItem;
    SnapLine *m_lineX = nullptr;
    SnapLine *m_lineY = nullptr;
    int m_selectedStateCount = 0;
    int m_selectedBaseItemCount = 0;
    int m_selectedStateTypeCount = 0;
    bool m_autoLayoutRunning = false;
    bool m_initializing = false;
    bool m_topMostScene = false;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
