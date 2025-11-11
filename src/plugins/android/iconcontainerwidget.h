// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/idocument.h>

#include <QLabel>
#include <QToolBar>

class IconWidget;

class IconEditor : public Core::IEditor
{
public:
    IconEditor(QWidget *widget) : m_widget(widget) {
        m_toolBar = new QToolBar(widget);
        m_document = std::make_unique<Core::IDocument>();
        setWidget(widget);
    }

    Core::IDocument *document() const override {
        return m_document.get();
    }

    QWidget *toolBar() override {
        return m_toolBar;
    }

private:
    QWidget *m_widget = nullptr;
    QToolBar *m_toolBar;
    QList<IconWidget *> m_iconButtons;
    std::unique_ptr<Core::IDocument> m_document;
};


class IconContainerWidget
{
public:
    IconContainerWidget(QWidget *parent);
};

