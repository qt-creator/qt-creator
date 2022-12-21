// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "core_global.h"

#include <QPointer>
#include <QWidget>

namespace Core {
namespace Internal { class FindToolBar; }

class CORE_EXPORT FindToolBarPlaceHolder : public QWidget
{
    Q_OBJECT
public:
    explicit FindToolBarPlaceHolder(QWidget *owner, QWidget *parent = nullptr);
    ~FindToolBarPlaceHolder() override;

    static const QList<FindToolBarPlaceHolder *> allFindToolbarPlaceHolders();

    QWidget *owner() const;
    bool isUsedByWidget(QWidget *widget);

    void setWidget(Internal::FindToolBar *widget);

    static FindToolBarPlaceHolder *getCurrent();
    static void setCurrent(FindToolBarPlaceHolder *placeHolder);

    void setLightColored(bool lightColored);
    bool isLightColored() const;

private:
    QWidget *m_owner;
    QPointer<Internal::FindToolBar> m_subWidget;
    bool m_lightColored = false;
    static FindToolBarPlaceHolder *m_current;
};

} // namespace Core
