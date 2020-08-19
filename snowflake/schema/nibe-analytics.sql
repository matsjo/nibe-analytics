use role sysadmin;

-- Create the database

create or replace database nibe;
use schema nibe.public;

-- Create a storage integration and connect an Snowflake external stage to it
-- Example is AWS s3: but ABS or GCS works as well.

use role accountadmin;

CREATE STORAGE INTEGRATION s3_int
  TYPE = EXTERNAL_STAGE
  STORAGE_PROVIDER = S3
  ENABLED = TRUE
  STORAGE_AWS_ROLE_ARN = '< Your ARN goes here>'
  STORAGE_ALLOWED_LOCATIONS = ('s3://< Your s3 bucket goes here/');

grant usage on integration s3_int to role sysadmin;

-- Check the integration

show integrations;
describe integration s3_int;

-- Create an external  stage pointing to your bucket

use role sysadmin;

create or replace stage nibeStage
  storage_integration = s3_int
  url = 's3://< Your s3 bucket goes here/'
  file_format = (type =JSON )
  copy_options = (on_error='skip_file', purge = true);

-- Check the that we can read the stage

ls @nibeStage;

-- Create a Virtual Warehouse

CREATE WAREHOUSE NIBE_WH WITH
  WAREHOUSE_SIZE = 'SMALL'
  WAREHOUSE_TYPE = 'STANDARD'
  AUTO_SUSPEND = 300
  AUTO_RESUME = TRUE
  MIN_CLUSTER_COUNT = 1
  MAX_CLUSTER_COUNT = 2
  SCALING_POLICY = 'STANDARD';

-- create a target table for the loaded data

create or replace table rawData (json variant);

-- create and refresh the pipe which loads the data
-- If we have the data files in the s3 bucket they should now end up in the rawData table

create or replace pipe nibePipe auto_ingest = true as copy into rawData from @nibeStage;

alter pipe nibePipe refresh;

-- If we have a lot of files to load this can take some minutes

select system$pipe_status('nibe.public.nibePipe');
select dateadd('hour',2,JSON:"@timestamp"::timestamp) timestamp, json from rawData order by 1 desc limit 500;
select count(*) from rawData;

-- Create a view making it easier to write SQL against some interesting rawData content

-- Create a view making it easier to write SQL against some interesting rawData content

create or replace view logData as (
select to_timestamp_ltz(JSON:"@timestamp") TimeStamp,
  JSON:"AddHeatingDiff"::number AddHeatingDiff,
  JSON:"AddHeatingRunTime"::decimal(38,2) AddHeatingRunTime,
  JSON:"AddHeatingStart"::number AddHeatingStart,
  JSON:"BrineTempIn"::decimal(38,2) BrineTempIn,
  JSON:"BrineTempOut"::decimal(38,2) BrineTempOut,
  BrineTempOut - BrineTempIn BrineTempDiff,
  JSON:"BulbTemp"::decimal(38,2) BulbTemp,
  JSON:"CompRunTime"::decimal(38,2) CompRunTime,
  JSON:"CompressorStarts"::number CompressorStarts,
  JSON:"CondTemp"::number CondTemp,
  JSON:"Curve"::number Curve,
  JSON:"CurveOffset"::number CurveOffset,
  JSON:"DegreeMinutes"::number DegreeMinutes,
  JSON:"DiffHPAdd"::number DiffHPAdd,
  JSON:"ExtAdjustment"::number ExtAdjustment,
  JSON:"FlowTempCalc"::number FlowTempCalc,
  JSON:"FlowTempMax"::number FlowTempMax,
  JSON:"FlowTempMin"::number FlowTempMin,
  JSON:"FlowTempOut"::decimal(38,2) FlowTempOut,
  JSON:"FlowTempReturn"::decimal(38,2) FlowTempReturn,
  FlowTempOut - FlowTempReturn FlowTempDiff,
  JSON:"FlowTempReturnMax"::number FlowTempReturnMax,
  JSON:"FlowdiffHP"::decimal(38,2) FlowdiffHP,
  JSON:"HGTemp"::decimal(38,2) HGTemp,
  JSON:"HWRunTime"::decimal(38,2) HWRunTime,
  JSON:"HWStartTemp" HWStartTemp,
  JSON:"HWStopTemp" HWStopTemp,
  JSON:"HWTemp"::decimal(38,2) HWTemp,
  JSON:"HWXIntervall"::number HWXIntervall,
  JSON:"LiquidLineTemp"::decimal(38,2) LiquidLineTemp,
  JSON:"OutdoorTemp"::decimal(38,1) OutdoorTemp,
  JSON:"OutdoorTempAvg24h"::decimal(38,1) OutdoorTempAvg24h,
  JSON:"Phase1Amp"::decimal(38,2) Phase1Amp,
  JSON:"Phase2Amp"::decimal(38,2) Phase2Amp,
  JSON:"Phase3Amp"::decimal(38,2) Phase3Amp,
  JSON:"SummerModeTemp"::number SummerModeTemp,
  JSON:"XHWStopCompressor"::number XHWStopCompressor,
  JSON:"XHWStopTemp"::number XHWStopTemp
        from rawData);

select * from logData limit 100;

-- Create a view to show the last received logpost to simplify dashboarding

create or replace view logNow as
( select * from logData order by timestamp desc limit 1);

select * from logNow;

-- So a couple of minutes and we are up and running and can start analyze the Nibe data
