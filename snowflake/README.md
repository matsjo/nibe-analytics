# Snowflake instruction

- Create a Snowflake trial account at https://trial.snowflake.com

- Login to your to your Snowflake GUI Console and as ACCOUNTADMIN execute the SQL script https://github.com/matsjo/nibe-analytics/blob/master/snowflake/schema/nibe-analytics.sql

- The script will

-- Create the database

-- Create a storage integration and connect an Snowflake external stage to it
-- Example is AWS s3: but ABS or GCS works as well.

-- Check the integration

-- Create an external  stage pointing to your bucket
-- Check the that we can read the stage

-- Create a Snowflake Virtual Warehouse

-- Create a target table for the loaded data

-- Create and refresh the pipe which loads the data

-- Create a view making it easier to write SQL against some interesting rawData content
-- Create a view to show the last received logpost to simplify dashboarding

 
