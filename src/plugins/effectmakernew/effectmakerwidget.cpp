// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerwidget.h"

#include "effectmakercontextobject.h"
#include "effectmakermodel.h"
#include "effectmakernodesmodel.h"
#include "effectmakerview.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "qqmlcontext.h"
#include "theme.h"

#include <coreplugin/icore.h>

#include <studioquickwidget.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QQmlEngine>

namespace EffectMaker {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

EffectMakerWidget::EffectMakerWidget(EffectMakerView *view)
    : m_effectMakerModel{new EffectMakerModel(this)}
    , m_effectMakerNodesModel{new EffectMakerNodesModel(this)}
    , m_effectMakerView(view)
    , m_quickWidget{new StudioQuickWidget(this)}
{
    setWindowTitle(tr("Effect Maker", "Title of effect maker widget"));
    setMinimumWidth(250);

    m_quickWidget->quickWidget()->installEventFilter(this);

    // create the inner widget
    m_quickWidget->quickWidget()->setObjectName(QmlDesigner::Constants::OBJECT_NAME_EFFECT_MAKER);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QmlDesigner::Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->setClearColor(QmlDesigner::Theme::getColor(
        QmlDesigner::Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.data());

    setStyleSheet(QmlDesigner::Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    QmlDesigner::QmlDesignerPlugin::trackWidgetFocusTime(this, QmlDesigner::Constants::EVENT_EFFECTMAKER_TIME);

    auto map = m_quickWidget->registerPropertyMap("EffectMakerBackend");
    map->setProperties({{"effectMakerNodesModel", QVariant::fromValue(m_effectMakerNodesModel.data())},
                        {"effectMakerModel", QVariant::fromValue(m_effectMakerModel.data())},
                        {"rootView", QVariant::fromValue(this)}});
}


bool EffectMakerWidget::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj)
    Q_UNUSED(event)

    // TODO

    return false;
}

void EffectMakerWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    Q_UNUSED(callback)
}

StudioQuickWidget *EffectMakerWidget::quickWidget() const
{
    return m_quickWidget.data();
}

QPointer<EffectMakerModel> EffectMakerWidget::effectMakerModel() const
{
    return m_effectMakerModel;
}

QPointer<EffectMakerNodesModel> EffectMakerWidget::effectMakerNodesModel() const
{
    return m_effectMakerNodesModel;
}

void EffectMakerWidget::addEffectNode(const QString &nodeQenPath)
{
    m_effectMakerModel->addNode(nodeQenPath);
}

void EffectMakerWidget::focusSection(int section)
{
    Q_UNUSED(section)
}

QSize EffectMakerWidget::sizeHint() const
{
    return {420, 420};
}

QString EffectMakerWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectMakerQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectMakerQmlSources").toString();
}

void EffectMakerWidget::initView()
{
    auto ctxObj = new EffectMakerContextObject(m_quickWidget->rootContext());
    m_quickWidget->rootContext()->setContextObject(ctxObj);

    m_backendModelNode.setup(m_effectMakerView->rootModelNode());
    m_quickWidget->rootContext()->setContextProperty("modelNodeBackend", &m_backendModelNode);

    // init the first load of the QML UI elements
    reloadQmlSource();
}

void EffectMakerWidget::reloadQmlSource()
{
    const QString effectMakerQmlPath = qmlSourcesPath() + "/EffectMaker.qml";
    QTC_ASSERT(QFileInfo::exists(effectMakerQmlPath), return);
    m_quickWidget->setSource(QUrl::fromLocalFile(effectMakerQmlPath));
}

} // namespace EffectMaker

