// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposercontextobject.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <documentmanager.h>
#include <nodemetainfo.h>
#include <rewritingexception.h>
#include <qmldesignerplugin.h>
#include <qmldesigner/components/propertyeditor/qmlmodelnodeproxy.h>
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

namespace EffectComposer {

EffectComposerContextObject::EffectComposerContextObject(QQmlContext *context, QObject *parent)
    : QObject(parent)
    , m_qmlContext(context)
{
}

QString EffectComposerContextObject::convertColorToString(const QVariant &color)
{
    QString colorString;
    QColor theColor;
    if (color.canConvert(QMetaType(QMetaType::QColor))) {
        theColor = color.value<QColor>();
    } else if (color.canConvert(QMetaType(QMetaType::QVector3D))) {
        auto vec = color.value<QVector3D>();
        theColor = QColor::fromRgbF(vec.x(), vec.y(), vec.z());
    }

    colorString = theColor.name(QColor::HexArgb);

    return colorString;
}

// TODO: this method is used by the ColorEditor helper widget, check if at all needed?
QColor EffectComposerContextObject::colorFromString(const QString &colorString)
{
    return colorString;
}

int EffectComposerContextObject::majorVersion() const
{
    return m_majorVersion;
}

void EffectComposerContextObject::setMajorVersion(int majorVersion)
{
    if (m_majorVersion == majorVersion)
        return;

    m_majorVersion = majorVersion;

    emit majorVersionChanged();
}

void EffectComposerContextObject::setStateName(const QString &newStateName)
{
    if (newStateName == m_stateName)
        return;

    m_stateName = newStateName;
    emit stateNameChanged();
}

void EffectComposerContextObject::setAllStateNames(const QStringList &allStates)
{
    if (allStates == m_allStateNames)
        return;

    m_allStateNames = allStates;
    emit allStateNamesChanged();
}

void EffectComposerContextObject::setIsBaseState(bool newIsBaseState)
{
    if (newIsBaseState == m_isBaseState)
        return;

    m_isBaseState = newIsBaseState;
    emit isBaseStateChanged();
}

void EffectComposerContextObject::setSelectionChanged(bool newSelectionChanged)
{
    if (newSelectionChanged == m_selectionChanged)
        return;

    m_selectionChanged = newSelectionChanged;
    emit selectionChangedChanged();
}

void EffectComposerContextObject::setBackendValues(QQmlPropertyMap *newBackendValues)
{
    if (newBackendValues == m_backendValues)
        return;

    m_backendValues = newBackendValues;
    emit backendValuesChanged();
}

void EffectComposerContextObject::setModel(QmlDesigner::Model *model)
{
    m_model = model;
}

void EffectComposerContextObject::triggerSelectionChanged()
{
    setSelectionChanged(!m_selectionChanged);
}

void EffectComposerContextObject::hideCursor()
{
    if (QApplication::overrideCursor())
        return;

    QApplication::setOverrideCursor(QCursor(Qt::BlankCursor));

    if (QWidget *w = QApplication::activeWindow())
        m_lastPos = QCursor::pos(w->screen());
}

void EffectComposerContextObject::restoreCursor()
{
    if (!QApplication::overrideCursor())
        return;

    QApplication::restoreOverrideCursor();

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

void EffectComposerContextObject::holdCursorInPlace()
{
    if (!QApplication::overrideCursor())
        return;

    if (QWidget *w = QApplication::activeWindow())
        QCursor::setPos(w->screen(), m_lastPos);
}

int EffectComposerContextObject::devicePixelRatio()
{
    if (QWidget *w = QApplication::activeWindow())
        return w->devicePixelRatio();

    return 1;
}

QStringList EffectComposerContextObject::allStatesForId(const QString &id)
{
    if (m_model) {
        const QmlDesigner::QmlObjectNode node = m_model->modelNodeForId(id);
        if (node.isValid())
            return node.allStateNames();
    }

      return {};
}

bool EffectComposerContextObject::isBlocked(const QString &) const
{
      return false;
}

} // namespace EffectComposer

