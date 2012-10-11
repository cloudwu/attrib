local c = require "attrib.c"
local attrib = {}

local function extract(s, set)
	local lv,rv = string.match(s,"%s*([^%s=]+)%s*=(.+)")
	assert(lv and rv)
	set[lv] = true
	for opt, value in string.gmatch(rv,"%s*([()+*/%-%^]*)%s*([^%s()+*/%-%^]*)") do
		if value and value ~="" and string.match(value,"([%d%.]+)") ~= value then
			set[value] = true
		end
	end
end

local function convert(s , set)
	local lv,rv = string.match(s,"%s*([^%s=]+)%s*=(.+)")
	assert(lv and rv)
	local reg = set[lv]
	local tmp = {}
	for opt, value in string.gmatch(rv,"%s*([()+*/%-%^]*)%s*([^%s()+*/%-%^]*)") do
		if opt then
			table.insert(tmp, opt)
		end
		if value and value ~="" then
			local r = set[value]
			if r then
				table.insert(tmp, "R" .. r)
			else
				table.insert(tmp, value)
			end
		end
	end
	return reg, table.concat(tmp)
end

function attrib.expression(t)
	local set = {}
	for _, v in ipairs(t) do
		extract(v,set)
	end
	local n = 0
	for k,v in pairs(set) do
		set[k] = n
		n = n+1
	end

	local tmp = {}
	for _,v in ipairs(t) do
		local r,s = convert(v,set)
		table.insert(tmp, r)
		table.insert(tmp, s)
	end

	return { __cobj = c.expression(tmp) , __map = set }
end

local function _next(t,prev)
	local k,idx = next(t.__map, prev)
	if k then
		return k, t.__cobj[idx]
	end
end

local meta = {
	__index = function(t,key)
		return t.__cobj[t.__map[key]]
	end,
	__newindex = function(t,key, value)
		t.__cobj[t.__map[key]] = value
	end,
	__call = function(t, key, value)
		return t.__cobj(t.__map[key], value)
	end,
	__pairs = function(t) return _next, t end,
}

function attrib.new(e)
	local obj = { __cobj = c.attrib(e.__cobj) , __map = e.__map,  }
	return setmetatable(obj, meta)
end

return attrib