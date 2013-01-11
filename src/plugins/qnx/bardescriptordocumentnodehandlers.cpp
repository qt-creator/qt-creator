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

#include "bardescriptordocumentnodehandlers.h"
#include "bardescriptoreditorwidget.h"

#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QDomNode>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorDocumentAbstractNodeHandler::BarDescriptorDocumentAbstractNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
    , m_order(0xFFFF)
{
}

BarDescriptorDocumentAbstractNodeHandler::~BarDescriptorDocumentAbstractNodeHandler()
{
}

bool BarDescriptorDocumentAbstractNodeHandler::handle(const QDomNode &node)
{
    if (m_order == 0xFFFF)
        m_order = node.lineNumber();

    return fromNode(node);
}

void BarDescriptorDocumentAbstractNodeHandler::clear()
{
    m_order = 0xFFFF;
}

int BarDescriptorDocumentAbstractNodeHandler::order() const
{
    return m_order;
}

BarDescriptorEditorWidget *BarDescriptorDocumentAbstractNodeHandler::editorWidget() const
{
    return m_editorWidget;
}

bool BarDescriptorDocumentAbstractNodeHandler::canHandleSimpleTextElement(const QDomNode &node, const QString &tagName) const
{
    QDomElement element = node.toElement();
    if (element.isNull())
        return false;

    if (element.tagName().toLower() != tagName.toLower())
        return false;

    QDomText textNode = element.firstChild().toText();
    if (textNode.isNull())
        return false;

    return true;
}

QString BarDescriptorDocumentAbstractNodeHandler::loadSimpleTextElement(const QDomNode &node)
{
    QDomElement element = node.toElement();
    QDomText textNode = element.firstChild().toText();
    return textNode.data();
}

QDomElement BarDescriptorDocumentAbstractNodeHandler::createSimpleTextElement(QDomDocument &doc, const QString &tagName, const QString &textValue) const
{
    if (textValue.isEmpty())
        return QDomElement();

    QDomElement elem = doc.createElement(tagName);
    elem.appendChild(doc.createTextNode(textValue));
    return elem;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentIdNodeHandler::BarDescriptorDocumentIdNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentIdNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("id"));
}

bool BarDescriptorDocumentIdNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    editorWidget()->setPackageId(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentIdNodeHandler::toNode(QDomDocument &doc) const
{
    return createSimpleTextElement(doc, QLatin1String("id"), editorWidget()->packageId());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentVersionNumberNodeHandler::BarDescriptorDocumentVersionNumberNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentVersionNumberNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("versionNumber"));
}

bool BarDescriptorDocumentVersionNumberNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    editorWidget()->setPackageVersion(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentVersionNumberNodeHandler::toNode(QDomDocument &doc) const
{
    return createSimpleTextElement(doc, QLatin1String("versionNumber"), editorWidget()->packageVersion());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentBuildIdNodeHandler::BarDescriptorDocumentBuildIdNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentBuildIdNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("buildId"));
}

bool BarDescriptorDocumentBuildIdNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    editorWidget()->setPackageBuildId(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentBuildIdNodeHandler::toNode(QDomDocument &doc) const
{
    return createSimpleTextElement(doc, QLatin1String("buildId"), editorWidget()->packageBuildId());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentApplicationNameNodeHandler::BarDescriptorDocumentApplicationNameNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentApplicationNameNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("name"));
}

