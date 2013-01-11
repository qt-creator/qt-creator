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

#include "bardescriptordocument.h"

#include "qnxconstants.h"
#include "bardescriptoreditorwidget.h"
#include "bardescriptordocumentnodehandlers.h"

#include <coreplugin/editormanager/ieditor.h>
#include <utils/qtcassert.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QTextCodec>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorDocument::BarDescriptorDocument(BarDescriptorEditorWidget *editorWidget)
    : Core::TextDocument(editorWidget)
    , m_nodeHandlers(QList<BarDescriptorDocumentAbstractNodeHandler *>())
    , m_editorWidget(editorWidget)
{
    // General
    registerNodeHandler(new BarDescriptorDocumentIdNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentVersionNumberNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentBuildIdNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentAuthorNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentAuthorIdNodeHandler(m_editorWidget));

    // Application
    registerNodeHandler(new BarDescriptorDocumentApplicationNameNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentApplicationDescriptionNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentApplicationIconNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentSplashScreenNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentInitialWindowNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentArgNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentActionNodeHandler(m_editorWidget));
    registerNodeHandler(new BarDescriptorDocumentEnvNodeHandler(m_editorWidget));

    // Assets
    registerNodeHandler(new BarDescriptorDocumentAssetNodeHandler(m_editorWidget));
}

BarDescriptorDocument::~BarDescriptorDocument()
{
    while (!m_nodeHandlers.isEmpty()) {
        BarDescriptorDocumentAbstractNodeHandler *nodeHandler = m_nodeHandlers.takeFirst();
        delete nodeHandler;
    }
}

bool BarDescriptorDocument::open(QString *errorString, const QString &fileName) {
    QString contents;
    if (read(fileName, &contents, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    m_fileName = fileName;
    m_editorWidget->editor()->setDisplayName(QFileInfo(fileName).fileName());

    bool result = loadContent(contents);

    if (!result)
        *errorString = tr("%1 does not appear to be a valid application descriptor file").arg(QDir::toNativeSeparators(fileName));

    return result;
}

bool BarDescriptorDocument::save(QString *errorString, const QString &fileName, bool autoSave)
{
    QTC_ASSERT(!autoSave, return false);
    QTC_ASSERT(fileName.isEmpty(), return false);

    bool result = write(m_fileName, xmlSource(), errorString);
    if (!result)
        return false;

    m_editorWidget->setDirty(false);
    emit changed();
    return true;
}

QString BarDescriptorDocument::fileName() const
{
    return m_fileName;
}

QString BarDescriptorDocument::defaultPath() const
{
    QFileInfo fi(fileName());
    return fi.absolutePath();
}

QString BarDescriptorDocument::suggestedFileName() const
{
    QFileInfo fi(fileName());
    return fi.fileName();
}

QString BarDescriptorDocument::mimeType() const
{
    return QLatin1String(Constants::QNX_BAR_DESCRIPTOR_MIME_TYPE);
}

bool BarDescriptorDocument::shouldAutoSave() const
{
    return false;
}

bool BarDescriptorDocument::isModified() const
{
    return m_editorWidget->isDirty();
}

bool BarDescriptorDocument::isSaveAsAllowed() const
{
    return false;
}

Core::IDocument::ReloadBehavior BarDescriptorDocument::reloadBehavior(Core::IDocument::ChangeTrigger state, Core::IDocument::ChangeType type) const
{
    if (type == TypeRemoved || type == TypePermissions)
        return BehaviorSilent;
    if (type == TypeContents && state == TriggerInternal && !isModified())
        return BehaviorSilent;
    return BehaviorAsk;
}

bool BarDescriptorDocument::reload(QString *errorString, Core::IDocument::ReloadFlag flag, Core::IDocument::ChangeType type)
{
    Q_UNUSED(type);

    if (flag == Core::IDocument::FlagIgnore)
        return true;

    return open(errorString, m_fileName);
}

void BarDescriptorDocument::rename(const QString &newName)
{
    const QString oldFilename = m_fileName;
    m_fileName = newName;
    m_editorWidget->editor()->setDisplayName(QFileInfo(m_fileName).fileName());
    emit fileNameChanged(oldFilename, newName);
    emit changed();
}

QString BarDescriptorDocument::xmlSource() const
{
    QDomDocument doc;
    doc.appendChild(doc.createProcessingInstruction(QLatin1String("xml"), QLatin1String("version='1.0' encoding='") + QLatin1String(codec()->name()) + QLatin1String("' standalone='no'")));

    // QNX
    QDomElement rootElem = doc.createElement(QLatin1String("qnx"));
    rootElem.setAttribute(QLatin1String("xmlns"), QLatin1String("http://www.qnx.com/schemas/application/1.0"));

    QMap<int, BarDescriptorDocumentAbstractNodeHandler*> nodeHandlerMap;
    foreach (BarDescriptorDocumentAbstractNodeHandler *nodeHandler, m_nodeHandlers)
        nodeHandlerMap.insertMulti(nodeHandler->order(), nodeHandler);

    QList<BarDescriptorDocumentAbstractNodeHandler*> nodeHandlers = nodeHandlerMap.values();
    foreach (BarDescriptorDocumentAbstractNodeHandler *nodeHandler, nodeHandlers)
        rootElem.appendChild(nodeHandler->toNode(doc));

    doc.appendChild(rootElem);

    return doc.toString(4);
}

bool BarDescriptorDocument::loadContent(const QString &xmlSource, QString *errorMessage, int *errorLine)
{
    QDomDocument doc;
    bool result = doc.setContent(xmlSource, errorMessage, errorLine);
    if (!result)
        return false;

    QDomElement docElem = doc.documentElement();
    if (docElem.tagName() != QLatin1String("qnx"))
        return false;

    m_editorWidget->clear();

    removeUnknownNodeHandlers();
    foreach (BarDescriptorDocumentAbstractNodeHandler *nodeHandler, m_nodeHandlers)
        nodeHandler->clear();

    QDomNode node = docElem.firstChildElement();
    while (!node.isNull()) {
        BarDescriptorDocumentAbstractNodeHandler *nodeHandler = nodeHandlerForDomNode(node);
        if (!nodeHandler) {
            nodeHandler = new BarDescriptorDocumentUnknownNodeHandler(m_editorWidget);
            registerNodeHandler(nodeHandler);
        }

        if (!nodeHandler->handle(node))
            return false;

        node = node.nextSibling();
    }

    m_editorWidget->setXmlSource(xmlSource);

    return true;
}

void BarDescriptorDocument::registerNodeHandler(BarDescriptorDocumentAbstractNodeHandler *nodeHandler)
{
    m_nodeHandlers << nodeHandler;
}

BarDescriptorDocumentAbstractNodeHandler *BarDescriptorDocument::nodeHandlerForDomNode(const QDomNode &node)
{
    foreach (BarDescriptorDocumentAbstractNodeHandler *handler, m_nodeHandlers) {
        if (handler->canHandle(node) && !dynamic_cast<BarDescriptorDocumentUnknownNodeHandler*>(handler))
            return handler;
    }

    return 0;
}

void BarDescriptorDocument::removeUnknownNodeHandlers()
{
    for (int i = m_nodeHandlers.size() - 1; i >= 0; --i) {
        BarDescriptorDocumentUnknownNodeHandler *nodeHandler = dynamic_cast<BarDescriptorDocumentUnknownNodeHandler*>(m_nodeHandlers[i]);
        if (nodeHandler) {
            m_nodeHandlers.removeAt(i);
            delete nodeHandler;
        }
    }
}
