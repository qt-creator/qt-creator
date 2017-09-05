/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"
#include "completinglineedit.h"

#include <QAbstractButton>

#include <functional>

QT_BEGIN_NAMESPACE
class QEvent;
QT_END_NAMESPACE

namespace Utils {

class FancyLineEditPrivate;

class QTCREATOR_UTILS_EXPORT IconButton: public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(float iconOpacity READ iconOpacity WRITE setIconOpacity)
    Q_PROPERTY(bool autoHide READ hasAutoHide WRITE setAutoHide)
public:
    explicit IconButton(QWidget *parent = 0);
    void paintEvent(QPaintEvent *event);
    float iconOpacity() { return m_iconOpacity; }
    void setIconOpacity(float value) { m_iconOpacity = value; update(); }
    void animateShow(bool visible);

    void setAutoHide(bool hide) { m_autoHide = hide; }
    bool hasAutoHide() const { return m_autoHide; }

    QSize sizeHint() const;

protected:
    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);

private:
    float m_iconOpacity;
    bool m_autoHide;
    QIcon m_icon;
};

class QTCREATOR_UTILS_EXPORT FancyLineEdit : public CompletingLineEdit
{
    Q_OBJECT
    Q_ENUMS(Side)

    // Validation.
    Q_PROPERTY(QString initialText READ initialText WRITE setInitialText DESIGNABLE true)
    Q_PROPERTY(QColor errorColor READ errorColor WRITE setErrorColor DESIGNABLE true)
    Q_PROPERTY(QColor okColor READ okColor WRITE setOkColor DESIGNABLE true)

public:
    enum Side {Left = 0, Right = 1};

    explicit FancyLineEdit(QWidget *parent = 0);
    ~FancyLineEdit();

    QIcon buttonIcon(Side side) const;
    void setButtonIcon(Side side, const QIcon &icon);

    QMenu *buttonMenu(Side side) const;
    void setButtonMenu(Side side, QMenu *menu);

    void setButtonVisible(Side side, bool visible);
    bool isButtonVisible(Side side) const;
    QAbstractButton *button(Side side) const;

    void setButtonToolTip(Side side, const QString &);
    void setButtonFocusPolicy(Side side, Qt::FocusPolicy policy);

    // Set whether tabbing in will trigger the menu.
    void setMenuTabFocusTrigger(Side side, bool v);
    bool hasMenuTabFocusTrigger(Side side) const;

    // Set if icon should be hidden when text is empty
    void setAutoHideButton(Side side, bool h);
    bool hasAutoHideButton(Side side) const;


    // Completion

    // Enable a history completer with a history of entries.
    void setHistoryCompleter(const QString &historyKey, bool restoreLastItemFromHistory = false);
    // Sets a completer that is not a history completer.
    void setSpecialCompleter(QCompleter *completer);


    // Filtering

    // Enables filtering
    void setFiltering(bool on);


    //  Validation

    // line edit, (out)errorMessage -> valid?
    typedef std::function<bool(FancyLineEdit *, QString *)> ValidationFunction;
    enum State { Invalid, DisplayingInitialText, Valid };

    State state() const;
    bool isValid() const;
    QString errorMessage() const;

    QString initialText() const;
    void setInitialText(const QString &);

    QColor errorColor() const;
    void setErrorColor(const  QColor &c);

    QColor okColor() const;
    void setOkColor(const  QColor &c);

    void setValidationFunction(const ValidationFunction &fn);
    static ValidationFunction defaultValidationFunction();
    void validate();
    void onEditingFinished();

protected:
    // Custom behaviour can be added here.
    virtual void handleChanged(const QString &) {}

signals:
    void buttonClicked(Utils::FancyLineEdit::Side side);
    void leftButtonClicked();
    void rightButtonClicked();

    void filterChanged(const QString &);

    void validChanged(bool validState);
    void validReturnPressed();

protected:
    void resizeEvent(QResizeEvent *e);

    virtual QString fixInputString(const QString &string);

private:
    void iconClicked();

    static bool validateWithValidator(FancyLineEdit *edit, QString *errorMessage);
    // Unimplemented, to force the user to make a decision on
    // whether to use setHistoryCompleter() or setSpecialCompleter().
    void setCompleter(QCompleter *);

    void updateMargins();
    void updateButtonPositions();
    friend class FancyLineEditPrivate;

    FancyLineEditPrivate *d;
};

} // namespace Utils
