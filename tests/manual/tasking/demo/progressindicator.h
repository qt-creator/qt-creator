// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PROGRESSINDICATOR_H
#define PROGRESSINDICATOR_H

#include <QObject>

class ProgressIndicator : public QObject
{
public:
    ProgressIndicator(QWidget *parent = nullptr);

    void show();
    void hide();

private:
    class ProgressIndicatorWidget *m_widget = nullptr;
};

#endif // PROGRESSINDICATOR_H
