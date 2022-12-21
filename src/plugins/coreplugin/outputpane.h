// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <utils/id.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QSplitter;
QT_END_NAMESPACE

namespace Core {

class OutputPanePlaceHolderPrivate;

class CORE_EXPORT OutputPanePlaceHolder : public QWidget
{
    Q_OBJECT

public:
    explicit OutputPanePlaceHolder(Utils::Id mode, QSplitter *parent = nullptr);
    ~OutputPanePlaceHolder() override;

    static OutputPanePlaceHolder *getCurrent();
    static bool isCurrentVisible();

    bool isMaximized() const;
    void setMaximized(bool maximize);
    void ensureSizeHintAsMinimum();
    int nonMaximizedSize() const;

signals:
    void visibilityChangeRequested(bool visible);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *) override;

private:
    void setHeight(int height);
    void currentModeChanged(Utils::Id mode);

    OutputPanePlaceHolderPrivate *d;
};

} // namespace Core
