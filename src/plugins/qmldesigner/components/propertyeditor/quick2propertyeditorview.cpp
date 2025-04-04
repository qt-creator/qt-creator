// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "quick2propertyeditorview.h"

#include <qmldesignerconstants.h>
#include <scripteditorbackend.h>

#include "aligndistribute.h"
#include "annotationeditor/annotationeditor.h"
#include "assetimageprovider.h"
#include "bindingeditor/actioneditor.h"
#include "bindingeditor/bindingeditor.h"
#include "colorpalettebackend.h"
#include "fileresourcesmodel.h"
#include "gradientmodel.h"
#include "gradientpresetcustomlistmodel.h"
#include "gradientpresetdefaultlistmodel.h"
#include "instanceimageprovider.h"
#include "itemfiltermodel.h"
#include "listvalidator.h"
#include "propertychangesmodel.h"
#include "propertyeditorcontextobject.h"
#include "propertyeditorqmlbackend.h"
#include "propertyeditorvalue.h"
#include "propertymodel.h"
#include "propertynamevalidator.h"
#include "qmlanchorbindingproxy.h"
#include "qmlmaterialnodeproxy.h"
#include "qmltexturenodeproxy.h"
#include "richtexteditor/richtexteditorproxy.h"
#include "propertyeditordynamicpropertiesproxymodel.h"
#include "theme.h"
#include "tooltip.h"

namespace QmlDesigner {

Quick2PropertyEditorView::Quick2PropertyEditorView(AsynchronousImageCache &imageCache)
    : QQuickWidget()
{
    setObjectName(Constants::OBJECT_NAME_PROPERTY_EDITOR);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    Theme::setupTheme(engine());
    engine()->addImageProvider("qmldesigner_thumbnails",
                               new AssetImageProvider(imageCache));

    m_instanceImageProvider = new InstanceImageProvider();
    engine()->addImageProvider("nodeInstance", m_instanceImageProvider);
}

InstanceImageProvider *Quick2PropertyEditorView::instanceImageProvider() const
{
    return m_instanceImageProvider;
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
        ListValidator::registerDeclarativeType();
        ColorPaletteBackend::registerDeclarativeType();
        QmlAnchorBindingProxy::registerDeclarativeType();
        QmlMaterialNodeProxy::registerDeclarativeType();
        QmlTextureNodeProxy::registerDeclarativeType();
        BindingEditor::registerDeclarativeType();
        ActionEditor::registerDeclarativeType();
        AnnotationEditor::registerDeclarativeType();
        AlignDistribute::registerDeclarativeType();
        Tooltip::registerDeclarativeType();
        EasingCurveEditor::registerDeclarativeType();
        RichTextEditorProxy::registerDeclarativeType();
        PropertyEditorDynamicPropertiesProxyModel::registerDeclarativeType();
        DynamicPropertyRow::registerDeclarativeType();
        PropertyChangesModel::registerDeclarativeType();
        PropertyModel::registerDeclarativeType();
        PropertyNameValidator::registerDeclarativeType();
        ScriptEditorBackend::registerDeclarativeType();

        const QString resourcePath = PropertyEditorQmlBackend::propertyEditorResourcesPath();

        QUrl regExpUrl = QUrl::fromLocalFile(resourcePath + "/RegExpValidator.qml");
        qmlRegisterType(regExpUrl, "HelperWidgets", 2, 0, "RegExpValidator");
        qmlRegisterUncreatableType<PropertyEditorContextObject>("PropertyToolBarAction", 2, 0, "ToolBarAction", "Enum type");
    }
}

} //QmlDesigner
