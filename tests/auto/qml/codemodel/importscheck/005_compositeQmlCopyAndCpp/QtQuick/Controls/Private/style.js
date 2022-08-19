// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

.pragma library

function underlineAmpersands(match, p1, p2, p3) {
    if (p2 === "&")
        return p1.concat(p2, p3)
    return p1.concat("<u>", p2, "</u>", p3)
}

function removeAmpersands(match, p1, p2, p3) {
    return p1.concat(p2, p3)
}

function replaceAmpersands(text, replaceFunction) {
    return text.replace(/([^&]*)&(.)([^&]*)/g, replaceFunction)
}

function stylizeMnemonics(text) {
    return replaceAmpersands(text, underlineAmpersands)
}

function removeMnemonics(text) {
    return replaceAmpersands(text, removeAmpersands)
}
