import paho.mqtt.client as mqtt
import datetime as dt
import traceback


TEST_BROKER = '10.129.23.30'
PROD_BROKER = '10.129.23.41'

BROKER = PROD_BROKER

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("nodemcu/kresit/#", qos=2)

def on_message(client, userdata, msg):
    try:
       cur_time = dt.datetime.now()
       print 'Received message %d %s %s at %s'  %(msg.mid, msg.topic, msg.payload, cur_time.strftime('%Y-%m-%d %H:%M:%S'))
       received_msg_topic = msg.topic
       stream, location, sensor_type, id = received_msg_topic.split('/')
       #print stream, location, sensor_type, id
       data = None
       publisher_topic = None

       # diff parsers for different type of sensors
       if sensor_type == 'temp':
          temp = float(str(msg.payload))/10000.0
          data = '%0.4f,%f' % (time.time(), temp)
          publisher_topic = 'data/%s/%s/%s' %(location, sensor_type, id)
       
       elif sensor_type == 'dht':
        parts = str(msg.payload).split(',')
        
        if len(parts) == 3:
            if len(parts[0]) != 0 and len(parts[1]) != 0 and len(parts[2]) != 0:
              data = '%0.4f,%d,%0.3f,%0.3f' % (float(cur_time.strftime('%s')), int(parts[0]), float(parts[1]), float(parts[2]))
              publisher_topic = 'data/%s/%s/%s' %(location, sensor_type, id)
            else:
              print 'empty data string obtained ..'
        
        elif len(parts) == 4:
            if len(parts[0]) != 0 and len(parts[1]) != 0 and len(parts[2]) != 0 and len(parts[3]) != 0:
              data = '%0.4f,%d,%0.3f,%0.3f,%0.3f' % (float(cur_time.strftime('%s')), int(parts[0]), float(parts[1]), float(parts[2]), float(parts[3]))
              publisher_topic = 'data/%s/%s/%s' %(location, sensor_type, id)
            else:
              print 'empty data string obtained ..'
        
        else:
          print 'data does not conform to desired format"<id,temperature,humidity>" -- ',
       
       elif sensor_type == 'emon':
        parts = str(msg.payload).split(',')

        if len(parts) == 6:
          if len(parts[0]) != 0: 
            data = '%d,%d,%d,%d,%d,%d,%0.4f' %(int(parts[0]), int(parts[1]),int(parts[2]),int(parts[3]),int(parts[4]),int(parts[5]), float(cur_time.strftime('%s')) )
            publisher_topic = 'data/%s/%s/%s' %(location, sensor_type, id)
          else:
            print 'empty data string'
        else:

          print 'data does not conform to the desired format "<stat1,stat2,stat3,stat4,stat5,stat6>" -- ',

       else:
          pass
       
       if publisher_topic is not None:

        client.publish(topic=publisher_topic, payload=data, qos=2)
        print publisher_topic, data
       else:
        print 'Given sensor type is not handled'
       print '--'*20
    except Exception as e:
      traceback.print_exc()
      #print traceback.format_exc() #this also works
      #print "poda - there is an error"
      print '--'*20
      pass


client = mqtt.Client("DHT_PROSUMER",clean_session=False)
client.on_connect = on_connect
client.on_message = on_message
client.connect(BROKER, 1883, 60)
client.loop_forever()
