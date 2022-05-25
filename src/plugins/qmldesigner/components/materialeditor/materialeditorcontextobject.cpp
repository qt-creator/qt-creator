/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "materialeditorcontextobject.h"

#include <abstractview.h>
#include <nodemetainfo.h>
#include <rewritingexception.h>
#include <qmldesignerplugin.h>
#include <qmlmodelnodeproxy.h>
#include <qmlobjectnode.h>
#include <qmltimeline.h>

#include <coreplugin/messagebox.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QCursor>
#include <QMessageBox>
#include <QQmlContext>
#include <QWindow>

#include <coreplugin/icore.h>

namespace QmlDesigner {

MaterialEditorContextObject::MaterialEditorContextObject(QObject *parent)
    : QObject(parent)
{
    qmlRegisterUncreatableType<MaterialEditorContextObject>("ToolBarAction", 1, 0, "ToolBarAction", "Enum type");
}

QString MaterialEditorContextObject::convertColorToString(const QVariant &color)
{
    QString colorString;
    QColor theColor;
    if (color.canConvert(QVariant::Color)) {
        theColor = color.value<QColor>();
    } else if (color.canConvert(QVariant::Vector3D)) {
        auto vec = color.value<QVector3D>();
        theColor = QColor::fromRgbF(vec.x(), vec.y(), vec.z());
    }

    colorString = theColor.name();

    if (theColor.alpha() != 255) {
        QString hexAlpha = QString("%1").arg(theColor.alpha(), 2, 16, QLatin1Char('0'));
        colorString.remove(0, 1);
        colorString.prepend(hexAlpha);
        colorString.prepend(QStringLiteral("#"));
    }
    return colorString;
}

// TODO: this method is used by the ColorEditor helper widget, check if at all needed?
QColor MaterialEditorContextObject::colorFromString(const QString &colorString)
{
    return colorString;
}

void MaterialEditorContextObject::changeTypeName(const QString &typeName)
{
    QTC_ASSERT(m_model && m_model->rewriterView(), return);
    QTC_ASSERT(m_selectedMaterial.isValid(), return);

    if (m_selectedMaterial.simplifiedTypeName() == typeName)
        return;

    // Ideally we should not misuse the rewriterView
    // If we add more code here we have to forward the material editor view
    RewriterView *rewriterView = m_model->rewriterView();

    rewriterView->executeInTransaction("MaterialEditorContextObject::changeTypeName", [&] {
        NodeMetaInfo metaInfo = m_model->metaInfo(typeName.toLatin1());

        QTC_ASSERT(metaInfo.isValid(), return);

        // Create a list of properties available for the new type
        PropertyNameList propertiesAndSignals(metaInfo.propertyNames());
        // Add signals to the list
        const PropertyNameList signalNames = metaInfo.signalNames();
        for (const PropertyName &signal : signalNames) {
            if (signal.isEmpty())
                continue;

            PropertyName name = signal;
            QChar firstChar = QChar(signal.at(0)).toUpper().toLatin1();
            name[0] = firstChar.toLatin1();
            name.prepend("on");
            propertiesAndSignals.append(name);
        }

        // Add dynamic properties and respective change signals
        const QList<AbstractProperty> matProps = m_selectedMaterial.properties();
        for (const auto &property : matProps) {
            if (!property.isDynamic())
                continue;

            // Add dynamic property
            propertiesAndSignals.append(property.name());
            // Add its change signal
            PropertyName name = property.name();
            QChar firstChar = QChar(property.name().at(0)).toUpper().toLatin1();
            name[0] = firstChar.toLatin1();
            name.prepend("on");
            name.append("Changed");
            propertiesAndSignals.append(name);
        }

        // Compare current properties and signals with the ones available for change type
        QList<PropertyName> incompatibleProperties;
        for (const auto &property : matProps) {
            if (!propertiesAndSignals.contains(property.name()))
                incompatibleProperties.append(property.name());
        }

        Utils::sort(incompatibleProperties);

        // Create a dialog showing incompatible properties and signals
        if (!incompatibleProperties.empty()) {
            QString detailedText = tr("<b>Incompatible properties:</b><br>");

            for (const auto &p : std::as_const(incompatibleProperties))
                detailedText.append("- " + QString::fromUtf8(p) + "<br>");

            detailedText.chop(QString("<br>").size());

            QMessageBox msgBox;
            msgBox.setTextFormat(Qt::RichText);
            msgBox.setIcon(QMessageBox::Question);
            msgBox.setWindowTitle(tr("Change Type"));
            msgBox.setText(tr("Changing the type from %1 to %2 can't be done without removing incompatible properties.<br><br>%3")
                                   .arg(m_selectedMaterial.simplifiedTypeName(), typeName, detailedText));
            msgBox.setInformativeText(tr("Do you want to continue by removing incompatible properties?"));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Ok);

            if (msgBox.exec() == QMessageBox::Cancel)
                return;

            for (const auto &p : std::as_const(incompatibleProperties))
                m_selectedMaterial.removeProperty(p);
        }

        if (m_selectedMaterial.isRootNode())
            rewriterView->changeRootNodeType(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());
        else
            m_selectedMaterial.changeType(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());
    });
}

