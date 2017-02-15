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

#include "propertyeditorcontextobject.h"

#include <abstractview.h>
#include <nodemetainfo.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <rewritingexception.h>

#include <coreplugin/messagebox.h>
#include <utils/algorithm.h>

#include <QQmlContext>

static uchar fromHex(const uchar c, const uchar c2)
{
    uchar rv = 0;
    if (c >= '0' && c <= '9')
        rv += (c - '0') * 16;
    else if (c >= 'A' && c <= 'F')
        rv += (c - 'A' + 10) * 16;
    else if (c >= 'a' && c <= 'f')
        rv += (c - 'a' + 10) * 16;

    if (c2 >= '0' && c2 <= '9')
        rv += (c2 - '0');
    else if (c2 >= 'A' && c2 <= 'F')
        rv += (c2 - 'A' + 10);
    else if (c2 >= 'a' && c2 <= 'f')
        rv += (c2 - 'a' + 10);

    return rv;
}

static uchar fromHex(const QString &s, int idx)
{
    uchar c = s.at(idx).toLatin1();
    uchar c2 = s.at(idx + 1).toLatin1();
    return fromHex(c, c2);
}

QColor convertColorFromString(const QString &s)
{
    if (s.length() == 9 && s.startsWith(QLatin1Char('#'))) {
        uchar a = fromHex(s, 1);
        uchar r = fromHex(s, 3);
        uchar g = fromHex(s, 5);
        uchar b = fromHex(s, 7);
        return QColor(r, g, b, a);
    } else {
        QColor rv(s);
        return rv;
    }
}

namespace QmlDesigner {

PropertyEditorContextObject::PropertyEditorContextObject(QObject *parent) :
    QObject(parent),
    m_isBaseState(false),
    m_selectionChanged(false),
    m_backendValues(0),
    m_qmlComponent(0),
    m_qmlContext(0)
{

}

QString PropertyEditorContextObject::convertColorToString(const QColor &color)
{
    QString colorString = color.name();

    if (color.alpha() != 255) {
        const QString hexAlpha = QString::number(color.alpha(), 16);
        colorString.remove(0,1);
        colorString.prepend(hexAlpha);
        colorString.prepend(QStringLiteral("#"));
    }
    return colorString;
}

QColor PropertyEditorContextObject::colorFromString(const QString &colorString)
{
    return convertColorFromString(colorString);
}

QString PropertyEditorContextObject::translateFunction()
{
    if (QmlDesignerPlugin::instance()->settings().value(
            DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt())

        switch (QmlDesignerPlugin::instance()->settings().value(
                    DesignerSettingsKey::TYPE_OF_QSTR_FUNCTION).toInt()) {
        case 0: return QLatin1String("qsTr");
        case 1: return QLatin1String("qsTrId");
        case 2: return QLatin1String("qsTranslate");
        default:
            break;
        }
    return QLatin1String("qsTr");
}

QStringList PropertyEditorContextObject::autoComplete(const QString &text, int pos, bool explicitComplete, bool filter)
{
    if (m_model && m_model->rewriterView())
        return  Utils::filtered(m_model->rewriterView()->autoComplete(text, pos, explicitComplete), [filter](const QString &string) {
            return !filter || (!string.isEmpty() && string.at(0).isUpper()); });

    return QStringList();
}

void PropertyEditorContextObject::toogleExportAlias()
{
    if (!m_model || !m_model->rewriterView())
        return;

    /* Ideally we should not missuse the rewriterView
     * If we add more code here we have to forward the property editor view */
    RewriterView *rewriterView = m_model->rewriterView();

    if (rewriterView->selectedModelNodes().isEmpty())
        return;

    ModelNode selectedNode = rewriterView->selectedModelNodes().first();

    if (QmlObjectNode::isValidQmlObjectNode(selectedNode)) {
        QmlObjectNode objectNode(selectedNode);

        PropertyName modelNodeId = selectedNode.id().toUtf8();
        ModelNode rootModelNode = rewriterView->rootModelNode();

        try {
            RewriterTransaction transaction =
                    rewriterView->beginRewriterTransaction(QByteArrayLiteral("PropertyEditorContextObject:toogleExportAlias"));

            if (!objectNode.isAliasExported())
                objectNode.ensureAliasExport();
            else
                if (rootModelNode.hasProperty(modelNodeId))
                    rootModelNode.removeProperty(modelNodeId);

            transaction.commit();
        }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
            exception.showException();
        }
    }

}

void PropertyEditorContextObject::changeTypeName(const QString &typeName)
{

    if (!m_model || !m_model->rewriterView())
        return;

    /* Ideally we should not missuse the rewriterView
     * If we add more code here we have to forward the property editor view */
    RewriterView *rewriterView = m_model->rewriterView();

    if (rewriterView->selectedModelNodes().isEmpty())
        return;

    ModelNode selectedNode = rewriterView->selectedModelNodes().first();

    try {
        RewriterTransaction transaction =
                rewriterView->beginRewriterTransaction(QByteArrayLiteral("PropertyEditorContextObject:changeTypeName"));

        NodeMetaInfo metaInfo = m_model->metaInfo(typeName.toLatin1());
        if (!metaInfo.isValid()) {
            Core::AsynchronousMessageBox::warning(tr("Invalid Type"),  tr("%1 is an invalid type.").arg(typeName));
            return;
        }
        if (selectedNode.isRootNode())
             rewriterView->changeRootNodeType(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());
        else
            selectedNode.changeType(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());

        transaction.commit();
    }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
        exception.showException();
    }


}

