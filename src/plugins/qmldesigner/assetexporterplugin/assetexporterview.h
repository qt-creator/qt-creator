// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "abstractview.h"

#include "utils/fileutils.h"

#include <QObject>
#include <QTimer>

#include <memory>

namespace Core {
class IEditor;
}
namespace QmlDesigner {


class AssetExporterView : public AbstractView
{
    Q_OBJECT
public:
    enum class LoadState {
        Idle = 1,
        Busy,
        Exausted,
        QmlErrorState,
        Loaded
    };

    AssetExporterView(ExternalDependenciesInterface &externalDependencies);

    bool loadQmlFile(const Utils::FilePath &path, uint timeoutSecs = 10);
    bool saveQmlFile(QString *error) const;

    void modelAttached(Model *model) override;
    void instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &informationChangeHash) override;
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) override;

    LoadState loadingState() const { return m_state; }
    bool inErrorState() const;

signals:
    void loadingFinished();
    void loadingError(LoadState);
    void previewChanged();

private:
    bool isLoaded() const;
    void setState(LoadState state);
    void handleMaybeDone();
    void handleTimerTimeout();

    Core::IEditor *m_currentEditor = nullptr;
    QTimer m_timer;
    int m_retryCount = 0;
    LoadState m_state = LoadState::Idle;
    bool m_waitForPuppet = false;
};

}

QT_BEGIN_NAMESPACE
QDebug operator<<(QDebug os, const QmlDesigner::AssetExporterView::LoadState &s);
QT_END_NAMESPACE
