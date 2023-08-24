// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "coloreditorcontextobject.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <documentmanager.h>
#include <nodemetainfo.h>
#include <rewritingexception.h>
#include <qmldesignerplugin.h>
#include <qmlmodelnodeproxy.h>
#include <qmlobjectnode.h>
#include <qmltimeline.h>
#include <qmltimelinekeyframegroup.h>
#include <variantproperty.h>

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

ColorEditorContextObject::ColorEditorContextObject(QQmlContext *context, QObject *parent)
    : QObject(parent)
    , m_qmlContext(context)
{
}

QString ColorEditorContextObject::convertColorToString(const QVariant &color)
{
    QString colorString;
    QColor theColor;
    if (color.canConvert(QVariant::Color)) {
        theColor = color.value<QColor>();
    } else if (color.canConvert(QVariant::Vector3D)) {
        auto vec = color.value<QVector3D>();
        theColor = QColor::fromRgbF(vec.x(), vec.y(), vec.z());
    }

    colorString = theColor.name(QColor::HexArgb);

    return colorString;
}

// TODO: this method is used by the ColorEditor helper widget, check if at all needed?
QColor ColorEditorContextObject::colorFromString(const QString &colorString)
{
    return colorString;
}

int ColorEditorContextObject::majorVersion() const
{
    return m_majorVersion;
}

void ColorEditorContextObject::setMajorVersion(int majorVersion)
{
    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

void ColorEditorContextObject::setStateName(const QString &newStateName)
{
    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void ColorEditorContextObject::setAllStateNames(const QStringList &allStates)
{
    if (allStates == m_allStateNames)
        return;

    m_allStateNames = allStates;
    emit allStateNamesChanged();
}

void ColorEditorContextObject::setIsBaseState(bool newIsBaseState)
{
    if (newIsBaseState == m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void ColorEditorContextObject::setSelectionChanged(bool newSelectionChanged)
{
    if (newSelectionChanged == m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void ColorEditorContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    if (newBackendValues == m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void ColorEditorContextObject::setModel(Model *model)
{
    m_model = model;
}

void ColorEditorContextObject::triggerSelectionChanged()
{
    setSelectionChanged(!m_selectionChanged);
}

void ColorEditorContextObject::hideCursor()
{
    if (QApplication::overrideCursor())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWidget *w = QApplication::activeWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void ColorEditorContextObject::restoreCursor()
{
    if (!QApplication::overrideCursor())
        return;

    QApplication::restoreOverrideCursor();

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void ColorEditorContextObject::holdCursorInPlace()
{
    if (!QApplication::overrideCursor())
        return;

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int ColorEditorContextObject::devicePixelRatio()
{
    if (QWidget *w = QApplication::activeWindow())
        return w->devicePixelRatio();

    return 1;
}

QStringList ColorEditorContextObject::allStatesForId(const QString &id)
{
      if (m_model && m_model->rewriterView()) {
          const QmlObjectNode node = m_model->rewriterView()->modelNodeForId(id);
          if (node.isValid())
              return node.allStateNames();
      }

      return {};
}

bool ColorEditorContextObject::isBlocked(const QString &) const
{
      return false;
}

} // QmlDesigner
