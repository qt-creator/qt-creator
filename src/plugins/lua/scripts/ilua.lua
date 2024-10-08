local a = require('async')
local inspect = require('inspect')

local prompt = '> '

local function errHandler(err)
    return debug.traceback(err, 2)
end

---Returns the number of arguments and a table with the arguments
---We need this because select('#', ...) is the only way to figure out the number of arguments
---because #table will not count nil values
---@return integer nArguments The number of arguments provided to wrap(...)
---@return table arguments The arguments packed a table
local function wrap(...) return select('#', ...), { ... } end

local function eval(code)
    if #code == 0 then
        return
    end

    --We load the code once as is, and once by adding a return statement in front of it
    local asFunc, errFunc = load(code)
    local asReturn = load('return ' .. code)
    --If the code with return statement did not compile we will use the code without it
    if not asReturn then
        --If the code without the return did not compile either we print an error
        if not asFunc then
            print("__ERROR__" .. errFunc)
            return
        end
        asReturn = asFunc
    end

    --We call the compiled code in protected mode, which will capture and errors thrown by it.
    local numberOfReturnValues, result = wrap(xpcall(asReturn, errHandler))

    --result[1] contains true or false depending on whether the function ran successfully
    if not result[1] then
        print("__ERROR__" .. result[2])
    --numberOfReturnValues is the real number of values returned from xpcall
    elseif numberOfReturnValues > 1 then
        --We concatenate all the return values into a single string
        local str = ""
        --Skip the first value which is the boolean from xpcall
        for i = 2, numberOfReturnValues do
            str = str .. inspect(result[i]) .. '\t'
        end
        print(str)
    end
end

print(LuaCopyright)

--Main Loop
a.sync(function()
    repeat
        local input = a.wait(readline(prompt))
        eval(input)
    until false
end)()
