-- Copyright (c) 2023 Chris Antos
-- SPDX-License-Identifier: MIT

-- luacheck: globals vscode_shell_integration NONL
if vscode_shell_integration == nil then
    vscode_shell_integration = true
end

if not vscode_shell_integration then
    return
end

local function is_vscode()
    local term_program = os.getenv("term_program") or ""
    if term_program:lower() == "vscode" then
        return true
    end
end

local function send_context()
    if is_vscode() then
        local codes = ""
        codes = codes .. "\027]633;D;" .. os.geterrorlevel() .. "\a" -- send command exit code
        codes = codes .. "\027]633;P;Cwd=" .. os.getcwd() .. "\a" -- send cwd as title
        clink.print(codes, NONL)
    end
end

local p = clink.promptfilter(-999)

function p:filter() -- luacheck: no unused
    -- Nothing to do here, but the filter function must be defined.
end

function p:surround() -- luacheck: no unused
    if is_vscode() then
        local pre, suf
        local rpre, rsuf

        -- ESC codes surrounding prompt string
        pre = "\027]633;A\a" -- copied from shellIntegration-rc.zsh
        suf = "\027]633;B\a" -- copied from shellIntegration-rc.zsh

        -- ESC codes surrounding right side prompt string
        rpre = "\027]633;H\a" -- copied from shellIntegration-rc.zsh
        rsuf = "\027]633;I\a" -- copied from shellIntegration-rc.zsh

        return pre, suf, rpre, rsuf
    end
end

clink.onbeginedit(send_context)
