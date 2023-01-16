// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/mimeutils.h>

#include <QtCore/QVariant>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>

namespace Core::Internal {

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
public:
    explicit MimeTypeMagicDialog(QWidget *parent = nullptr);

    void setMagicData(const MagicData &data);
    MagicData magicData() const;

private:
    void setToRecommendedValues();
    void applyRecommended(bool checked);
    void validateAccept();
    Utils::MimeMagicRule createRule(QString *errorMessage = nullptr) const;

    int m_customRangeStart = 0;
    int m_customRangeEnd = 0;
    int m_customPriority = 50;

    QLineEdit *m_valueLineEdit;
    QComboBox *m_typeSelector;
    QLineEdit *m_maskLineEdit;
    QGroupBox *m_useRecommendedGroupBox;
    QLabel *m_noteLabel;
    QLabel *m_startRangeLabel;
    QLabel *m_endRangeLabel;
    QLabel *m_priorityLabel;
    QSpinBox *m_prioritySpinBox;
    QSpinBox *m_startRangeSpinBox;
    QSpinBox *m_endRangeSpinBox;
};

} // Core::Internal

Q_DECLARE_METATYPE(Core::Internal::MagicData)
