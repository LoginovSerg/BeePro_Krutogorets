#!/usr/bin/env python3

import subprocess
import paho.mqtt.client as mqtt
import time
import datetime
from influxdb_client import InfluxDBClient, Point
import json


broker_address="192.168.1.100"
database = 'MyDatabase'

username = ''
password = ''
retention_policy = 'autogen'
bucket = f'{database}/{retention_policy}'

def on_connect(mqtt, userdata, flags, rc):
  print("Connected with result code "+str(rc))
  mqtt.subscribe("scale/#")
#  mqtt.subscribe("+/#")

def on_message(mqtt, userdata, msg):
  now = datetime.datetime.now()
  print (now.strftime("%H:%M:%S "), end='', flush=True)

  try:
    if msg.topic == "scale/7CE668":    # Scale 1      править топик!
#      print ("7CE668=%s" % msg.payload.decode())
      m_decode=str(msg.payload.decode("utf-8","ignore"))
      m_in=json.loads(m_decode) #decode json data
      T68 = m_in["T"]
      RH68 = m_in["RH"]
      W68 = m_in["WEIGHT"]
      V68 = m_in["V"]
#      print("T=%.1f RH=%d W=%.3f V=%.3f" % (T68, RH68, W68, V68))

      with InfluxDBClient(url='http://localhost:8086', token=f'{username}:{password}', org='-') as client:
        with client.write_api() as write_api:
          p1 = Point("Krutogorie").tag("host", "scale").field("T68", T68).field("RH68", RH68).field("W68", W68).field("V68", V68)
          write_api.write(bucket=bucket, record=[p1])
    else:
      print ("TOP=%s PL=%s" % (msg.topic, msg.payload.decode()))

  except:
    print ("EXCEPTION!")

mqtt = mqtt.Client()
mqtt.connect(broker_address,1883,60)
mqtt.on_connect = on_connect
mqtt.on_message = on_message
mqtt.publish("scale/python","connect")

while True:
    mqtt.loop()
    time.sleep(0.5)
