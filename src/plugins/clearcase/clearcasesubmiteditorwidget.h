// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/submiteditorwidget.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ClearCase::Internal {

class ActivitySelector;

class ClearCaseSubmitEditorWidget : public VcsBase::SubmitEditorWidget
{
    Q_OBJECT

public:
    ClearCaseSubmitEditorWidget();

    QString activity() const;
    bool isIdentical() const;
    bool isPreserve() const;
    void setActivity(const QString &act);
    bool activityChanged() const;
    void addKeep();
    void addActivitySelector(bool isUcm);

protected:
    QString commitName() const override;

private:
    ActivitySelector *m_actSelector = nullptr;
    QCheckBox *m_chkIdentical;
    QCheckBox *m_chkPTime;
    QVBoxLayout *m_verticalLayout;
};

} // ClearCase::Internal
