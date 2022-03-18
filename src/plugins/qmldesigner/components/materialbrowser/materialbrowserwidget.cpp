/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "materialbrowserwidget.h"
#include "materialbrowsermodel.h"

#include <theme.h>

#include <designeractionmanager.h>
#include <designermcumanager.h>
#include <documentmanager.h>
#include <qmldesignerconstants.h>

#include <utils/algorithm.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>

#include <QImageReader>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QShortcut>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

class PreviewImageProvider : public QQuickImageProvider
{
    QHash<qint32, QPixmap> m_pixmaps;

public:
    PreviewImageProvider()
        : QQuickImageProvider(Pixmap) {}

    void setPixmap(const ModelNode &node, const QPixmap &pixmap)
    {
        m_pixmaps.insert(node.internalId(), pixmap);
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(requestedSize)

        QPixmap pixmap{150, 150};

        qint32 internalId = id.toInt();
        if (m_pixmaps.contains(internalId))
            pixmap = m_pixmaps.value(internalId);

        if (size)
            *size = pixmap.size();

        return pixmap;
    }
};

bool MaterialBrowserWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusOut) {
        if (obj == m_quickWidget.data())
            QMetaObject::invokeMethod(m_quickWidget->rootObject(), "closeContextMenu");
    }

    return QObject::eventFilter(obj, event);
}

MaterialBrowserWidget::MaterialBrowserWidget()
    : m_materialBrowserModel(new MaterialBrowserModel(this))
    , m_quickWidget(new QQuickWidget(this))
    , m_previewImageProvider(new PreviewImageProvider())
{
    setWindowTitle(tr("Material Browser", "Title of material browser widget"));
    setMinimumWidth(120);

    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    m_quickWidget->setClearColor(Theme::getColor(Theme::Color::DSpanelBackground));

    m_quickWidget->rootContext()->setContextProperties({
        {"rootView", QVariant::fromValue(this)},
        {"materialBrowserModel", QVariant::fromValue(m_materialBrowserModel.data())},
    });

    m_quickWidget->engine()->addImageProvider("materialBrowser", m_previewImageProvider);
    Theme::setupTheme(m_quickWidget->engine());
    m_quickWidget->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(m_quickWidget.data());

    updateSearch();

    setStyleSheet(Theme::replaceCssColors(
        QString::fromUtf8(Utils::FileReader::fetchQrc(":/qmldesigner/stylesheet.css"))));

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_F8), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &MaterialBrowserWidget::reloadQmlSource);

    reloadQmlSource();
}

void MaterialBrowserWidget::updateMaterialPreview(const ModelNode &node, const QPixmap &pixmap)
{
    m_previewImageProvider->setPixmap(node, pixmap);
    int idx = m_materialBrowserModel->materialIndex(node);
    if (idx != -1)
        QMetaObject::invokeMethod(m_quickWidget->rootObject(), "refreshPreview", Q_ARG(QVariant, idx));
}

QList<QToolButton *> MaterialBrowserWidget::createToolBarWidgets()
{
    return {};
}

void MaterialBrowserWidget::handleSearchfilterChanged(const QString &filterText)
{
    if (filterText != m_filterText) {
        m_filterText = filterText;
        updateSearch();
    }
}

QString MaterialBrowserWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/materialBrowserQmlSource";
#endif
    return Core::ICore::resourcePath("qmldesigner/materialBrowserQmlSource").toString();
}

void MaterialBrowserWidget::clearSearchFilter()
{
    QMetaObject::invokeMethod(m_quickWidget->rootObject(), "clearSearchFilter");
}

void MaterialBrowserWidget::reloadQmlSource()
{
    const QString materialBrowserQmlPath = qmlSourcesPath() + "/MaterialBrowser.qml";

    QTC_ASSERT(QFileInfo::exists(materialBrowserQmlPath), return);

    m_quickWidget->engine()->clearComponentCache();
    m_quickWidget->setSource(QUrl::fromLocalFile(materialBrowserQmlPath));
}

void MaterialBrowserWidget::updateSearch()
{
    m_materialBrowserModel->setSearchText(m_filterText);
    m_quickWidget->update();
}

QQuickWidget *MaterialBrowserWidget::quickWidget() const
{
    return m_quickWidget.data();
}

QPointer<MaterialBrowserModel> MaterialBrowserWidget::materialBrowserModel() const
{
    return m_materialBrowserModel;
}

} // namespace QmlDesigner
