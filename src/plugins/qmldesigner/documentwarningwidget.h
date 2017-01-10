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

#include <utils/faketooltip.h>
#include <documentmessage.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE


namespace QmlDesigner {

class DocumentWarningWidget : public Utils::FakeToolTip
{
    Q_OBJECT
    enum Mode{
        NoMode = -1,
        ErrorMode,
        WarningMode
    };

public:
    explicit DocumentWarningWidget(QWidget *parent);

    void setErrors(const QList<DocumentMessage> &errors);
    void setWarnings(const QList<DocumentMessage> &warnings);

    bool warningsEnabled() const;
    bool gotoCodeWasClicked();
    void emitGotoCodeClicked(const DocumentMessage &message);
signals:
    void gotoCodeClicked(const QString filePath, int codeLine, int codeColumn);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
private:
    void setMessages(const QList<DocumentMessage> &messages);
    void moveToParentCenter();
    void refreshContent();
    QString generateNavigateLinks();
    void ignoreCheckBoxToggled(bool b);

    QLabel *m_headerLabel;
    QLabel *m_messageLabel;
    QLabel *m_navigateLabel;
    QCheckBox *m_ignoreWarningsCheckBox;
    QPushButton *m_continueButton;
    QList<DocumentMessage> m_messages;
    int m_currentMessage = 0;
    Mode m_mode = NoMode;
    bool m_gotoCodeWasClicked = false;
};

} // namespace Designer
