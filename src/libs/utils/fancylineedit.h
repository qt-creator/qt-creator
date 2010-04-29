/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FANCYLINEEDIT_H
#define FANCYLINEEDIT_H

#include "utils_global.h"

#include <QtGui/QLineEdit>
#include <QtGui/QPaintEvent>
#include <QtGui/QAbstractButton>

namespace Utils {

class FancyLineEditPrivate;

class IconButton: public QAbstractButton
{
    Q_OBJECT
    Q_PROPERTY(float iconOpacity READ iconOpacity WRITE setIconOpacity)
public:
    IconButton(QWidget *parent = 0);
    void paintEvent(QPaintEvent *event);
    float iconOpacity() { return m_iconOpacity; }
    void setIconOpacity(float value) { m_iconOpacity = value; update(); }
    void animateShow(bool visible);

private:
    float m_iconOpacity;
};


/* A line edit with an embedded pixmap on one side that is connected to
 * a menu. Additionally, it can display a grayed hintText (like "Type Here to")
 * when not focused and empty. When connecting to the changed signals and
 * querying text, one has to be aware that the text is set to that hint
 * text if isShowingHintText() returns true (that is, does not contain
 * valid user input).
 */
class QTCREATOR_UTILS_EXPORT FancyLineEdit : public QLineEdit
{
    Q_DISABLE_COPY(FancyLineEdit)
    Q_OBJECT
    Q_ENUMS(Side)
    Q_PROPERTY(QPixmap pixmap READ pixmap WRITE setPixmap DESIGNABLE true)
    Q_PROPERTY(Side side READ side WRITE setSide DESIGNABLE true)
    Q_PROPERTY(bool menuTabFocusTrigger READ hasMenuTabFocusTrigger WRITE setMenuTabFocusTrigger  DESIGNABLE true)
    Q_PROPERTY(bool autoHideIcon READ autoHideIcon WRITE setAutoHideIcon DESIGNABLE true)

public:
    enum Side {Left, Right};

    explicit FancyLineEdit(QWidget *parent = 0);
    ~FancyLineEdit();

    QPixmap pixmap() const;

    void setMenu(QMenu *menu);
    QMenu *menu() const;

    void setSide(Side side);
    Side side() const;

    void setButtonToolTip(const QString &);
    void setButtonFocusPolicy(Qt::FocusPolicy policy);

    // Set whether tabbing in will trigger the menu.
    bool hasMenuTabFocusTrigger() const;
    void setMenuTabFocusTrigger(bool v);

    // Set if icon should be hidden when text is empty
    bool autoHideIcon() const;
    void setAutoHideIcon(bool h);

signals:
    void buttonClicked();

public slots:
    void setPixmap(const QPixmap &pixmap);
    void checkButton(const QString &);
    void iconClicked();

protected:
    virtual void resizeEvent(QResizeEvent *e);

private:
    friend class Utils::FancyLineEditPrivate;
    bool isSideStored() const;

    FancyLineEditPrivate *m_d;
    QString m_oldText;
};

} // namespace Utils

#endif // FANCYLINEEDIT_H
