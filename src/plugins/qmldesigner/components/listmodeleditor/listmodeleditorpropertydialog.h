// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <nodeinstanceglobal.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace QmlDesigner {

class ListModelEditorPropertyDialog : public QDialog
{
    Q_OBJECT

public:
    ListModelEditorPropertyDialog(const PropertyNameList &existingProperties);
    QString propertyName() const;

private:
    void secondaryValidation(const QString &text);
    void setError(const QString &errorString);

    const PropertyNameList m_existingProperties;
    QLineEdit *m_lineEdit = nullptr;
    QLabel *m_errorLabel = nullptr;
    QPushButton *m_okButton = nullptr;
};

} // namespace QmlDesigner
