// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QImage>
#include <QPointer>
#include <QTimer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QTextDocument;
class QScrollBar;
class QStyleOptionComplex;
class QWheelEvent;
QT_END_NAMESPACE

namespace Utils {
class PlainTextEdit;
}

namespace Core {

class MinimapOverlay : public QWidget
{
public:
    explicit MinimapOverlay(Utils::PlainTextEdit *editor);

    void paintMinimap(QPainter *painter) const;
    void scheduleUpdate();

protected:
    void paintEvent(QPaintEvent *paintEvent) override;
    bool eventFilter(QObject *object, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void onDocumentChanged();
    void updateImage();
    int miniLineHeight() const { return m_pixelsPerLine + m_lineGap; }
    void doMove();
    void doResize();

    QPointer<Utils::PlainTextEdit> m_editor;
    QPointer<QTextDocument> m_doc;
    QPointer<QScrollBar> m_vScroll;

    const int m_minimapWidth = 100;
    const qreal m_minimapAlpha = 0.65;
    const int m_pixelsPerLine = 2;
    const int m_lineGap = 1;
    int m_scrollbarDefaultWidth;

    QImage m_minimap;
    QTimer m_updateTimer;
};

} // namespace Core
