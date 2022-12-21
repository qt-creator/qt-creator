// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
