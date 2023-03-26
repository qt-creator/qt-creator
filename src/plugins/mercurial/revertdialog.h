// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace Mercurial::Internal {

class RevertDialog : public QDialog
{
public:
    RevertDialog(QWidget *parent = nullptr);
    ~RevertDialog() override;

    QString revision() const;

private:
    QLineEdit *m_revisionLineEdit;
};

} // Mercurial::Internal
