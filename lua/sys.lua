#!/usr/bin/lua

local ubus = require "ubus"
local uloop = require "uloop"


uloop.init()

local conn = ubus.connect()
if not conn then
        error("Failed to connect to ubus")
end

local command = {}
local worker_info = {}

worker_info['name'] = "sysapi";
worker_info['key'] = "sys";
worker_info['path'] = "sys_api";
worker_info['method'] = "sys_status";

print(worker_info['name'])


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
if ret then
	for k , v in pairs(ret)
	do
		print(k..":"..v)
	end
end
conn:add(sysapi)

local ttimer
local test_timer = function ()
	print("hello man!")
	ttimer:set(2000)
end

ttimer = uloop.timer(test_timer,2000)

local ctimer 
local check_timer = function()
	local res = conn:call("master","check",{key=worker_info['key'],path=worker_info['path'],method=worker_info['method']})

	if res and (not res['result'] or res['result'] == false) then
		print("sysapi is disconnect from master,need re-register")
		local ret  = conn:call("master","register",{id=1,name="sysapi",key="sys",path="sys_api",method="sys_status"})
	end
	
	ctimer:set(5000)
end

ctimer = uloop.timer(check_timer,5000);

uloop.run()
