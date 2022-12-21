// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <utils/crumblepath.h>
#include <utils/fileutils.h>
#include <modelnode.h>

namespace QmlDesigner {

class CrumbleBar : public QObject
{
    Q_OBJECT
public:
    explicit CrumbleBar(QObject *parent = nullptr);
    ~CrumbleBar() override;

    void pushFile(const Utils::FilePath &fileName);
    void pushInFileComponent(const ModelNode &modelNode);

    void nextFileIsCalledInternally();

    Utils::CrumblePath *crumblePath();

private:
    void onCrumblePathElementClicked(const QVariant &data);
    void updateVisibility();
    bool showSaveDialog();

private:
    bool m_isInternalCalled = false;
    Utils::CrumblePath *m_crumblePath = nullptr;
};

class CrumbleBarInfo {
public:
    Utils::FilePath fileName;
    QString displayName;
    ModelNode modelNode;
};

bool operator ==(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
bool operator !=(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CrumbleBarInfo)
