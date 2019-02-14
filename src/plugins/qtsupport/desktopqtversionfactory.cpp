/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
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

#include "desktopqtversionfactory.h"
#include "desktopqtversion.h"
#include <qtsupport/qtsupportconstants.h>

using namespace QtSupport;
using namespace QtSupport::Internal;

DesktopQtVersionFactory::DesktopQtVersionFactory()
{
    setQtVersionCreator([] { return new DesktopQtVersion; });
    setSupportedType(Constants::DESKTOPQT);
    setPriority(0); // Lowest of all, we want to be the fallback
}

BaseQtVersion *DesktopQtVersionFactory::create(const Utils::FileName &qmakePath, ProFileEvaluator *evaluator)
{
    Q_UNUSED(evaluator);
    // we are the fallback :) so we don't care what kind of qt it is
    return new DesktopQtVersion(qmakePath);
}
