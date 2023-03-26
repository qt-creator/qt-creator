// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <qmldesignercomponents_global.h>

#include <QPointer>
#include <QWidgetAction>

#include <array>

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT ZoomAction : public QWidgetAction
{
    Q_OBJECT

signals:
    void zoomLevelChanged(double zoom);

public:
    ZoomAction(QObject *parent);

    static std::array<double, 27> zoomLevels();
    static int indexOf(double zoom);

    void setZoomFactor(double zoom);
    double setNextZoomFactor(double zoom);
    double setPreviousZoomFactor(double zoom);

    int currentIndex() const;

protected:
    QWidget *createWidget(QWidget *parent) override;

private:
    void emitZoomLevelChanged(int index);

    static std::array<double, 27> m_zooms;
    QPointer<QComboBox> m_combo;
    int m_index = -1;
};

} // namespace QmlDesigner
