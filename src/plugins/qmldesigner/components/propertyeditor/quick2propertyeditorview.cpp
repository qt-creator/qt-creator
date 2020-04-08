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

#include "quick2propertyeditorview.h"

#include "propertyeditorvalue.h"
#include "fileresourcesmodel.h"
#include "gradientmodel.h"
#include "gradientpresetdefaultlistmodel.h"
#include "gradientpresetcustomlistmodel.h"
#include "itemfiltermodel.h"
#include "simplecolorpalettemodel.h"
#include "bindingeditor/bindingeditor.h"
#include "bindingeditor/actioneditor.h"
#include "annotationeditor/annotationeditor.h"
#include "qmlanchorbindingproxy.h"
#include "theme.h"
#include "aligndistribute.h"
#include "propertyeditorcontextobject.h"
#include "tooltip.h"

namespace QmlDesigner {

Quick2PropertyEditorView::Quick2PropertyEditorView(QWidget *parent) :
    QQuickWidget(parent)
{
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    Theme::setupTheme(engine());
}

void Quick2PropertyEditorView::registerQmlTypes()
{
    static bool declarativeTypesRegistered = false;
    if (!declarativeTypesRegistered) {
        declarativeTypesRegistered = true;
        PropertyEditorValue::registerDeclarativeTypes();
        FileResourcesModel::registerDeclarativeType();
        GradientModel::registerDeclarativeType();
        GradientPresetDefaultListModel::registerDeclarativeType();
        GradientPresetCustomListModel::registerDeclarativeType();
        ItemFilterModel::registerDeclarativeType();
        SimpleColorPaletteModel::registerDeclarativeType();
        Internal::QmlAnchorBindingProxy::registerDeclarativeType();
        BindingEditor::registerDeclarativeType();
        ActionEditor::registerDeclarativeType();
        AnnotationEditor::registerDeclarativeType();
        AlignDistribute::registerDeclarativeType();
        Tooltip::registerDeclarativeType();
        EasingCurveEditor::registerDeclarativeType();
    }
}

bool Quick2PropertyEditorView::event(QEvent *e)
{
    static std::vector<QKeySequence> overrideSequences = { QKeySequence(Qt::SHIFT + Qt::Key_Up),
                                                           QKeySequence(Qt::SHIFT + Qt::Key_Down),
                                                           QKeySequence(Qt::CTRL + Qt::Key_Up),
                                                           QKeySequence(Qt::CTRL + Qt::Key_Down)
                                                         };

    if (e->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(e);

        static const Qt::KeyboardModifiers relevantModifiers = Qt::ShiftModifier
                | Qt::ControlModifier
                | Qt::AltModifier
                | Qt::MetaModifier;

        QKeySequence keySqeuence(keyEvent->key() | (keyEvent->modifiers() & relevantModifiers));
        for (const QKeySequence &overrideSequence : overrideSequences)
            if (keySqeuence.matches(overrideSequence)) {
                keyEvent->accept();
                return true;
            }
    }

    return QQuickWidget::event(e);
}

} //QmlDesigner
