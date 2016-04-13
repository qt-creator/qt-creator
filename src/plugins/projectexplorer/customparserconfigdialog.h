/****************************************************************************
**
** Copyright (C) 2016 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#include "customparser.h"

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLineEdit;
QT_END_NAMESPACE

namespace ProjectExplorer {
namespace Internal {

namespace Ui { class CustomParserConfigDialog; }

class CustomParserConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomParserConfigDialog(QDialog *parent = nullptr);
    ~CustomParserConfigDialog() override;

    void setExampleSettings();
    void setSettings(const CustomParserSettings &settings);
    CustomParserSettings settings() const;
    void setErrorPattern(const QString &errorPattern);
    QString errorPattern() const;
    void setErrorFileNameCap(int errorFileNameCap);
    int errorFileNameCap() const;
    void setErrorLineNumberCap(int errorLineNumberCap);
    int errorLineNumberCap() const;
    void setErrorMessageCap(int errorMessageCap);
    int errorMessageCap() const;
    void setErrorChannel(CustomParserExpression::CustomParserChannel errorChannel);
    CustomParserExpression::CustomParserChannel errorChannel() const;
    void setErrorExample(const QString &errorExample);
    QString errorExample() const;
    void setWarningPattern(const QString &warningPattern);
    QString warningPattern() const;
    void setWarningFileNameCap(int warningFileNameCap);
    int warningFileNameCap() const;
    void setWarningLineNumberCap(int warningLineNumberCap);
    int warningLineNumberCap() const;
    void setWarningMessageCap(int warningMessageCap);
    int warningMessageCap() const;
    void setWarningChannel(CustomParserExpression::CustomParserChannel warningChannel);
    CustomParserExpression::CustomParserChannel warningChannel() const;
    void setWarningExample(const QString &warningExample);
    QString warningExample() const;

    bool isDirty() const;

private:
    void changed();

    bool checkPattern(QLineEdit *pattern, const QString &outputText,
                      QString *errorMessage, QRegularExpressionMatch *match);

    Ui::CustomParserConfigDialog *ui;
    bool m_dirty;
};

} // namespace Internal
} // namespace ProjectExplorer
