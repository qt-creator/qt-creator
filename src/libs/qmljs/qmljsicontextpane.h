// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

#include "qmljs_global.h"

#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastfwd_p.h>

namespace TextEditor { class TextEditorWidget; }

namespace QmlJS {

class ScopeChain;

class QMLJS_EXPORT IContextPane : public QObject
{
     Q_OBJECT

public:
    IContextPane(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IContextPane() {}
    virtual void apply(TextEditor::TextEditorWidget *editorWidget, Document::Ptr document, const ScopeChain *scopeChain, AST::Node *node, bool update, bool force = false) = 0;
    virtual void setEnabled(bool) = 0;
    virtual bool isAvailable(TextEditor::TextEditorWidget *editorWidget, Document::Ptr document, AST::Node *node) = 0;
    virtual QWidget* widget() = 0;
signals:
    void closed();
};

} // namespace QmlJS
