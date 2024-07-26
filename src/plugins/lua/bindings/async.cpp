// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../luaengine.h"

namespace Lua::Internal {

static const char *async_source = R"(
-- From: https://github.com/ms-jpq/lua-async-await
-- Licensed under MIT
local co = coroutine
-- use with wrap
local pong = function(func, callback)
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
-- use with pong, creates thunk factory
local wrap = function(func)
    assert(type(func) == "function", "type error :: expected func")
    local factory = function(...)
        local params = { ... }
        local thunk = function(step)
            table.insert(params, step)
            return func(table.unpack(params))
        end
        return thunk
    end
    return factory
end
-- many thunks -> single thunk
local join = function(thunks)
    local len = #thunks
    local done = 0
    local acc = {}

    local thunk = function(step)
        if len == 0 then
            return step()
        end
        for i, tk in ipairs(thunks) do
            assert(type(tk) == "function", "thunk must be function")
            local callback = function(...)
                acc[i] = ...
                done = done + 1
                if done == len then
                    step(acc)
                end
            end
            tk(callback)
        end
    end
    return thunk
end
-- sugar over coroutine
local await = function(defer)
    local _, isMain = coroutine.running()
    assert(not isMain, "a.wait was called outside of a running coroutine. You need to start one using a.sync(my_function)() first")
    assert(type(defer) == "function", "type error :: expected func :: was: " .. type(defer))
    return co.yield(defer)
end
local await_all = function(defer)
    local _, isMain = coroutine.running()
    assert(not isMain, "a.wait_all was called outside of a running coroutine. You need to start one using a.sync(my_function)() first")
    assert(type(defer) == "table", "type error :: expected table")
    return co.yield(join(defer))
end
return {
    sync = wrap(pong),
    wait = await,
    wait_all = await_all,
    wrap = wrap,
}
)";

void setupAsyncModule()
{
    registerProvider("async", [](sol::state_view lua) -> sol::object {
        sol::protected_function_result res = lua.script(async_source, "async.cpp");
        return res.get<sol::table>(0);
    });
}

} // namespace Lua::Internal
