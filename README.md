# Nibe-Analytics

Project for data collection, analytics and optimization based on SQL and Python ML for legacy Nibe Fighter heat pumps. This as the Nibe heat curve funcion is static and  basic in comparison with a trained machine learning scenario. Goal is to decrease power consumption by 10% with same comfort. 
  

Initial scope is:

  1. Build an Arduino to dump the Nibe data bus without paying +1K Euros for an RCU-11 unit from Nibe - https://github.com/matsjo/nibe-analytics/blob/master/nibe-data-dump/README.md
  2. Arduino Sketch to decode and write a JSON to Logstash - https://github.com/matsjo/nibe-analytics/blob/master/nibe-data-dump/nibe-data-dump.ino  
  3. Extract and Load the data to cloud storage via Logstash - https://github.com/matsjo/nibe-analytics/blob/master/logstash/README.md
  4. Load and Transform the data into a Snowflake Data Warehouse - https://github.com/matsjo/nibe-analytics/blob/master/snowflake/README.md 
  5. Create a Nibe near-real time dashboard in Snowflake Snowsight - COMING SOON.
  6. Perform Feature Engineering and Machine Learning on the sensor data to understand how to optimize the pump operation - ROAD MAP
  7. Figure out how to manage the pump programmatically via the Arduino. - SLOWLY IN PROGRESS
  8. Programmatically optimize the pump operation by adjusting the config in near real time - ROAD MAP. 

The Arduino solution can be used as a replacement for the Nibe RCU-10 or RCU-11 commercial modules.  

The project should work with the listed pumps below (list is based on pumps compatible with RCU-11/10):

- Nibe Fighter 360p (v) 
- Nibe Fighter 113x (v)
- Nibe Fighter 1150 (v)
- Nibe Fighter 123x
- Nibe Fighter 1250
- Nibe Fighter 1320
- Nibe Fighter 1330

(v = verified - Please add your model to the list of verified heat pumps)

## Some requirements

- A compatible Nibe Geothermal Heatpump w Nibe Software version 2.0 or above.
- Arduino board incl Wifi and RS486 shields.
- A web service able to receive http msgs and dump them to cloud storage - Git based on logstash and AWS s3 but works with any http service and a cloud storage compatible with Snowflake.
- A Snowflake account - https://trial.snowflake.com/
- Arduino C/C++, SQL and Python skills helps.


