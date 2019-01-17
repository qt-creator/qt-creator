/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include <projectexplorer/runconfiguration.h>
#include <extensionsystem/iplugin.h>
#include <qmljs/qmljsdialect.h>
#include <QUrl>
#include <QThread>

namespace Core { class IEditor; }

namespace QmlPreview {

typedef bool (*QmlPreviewFileClassifier) (const QString &);
typedef QByteArray (*QmlPreviewFileLoader)(const QString &, bool *);
typedef void (*QmlPreviewFpsHandler)(quint16[8]);
typedef QList<ProjectExplorer::RunControl *> QmlPreviewRunControlList;

namespace Internal {

class QmlPreviewPlugin : public ExtensionSystem::IPlugin
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
    Q_PROPERTY(QString locale READ locale WRITE setLocale NOTIFY localeChanged)

public:
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;
    QList<QObject *> createTestObjects() const override;

    QString previewedFile() const;
    void setPreviewedFile(const QString &previewedFile);
    QmlPreviewRunControlList runningPreviews() const;

    void setFileLoader(QmlPreviewFileLoader fileLoader);
    QmlPreviewFileLoader fileLoader() const;

    QmlPreviewFileClassifier fileClassifier() const;
    void setFileClassifier(QmlPreviewFileClassifier fileClassifer);

    float zoomFactor() const;
    void setZoomFactor(float zoomFactor);

    QmlPreview::QmlPreviewFpsHandler fpsHandler() const;
    void setFpsHandler(QmlPreview::QmlPreviewFpsHandler fpsHandler);

    QString locale() const;
    void setLocale(const QString &locale);

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
    void localeChanged(const QString &locale);

private:
    void previewCurrentFile();
    void onEditorChanged(Core::IEditor *editor);
    void onEditorAboutToClose(Core::IEditor *editor);
    void setDirty();
    void addPreview(ProjectExplorer::RunControl *preview);
    void removePreview(ProjectExplorer::RunControl *preview);
    void attachToEditor();
    void checkEditor();
    void checkFile(const QString &fileName);
    void triggerPreview(const QString &changedFile, const QByteArray &contents);

    QThread m_parseThread;
    QString m_previewedFile;
    QmlPreviewFileLoader m_fileLoader = nullptr;
    Core::IEditor *m_lastEditor = nullptr;
    QmlPreviewRunControlList m_runningPreviews;
    bool m_dirty = false;
    QmlPreview::QmlPreviewFileClassifier m_fileClassifer = nullptr;
    float m_zoomFactor = -1.0;
    QmlPreview::QmlPreviewFpsHandler m_fpsHandler = nullptr;
    QString m_locale;
};

} // namespace Internal
} // namespace QmlPreview

Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFileLoader)
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFileClassifier)
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewFpsHandler)
Q_DECLARE_METATYPE(QmlPreview::QmlPreviewRunControlList)
