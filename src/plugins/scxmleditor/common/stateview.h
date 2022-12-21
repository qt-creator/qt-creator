// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

namespace ScxmlEditor {

namespace PluginInterface {
class GraphicsScene;
class StateItem;
class ScxmlDocument;
class ScxmlUiFactory;
} // namespace PluginInterface

namespace Common {

class GraphicsView;

class StateView : public QWidget
{
    Q_OBJECT

public:
    explicit StateView(PluginInterface::StateItem *state, QWidget *parent = nullptr);
    explicit StateView(QWidget *parent = nullptr);
    ~StateView() override;

    PluginInterface::StateItem *parentState() const;
    PluginInterface::GraphicsScene *scene() const;
    GraphicsView *view() const;
    void setUiFactory(PluginInterface::ScxmlUiFactory *uifactory);
    void setDocument(PluginInterface::ScxmlDocument *doc);
    void clear();

private:
    void closeView();
    void init();
    void initScene();

    PluginInterface::StateItem *m_parentState = nullptr;
    PluginInterface::GraphicsScene *m_scene = nullptr;
    bool m_isMainView;
    GraphicsView *m_graphicsView;
};

} // namespace Common
} // namespace ScxmlEditor
