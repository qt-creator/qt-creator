// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <sol/sol.hpp>

namespace Lua::Async {

/*!
    \brief Start an async function.

    To run an async function, you need to provide a callback function that will be called when the
    async function is done. The callback function will receive the return value of the function.

    \code
        local Utils = require("Utils")
        local a = require("async")

        local function foo()
            print("Waiting 1 second ...")
            a.wait(Utils.waitms(1000))
            print("Done")

            return "done"
        end
    \endcode

    \code
        sol::function foo = lua["foo"];
        ::Lua::Async::start(foo, [](const QString &result) {
            // Will be called after one second.
            qDebug() << "Result:" << result;
        });
    \endcode
*/
template<class... Args>
inline void start(sol::function func, std::function<void(Args...)> callback)
{
    sol::state_view lua = func.lua_state();
    sol::protected_function start = lua.script(R"(
local co = coroutine
return function(func, callback)
    assert(type(func) == "function", "type error :: expected func")
    local thread = co.create(func)
    local step = nil
    step = function(...)
        local stat, ret = co.resume(thread, ...)
        if not stat then
            print(ret)
            print(debug.traceback(thread))
        end
        assert(stat, ret)
        if co.status(thread) == "dead" then
            (callback or function() end)(ret)
        else
            assert(type(ret) == "function", "type error :: expected func")
            ret(step)
        end
    end
    step()
end
)");
    start(func, callback);
}
} // namespace Lua::Async
