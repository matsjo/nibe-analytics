SELECT DATE_TRUNC('HOUR', timestamp) AS timestamp,
       avg(brinetempin) AS "BrineTempIn",
       avg(brinetempout) AS "BrineTempOut"
FROM nibe
WHERE timestamp >= '2020-06-01 00:00:00'
  AND timestamp <= '2020-06-14 00:00:00'
GROUP BY DATE_TRUNC('HOUR', timestamp)
ORDER BY 1 DESC
LIMIT 10000;
