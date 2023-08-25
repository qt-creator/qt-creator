// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlpreview_global.h"
#include "qmldebugtranslationclient.h"

#include <projectexplorer/runcontrol.h>
#include <extensionsystem/iplugin.h>
#include <qmljs/qmljsdialect.h>

#include <QUrl>
#include <QThread>

namespace Core { class IEditor; }

namespace ProjectExplorer { class RunControl; }

namespace QmlDebug { class QmlDebugConnection; }

namespace QmlPreview {

using QmlPreviewFileClassifier = bool (*)(const QString &);
using QmlPreviewFileLoader = QByteArray (*)(const QString &, bool *);
using QmlPreviewFpsHandler = void (*)(quint16[8]);
using QmlPreviewRunControlList = QList<ProjectExplorer::RunControl *>;
using QmlDebugTranslationClientFactoryFunction =
    std::function<std::unique_ptr<QmlDebugTranslationClient>(QmlDebug::QmlDebugConnection *)>;
using QmlPreviewRefreshTranslationFunction = std::function<void ()>;

class QMLPREVIEW_EXPORT QmlPreviewPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlPreview.json")
    Q_PROPERTY(QString previewedFile READ previewedFile WRITE setPreviewedFile
               NOTIFY previewedFileChanged)
    Q_PROPERTY(QmlPreview::QmlPreviewRunControlList runningPreviews READ runningPreviews
               NOTIFY runningPreviewsChanged)
    Q_PROPERTY(QmlPreview::QmlPreviewFileLoader fileLoader READ fileLoader
               WRITE setFileLoader NOTIFY fileLoaderChanged)
    Q_PROPERTY(QmlPreview::QmlPreviewFileClassifier fileClassifer READ fileClassifier
               WRITE setFileClassifier NOTIFY fileClassifierChanged)
    Q_PROPERTY(QmlPreview::QmlPreviewFpsHandler fpsHandler READ fpsHandler
               WRITE setFpsHandler NOTIFY fpsHandlerChanged)
    Q_PROPERTY(float zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(QString localeIsoCode READ localeIsoCode WRITE setLocaleIsoCode NOTIFY localeIsoCodeChanged)

public:
    ~QmlPreviewPlugin() override;

    void initialize() override;
    ShutdownFlag aboutToShutdown() override;

    QString previewedFile() const;
    void setPreviewedFile(const QString &previewedFile);
    QmlPreviewRunControlList runningPreviews() const;
    void stopAllPreviews();

    void setFileLoader(QmlPreviewFileLoader fileLoader);
    QmlPreviewFileLoader fileLoader() const;

    QmlPreviewFileClassifier fileClassifier() const;
    void setFileClassifier(QmlPreviewFileClassifier fileClassifer);

    float zoomFactor() const;
    void setZoomFactor(float zoomFactor);

    QmlPreview::QmlPreviewFpsHandler fpsHandler() const;
    void setFpsHandler(QmlPreview::QmlPreviewFpsHandler fpsHandler);

    QString localeIsoCode() const;
    void setLocaleIsoCode(const QString &localeIsoCode);

    void setQmlDebugTranslationClientCreator(QmlDebugTranslationClientFactoryFunction creator);
    void setRefreshTranslationsFunction(QmlPreviewRefreshTranslationFunction refreshTranslationsFunction);

    void previewCurrentFile();
    void addPreview(ProjectExplorer::RunControl *preview);
    void removePreview(ProjectExplorer::RunControl *preview);

signals:
    void checkDocument(const QString &name, const QByteArray &contents,
                       QmlJS::Dialect::Enum dialect);
    void updatePreviews(const QString &previewedFile, const QString &changedFile,
                        const QByteArray &contents);
    void previewedFileChanged(const QString &previewedFile);
    void runningPreviewsChanged(const QmlPreviewRunControlList &runningPreviews);
    void rerunPreviews();
    void fileLoaderChanged(QmlPreviewFileLoader fileLoader);
    void fileClassifierChanged(QmlPreviewFileClassifier fileClassifer);
    void fpsHandlerChanged(QmlPreview::QmlPreviewFpsHandler fpsHandler);

    void zoomFactorChanged(float zoomFactor);
    void localeIsoCodeChanged(const QString &localeIsoCode);

private:
    class QmlPreviewPluginPrivate *d = nullptr;
};

} // namespace QmlPreview

Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFileLoader)
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFileClassifier)
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFpsHandler)
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewRunControlList)
