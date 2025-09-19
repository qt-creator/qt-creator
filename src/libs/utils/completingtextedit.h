// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"
#include "fancyiconbutton.h"

#include <QTextEdit>

QT_BEGIN_NAMESPACE
class QCompleter;
class QEvent;
class QKeyEvent;
class QFocusEvent;
class QResizeEvent;
QT_END_NAMESPACE

namespace Utils {

class FilePath;

class QTCREATOR_UTILS_EXPORT CompletingTextEdit : public QTextEdit
{
    Q_OBJECT
    Q_PROPERTY(int completionLengthThreshold
               READ completionLengthThreshold WRITE setCompletionLengthThreshold)
    Q_PROPERTY(CompletionBehavior completionBehavior READ completionBehavior WRITE setCompletionBehavior)

public:
    enum class CompletionBehavior {
        OnKeyPress,
        OnTextChange
    };
    Q_ENUM(CompletionBehavior)

    CompletingTextEdit(QWidget *parent = nullptr);
    ~CompletingTextEdit() override;

    void setCompleter(QCompleter *c);
    QCompleter *completer() const;

    int completionLengthThreshold() const;
    void setCompletionLengthThreshold(int len);

    CompletionBehavior completionBehavior() const;
    void setCompletionBehavior(CompletionBehavior behavior);

    void setRightSideIconPath(const FilePath &path);
    void setRightSideIcon(const QIcon &icon);

signals:
    void returnPressed();
    void rightSideIconClicked();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    void focusInEvent(QFocusEvent *e) override;
    bool event(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void updateIconPosition();
    void updateTextMargins();
    void handleIconClick();
    QString getCompletionPrefix() const;
    void updateCompletion();
    void showCompleterPopup();

private:
    class CompletingTextEditPrivate *d;
    FancyIconButton *m_rightButton = nullptr;
    CompletionBehavior m_completionBehavior = CompletionBehavior::OnKeyPress;
};

} // namespace Utils
