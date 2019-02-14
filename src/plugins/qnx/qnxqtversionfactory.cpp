/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxqtversionfactory.h"

#include "qnxconstants.h"
#include "qnxqtversion.h"

#include <qtsupport/profilereader.h>

using namespace Qnx;
using namespace Qnx::Internal;

QnxQtVersionFactory::QnxQtVersionFactory()
{
    setQtVersionCreator([] { return new QnxQtVersion; });
    setSupportedType(Constants::QNX_QNX_QT);
    setPriority(50);
}

QtSupport::BaseQtVersion *QnxQtVersionFactory::create(const Utils::FileName &qmakePath,
                                                      ProFileEvaluator *evaluator)
{
    if (evaluator->contains(QLatin1String("QNX_CPUDIR"))) {
        return new QnxQtVersion(qmakePath);
    }

    return nullptr;
}
