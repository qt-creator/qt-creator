// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QRadioButton;
class QTextStream;
QT_END_NAMESPACE

namespace ClearCase::Internal {

class VersionSelector : public QDialog
{
    Q_OBJECT

public:
    explicit VersionSelector(const QString &fileName, const QString &message,
                             QWidget *parent = nullptr);
    ~VersionSelector() override;
    bool isUpdate() const;

private:
    bool readValues();

    QTextStream *m_stream;
    QString m_versionID, m_createdBy, m_createdOn, m_message;

    QRadioButton *m_updatedRadioButton;
};

} // ClearCase::Internal
