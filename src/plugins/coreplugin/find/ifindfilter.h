// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/filesearch.h>

QT_BEGIN_NAMESPACE
class QWidget;
class QKeySequence;
class Pixmap;
QT_END_NAMESPACE

namespace Utils { class QtcSettings; }

namespace Core {

class CORE_EXPORT IFindFilter : public QObject
{
    Q_OBJECT

public:
    IFindFilter();
    ~IFindFilter() override;

    static const QList<IFindFilter *> allFindFilters();

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    ///
    virtual bool isEnabled() const = 0;
    virtual bool isValid() const { return true; }
    virtual QKeySequence defaultShortcut() const;
    virtual bool isReplaceSupported() const { return false; }
    virtual bool showSearchTermInput() const { return true; }
    virtual Utils::FindFlags supportedFindFlags() const;

    virtual void findAll(const QString &txt, Utils::FindFlags findFlags) = 0;
    virtual void replaceAll(const QString &txt, Utils::FindFlags findFlags)
    { Q_UNUSED(txt) Q_UNUSED(findFlags) }

    virtual QWidget *createConfigWidget() { return nullptr; }
    virtual void writeSettings(Utils::QtcSettings *settings) { Q_UNUSED(settings) }
    virtual void readSettings(Utils::QtcSettings *settings) { Q_UNUSED(settings) }

    static QPixmap pixmapForFindFlags(Utils::FindFlags flags);
    static QString descriptionForFindFlags(Utils::FindFlags flags);
signals:
    void enabledChanged(bool enabled);
    void validChanged(bool valid);
    void displayNameChanged();
};

} // namespace Core
