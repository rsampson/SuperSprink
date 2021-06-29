#!/usr/bin/python3
#  supersprink.py
#  
#  Copyright 2020 r. sampson
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
import weatherkey as wk
import yaml
import paho.mqtt.client as mqtt
import time
import logging
import arrow  #time and date functions
import json
import requests
import sys

def configLog():
	'''log time date into sprinkler.log'''
	logging.basicConfig(filename='sprinkler.log',
						format='%(asctime)s %(levelname)-8s %(message)s',
						level=logging.INFO,
						datefmt='%Y-%m-%d %H:%M:%S')

def openConfig():
	'''configure the valves from file settings in yaml format'''
	with open("config.yaml", "r") as yamlfile:
		logging.info('Config read successful')
		return(yaml.full_load(yamlfile))

# access returned data structure "config", example: config['valves'][1]['runtime']['Aug'] ras

def onconnect(client, userdata, flags, rc):
	'''mqtt broker connection callback'''
	if rc==0:
		client.connected_flag=True
	else:
		client.connected_flag=False

def startBroker():
	'''communicate to controller box with mqtt'''
	mqttBroker = "spigot.local"
	client = mqtt.Client("Sprinkler_valve")
	client.on_connect=onconnect  #bind call back function
	client.connect(mqttBroker)
	client.loop_start()  #Start loop needed for call backs
	time.sleep(4) # Wait for connection setup to complete
	if (client.connected_flag == True):
		logging.info('Connected to mqtt broker')
	else:
		logging.info('Mqtt broker connection failed')
	return (client)

def openValve(client, valvenum=0, timeopen=10):
	'''open a given valve for a specified # of seconds, and log these events'''
	logging.info('Opening valve # %s for %s minutes', valvenum, timeopen)
	client.publish("inTopic", valvenum)
	time.sleep(timeopen * 60)
	logging.info('Closing all valves')
	client.publish("inTopic", 0)
	time.sleep(1) # repeat shut off for good measure
	client.publish("inTopic", 0)
	
def quitBroker(client):
	client.loop_stop()
	client.disconnect()
	logging.info('Quit broker')
	
def willItRain():
	'''using weatherapi.com to retrieve weather data''' 
	response = requests.get("http://api.weatherapi.com/v1/forecast.json?key= " + wk.WEATHERKEY + "&q=carlsbad&days=1")
	if response.status_code == 200:
		json_string = response.content.decode('utf-8')
		weather_data = json.loads(json_string)
		precip_in = weather_data['current']['precip_in']
		will_it_rain = weather_data['forecast']['forecastday'][0]['day']["daily_will_it_rain"]
		return( (precip_in > .25) and (will_it_rain == 1))
	else:
		loggin.info('failed to get weather')
		return(False)
        
def runValves(data, month = "Jan"):
	client = startBroker()
	for v in data['valves']:
		logging.info(v['alias'])
		openValve(client, v['valve_number'], v['runtime'][month] )
	quitBroker(client)
	
def operateValves(data):
	'''run sprinklers if time matches config starttime and weekday'''
	now = arrow.now()
	day = now.format("ddd") # Mon, Tue, Wed ...
	hour = now.format("H") # 0-24
	month = now.format('MMM') # Jan,Feb,Mar ...

	if(int(hour) == data['starttime'] and day in data['weekday']):
		logging.info('Today is a watering day')
		if(willItRain()):
			logging.info('skipped due to rain')
		else: #all conditions met to run valves
			logging.info('good weather')
			runValves(data, month)
	elif (len(sys.argv) >= 2 and sys.argv[1] == "now"): # run now for test
		runValves(data, month)
	else:
		logging.info('null run')

def main():
	'''main should get called by cron once per hour'''
	configLog()
	config = openConfig()
	operateValves(config)


def convertJson():
	'''optionally convert config file to json'''
	with open("config.json", "w") as json_out:
		yaml_object = openConfig()
		json.dump(yaml_object, json_out, indent=4, sort_keys=True)


if __name__ == "__main__":
	main()
