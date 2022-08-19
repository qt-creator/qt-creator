// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "ui_mimetypemagicdialog.h"

#include <utils/mimeutils.h>

namespace Core {
namespace Internal {

class MagicData
{
public:
    MagicData()
        : m_rule(Utils::MimeMagicRule::String, QByteArray(" "), 0, 0)
    {
    }

    MagicData(Utils::MimeMagicRule rule, int priority)
        : m_rule(rule)
        , m_priority(priority)
    {
    }

    bool operator==(const MagicData &other) const;
    bool operator!=(const MagicData &other) { return !(*this == other); }

    static QByteArray normalizedMask(const Utils::MimeMagicRule &rule);

    Utils::MimeMagicRule m_rule;
    int m_priority = 0;
};

class MimeTypeMagicDialog : public QDialog
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::MimeTypeMagicDialog)
public:
    explicit MimeTypeMagicDialog(QWidget *parent = nullptr);

    void setMagicData(const MagicData &data);
    MagicData magicData() const;

private:
    void setToRecommendedValues();
    void applyRecommended(bool checked);
    void validateAccept();
    Utils::MimeMagicRule createRule(QString *errorMessage = nullptr) const;

    Ui::MimeTypeMagicDialog ui;
    int m_customRangeStart = 0;
    int m_customRangeEnd = 0;
    int m_customPriority = 50;
};

} // Internal
} // Core

Q_DECLARE_METATYPE(Core::Internal::MagicData)
