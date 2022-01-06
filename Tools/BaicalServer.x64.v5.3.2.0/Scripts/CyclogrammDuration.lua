-----------------------------------------------------------------------------------------------------------------------
-- <Info>Calculation cyclogramm statistics</Info>
-- minimal duration in ms when cyclogramm is on ON state
-- minimal duration in ms when cyclogramm is on OFF state
-- maximal duration in ms when cyclogramm is on ON state
-- maximal duration in ms when cyclogramm is on OFF state
-- printing time for last hit of every value
-----------------------------------------------------------------------------------------------------------------------

--using local to speedup access to variables 
local gContext
local gsName
local gMin
local gMax

local gValuePrev
local gTimePrev

local gDurationOnMin
local gTimeOnMin

local gDurationOnMax
local gTimeOnMax

local gDurationOnAvg
local gDurationOnAvgCnt

local gDurationOffMin
local gTimeOffMin

local gDurationOffMax
local gTimeOffMax

local gDurationOffAvg
local gDurationOffAvgCnt

local gDurationCycleMin
local gTimeCycleMin

local gDurationCycleMax
local gTimeCycleMax

local gDurationCycleAvg
local gDurationCycleAvgCnt

local gTimePrevCycle

local g100nsInMs

-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process to initialize the script, provides important for analysis values
-- Parameters:
-- iContext: (int) calling context value, should be saved for further calls, like P7_Telemetry_Fotmat_Time(...)
-- sName   : (str) Counter name (ID)
-- iMin    : (int) expected minimum value of the counter's values
-- iMax    : (int) expected maximum value of the counter's values
------------
-- Return  : nothing
function P7_Telemetry_Init(iContext, sName, iMin, iMax)
   --Save context
   gContext = iContext
   gsName = sName
   gMin = iMin
   gMax = iMax
   
   --Assign values for all global variables, this step has to be done each time when script is initializing or re-initializing 
   gValuePrev = -1
   gTimePrev = -1
   
   gDurationOnMin = 4294967296
   gTimeOnMin = 0
   
   gDurationOnMax = 0
   gTimeOnMax = 0
   
   gDurationOnAvg = 0
   gDurationOnAvgCnt = 0
   
   
   gDurationOffMin = 4294967296
   gTimeOffMin = 0
   
   gDurationOffMax = 0
   gTimeOffMax = 0
   
   gDurationOffAvg = 0
   gDurationOffAvgCnt = 0
   
   
   gDurationCycleMin = 4294967296
   gTimeCycleMin = 0
   
   gDurationCycleMax = 0
   gTimeCycleMax = 0
   
   gDurationCycleAvg = 0
   gDurationCycleAvgCnt = 0
   
   gTimePrevCycle = -1
   
   
   g100nsInMs = 10000 --count of 100 ns intervals in 1ms
end

-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process to add another sample for analysis
-- Parameters:
-- iTime  : (int) time of the sample (100ns accuracy)
-- iValue : (int) sample's value
------------
-- Return : nothing
function P7_Telemetry_Push_Sample(iTime, iValue)
    local lDuration = iTime - gTimePrev;   
    
    if iValue >= 1 then
        if gTimePrevCycle > 0 then
            local lCycleDuration = iTime - gTimePrevCycle;   
        
            gDurationCycleAvg = gDurationCycleAvg + lCycleDuration
            gDurationCycleAvgCnt = gDurationCycleAvgCnt + 1

            if lCycleDuration < gDurationCycleMin then 
                gDurationCycleMin = lCycleDuration
                gTimeCycleMin = iTime
            end  
            if lCycleDuration > gDurationCycleMax then 
                gDurationCycleMax = lCycleDuration
                gTimeCycleMax = iTime
            end  
        end
        gTimePrevCycle = iTime; 
    end  
    
    --Case from 0 to 1
    if gValuePrev >= 0 and gValuePrev < 1 and iValue >= 1 then --PROBABLY: Lua uses double as base type, comparing double to 0 sometimes failed because of 0.000000001 values
        gDurationOffAvg = gDurationOffAvg + lDuration
        gDurationOffAvgCnt = gDurationOffAvgCnt + 1
        if lDuration < gDurationOffMin then 
            gDurationOffMin = lDuration
            gTimeOffMin = iTime
        end  
        if lDuration > gDurationOffMax then 
            gDurationOffMax = lDuration
            gTimeOffMax = iTime
        end  
    elseif gValuePrev >= 1 and iValue < 1 then   --Case from 1 to 0
        gDurationOnAvg = gDurationOnAvg + lDuration
        gDurationOnAvgCnt = gDurationOnAvgCnt + 1
        
        if lDuration < gDurationOnMin then 
            gDurationOnMin = lDuration
            gTimeOnMin = iTime
        end  
        if lDuration > gDurationOnMax then 
            gDurationOnMax = lDuration
            gTimeOnMax = iTime
        end  
    end

    gTimePrev = iTime
    gValuePrev = iValue
end


-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process around 4 times per second to get analysis report
-- Return : (txt) report, HTML 4 tags are accepted
function P7_Telemetry_Get_Report()
  return [[<table width="100%" border="0">
  <tr bgcolor="#cce6ff">
    <th>Min ON duration</th>
    <th>Max ON duration</th> 
    <th>Avg ON duration</th> 
    <th>Min OFF duration</th> 
    <th>Max OFF duration</th> 
    <th>Avg OFF duration</th> 
    <th>Min Cycle duration</th> 
    <th>Max Cycle duration</th> 
    <th>Avg Cycle duration</th> 
  </tr>
  <tr bgcolor="#ebebe0"> 
    <td align="center">]] .. string.format("%.4f", gDurationOnMin / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring(gDurationOnMax / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring((gDurationOnAvg / gDurationOnAvgCnt) / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring(gDurationOffMin / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring(gDurationOffMax / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring((gDurationOffAvg / gDurationOffAvgCnt) / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring(gDurationCycleMin / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring(gDurationCycleMax / g100nsInMs) .. [[ms</td>
    <td align="center">]] .. tostring((gDurationCycleAvg / gDurationCycleAvgCnt) / g100nsInMs) .. [[ms</td>
  </tr>
  <tr bgcolor="#ebebe0">
    <td align="center">]] .. P7_Telemetry_Fotmat_Time(gContext, gTimeOnMin) .. [[</td>
    <td align="center">]] .. P7_Telemetry_Fotmat_Time(gContext, gTimeOnMax) .. [[</td>
    <td align="center">-</td>
    <td align="center">]] .. P7_Telemetry_Fotmat_Time(gContext, gTimeOffMin) .. [[</td>
    <td align="center">]] .. P7_Telemetry_Fotmat_Time(gContext, gTimeOffMax) .. [[</td>
    <td align="center">-</td>
    <td align="center">]] .. P7_Telemetry_Fotmat_Time(gContext, gTimeCycleMin) .. [[</td>
    <td align="center">]] .. P7_Telemetry_Fotmat_Time(gContext, gTimeCycleMax) .. [[</td>
    <td align="center">-</td>
  </tr>
</table>]]
end