void MaterialEditorContextObject::insertKeyframe(const QString &propertyName)
{
    QTC_ASSERT(m_model && m_model->rewriterView(), return);
    QTC_ASSERT(m_selectedMaterial.isValid(), return);

    // Ideally we should not missuse the rewriterView
    //  If we add more code here we have to forward the material editor view
    RewriterView *rewriterView = m_model->rewriterView();

    QmlTimeline timeline = rewriterView->currentTimeline();

    QTC_ASSERT(timeline.isValid(), return);

    rewriterView->executeInTransaction("MaterialEditorContextObject::insertKeyframe", [&] {
        timeline.insertKeyframe(m_selectedMaterial, propertyName.toUtf8());
    });
}

int MaterialEditorContextObject::majorVersion() const
{
    return m_majorVersion;
}

void MaterialEditorContextObject::setMajorVersion(int majorVersion)
{
    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

bool MaterialEditorContextObject::hasActiveTimeline() const
{
    return m_hasActiveTimeline;
}

void MaterialEditorContextObject::setHasActiveTimeline(bool b)
{
    if (b == m_hasActiveTimeline)
        return;

    m_hasActiveTimeline = b;
    emit hasActiveTimelineChanged();
}

bool MaterialEditorContextObject::hasQuick3DImport() const
{
    return m_hasQuick3DImport;
}

void MaterialEditorContextObject::setHasQuick3DImport(bool b)
{
    if (b == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = b;
    emit hasQuick3DImportChanged();
}

bool MaterialEditorContextObject::hasModelSelection() const
{
    return m_hasModelSelection;
}

void MaterialEditorContextObject::setHasModelSelection(bool b)
{
    if (b == m_hasModelSelection)
        return;

    m_hasModelSelection = b;
    emit hasModelSelectionChanged();
}

void MaterialEditorContextObject::setSelectedMaterial(const ModelNode &matNode)
{
    m_selectedMaterial = matNode;
}

void MaterialEditorContextObject::setSpecificsUrl(const QUrl &newSpecificsUrl)
{
    if (newSpecificsUrl == m_specificsUrl)
        return;

    m_specificsUrl = newSpecificsUrl;
    emit specificsUrlChanged();
}

void MaterialEditorContextObject::setStateName(const QString &newStateName)
{
    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void MaterialEditorContextObject::setAllStateNames(const QStringList &allStates)
{
    if (allStates == m_allStateNames)
        return;

    m_allStateNames = allStates;
    emit allStateNamesChanged();
}

void MaterialEditorContextObject::setIsBaseState(bool newIsBaseState)
{
    if (newIsBaseState == m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void MaterialEditorContextObject::setSelectionChanged(bool newSelectionChanged)
{
    if (newSelectionChanged == m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void MaterialEditorContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    if (newBackendValues == m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void MaterialEditorContextObject::setModel(Model *model)
{
    m_model = model;
}

void MaterialEditorContextObject::triggerSelectionChanged()
{
    setSelectionChanged(!m_selectionChanged);
}

void MaterialEditorContextObject::setHasAliasExport(bool hasAliasExport)
{
    if (m_aliasExport == hasAliasExport)
        return;

    m_aliasExport = hasAliasExport;
    emit hasAliasExportChanged();
}

void MaterialEditorContextObject::hideCursor()
{
    if (QApplication::overrideCursor())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWidget *w = QApplication::activeWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void MaterialEditorContextObject::restoreCursor()
{
    if (!QApplication::overrideCursor())
        return;

    QApplication::restoreOverrideCursor();

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void MaterialEditorContextObject::holdCursorInPlace()
{
    if (!QApplication::overrideCursor())
        return;

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int MaterialEditorContextObject::devicePixelRatio()
{
    if (QWidget *w = QApplication::activeWindow())
        return w->devicePixelRatio();

    return 1;
}

QStringList MaterialEditorContextObject::allStatesForId(const QString &id)
{
      if (m_model && m_model->rewriterView()) {
          const QmlObjectNode node = m_model->rewriterView()->modelNodeForId(id);
          if (node.isValid())
              return node.allStateNames();
      }

      return {};
}

bool MaterialEditorContextObject::isBlocked(const QString &propName) const
{
    if (!m_selectedMaterial.isValid())
        return false;

    if (!m_model || !m_model->rewriterView())
        return false;

    if (QmlObjectNode(m_selectedMaterial).isBlocked(propName.toUtf8()))
        return true;

    return false;
}

} // QmlDesigner
