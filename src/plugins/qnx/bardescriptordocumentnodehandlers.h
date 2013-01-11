/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BARDESCRIPTORDOCUMENTNODEHANDLERS_H
#define QNX_INTERNAL_BARDESCRIPTORDOCUMENTNODEHANDLERS_H

#include <QDomNode>
#include <QSharedPointer>

namespace Qnx {
namespace Internal {

class BarDescriptorEditorWidget;

class BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentAbstractNodeHandler(BarDescriptorEditorWidget *editorWidget);
    virtual ~BarDescriptorDocumentAbstractNodeHandler();

    virtual bool canHandle(const QDomNode &node) const = 0;
    bool handle(const QDomNode &node);
    virtual QDomNode toNode(QDomDocument &doc) const = 0;

    void clear();
    int order() const;

protected:
    BarDescriptorEditorWidget *editorWidget() const;

    virtual bool fromNode(const QDomNode &node) = 0;

    bool canHandleSimpleTextElement(const QDomNode &node, const QString &tagName) const;
    QString loadSimpleTextElement(const QDomNode &node);
    QDomElement createSimpleTextElement(QDomDocument &doc, const QString &tagName, const QString &textValue) const;

private:
    BarDescriptorEditorWidget *m_editorWidget;

    int m_order;
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentIdNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentIdNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentVersionNumberNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentVersionNumberNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentBuildIdNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentBuildIdNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentApplicationNameNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentApplicationNameNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentApplicationDescriptionNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentApplicationDescriptionNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentApplicationIconNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentApplicationIconNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentSplashScreenNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentSplashScreenNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentAssetNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentAssetNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentInitialWindowNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentInitialWindowNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentActionNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentActionNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentArgNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentArgNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentEnvNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentEnvNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentAuthorNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentAuthorNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentAuthorIdNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentAuthorIdNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);
};

// ----------------------------------------------------------------------------

class BarDescriptorDocumentUnknownNodeHandler : public BarDescriptorDocumentAbstractNodeHandler
{
public:
    BarDescriptorDocumentUnknownNodeHandler(BarDescriptorEditorWidget *editorWidget);

    bool canHandle(const QDomNode &node) const;
    QDomNode toNode(QDomDocument &doc) const;

protected:
    bool fromNode(const QDomNode &node);

private:
    QDomNode m_node;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BARDESCRIPTORDOCUMENTNODEHANDLERS_H
