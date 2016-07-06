libs:= -lubox -lubus -lmosquitto
bin:=device_manager

$(bin): master.o mqtt_msg.o worker.o mqtt_worker.o config.o
	#cc -o $(bin) master.c mqtt_msg.c worker.c mqtt_worker.c $(libs)
	cc -o $(bin) *.o $(libs)


clean:
	rm *.o $(bin)
