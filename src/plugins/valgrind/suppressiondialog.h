// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlprotocol/error.h"

#include <utils/pathchooser.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Valgrind {
namespace Internal {

class MemcheckErrorView;
class ValgrindBaseSettings;

class SuppressionDialog : public QDialog
{
public:
    SuppressionDialog(MemcheckErrorView *view,
                      const QList<XmlProtocol::Error> &errors);
    static void maybeShow(MemcheckErrorView *view);

private:
    void validate();
    void accept() override;
    void reject() override;

    MemcheckErrorView *m_view;
    ValgrindBaseSettings *m_settings;
    bool m_cleanupIfCanceled;
    QList<XmlProtocol::Error> m_errors;

    Utils::PathChooser *m_fileChooser;
    QPlainTextEdit *m_suppressionEdit;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Valgrind
