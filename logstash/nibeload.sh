#!/bin/bash

# If you by any reasons can't use Snowpipe auto-ingest (GCP for example) then manually refresh your pipe with
# Requirements:
#                 snowsql to be installed on your logstash host
#                 create a snowsql config file for your connection

echo "alter pipe nibePipe refresh;" | /<your snowqsql path>/snowsql --config /< your home directory>/.snowsql/config -c nibe.aws
