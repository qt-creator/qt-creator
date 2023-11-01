// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFrame>

QT_BEGIN_NAMESPACE
class QHBoxLayout;
class QToolButton;
QT_END_NAMESPACE

namespace ScxmlEditor::Common {

class ColorPicker : public QFrame
{
    Q_OBJECT

public:
    explicit ColorPicker(const QString &key, QWidget *parent = nullptr);
    ~ColorPicker() override;

    void setLastUsedColor(const QString &colorName);

signals:
    void colorSelected(const QString &colorName);

private:
    QToolButton *createButton(const QColor &color);

    QStringList m_lastUsedColorNames;
    QVector<QToolButton*> m_lastUsedColorButtons;
    QString m_key;

    QHBoxLayout *m_lastUsedColorContainer;
};

} // namespace ScxmlEditor::Common
