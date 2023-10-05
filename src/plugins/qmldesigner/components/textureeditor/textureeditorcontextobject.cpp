// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textureeditorcontextobject.h"

#include "abstractview.h"
#include "documentmanager.h"
#include "model.h"
#include "qmldesignerplugin.h"
#include "qmlobjectnode.h"
#include "qmltimeline.h"

#include <coreplugin/messagebox.h>
#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QCursor>
#include <QMessageBox>
#include <QQmlContext>
#include <QWindow>

#include <coreplugin/icore.h>

namespace QmlDesigner {

TextureEditorContextObject::TextureEditorContextObject(QQmlContext *context, QObject *parent)
    : QObject(parent)
    , m_qmlContext(context)
{
    qmlRegisterUncreatableType<TextureEditorContextObject>("TextureToolBarAction", 1, 0, "ToolBarAction", "Enum type");
}

QQmlComponent *TextureEditorContextObject::specificQmlComponent()
{
    if (m_specificQmlComponent)
        return m_specificQmlComponent;

    m_specificQmlComponent = new QQmlComponent(m_qmlContext->engine(), this);
    m_specificQmlComponent->setData(m_specificQmlData.toUtf8(), QUrl::fromLocalFile("specifics.qml"));

    return m_specificQmlComponent;
}

QString TextureEditorContextObject::convertColorToString(const QVariant &color)
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
QColor TextureEditorContextObject::colorFromString(const QString &colorString)
{
    return colorString;
}

void TextureEditorContextObject::insertKeyframe(const QString &propertyName)
{
    QTC_ASSERT(m_model && m_model->rewriterView(), return);
    QTC_ASSERT(m_selectedTexture.isValid(), return);

    // Ideally we should not missuse the rewriterView
    //  If we add more code here we have to forward the material editor view
    RewriterView *rewriterView = m_model->rewriterView();

    QmlTimeline timeline = rewriterView->currentTimeline();

    QTC_ASSERT(timeline.isValid(), return);

    rewriterView->executeInTransaction("TextureEditorContextObject::insertKeyframe", [&] {
        timeline.insertKeyframe(m_selectedTexture, propertyName.toUtf8());
    });
}

int TextureEditorContextObject::majorVersion() const
{
    return m_majorVersion;
}

void TextureEditorContextObject::setMajorVersion(int majorVersion)
{
    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

QString TextureEditorContextObject::activeDragSuffix() const
{
    return m_activeDragSuffix;
}

void TextureEditorContextObject::setActiveDragSuffix(const QString &suffix)
{
    if (m_activeDragSuffix != suffix) {
        m_activeDragSuffix = suffix;
        emit activeDragSuffixChanged();
    }
}

bool TextureEditorContextObject::hasActiveTimeline() const
{
    return m_hasActiveTimeline;
}

void TextureEditorContextObject::setHasActiveTimeline(bool b)
{
    if (b == m_hasActiveTimeline)
        return;

    m_hasActiveTimeline = b;
    emit hasActiveTimelineChanged();
}

bool TextureEditorContextObject::hasQuick3DImport() const
{
    return m_hasQuick3DImport;
}

void TextureEditorContextObject::setHasQuick3DImport(bool b)
{
    if (b == m_hasQuick3DImport)
        return;

    m_hasQuick3DImport = b;
    emit hasQuick3DImportChanged();
}

bool TextureEditorContextObject::hasMaterialLibrary() const
{
    return m_hasMaterialLibrary;
}

void TextureEditorContextObject::setHasMaterialLibrary(bool b)
{
    if (b == m_hasMaterialLibrary)
        return;

    m_hasMaterialLibrary = b;
    emit hasMaterialLibraryChanged();
}

bool TextureEditorContextObject::isQt6Project() const
{
    return m_isQt6Project;
}

void TextureEditorContextObject::setIsQt6Project(bool b)
{
    if (m_isQt6Project == b)
        return;

    m_isQt6Project = b;
    emit isQt6ProjectChanged();
}

bool TextureEditorContextObject::hasSingleModelSelection() const
{
    return m_hasSingleModelSelection;
}

void TextureEditorContextObject::setHasSingleModelSelection(bool b)
{
    if (b == m_hasSingleModelSelection)
        return;

    m_hasSingleModelSelection = b;
    emit hasSingleModelSelectionChanged();
}

void TextureEditorContextObject::setSelectedMaterial(const ModelNode &matNode)
{
    m_selectedTexture = matNode;
}

void TextureEditorContextObject::setSpecificsUrl(const QUrl &newSpecificsUrl)
{
    if (newSpecificsUrl == m_specificsUrl)
        return;

    m_specificsUrl = newSpecificsUrl;
    emit specificsUrlChanged();
}

void TextureEditorContextObject::setSpecificQmlData(const QString &newSpecificQmlData)
{
    if (newSpecificQmlData == m_specificQmlData)
        return;

    m_specificQmlData = newSpecificQmlData;

    delete m_specificQmlComponent;
    m_specificQmlComponent = nullptr;

    emit specificQmlComponentChanged();
    emit specificQmlDataChanged();
}

void TextureEditorContextObject::setStateName(const QString &newStateName)
{
    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void TextureEditorContextObject::setAllStateNames(const QStringList &allStates)
{
    if (allStates == m_allStateNames)
        return;

    m_allStateNames = allStates;
    emit allStateNamesChanged();
}

void TextureEditorContextObject::setIsBaseState(bool newIsBaseState)
{
    if (newIsBaseState == m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void TextureEditorContextObject::setSelectionChanged(bool newSelectionChanged)
{
    if (newSelectionChanged == m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void TextureEditorContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    if (newBackendValues == m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void TextureEditorContextObject::setModel(Model *model)
{
    m_model = model;
}

void TextureEditorContextObject::triggerSelectionChanged()
{
    setSelectionChanged(!m_selectionChanged);
}

void TextureEditorContextObject::setHasAliasExport(bool hasAliasExport)
{
    if (m_aliasExport == hasAliasExport)
        return;

    m_aliasExport = hasAliasExport;
    emit hasAliasExportChanged();
}

void TextureEditorContextObject::hideCursor()
{
    if (QApplication::overrideCursor())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWidget *w = QApplication::activeWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void TextureEditorContextObject::restoreCursor()
{
    if (!QApplication::overrideCursor())
        return;

    QApplication::restoreOverrideCursor();

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void TextureEditorContextObject::holdCursorInPlace()
{
    if (!QApplication::overrideCursor())
        return;

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int TextureEditorContextObject::devicePixelRatio()
{
    if (QWidget *w = QApplication::activeWindow())
        return w->devicePixelRatio();

    return 1;
}

QStringList TextureEditorContextObject::allStatesForId(const QString &id)
{
      if (m_model && m_model->rewriterView()) {
          const QmlObjectNode node = m_model->rewriterView()->modelNodeForId(id);
          if (node.isValid())
              return node.allStateNames();
      }

      return {};
}

bool TextureEditorContextObject::isBlocked(const QString &) const
{
    return false;
}

void TextureEditorContextObject::goIntoComponent()
{
    QTC_ASSERT(m_model, return);
    DocumentManager::goIntoComponent(m_selectedTexture);
}

QString TextureEditorContextObject::resolveResourcePath(const QString &path)
{
    if (Utils::FilePath::fromString(path).isAbsolutePath())
        return path;
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument()
            ->fileName().absolutePath().pathAppended(path).cleanPath().toString();
}

} // QmlDesigner
