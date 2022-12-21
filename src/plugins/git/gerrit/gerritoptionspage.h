// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QSharedPointer>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QSpinBox;
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }
namespace Gerrit {
namespace Internal {

class GerritParameters;

class GerritOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GerritOptionsWidget(QWidget *parent = nullptr);

    GerritParameters parameters() const;
    void setParameters(const GerritParameters &);

private:
    QLineEdit *m_hostLineEdit;
    QLineEdit *m_userLineEdit;
    Utils::PathChooser *m_sshChooser;
    Utils::PathChooser *m_curlChooser;
    QSpinBox *m_portSpinBox;
    QCheckBox *m_httpsCheckBox;
};

class GerritOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    GerritOptionsPage(const QSharedPointer<GerritParameters> &p, QObject *parent = nullptr);
    ~GerritOptionsPage() override;

    QWidget *widget() override;
    void apply() override;
    void finish() override;

signals:
    void settingsChanged();

private:
    const QSharedPointer<GerritParameters> &m_parameters;
    QPointer<GerritOptionsWidget> m_widget;
};

} // namespace Internal
} // namespace Gerrit
