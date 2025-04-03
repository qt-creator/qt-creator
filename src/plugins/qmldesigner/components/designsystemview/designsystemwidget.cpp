// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designsystemwidget.h"
#include "designsysteminterface.h"
#include "designsystemview.h"

#include <designersettings.h>
#include <theme.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <QApplication>

#include <QBoxLayout>
#include <QFileInfo>
#include <QKeySequence>
#include <QShortcut>

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
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

DesignSystemWidget::DesignSystemWidget(DesignSystemView *view, DesignSystemInterface *interface)
    : m_designSystemView(view)
    , m_qmlSourceUpdateShortcut(nullptr)
{
    engine()->addImportPath(qmlSourcesPath());
    engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    engine()->addImportPath(qmlSourcesPath() + "/imports");

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F10), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &DesignSystemWidget::reloadQmlSource);

    quickWidget()->setObjectName(Constants::OBJECT_NAME_DESIGN_SYSTEM);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto map = registerPropertyMap("DesignSystemBackend");
    map->setProperties({{"dsInterface", QVariant::fromValue(interface)}});

    Theme::setupTheme(engine());

    setWindowTitle(tr("Design Tokens", "Title of Editor widget"));
    setMinimumSize(QSize(195, 195));

    // init the first load of the QML UI elements
    reloadQmlSource();
}

DesignSystemWidget::~DesignSystemWidget() = default;

QString DesignSystemWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/designsystem";
#endif
    return Core::ICore::resourcePath("qmldesigner/designsystem").toUrlishString();
}

void DesignSystemWidget::showEvent(QShowEvent *event)
{
    StudioQuickWidget::showEvent(event);
    update();
    QMetaObject::invokeMethod(rootObject(), "showEvent");
}

void DesignSystemWidget::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_DESIGNSYSTEM_TIME,
                                               m_usageTimer.elapsed());
    StudioQuickWidget::focusOutEvent(focusEvent);
}

void DesignSystemWidget::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    StudioQuickWidget::focusInEvent(focusEvent);
}

void DesignSystemWidget::reloadQmlSource()
{
    QString statesListQmlFilePath = qmlSourcesPath() + QStringLiteral("/Main.qml");
    QTC_ASSERT(QFileInfo::exists(statesListQmlFilePath), return );
    setSource(QUrl::fromLocalFile(statesListQmlFilePath));

    if (!rootObject()) {
        QString errorString;
        for (const QQmlError &error : errors())
            errorString += "\n" + error.toString();

        Core::AsynchronousMessageBox::warning(tr("Cannot Create QtQuick View"),
                                              tr("StatesEditorWidget: %1 cannot be created.%2")
                                                  .arg(qmlSourcesPath(), errorString));
        return;
    }
}

} // namespace QmlDesigner
