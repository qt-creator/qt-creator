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

#include "aligndistribute.h"
#include "annotationeditor/annotationeditor.h"
#include "bindingeditor/actioneditor.h"
#include "bindingeditor/bindingeditor.h"
#include "colorpalettebackend.h"
#include "fileresourcesmodel.h"
#include "gradientmodel.h"
#include "gradientpresetcustomlistmodel.h"
#include "gradientpresetdefaultlistmodel.h"
#include "itemfiltermodel.h"
#include "propertyeditorcontextobject.h"
#include "propertyeditorqmlbackend.h"
#include "propertyeditorvalue.h"
#include "qmlanchorbindingproxy.h"
#include "richtexteditor/richtexteditorproxy.h"
#include "theme.h"
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
        ColorPaletteBackend::registerDeclarativeType();
        Internal::QmlAnchorBindingProxy::registerDeclarativeType();
        BindingEditor::registerDeclarativeType();
        ActionEditor::registerDeclarativeType();
        AnnotationEditor::registerDeclarativeType();
        AlignDistribute::registerDeclarativeType();
        Tooltip::registerDeclarativeType();
        EasingCurveEditor::registerDeclarativeType();
        RichTextEditorProxy::registerDeclarativeType();

        const QString resourcePath = PropertyEditorQmlBackend::propertyEditorResourcesPath();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QUrl regExpUrl = QUrl::fromLocalFile(resourcePath + "/RegExpValidator.qml");
        qmlRegisterType(regExpUrl, "HelperWidgets", 2, 0, "RegExpValidator");

        const QString qtPrefix = "/Qt6";
#else
        const QString qtPrefix = "/Qt5";
#endif
        qmlRegisterType(QUrl::fromLocalFile(resourcePath + qtPrefix + "HelperWindow.qml"),
                        "HelperWidgets",
                        2,
                        0,
                        "HelperWindow");
    }
}

} //QmlDesigner
