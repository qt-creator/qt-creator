// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/** Absolute path of the directory holding the WebAssembly application. */
export declare const directory: string;

/** Absolute paths of the files a page needs to load the application. */
export declare const files: {
    /** Qt's loader, which defines qtLoad(). */
    loader: string;
    /** The Emscripten module factory, loaded before calling qtLoad(). */
    script: string;
    /** The WebAssembly binary. */
    wasm: string;
    /** Qt's generic HTML shell, a working example of the above. */
    html: string;
};
