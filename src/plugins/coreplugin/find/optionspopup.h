// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/core_global.h>

#include <utils/id.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT OptionsPopup : public QWidget
{
    Q_OBJECT

public:
    OptionsPopup(QWidget *parent, const QVector<Utils::Id> &commands);

protected:
    bool event(QEvent *ev) override;
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QCheckBox *createCheckboxForCommand(Utils::Id id);
};

} // namespace Core
