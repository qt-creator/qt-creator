// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "devicemanagerwidget.h"

#include <designersettings.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qrcodegen/src/qrcodeimageprovider.h>
#include <theme.h>

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

namespace QmlDesigner::DeviceShare {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toUrlishString();
}

DeviceManagerWidget::DeviceManagerWidget(DeviceManager &deviceManager, QWidget *parent)
    : StudioQuickWidget(parent)
    , m_deviceManagerModel(deviceManager)
{
    engine()->addImageProvider(QLatin1String("QrGenerator"), new QrCodeImageProvider);
    engine()->addImportPath(qmlSourcesPath());
    engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    engine()->addImportPath(qmlSourcesPath() + "/imports");

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F10), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &DeviceManagerWidget::reloadQmlSource);

    quickWidget()->setObjectName(Constants::OBJECT_NAME_DEVICE_MANAGER);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto map = registerPropertyMap("DeviceManagerBackend");
    map->setProperties({{"deviceManagerModel", QVariant::fromValue(&m_deviceManagerModel)}});

    Theme::setupTheme(engine());

    setWindowFlags(Qt::Dialog);
    setWindowTitle(tr("Device Manager", "Title of device manager widget"));
    setMinimumSize(QSize(195, 195));
    resize(1020, 720);

    // init the first load of the QML UI elements
    reloadQmlSource();
}

DeviceManagerWidget::~DeviceManagerWidget() = default;

QString DeviceManagerWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/devicemanager";
#endif
    return Core::ICore::resourcePath("qmldesigner/devicemanager").toUrlishString();
}

void DeviceManagerWidget::showEvent(QShowEvent *event)
{
    StudioQuickWidget::showEvent(event);
    update();
}

void DeviceManagerWidget::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_DEVICEMANAGER_TIME,
                                               m_usageTimer.elapsed());
    StudioQuickWidget::focusOutEvent(focusEvent);
}

void DeviceManagerWidget::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    StudioQuickWidget::focusInEvent(focusEvent);
}

void DeviceManagerWidget::reloadQmlSource()
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

} // namespace QmlDesigner::DeviceShare
