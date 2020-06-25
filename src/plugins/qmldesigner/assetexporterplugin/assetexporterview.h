/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

    AssetExporterView(QObject *parent = nullptr);

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

QDebug operator<<(QDebug os, const QmlDesigner::AssetExporterView::LoadState &s);
