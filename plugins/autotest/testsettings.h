/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTSETTINGS_H
#define TESTSETTINGS_H

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Autotest {
namespace Internal {

enum MetricsType {
    Walltime,
    TickCounter,
    EventCounter,
    CallGrind,
    Perf
};

struct TestSettings
{
    TestSettings();
    void toSettings(QSettings *s) const;
    void fromSettings(const QSettings *s);
    bool equals(const TestSettings &rhs) const;
    static QString metricsTypeToOption(const MetricsType type);

    int timeout;
    MetricsType metrics;
    bool omitInternalMssg;
    bool omitRunConfigWarn;
};

inline bool operator==(const TestSettings &s1, const TestSettings &s2) { return s1.equals(s2); }
inline bool operator!=(const TestSettings &s1, const TestSettings &s2) { return !s1.equals(s2); }

} // namespace Internal
} // namespace Autotest

#endif // TESTSETTINGS_H
