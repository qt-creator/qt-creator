/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Axivion Add-on.
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

#pragma once

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Axivion::Internal {

class AxivionSettings
{
public:
    AxivionSettings();
    void toSettings(QSettings *s) const;
    void fromSettings(QSettings *s);
};

} // namespace Axivion::Internal
