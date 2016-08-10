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

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <texteditor/basehoverhandler.h>

#include <QColor>

QT_BEGIN_NAMESPACE
template <class> class QList;
QT_END_NAMESPACE

namespace QmlJS {
class ScopeChain;
class Context;
typedef QSharedPointer<const Context> ContextPtr;
class Value;
class ObjectValue;
}

namespace QmlJSEditor {
namespace Internal {

class QmlJSEditorWidget;

class QmlJSHoverHandler : public TextEditor::BaseHoverHandler
{
    Q_OBJECT

public:
    QmlJSHoverHandler();

private:
    void reset();

    void identifyMatch(TextEditor::TextEditorWidget *editorWidget, int pos) override;
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

    QmlJS::ModelManagerInterface *m_modelManager;
    QColor m_colorTip;
};

} // namespace Internal
} // namespace QmlJSEditor
