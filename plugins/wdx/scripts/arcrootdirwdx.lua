local cached_file = nil
local root_dirs_count = 0
local root_files_count = 0
local z7cmd = '7z'

function ContentSetDefaultParams(IniFileName, PlugApiVerHi, PlugApiVerLow)
  z7cmd = get_output("sh -c 'which 7zz || which 7z' 2>/dev/null"):sub(1, -2)
end

function ContentGetDetectString()
    return 'EXT="7Z" | EXT="ZIP" | EXT="RAR" | EXT="XZ" | EXT="GZ" | EXT="Z" | EXT="LZMA" | EXT="BZ2" | EXT="TAR" | EXT="ZIPX" | EXT="ZST"'
end

function ContentGetSupportedField(FieldIndex)
    if FieldIndex == 0 then
        return "root dir", '', 6
    elseif FieldIndex == 1 then
        return "root dirs count ", '', 2
    elseif FieldIndex == 2 then
        return "root files count", '', 2
    end
    return '', '', 0 -- ft_nomorefields
end

function ContentGetValue(FileName, FieldIndex, UnitIndex, flags)
    if FieldIndex > 2 then
        return nil
    end
    if FileName ~= cached_file then
        local command = z7cmd .. " l -slt " .. FileName:gsub(' ', '\\ ') .. " -p 2>/dev/null"
        local prefix = "^Path = "
        if FileName:find("%.tar%.?") then
            command = "tar -tf " .. FileName:gsub(' ', '\\ ') .. " 2>/dev/null"
            prefix = '^'
        end
        local output = get_output(command)
        print(command, output)
        if output == '' or output == nil then
            return nil
        end
        local root_dirs = {}
        local root_files = {}
        for line in output:gmatch("([^\n]*)\n?") do
            local dir = line:match(prefix .. "([^/\\]+)[/\\].*$")
            if not dir then
                local file = line:match(prefix .. "([^/\\]+)$")
                if file ~= nil and not root_dirs[file] then
                    root_files[file] = true
                end
            else
                if not root_dirs[dir] then
                    root_dirs[dir] = true
                    root_files[dir] = nil
                end
            end
        end
        root_dirs_count = 0
        for _ in pairs(root_dirs) do
            root_dirs_count = root_dirs_count + 1
        end
        root_files_count = 0
        for _ in pairs(root_files) do
            root_files_count = root_files_count + 1
        end
        cached_file = FileName
    end
    if FieldIndex == 0 then
        if root_dirs_count == 1 and root_files_count == 0 then
            return true
        else
            return false
        end
    elseif FieldIndex == 1 then
        return root_dirs_count
    elseif FieldIndex == 2 then
        return root_files_count
    end
    return nil
end

function get_output(command)
  if not command then
    return ''
  end
  local handle = io.popen(command, 'r')
  local output = handle:read("*a")
  handle:close()
  return output
end