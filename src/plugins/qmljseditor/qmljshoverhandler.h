// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljseditor/qmljseditor_global.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/basehoverhandler.h>

#include <QColor>
#include <QCoreApplication>

namespace QmlJS {
class ScopeChain;
class Context;
using ContextPtr = QSharedPointer<const Context>;
class Value;
class ObjectValue;
}

namespace QmlJSEditor {

class QmlJSEditorWidget;

class QMLJSEDITOR_EXPORT QmlJSHoverHandler : public TextEditor::BaseHoverHandler
{
public:
    QmlJSHoverHandler();

private:
    void reset();

    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override;
    void operateTooltip(TextEditor::TextEditorWidget *editorWidget, const QPoint &point) override;

    bool matchDiagnosticMessage(QmlJSEditorWidget *qmlEditor, int pos);
    bool matchColorItem(const QmlJS::ScopeChain &lookupContext,
                        const QmlJS::Document::Ptr &qmlDocument,
                        const QList<QmlJS::AST::Node *> &astPath,
                        unsigned pos);
    void handleOrdinaryMatch(const QmlJS::ScopeChain &lookupContext,
                             QmlJS::AST::Node *node);
    void handleImport(const QmlJS::ScopeChain &lookupContext,
                      QmlJS::AST::UiImport *node);

    void prettyPrintTooltip(const QmlJS::Value *value,
                            const QmlJS::ContextPtr &context);

    bool setQmlTypeHelp(const QmlJS::ScopeChain &scopeChain, const QmlJS::Document::Ptr &qmlDocument,
                        const QmlJS::ObjectValue *value, const QStringList &qName);
    bool setQmlHelpItem(const QmlJS::ScopeChain &lookupContext,
                        const QmlJS::Document::Ptr &qmlDocument,
                        QmlJS::AST::Node *node);

    QmlJS::ModelManagerInterface *m_modelManager = nullptr;
    QColor m_colorTip;
};

} // namespace QmlJSEditor
