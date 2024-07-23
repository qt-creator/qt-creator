--- luarocks install luafilesystem
local lfs = require "lfs"

function string:endswith(suffix)
    return self:sub(- #suffix) == suffix
end

function findLUpdate()
    if os.execute("lupdate -version 2>/dev/null") then
        return "lupdate"
    end
    QtDir = os.getenv("QTDIR")
    if QtDir then
        local path = QtDir .. "/bin/lupdate"
        if os.execute(path .. " -version 2>/dev/null") then
            return path
        end
    end
    return "lupdate"
end

LUpdatePath = findLUpdate()
TmpFiles = {}


local curdir, err = lfs.currentdir()
if not curdir then
    print("Error: " .. err)
    return
end

local folderName = curdir:match("([^/]+)$")
print("Working on: " .. curdir)
local pluginSpecName = folderName .. ".lua"

--- Noop tr function
function tr(str) return str end

local specScript, err = loadfile(curdir .. "/" .. pluginSpecName)
if not specScript then
    print("Error: " .. err)
    return
end

local spec, err = specScript()

if not spec then
    print("Error: " .. err)
    return
end

if not spec.languages then
    print("Error: No languages specified in plugin spec.")
    return
end

TrContext = spec.Name:gsub("[^a-zA-Z]", "_")

for file in lfs.dir(".") do
    if file ~= "." and file ~= ".." and file:endswith(".lua") and file ~= "lupdate.lua" then
        local f = io.open(file, "r")
        if f then
            local contents = f:read("a")
            local tmpname = os.tmpname()
            local tf = io.open(tmpname, "w")
            if tf then
                tf:write("--- class " .. TrContext .. " { Q_OBJECT \n")
                tf:write(contents)
                tf:write("--- }\n")
                tf:close()
                table.insert(TmpFiles, tmpname)
            end
        end
    end
end

AllFiles = table.concat(TmpFiles, "\n")
LstFileName = os.tmpname()
local lstFile = io.open(LstFileName, "w")

if lstFile then
    lstFile:write(AllFiles)
    lstFile:close()

    local allLangs = ""
    for _, lang in ipairs(spec.languages) do
        local name = "ts/" .. string.lower(folderName) .. "_" .. lang .. ".ts"
        allLangs = allLangs .. name .. " "
    end

    lfs.mkdir("ts")
    os.execute(LUpdatePath .. " @" .. LstFileName .. " -ts " .. allLangs)

    --- Cleanup
    os.remove(LstFileName)
    for _, file in ipairs(TmpFiles) do
        os.remove(file)
    end
end