int PropertyEditorContextObject::majorVersion() const
{
    return m_majorVersion;

}

int PropertyEditorContextObject::majorQtQuickVersion() const
{
    return m_majorQtQuickVersion;
}

int PropertyEditorContextObject::minorQtQuickVersion() const
{
    return m_minorQtQuickVersion;
}

void PropertyEditorContextObject::setMajorVersion(int majorVersion)
{
    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

void PropertyEditorContextObject::setMajorQtQuickVersion(int majorVersion)
{
    if (m_majorQtQuickVersion == majorVersion)
        return;

    m_majorQtQuickVersion = majorVersion;

    emit majorQtQuickVersionChanged();

}

void PropertyEditorContextObject::setMinorQtQuickVersion(int minorVersion)
{
    if (m_minorQtQuickVersion == minorVersion)
        return;

    m_minorQtQuickVersion = minorVersion;

    emit minorQtQuickVersionChanged();
}

int PropertyEditorContextObject::minorVersion() const
{
    return m_minorVersion;
}

void PropertyEditorContextObject::setMinorVersion(int minorVersion)
{
    if (m_minorVersion == minorVersion)
        return;

    m_minorVersion = minorVersion;

    emit minorVersionChanged();
}

void PropertyEditorContextObject::insertInQmlContext(QQmlContext *context)
{
    m_qmlContext = context;
    m_qmlContext->setContextObject(this);
}

QQmlComponent *PropertyEditorContextObject::specificQmlComponent()
{
    if (m_qmlComponent)
        return m_qmlComponent;

    m_qmlComponent = new QQmlComponent(m_qmlContext->engine(), this);

    m_qmlComponent->setData(m_specificQmlData.toUtf8(), QUrl::fromLocalFile(QStringLiteral("specfics.qml")));

    return m_qmlComponent;
}

void PropertyEditorContextObject::setGlobalBaseUrl(const QUrl &newBaseUrl)
{
    if (newBaseUrl == m_globalBaseUrl)
        return;

    m_globalBaseUrl = newBaseUrl;
    emit globalBaseUrlChanged();
}

void PropertyEditorContextObject::setSpecificsUrl(const QUrl &newSpecificsUrl)
{
    if (newSpecificsUrl == m_specificsUrl)
        return;

    m_specificsUrl = newSpecificsUrl;
    emit specificsUrlChanged();
}

void PropertyEditorContextObject::setSpecificQmlData(const QString &newSpecificQmlData)
{
    if (m_specificQmlData == newSpecificQmlData)
        return;

    m_specificQmlData = newSpecificQmlData;

    delete m_qmlComponent;
    m_qmlComponent = 0;

    emit specificQmlComponentChanged();
    emit specificQmlDataChanged();
}

void PropertyEditorContextObject::setStateName(const QString &newStateName)
{
    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void PropertyEditorContextObject::setIsBaseState(bool newIsBaseState)
{
    if (newIsBaseState ==  m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void PropertyEditorContextObject::setSelectionChanged(bool newSelectionChanged)
{
    if (newSelectionChanged ==  m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void PropertyEditorContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    if (newBackendValues ==  m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void PropertyEditorContextObject::setModel(Model *model)
{
    m_model = model;
}

void PropertyEditorContextObject::triggerSelectionChanged()
{
    setSelectionChanged(!m_selectionChanged);
}

void PropertyEditorContextObject::setHasAliasExport(bool hasAliasExport)
{
    if (m_aliasExport == hasAliasExport)
        return;

    m_aliasExport = hasAliasExport;
    emit hasAliasExportChanged();
}

} //QmlDesigner
