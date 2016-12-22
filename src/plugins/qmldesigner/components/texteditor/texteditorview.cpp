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

#include "texteditorview.h"

#include <designmodecontext.h>
#include <designdocument.h>
#include <modelnode.h>
#include <model.h>
#include <zoomaction.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>

#include <qmldesignerplugin.h>
#include <coreplugin/icore.h>
#include <texteditor/texteditor.h>

#include <QDebug>
#include <QPair>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <QPointer>

namespace QmlDesigner {

class DummyWidget : public QWidget {
public:
    DummyWidget(QWidget *parent = nullptr) : QWidget(parent) {
        QBoxLayout *layout = new QVBoxLayout(this);
        layout->setMargin(0);
    }
    void showEvent(QShowEvent *event) {
        if (m_widget.isNull())
            setWidget(QmlDesignerPlugin::instance()->currentDesignDocument()->textEditor()->duplicate()->widget());
        QWidget::showEvent(event);
    }

private:
    void setWidget(QWidget *widget) {
        if (m_widget)
            m_widget->deleteLater();
        m_widget = widget;

        layout()->addWidget(widget);
    }
    QPointer<QWidget> m_widget;
};


TextEditorView::TextEditorView(QObject *parent)
    : AbstractView(parent)
    , m_dummyWidget(new DummyWidget)
{
    // not completely sure that we need this to just call the right help method ->
    Internal::TextEditorContext *textEditorContext = new Internal::TextEditorContext(m_dummyWidget);
    Core::ICore::addContextObject(textEditorContext);
}

TextEditorView::~TextEditorView()
{
    m_textEditor->deleteLater();
    m_dummyWidget->deleteLater();
}

void TextEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);

    AbstractView::modelAttached(model);
}

void TextEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

void TextEditorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
}

void TextEditorView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void TextEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
}

void TextEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

void TextEditorView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

WidgetInfo TextEditorView::widgetInfo()
{
    return createWidgetInfo(m_dummyWidget, 0, "TextEditor", WidgetInfo::CentralPane, 0, tr("Text Editor"));
}

QString TextEditorView::contextHelpId() const
{
    if (m_textEditor) {
        QString contextHelpId = m_textEditor->contextHelpId();
        if (!contextHelpId.isEmpty())
            return m_textEditor->contextHelpId();
    }
    return AbstractView::contextHelpId();
}

void TextEditorView::nodeIdChanged(const ModelNode& /*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{
}

void TextEditorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/,
                                          const QList<ModelNode> &/*lastSelectedNodeList*/)
{
}

void TextEditorView::customNotification(const AbstractView * /*view*/, const QString &/*identifier*/, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
}

bool TextEditorView::changeToMoveTool()
{
    return true;
}

void TextEditorView::changeToDragTool()
{
}

bool TextEditorView::changeToMoveTool(const QPointF &/*beginPoint*/)
{
    return true;
}

void TextEditorView::changeToSelectionTool()
{
}

void TextEditorView::changeToResizeTool()
{
}

void TextEditorView::changeToTransformTools()
{
}

void TextEditorView::changeToCustomTool()
{
}

void TextEditorView::auxiliaryDataChanged(const ModelNode &/*node*/, const PropertyName &/*name*/, const QVariant &/*data*/)
{
}

void TextEditorView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void TextEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void TextEditorView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void TextEditorView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void TextEditorView::rewriterBeginTransaction()
{
}

void TextEditorView::rewriterEndTransaction()
{
}

void TextEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{
}
}

