// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../mcusupport_global.h"
#include "../settingshandler.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
QT_END_NAMESPACE

namespace McuSupport::Internal {

class McuKitCreationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit McuKitCreationDialog(const MessagesList &messages,
                                  const SettingsHandler::Ptr &settingsHandler,
                                  McuPackagePtr qtMCUPackage,
                                  QWidget *parent = nullptr);

private slots:
    void updateMessage(const int inc);

private:
    int m_currentIndex = -1;
    QLabel *m_iconLabel;
    QLabel *m_textLabel;
    QLabel *m_informationLabel;
    QLabel *m_qtMCUsPathLabel;
    QLabel *m_messageCountLabel;
    QPushButton *m_previousButton;
    QPushButton *m_nextButton;
    const MessagesList &m_messages;
};
} // namespace McuSupport::Internal
