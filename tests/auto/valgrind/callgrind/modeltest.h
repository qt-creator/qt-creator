// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {
class ParseData;
class Function;
class CallModel;
}
}

namespace Callgrind {
namespace Internal {
class CallgrindWidgetHandler;
}
}

class ModelTestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ModelTestWidget(Callgrind::Internal::CallgrindWidgetHandler *handler);
    virtual ~ModelTestWidget();

public slots:
    void showViewContextMenu(const QPoint &pos);

    void formatChanged(int);

public:
    QComboBox *m_format;
    QComboBox *m_event;
    Callgrind::Internal::CallgrindWidgetHandler *m_handler;
};
