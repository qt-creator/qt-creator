// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "insightwidget.h"
#include "insightmodel.h"
#include "insightview.h"

#include <componentcore/theme.h>
#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <invalidqmlsourceexception.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QApplication>
#include <QFileInfo>
#include <QShortcut>
#include <QBoxLayout>
#include <QKeySequence>

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>

enum {
    debug = false
};

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

InsightWidget::InsightWidget(InsightView *insightView, InsightModel *insightModel)
    : m_insightView(insightView)
    , m_qmlSourceUpdateShortcut(nullptr)
{
    engine()->addImportPath(qmlSourcesPath());
    engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    engine()->addImportPath(qmlSourcesPath() + "/imports");

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F11), this);
    connect(m_qmlSourceUpdateShortcut,
            &QShortcut::activated,
            this,
            &InsightWidget::reloadQmlSource);

    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    rootContext()->setContextProperties({{"insightModel", QVariant::fromValue(insightModel)}});

    Theme::setupTheme(engine());

    setWindowTitle(tr("Qt Insight", "Title of the widget"));
    setMinimumWidth(195);
    setMinimumHeight(195);

    // init the first load of the QML UI elements
    reloadQmlSource();
}

InsightWidget::~InsightWidget() = default;

QString InsightWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/insight";
#endif
    return Core::ICore::resourcePath("qmldesigner/insight").toString();
}

void InsightWidget::showEvent(QShowEvent *event)
{
    QQuickWidget::showEvent(event);
    update();
}

void InsightWidget::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_INSIGHT_TIME, m_usageTimer.elapsed());
    QQuickWidget::focusOutEvent(focusEvent);
}

void InsightWidget::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    QQuickWidget::focusInEvent(focusEvent);
}

void InsightWidget::reloadQmlSource()
{
    QString statesListQmlFilePath = qmlSourcesPath() + QStringLiteral("/Main.qml");
    QTC_ASSERT(QFileInfo::exists(statesListQmlFilePath), return );
    engine()->clearComponentCache();
    setSource(QUrl::fromLocalFile(statesListQmlFilePath));

    if (!rootObject()) {
        QString errorString;
        for (const QQmlError &error : errors())
            errorString += "\n" + error.toString();

        Core::AsynchronousMessageBox::warning(tr("Cannot Create QtQuick View"),
                                              tr("InsightWidget: %1 cannot be created.%2")
                                                  .arg(qmlSourcesPath(), errorString));
        return;
    }
}

} // namespace QmlDesigner