bool BarDescriptorDocumentApplicationNameNodeHandler::fromNode(const QDomNode &node)
{
    // TODO: Add support for localization

    if (!canHandle(node))
        return false;

    editorWidget()->setApplicationName(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentApplicationNameNodeHandler::toNode(QDomDocument &doc) const
{
    // TODO: Add support for localization

    return createSimpleTextElement(doc, QLatin1String("name"), editorWidget()->applicationName());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentApplicationDescriptionNodeHandler::BarDescriptorDocumentApplicationDescriptionNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentApplicationDescriptionNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("description"));
}

bool BarDescriptorDocumentApplicationDescriptionNodeHandler::fromNode(const QDomNode &node)
{
    // TODO: Add support for localization

    if (!canHandle(node))
        return false;

    editorWidget()->setApplicationDescription(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentApplicationDescriptionNodeHandler::toNode(QDomDocument &doc) const
{
    return createSimpleTextElement(doc, QLatin1String("description"), editorWidget()->applicationDescription());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentApplicationIconNodeHandler::BarDescriptorDocumentApplicationIconNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentApplicationIconNodeHandler::canHandle(const QDomNode &node) const
{
    QDomElement element = node.toElement();
    if (element.isNull())
        return false;

    if (element.tagName() != QLatin1String("icon"))
        return false;

    QDomElement imageElement = element.firstChild().toElement();
    if (imageElement.isNull())
        return false;

    if (imageElement.tagName() != QLatin1String("image"))
        return false;

    QDomText imageTextNode = imageElement.firstChild().toText();
    if (imageTextNode.isNull())
        return false;

    return true;
}

bool BarDescriptorDocumentApplicationIconNodeHandler::fromNode(const QDomNode &node)
{
    // TODO: Add support for localization

    if (!canHandle(node))
        return false;

    QDomNode imageNode = node.firstChild();
    QDomText imageTextNode = imageNode.firstChild().toText();
    editorWidget()->setApplicationIcon(imageTextNode.data());
    return true;
}

QDomNode BarDescriptorDocumentApplicationIconNodeHandler::toNode(QDomDocument &doc) const
{
    // TODO: Add support for localization
    const QString iconFileName = editorWidget()->applicationIconFileName();
    if (iconFileName.isEmpty())
        return QDomElement();

    QDomElement iconElement = doc.createElement(QLatin1String("icon"));
    iconElement.appendChild(createSimpleTextElement(doc, QLatin1String("image"), iconFileName));
    return iconElement;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentSplashScreenNodeHandler::BarDescriptorDocumentSplashScreenNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentSplashScreenNodeHandler::canHandle(const QDomNode &node) const
{
    QDomElement element = node.toElement();
    if (element.isNull())
        return false;

    if (element.tagName().toLower() != QLatin1String("splashscreens"))
        return false;

    QDomElement imageElement = element.firstChild().toElement();
    if (imageElement.isNull())
        return false;

    if (imageElement.tagName().toLower() != QLatin1String("image"))
        return false;

    QDomText imageTextNode = imageElement.firstChild().toText();
    if (imageTextNode.isNull())
        return false;

    return true;
}

bool BarDescriptorDocumentSplashScreenNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    QDomElement imageNode = node.firstChildElement();
    while (!imageNode.isNull()) {
        if (imageNode.tagName().toLower() == QLatin1String("image")) {
            QDomText imageTextNode = imageNode.firstChild().toText();
            editorWidget()->appendSplashScreen(imageTextNode.data());
        }
        imageNode = imageNode.nextSiblingElement();
    }
    return true;
}

QDomNode BarDescriptorDocumentSplashScreenNodeHandler::toNode(QDomDocument &doc) const
{
    QStringList splashScreens = editorWidget()->splashScreens();
    if (splashScreens.isEmpty())
        return QDomElement();

    QDomElement splashScreenElement = doc.createElement(QLatin1String("splashscreens"));
    foreach (const QString &splashScreen, splashScreens)
        splashScreenElement.appendChild(createSimpleTextElement(doc, QLatin1String("image"), splashScreen));

    return splashScreenElement;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentAssetNodeHandler::BarDescriptorDocumentAssetNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentAssetNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("asset"));
}

bool BarDescriptorDocumentAssetNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    QDomElement element = node.toElement();

    QString path = element.attribute(QLatin1String("path"));
    QString entry = element.attribute(QLatin1String("entry"));
    QDomText destNode = element.firstChild().toText();
    QString dest = destNode.data();

    BarDescriptorAsset asset;
    asset.source = path;
    asset.destination = dest;
    asset.entry = entry == QLatin1String("true");

    editorWidget()->addAsset(asset);
    return true;
}

QDomNode BarDescriptorDocumentAssetNodeHandler::toNode(QDomDocument &doc) const
{
    QDomDocumentFragment fragment = doc.createDocumentFragment();

    QList<BarDescriptorAsset> assets = editorWidget()->assets();
    foreach (const BarDescriptorAsset &asset, assets) {
        QDomElement assetElem = doc.createElement(QLatin1String("asset"));
        assetElem.setAttribute(QLatin1String("path"), asset.source);
        if (asset.entry) {
            assetElem.setAttribute(QLatin1String("type"), QLatin1String("Qnx/Elf"));
            assetElem.setAttribute(QLatin1String("entry"), QLatin1String("true"));
        }
        assetElem.appendChild(doc.createTextNode(asset.destination));
        fragment.appendChild(assetElem);
    }

    return fragment;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentInitialWindowNodeHandler::BarDescriptorDocumentInitialWindowNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentInitialWindowNodeHandler::canHandle(const QDomNode &node) const
{
    QDomElement element = node.toElement();
    if (element.isNull())
        return false;

    if (element.tagName() != QLatin1String("initialWindow"))
        return false;

    return true;
}

bool BarDescriptorDocumentInitialWindowNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    QDomElement child = node.firstChildElement();
    while (!child.isNull()) {
        if (child.tagName() == QLatin1String("aspectRatio")) {
            editorWidget()->setOrientation(loadSimpleTextElement(child));
        } else if (child.tagName() == QLatin1String("autoOrients")) {
            if (loadSimpleTextElement(child) == QLatin1String("true"))
                editorWidget()->setOrientation(QLatin1String("auto-orient"));
        } else if (child.tagName() == QLatin1String("systemChrome")) {
            editorWidget()->setChrome(loadSimpleTextElement(child));
        } else if (child.tagName() == QLatin1String("transparent")) {
            const QString transparent = loadSimpleTextElement(child);
            editorWidget()->setTransparent(transparent == QLatin1String("true"));
        }
        child = child.nextSiblingElement();
    }

    return true;
}

QDomNode BarDescriptorDocumentInitialWindowNodeHandler::toNode(QDomDocument &doc) const
{
    QDomElement element = doc.createElement(QLatin1String("initialWindow"));

    if (editorWidget()->orientation() == QLatin1String("auto-orient")) {
        element.appendChild(createSimpleTextElement(doc, QLatin1String("autoOrients"), QLatin1String("true")));
    } else if (!editorWidget()->orientation().isEmpty()) {
        element.appendChild(createSimpleTextElement(doc, QLatin1String("aspectRatio"), editorWidget()->orientation()));
        element.appendChild(createSimpleTextElement(doc, QLatin1String("autoOrients"), QLatin1String("false")));
    }
    element.appendChild(createSimpleTextElement(doc, QLatin1String("systemChrome"), editorWidget()->chrome()));
    element.appendChild(createSimpleTextElement(doc, QLatin1String("transparent"), editorWidget()->transparent() ? QLatin1String("true") : QLatin1String("false")));

    return element;
}

// ----------------------------------------------------------------------------


BarDescriptorDocumentActionNodeHandler::BarDescriptorDocumentActionNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentActionNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("action"));
}

bool BarDescriptorDocumentActionNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    QString value = loadSimpleTextElement(node);
    if (value != QLatin1String("run_native")) // This has no representation in the GUI, and is always added
        editorWidget()->checkPermission(value);

    return true;
}

QDomNode BarDescriptorDocumentActionNodeHandler::toNode(QDomDocument &doc) const
{
    QDomDocumentFragment frag = doc.createDocumentFragment();

    QDomElement runNativeElement = doc.createElement(QLatin1String("action"));
    runNativeElement.setAttribute(QLatin1String("system"), QLatin1String("true"));
    runNativeElement.appendChild(doc.createTextNode(QLatin1String("run_native")));
    frag.appendChild(runNativeElement);

    QStringList checkedIdentifiers = editorWidget()->checkedPermissions();
    foreach (const QString &identifier, checkedIdentifiers)
        frag.appendChild(createSimpleTextElement(doc, QLatin1String("action"), identifier));

    return frag;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentArgNodeHandler::BarDescriptorDocumentArgNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentArgNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("arg"));
}

bool BarDescriptorDocumentArgNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    editorWidget()->appendApplicationArgument(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentArgNodeHandler::toNode(QDomDocument &doc) const
{
    QDomDocumentFragment frag = doc.createDocumentFragment();

    QStringList arguments = editorWidget()->applicationArguments();
    foreach (const QString &argument, arguments)
        frag.appendChild(createSimpleTextElement(doc, QLatin1String("arg"), argument));

    return frag;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentEnvNodeHandler::BarDescriptorDocumentEnvNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentEnvNodeHandler::canHandle(const QDomNode &node) const
{
    QDomElement element = node.toElement();
    if (element.isNull())
        return false;

    if (element.tagName() != QLatin1String("env"))
        return false;

    if (!element.hasAttribute(QLatin1String("var")) || !element.hasAttribute(QLatin1String("value")))
        return false;

    return true;
}

bool BarDescriptorDocumentEnvNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    QDomElement element = node.toElement();

    QString var = element.attribute(QLatin1String("var"));
    QString value = element.attribute(QLatin1String("value"));

    Utils::EnvironmentItem item(var, value);
    editorWidget()->appendEnvironmentItem(item);
    return true;
}

QDomNode BarDescriptorDocumentEnvNodeHandler::toNode(QDomDocument &doc) const
{
    QDomDocumentFragment frag = doc.createDocumentFragment();
    QList<Utils::EnvironmentItem> environmentItems = editorWidget()->environment();

    foreach (const Utils::EnvironmentItem &item, environmentItems) {
        QDomElement element = doc.createElement(QLatin1String("env"));
        element.setAttribute(QLatin1String("var"), item.name);
        element.setAttribute(QLatin1String("value"), item.value);
        frag.appendChild(element);
    }

    return frag;
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentAuthorNodeHandler::BarDescriptorDocumentAuthorNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentAuthorNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("author"))
            || canHandleSimpleTextElement(node, QLatin1String("publisher"));
}

bool BarDescriptorDocumentAuthorNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    editorWidget()->setAuthor(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentAuthorNodeHandler::toNode(QDomDocument &doc) const
{
    return createSimpleTextElement(doc, QLatin1String("author"), editorWidget()->author());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentAuthorIdNodeHandler::BarDescriptorDocumentAuthorIdNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentAuthorIdNodeHandler::canHandle(const QDomNode &node) const
{
    return canHandleSimpleTextElement(node, QLatin1String("authorId"));
}

bool BarDescriptorDocumentAuthorIdNodeHandler::fromNode(const QDomNode &node)
{
    if (!canHandle(node))
        return false;

    editorWidget()->setAuthorId(loadSimpleTextElement(node));
    return true;
}

QDomNode BarDescriptorDocumentAuthorIdNodeHandler::toNode(QDomDocument &doc) const
{
    return createSimpleTextElement(doc, QLatin1String("authorId"), editorWidget()->authorId());
}

// ----------------------------------------------------------------------------

BarDescriptorDocumentUnknownNodeHandler::BarDescriptorDocumentUnknownNodeHandler(BarDescriptorEditorWidget *editorWidget)
    : BarDescriptorDocumentAbstractNodeHandler(editorWidget)
{
}

bool BarDescriptorDocumentUnknownNodeHandler::canHandle(const QDomNode &node) const
{
    Q_UNUSED(node);
    return true;
}

bool BarDescriptorDocumentUnknownNodeHandler::fromNode(const QDomNode &node)
{
    m_node = node.cloneNode();
    return true;
}

QDomNode BarDescriptorDocumentUnknownNodeHandler::toNode(QDomDocument &doc) const
{
    Q_UNUSED(doc);
    return m_node;
}
