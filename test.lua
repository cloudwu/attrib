local attrib = require "attrib"

local e = attrib.expression {
	"攻击 = 力量 * 10",
	"防御 = (攻击 - 3) * 力量",
}

local a = attrib.new(e)

a["力量"] = 3

for k,v in pairs(a) do
	print(k,v)
end

a("力量",1)

for k,v in pairs(a) do
	print(k,v)
end
