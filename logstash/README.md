# Logstash Readme

This is just an example on how we can listen to http calls from the Arduino and dump that to a cloud provider storage. Logstash can be replaced with basically any http server able to write json to a Snowflake supported cloud storage (currently AWS S3, Azure BLOB Storage and Google Cloud Storage.

To do that the required funcilaities are:

1. Listen to http port 8060 (configurable)
1. Save the incoming http payload (JSON) and save it to an s3 bucket in AWS.


### Instructions

1. Create an AWS s3 bucket in your AWS account - https://docs.aws.amazon.com/AmazonS3/latest/user-guide/create-bucket.html
1. Install Logstash https://www.elastic.co/guide/en/logstash/current/installing-logstash.html
1. Example conf is using the HTTP Input plugin - https://www.elastic.co/guide/en/logstash/current/plugins-inputs-http.html
1. and the AWS S3 plugin - https://www.elastic.co/guide/en/logstash/current/plugins-outputs-s3.html
1. Create a logstash.conf - https://github.com/matsjo/nibe-analytics/blob/master/logstash/config/logstash.aws.conf
1. Start listen for your pump data with `/<logstash dir>/bin/logstash -f config/logstash.conf.aws`



