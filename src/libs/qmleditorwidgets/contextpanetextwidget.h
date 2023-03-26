// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmleditorwidgets_global.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QFontComboBox;
class QLabel;
class QToolButton;
class QVariant;
QT_END_NAMESPACE

namespace QmlJS { class PropertyReader; }

namespace QmlEditorWidgets {

class ColorButton;
class FontSizeSpinBox;

class QMLEDITORWIDGETS_EXPORT ContextPaneTextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ContextPaneTextWidget(QWidget *parent = nullptr);

    void setProperties(QmlJS::PropertyReader *propertyReader);
    void setVerticalAlignmentVisible(bool);
    void setStyleVisible(bool);

    void onTextColorButtonToggled(bool);
    void onColorButtonToggled(bool);
    void onColorDialogApplied(const QColor &color);
    void onColorDialogCancled();
    void onFontSizeChanged(int value);
    void onFontFormatChanged();
    void onBoldCheckedChanged(bool value);
    void onItalicCheckedChanged(bool value);
    void onUnderlineCheckedChanged(bool value);
    void onStrikeoutCheckedChanged(bool value);
    void onCurrentFontChanged(const QFont &font);
    void onHorizontalAlignmentChanged();
    void onVerticalAlignmentChanged();
    void onStyleComboBoxChanged(int index);

signals:
    void propertyChanged(const QString &, const QVariant &);
    void removeProperty(const QString &);
    void removeAndChangeProperty(const QString &, const QString &, const QVariant &, bool removeFirst);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QFontComboBox *m_fontComboBox;
    ColorButton *m_colorButton;
    FontSizeSpinBox *m_fontSizeSpinBox;

    QToolButton *m_boldButton;
    QToolButton *m_italicButton;
    QToolButton *m_underlineButton;
    QToolButton *m_strikeoutButton;

    QLabel *m_styleLabel;
    QComboBox *m_styleComboBox;
    ColorButton *m_textColorButton;

    QToolButton *m_leftAlignmentButton;
    QToolButton *m_centerHAlignmentButton;
    QToolButton *m_rightAlignmentButton;

    QToolButton *m_topAlignmentButton;
    QToolButton *m_centerVAlignmentButton;
    QToolButton *m_bottomAlignmentButton;

    QString m_verticalAlignment;
    QString m_horizontalAlignment;
    int m_fontSizeTimer = -1;
};

} //QmlDesigner
