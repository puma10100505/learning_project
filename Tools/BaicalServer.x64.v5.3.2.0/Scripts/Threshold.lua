-----------------------------------------------------------------------------------------------------------------------
-- <Info>Script is counting amount of samples values equal or less than Min and equal or greater that Max</Info>
-----------------------------------------------------------------------------------------------------------------------

--N.B.: Please initialize all local values inside P7_Telemetry_Init function call
--      Function P7_Telemetry_Init will be called at script engine creation and each time when new window of data 
--      will be analysed
local gContext
local gsName
local gMin
local gMax
local gThresholdMinCount
local gThresholdMaxCount

-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process to initialize/reinitialize the script, provides important for analysis values
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
    gThresholdMinCount = 0
    gThresholdMaxCount = 0
end

-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process to add another sample for analysis
-- Parameters:
-- iTime  : (int) time of the sample (100ns accuracy)
-- iValue : (int) sample's value
------------
-- Return : nothing
function P7_Telemetry_Push_Sample(iTime, iValue)
    if iValue > gMax then 
      gThresholdMaxCount = gThresholdMaxCount + 1
    elseif iValue < gMin then
      gThresholdMinCount = gThresholdMinCount + 1
    end
end


-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process around 4 times per second to get analysis report
-- Return : (txt) report, HTML 4 tags are accepted
function P7_Telemetry_Get_Report()
    return [[<table width="100%" border="0">
                <tr bgcolor="#cce6ff">
                  <th>Min (]] .. tostring(gMin) .. [[) threshold</th>
                  <th>Max (]] .. tostring(gMax) .. [[) threshold</th> 
                </tr>
                <tr bgcolor="#ebebe0">
                  <td align="center">]] .. tostring(gThresholdMinCount) .. [[</td>
                  <td align="center">]] .. tostring(gThresholdMaxCount) .. [[</td>
                </tr>
            </table>]]
end
