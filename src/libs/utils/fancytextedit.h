// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "result.h"

#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QCompleter;
QT_END_NAMESPACE

namespace Utils {

class FancyTextEditPrivate;

class QTCREATOR_UTILS_EXPORT FancyTextEdit : public QWidget
{
    Q_OBJECT

public:
    enum Side { Left = 0, Right = 1 };
    Q_ENUM(Side)

    explicit FancyTextEdit(QWidget *parent = nullptr);
    ~FancyTextEdit() override;

    QString text() const;
    void setText(const QString &text);
    void selectAll();

    QString placeholderText() const;
    void setPlaceholderText(const QString &text);

    int lineCount() const;
    int maximumVisibleLines() const;
    void setMaximumVisibleLines(int lines);

    void setButtonIcon(Side side, const QIcon &icon);
    void setButtonVisible(Side side, bool visible);
    QAbstractButton *button(Side side) const;

    void setClearButtonEnabled(bool enabled);

    void setSpecialCompleter(QCompleter *completer);

    using ValidationFunction = std::function<Result<>(const QString &)>;
    void setValidationFunction(const ValidationFunction &function);
    void validate();
    bool isValid() const;

    void installEditFilter(QObject *filter);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void textChanged();
    void validChanged(bool validState);
    void returnPressed();
    void editingFinished();
    void leftButtonClicked();
    void rightButtonClicked();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    friend class FancyTextEditPrivate;

    FancyTextEditPrivate *d;
};

} // namespace Utils
