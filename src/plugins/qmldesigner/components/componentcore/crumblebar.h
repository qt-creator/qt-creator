// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <utils/crumblepath.h>
#include <utils/fileutils.h>
#include <modelnode.h>

namespace QmlDesigner {

class CrumbleBarInfo {
public:

    CrumbleBarInfo() = default;

    CrumbleBarInfo(Utils::FilePath f,
                   QString d,
                   ModelNode m) :
        fileName(f),
        displayName(d),
        modelNode(m)
    {}

    Utils::FilePath fileName;
    QString displayName;
    ModelNode modelNode;
};

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

    QStringList path() const;

    QList<CrumbleBarInfo> infos() const;

    void onCrumblePathElementClicked(const QVariant &data);

signals:
    void pathChanged();

private:
    void updateVisibility();
    bool showSaveDialog();
    void popElement();

private:
    bool m_isInternalCalled = false;
    Utils::CrumblePath *m_crumblePath = nullptr;
    QList<CrumbleBarInfo> m_pathes;
};

bool operator ==(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
bool operator !=(const CrumbleBarInfo &first, const CrumbleBarInfo &second);
} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CrumbleBarInfo)
