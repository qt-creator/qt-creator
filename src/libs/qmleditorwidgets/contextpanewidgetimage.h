// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include "contextpanewidget.h"
#include <qdrawutil.h>

#include <QLabel>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QLabel;
class QRadioButton;
class QSlider;
QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {

class FileWidget;

class PreviewLabel : public QLabel
{
    Q_OBJECT

public:
    PreviewLabel(QWidget *parent = nullptr);
    void setZoom(int);
    void setIsBorderImage(bool b);
    void setMargins(int left, int top, int right, int bottom);
    int leftMarging() const { return m_left; }
    int topMarging() const { return m_top; }
    int rightMarging() const { return m_right; }
    int bottomMarging() const { return m_bottom; }

signals:
    void leftMarginChanged();
    void topMarginChanged();
    void bottomMarginChanged();
    void rightMarginChanged();

protected:
    void paintEvent(QPaintEvent *event) final;
    void mousePressEvent(QMouseEvent * event) final;
    void mouseReleaseEvent(QMouseEvent * event) final;
    void mouseMoveEvent(QMouseEvent * event) final;
    void leaveEvent(QEvent* event ) final;
private:
    bool m_showBorders;
    int m_left, m_right, m_top, m_bottom;
    bool m_dragging_left;
    bool m_dragging_right;
    bool m_dragging_top;
    bool m_dragging_bottom;
    QPoint m_startPos;
    int m_zoom;
    bool m_isBorderImage;
    QLabel *m_hooverInfo;
};

class PreviewDialog : public DragWidget
{
    Q_OBJECT

public:
    PreviewDialog(QWidget *parent = nullptr);
    void setPixmap(const QPixmap &p, int zoom = 1);
    void setZoom(int z);
    void setIsBorderImage(bool b);
    PreviewLabel *previewLabel() const;
    int zoom() { return m_zoom; }

    void onTogglePane();
    void onSliderMoved(int value);

protected:
    void wheelEvent(QWheelEvent* event) final;

private:
    PreviewLabel *m_label;
    QSlider *m_slider;
    QLabel *m_zoomLabel;
    int m_zoom;
    QPixmap m_pixmap;
    bool m_isBorderImage;
};

class QMLEDITORWIDGETS_EXPORT ContextPaneWidgetImage : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneWidgetImage(QWidget *parent = nullptr, bool borderImage = false);
    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setPath(const QString& path);
    PreviewDialog* previewDialog();

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

public:
    void onStretchChanged();
    void onVerticalStretchChanged();
    void onHorizontalStretchChanged();
    void onFileNameChanged();
    void onPixmapDoubleClicked();
    void setPixmap(const QString &fileName);
    void onLeftMarginsChanged();
    void onTopMarginsChanged();
    void onBottomMarginsChanged();
    void onRightMarginsChanged();

protected:
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    FileWidget *m_fileWidget;
    QLabel *m_previewLabel;
    QLabel *m_sizeLabel;
    struct {
        QRadioButton *verticalTileRadioButton;
        QRadioButton *verticalStretchRadioButton;
        QRadioButton *verticalTileRadioButtonNoCrop;
        QRadioButton *horizontalTileRadioButton;
        QRadioButton *horizontalStretchRadioButton;
        QRadioButton *horizontalTileRadioButtonNoCrop;
    } m_borderImage;
    struct {
        QRadioButton *stretchRadioButton;
        QRadioButton *tileRadioButton;
        QRadioButton *horizontalStretchRadioButton;
        QRadioButton *verticalStretchRadioButton;
        QRadioButton *preserveAspectFitRadioButton;
        QRadioButton *cropAspectFitRadioButton;
    } m_image;

    QString m_path;
    QPointer<PreviewDialog> m_previewDialog;
    bool m_isBorderImage;
    bool m_previewWasVisible = false;
};

class LabelFilter: public QObject {

    Q_OBJECT
public:
    LabelFilter(QObject* parent =nullptr) : QObject(parent) {}
signals:
    void doubleClicked();
protected:
    bool eventFilter(QObject *obj, QEvent *event) final;
};

class WheelFilter: public QObject {

    Q_OBJECT
public:
    WheelFilter(QObject* parent =nullptr) : QObject(parent) {}
    void setTarget(QObject *target) { m_target = target; }
protected:
    bool eventFilter(QObject *obj, QEvent *event) final;
    QObject *m_target;
};





} //QmlDesigner
