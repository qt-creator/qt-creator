// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QStackedWidget>

namespace Core {
class IEditor;
class IMode;
}

namespace ScxmlEditor {

class ScxmlTextEditor;

namespace Internal {

class ScxmlEditorStack : public QStackedWidget {
    Q_OBJECT

public:
    ScxmlEditorStack(QWidget *parent = nullptr);

    void add(ScxmlTextEditor *editor, QWidget *widget);
    QWidget *widgetForEditor(ScxmlTextEditor *editor);
    void removeScxmlTextEditor(QObject*);
    bool setVisibleEditor(Core::IEditor *xmlEditor);

private:
    void modeAboutToChange(Utils::Id m);

    QVector<ScxmlTextEditor*> m_editors;
};

} // namespace Internal
} // namespace ScxmlEditor
