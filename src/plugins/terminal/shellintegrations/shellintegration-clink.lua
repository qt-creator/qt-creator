-- Copyright (c) 2023 Chris Antos
-- SPDX-License-Identifier: MIT

-- luacheck: globals gizmo_vscode_shell_integration NONL
if gizmo_vscode_shell_integration == nil then
    gizmo_vscode_shell_integration = true
end

if not gizmo_vscode_shell_integration then
    return
end

-- VSCode shell integration escape codes are described in:
-- https://code.visualstudio.com/docs/terminal/shell-integration

local function is_vscode()
    local term_program = os.getenv("term_program") or ""
    if term_program:lower() == "vscode" then
        return true
    end
end

local function escape_command_line(line)
    line = line:gsub("\\", "\\\\")
    line = line:gsub(" ", "\\x20")
    if line:find("[\x01-\x1f;]") then
        line = line:gsub(";", "\\x3b")
        for i = 0x01, 0x1f, 1 do
            line = line:gsub(string.char(i), string.format("\\x%02x", i))
        end
    end
    return line
end

local function begin_execution(line)
    if is_vscode() then
        local codes = ""

        -- Mark pre-execution.
        codes = codes .. "\027]633;C\a"

        -- Explicitly set the command line.
        codes = codes .. "\027]633;E;" .. escape_command_line(line) .. "\a"

        clink.print(codes, NONL)
    end
end

local function finish_execution()
    if is_vscode() then
        local codes = ""

        -- Mark execution finished with exit code.
        codes = codes .. "\027]633;D"
        if os.geterrorlevel and settings.get("cmd.get_errorlevel") then
            codes = codes .. ";" .. os.geterrorlevel()
        end
        codes = codes .. "\a"

        -- Set properties.
        codes = codes .. "\027]633;P;Cwd=" .. os.getcwd() .. "\a"
        codes = codes .. "\027]633;P;IsWindows=True\a"

        clink.print(codes, NONL)
    end
end

local p = clink.promptfilter(-999)

function p:filter() -- luacheck: no unused
    -- Nothing to do here, but the filter function must be defined.
end

function p:transientfilter() -- luacheck: no unused
    if (clink.version_encoded or 0) < 10040025 then
        -- Disabling transient prompt requires v1.4.25 or higher.
        return
    end
    if is_vscode() then
        -- VSCode gets confused about prompt position and annotations when
        -- transient prompt is used, so disable it in VSCode terminal windows.
        return nil, false
    end
end

function p:surround() -- luacheck: no unused
    if is_vscode() then
        local pre, suf
        local rpre, rsuf

        -- Mark prompt start and end.
        pre = "\027]633;A\a"
        suf = "\027]633;B\a"

        -- Mark right side prompt start and end.
        rpre = "\027]633;H\a"
        rsuf = "\027]633;I\a"

        return pre, suf, rpre, rsuf
    end
end

clink.onbeginedit(finish_execution)
clink.onendedit(begin_execution)
