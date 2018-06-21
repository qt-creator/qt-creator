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

#include "../remoteprocess/argumentscollector.h"
#include "directtunnel.h"
#include "forwardtunnel.h"

#include <ssh/sshconnection.h>

#include <QCoreApplication>
#include <QObject>
#include <QStringList>

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    bool parseSuccess;
    QSsh::SshConnectionParameters parameters
        = ArgumentsCollector(app.arguments()).collect(parseSuccess);
    parameters.setHost("127.0.0.1");
    if (!parseSuccess)
        return EXIT_FAILURE;

    DirectTunnel direct(parameters);

    parameters.setHost("127.0.0.2");
    ForwardTunnel forward(parameters);
    forward.run();

    QObject::connect(&forward, &ForwardTunnel::finished, &direct, &DirectTunnel::run);
    QObject::connect(&direct, &DirectTunnel::finished, &app, &QCoreApplication::exit);

    return app.exec();
}
