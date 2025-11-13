// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/helpitem.h>
#include <coreplugin/helpmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <texteditor/texteditor.h>

#include <QToolBar>
#include <QIcon>
#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>
#include <QHBoxLayout>

namespace Android::Internal {

class IconWidget;

class IconEditor : public Core::IEditor
{

public:
    explicit IconEditor(QWidget *widget);

    Core::IDocument *document() const override;
    QWidget *toolBar() override;

private:
    QWidget *m_widget = nullptr;
    IconWidget *ownWidget() const;
    QToolBar *m_toolBar = nullptr;
    std::unique_ptr<Core::IDocument> m_document;
};

class IconContainerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit IconContainerWidget(QWidget *parent);
    bool initialize(TextEditor::TextEditorWidget *textEditorWidget);

signals:
    void iconsModified();

private:
    TextEditor::TextEditorWidget *textEditor() const;
    static Utils::FilePath iconFile(const Utils::FilePath &path);
    bool hasIcons() const;
    void loadIcons();
    Utils::Result<void> saveIcons();
    QString iconFileName() const { return m_iconFileName; }
    void setIconFileName(const QString &name) { m_iconFileName = name; }

    QList<IconWidget *> m_iconButtons;
    QToolButton *m_masterIconButton = nullptr;
    QHBoxLayout * m_iconLayout = nullptr;
    QPointer<TextEditor::TextEditorWidget> m_textEditor = nullptr;
    QString m_iconFileName = QLatin1String("icon");
    bool m_hasIcons = false;
    Utils::FilePath m_manifestDir;
};

} // namespace Android::Internal
