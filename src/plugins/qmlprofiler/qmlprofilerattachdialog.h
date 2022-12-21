// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QDialog>

namespace ProjectExplorer { class Kit; }

namespace QmlProfiler {
namespace Internal {

class QmlProfilerAttachDialogPrivate;
class QmlProfilerAttachDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QmlProfilerAttachDialog(QWidget *parent = nullptr);
    ~QmlProfilerAttachDialog() override;

    int port() const;
    void setPort(const int port);

    ProjectExplorer::Kit *kit() const;
    void setKitId(Utils::Id id);

private:
    QmlProfilerAttachDialogPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler
