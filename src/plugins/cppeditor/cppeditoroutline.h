// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppoutlinemodel.h"

#include <QModelIndex>
#include <QObject>

#include <memory>

QT_BEGIN_NAMESPACE
class QAction;
class QSortFilterProxyModel;
class QTimer;
QT_END_NAMESPACE

namespace TextEditor { class TextEditorWidget; }
namespace Utils { class TreeViewComboBox; }

namespace CppEditor {
class CppEditorWidget;

namespace Internal {

class CppEditorOutline : public QObject
{
    Q_OBJECT

public:
    explicit CppEditorOutline(CppEditorWidget *editorWidget);

    QWidget *widget() const; // Must be deleted by client.

public slots:
    void updateIndex();

private:
    void updateNow();
    void updateIndexNow();
    void updateToolTip();
    void gotoSymbolInEditor();

    CppEditorOutline();

    bool isSorted() const;

    OutlineModel *m_model = nullptr; // Not owned

    CppEditorWidget *m_editorWidget = nullptr;

    Utils::TreeViewComboBox *m_combo = nullptr; // Not owned
    QSortFilterProxyModel *m_proxyModel = nullptr;
    QAction *m_sortAction = nullptr;
    QTimer *m_updateIndexTimer = nullptr;
};

} // namespace Internal
} // namespace CppEditor
