// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <QScrollArea>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QLineEdit;
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer {

class ToolChain;

// --------------------------------------------------------------------------
// ToolChainConfigWidget
// --------------------------------------------------------------------------

class PROJECTEXPLORER_EXPORT ToolChainConfigWidget : public QScrollArea
{
    Q_OBJECT

public:
    explicit ToolChainConfigWidget(ToolChain *tc);

    ToolChain *toolChain() const;

    void apply();
    void discard();
    bool isDirty() const;
    void makeReadOnly();

signals:
    void dirty();

protected:
    void setErrorMessage(const QString &);
    void clearErrorMessage();

    virtual void applyImpl() = 0;
    virtual void discardImpl() = 0;
    virtual bool isDirtyImpl() const = 0;
    virtual void makeReadOnlyImpl() = 0;

    void addErrorLabel();
    static QStringList splitString(const QString &s);
    QFormLayout *m_mainLayout;
    QLineEdit *m_nameLineEdit;

private:
    ToolChain *m_toolChain;
    QLabel *m_errorLabel = nullptr;
};

} // namespace ProjectExplorer
