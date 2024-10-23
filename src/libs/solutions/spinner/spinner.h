// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SPINNER_H
#define SPINNER_H

#include "spinner_global.h"

#include <QPointer>
#include <QWidget>

namespace SpinnerSolution {

Q_NAMESPACE_EXPORT(SPINNER_EXPORT)

enum class SpinnerSize { Small, Medium, Large };
Q_ENUM_NS(SpinnerSize)

enum class SpinnerState { NotRunning, Running };
Q_ENUM_NS(SpinnerState)

// TODO: SpinnerOverlay and SpinnerWidget?

class SpinnerOverlay;

class SPINNER_EXPORT Spinner : public QObject
{
    Q_OBJECT

public:
    explicit Spinner(SpinnerSize size, QWidget *parent = nullptr);
    ~Spinner() override;
    void setSize(SpinnerSize size);
    void setColor(const QColor &color);
    void show();
    void hide();
    bool isVisible() const;
    void setVisible(bool visible);

private:
    QPointer<SpinnerOverlay> m_widget;
};

class SPINNER_EXPORT SpinnerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SpinnerWidget(QWidget *parent = nullptr);
    void setState(SpinnerState state);
    void setDecorated(bool on);

private:
    class SpinnerWidgetPrivate *d;
};

} // namespace SpinnerSolution

#endif // SPINNER_H
