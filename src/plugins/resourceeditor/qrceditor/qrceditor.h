// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/minisplitter.h>

#include <utils/filepath.h>

#include <QUndoStack>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
QT_END_NAMESPACE

namespace ResourceEditor::Internal {

class RelativeResourceModel;
class ResourceIconView;
class ResourceView;

class QrcEditor : public Core::MiniSplitter
{
    Q_OBJECT
    friend class QrcEditorTest;

public:
    QrcEditor(RelativeResourceModel *model, QWidget *parent = nullptr);
    ~QrcEditor() override;

    void loaded(bool success);

    void setResourceDragEnabled(bool e);
    bool resourceDragEnabled() const;

    const QUndoStack *commandHistory() const { return &m_history; }

    void refresh();
    void editCurrentItem();

    QString currentResourcePath() const;

    void onUndo();
    void onRedo();

    bool isIconMode() const;
    void setIconMode(bool on);

signals:
    void itemActivated(const Utils::FilePath &fileName);
    void showContextMenu(const QPoint &globalPos, const Utils::FilePath &filePath);
    void undoStackChanged(bool canUndo, bool canRedo);

private:
    void updateCurrent();
    void updateHistoryControls();

    void resolveLocationIssues(QStringList &files);

    void onAliasChanged(const QString &alias);
    void onPrefixChanged(const QString &prefix);
    void onLanguageChanged(const QString &language);
    void onRemove();
    void onRemoveNonExisting();
    void onAddFiles();
    void onAddPrefix();

    void connectFileSignals(QAbstractItemView *view);
    QAbstractItemView *currentView() const;
    void limitSelectionToShownPrefix();

    QUndoStack m_history;
    RelativeResourceModel *m_model;
    ResourceView *m_treeview;
    ResourceIconView *m_iconview;
    QStackedWidget *m_viewStack;

    QString m_currentAlias;
    QString m_currentPrefix;
    QString m_currentLanguage;

    QPushButton *m_addFilesButton;
    QPushButton *m_removeButton;
    QPushButton *m_removeNonExistingButton;
    QLabel *m_aliasLabel;
    QLineEdit *m_aliasText;
    QLabel *m_prefixLabel;
    QLineEdit *m_prefixText;
    QLabel *m_languageLabel;
    QLineEdit *m_languageText;
};

#ifdef WITH_TESTS
QObject *createQrcEditorTest();
#endif

} // ResourceEditor::Internal
