#!/usr/bin/lua

local ubus = require "ubus"
local uloop = require "uloop"


uloop.init()

local conn = ubus.connect()
if not conn then
        error("Failed to connect to ubus")
end

local command = {}

local sysapi = {
			sys_api = {
				sys_status = {
					function (req,msg)
						if msg['msg'] then
							print(msg['msg']);
							local mac = "aa"
							--local interfaces = {[1]={ip="192.168.10.1"},[2]={ip="192.168.20.1"}}
							local interfaces = {{name="lan1",ip="192.168.10.1"},{name="wan1",ip="192.168.20.1"}}
							print("aaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
							--local interfaces = {}
							--interfaces[1] = {ip="192.168.10.1"}
							--interfaces[2] = {ip="192.168.20.1"}
							--conn:reply(req,{mac = mac,interfaces = interfaces} )
							conn:reply(req,{mac = mac,interfaces = "aaa"} )
						end
					end
					,{id = ubus.INT32,msg = ubus.STRING }

				}

			}
}
local ret  = conn:call("master","register",{id=1,name="sysapi",key="sys",path="sys_api",method="sys_status"})
for k , v in pairs(ret)
do
	print(k..":"..v)
end
conn:add(sysapi)
local ttimer
local test_timer = function ()
	print("hello man!")
	ttimer:set(2000)
end

ttimer = uloop.timer(test_timer,2000)
uloop.run()
