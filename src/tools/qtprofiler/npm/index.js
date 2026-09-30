// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

'use strict';

const path = require('path');

const directory = path.join(__dirname, 'dist');

module.exports = {
    directory,
    files: {
        loader: path.join(directory, 'qtloader.js'),
        script: path.join(directory, 'qtprofiler.js'),
        wasm: path.join(directory, 'qtprofiler.wasm'),
        html: path.join(directory, 'qtprofiler.html'),
    },
};
