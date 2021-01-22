# SuperSprink
Landscape irrigation sprinkler controller.

Code copied from:
 https://thenewstack.io/off-the-shelf-hacker-automated-yard-watering-project/
 https://thenewstack.io/off-the-shelf-hacker-adding-mqtt-and-cron-to-the-lawn-sprinkler-project/
  
Relay board data at:
  http://wiki.sunfounder.cc/index.php?title=8_Channel_5V_Relay_Module&utm_source=thenewstack&utm_medium=website

I've added a python script that is called by crontab each hour. This script manages the watering schedule and also checks for rain in the forcasts and supresses watering on days that have rain predicted.
