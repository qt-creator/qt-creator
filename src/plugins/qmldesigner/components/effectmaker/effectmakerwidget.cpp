// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectmakerwidget.h"

#include "effectmakermodel.h"
#include "effectmakerview.h"
#include "qmldesignerconstants.h"
#include "qmldesignerplugin.h"
#include "theme.h"

#include <coreplugin/icore.h>

#include <studioquickwidget.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QQmlEngine>

namespace QmlDesigner {

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
    , m_effectMakerView(view)
    , m_effectMakerWidget{new StudioQuickWidget(this)}
{
    setWindowTitle(tr("Effect Maker", "Title of effect maker widget"));
    setMinimumWidth(250);

    m_effectMakerWidget->quickWidget()->installEventFilter(this);

    // create the inner widget
    m_effectMakerWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_EFFECT_MAKER);
    m_effectMakerWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    Theme::setupTheme(m_effectMakerWidget->engine());
    m_effectMakerWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_effectMakerWidget->setClearColor(Theme::getColor(Theme::Color::QmlDesigner_BackgroundColorDarkAlternate));

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_effectMakerWidget.data());

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_EFFECTMAKER_TIME);

    auto map = m_effectMakerWidget->registerPropertyMap("EffectMakerBackend");
    map->setProperties({{"effectMakerModel", QVariant::fromValue(m_effectMakerModel.data())},
                        {"rootView", QVariant::fromValue(this)}});

    // init the first load of the QML UI elements
    reloadQmlSource();
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
    return m_effectMakerWidget.data();
}

QPointer<EffectMakerModel> EffectMakerWidget::effectMakerModel() const
{
    return m_effectMakerModel;
}

void EffectMakerWidget::focusSection(int section)
{
    Q_UNUSED(section)
}

QString EffectMakerWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/effectMakerQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/effectMakerQmlSources").toString();
}

void EffectMakerWidget::reloadQmlSource()
{
    const QString effectMakerQmlPath = qmlSourcesPath() + "/EffectMaker.qml";
    QTC_ASSERT(QFileInfo::exists(effectMakerQmlPath), return);
    m_effectMakerWidget->setSource(QUrl::fromLocalFile(effectMakerQmlPath));
}

} // namespace QmlDesigner
