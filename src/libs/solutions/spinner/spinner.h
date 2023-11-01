// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef SPINNER_H
#define SPINNER_H

#include "spinner_global.h"

#include <QObject>

namespace SpinnerSolution {

Q_NAMESPACE_EXPORT(SPINNER_EXPORT)

enum class SpinnerSize { Small, Medium, Large };
Q_ENUM_NS(SpinnerSize)

// TODO: SpinnerOverlay and SpinnerWidget?

class SPINNER_EXPORT Spinner : public QObject
{
    Q_OBJECT
public:
    explicit Spinner(SpinnerSize size, QWidget *parent = nullptr);
    void setSize(SpinnerSize size);
    void show();
    void hide();
    bool isVisible() const;
    void setVisible(bool visible);

private:
    class SpinnerWidget *m_widget = nullptr;
};

} // namespace SpinnerSolution

#endif // SPINNER_H
