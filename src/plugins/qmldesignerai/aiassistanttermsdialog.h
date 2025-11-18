// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class AiAssistantTermsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AiAssistantTermsDialog(QWidget *parent = nullptr);

private:
    void saveTerms();

    QPointer<QCheckBox> m_agreeCheckbox;
    QPointer<QTextEdit> m_termsText;
    QPointer<QPushButton> m_saveButton;
    QPointer<QPushButton> m_acceptButton;
    QPointer<QPushButton> m_rejectButton;
};

} // namespace QmlDesigner
