-----------------------------------------------------------------------------------------------------------------------
-- <Info>Put here script description</Info>
-----------------------------------------------------------------------------------------------------------------------

-----------------------------------------------------------------------------------------------------------------------
--Script variables.
--N.B.: Please initialize all local values inside P7_Telemetry_Init function call
--      Function P7_Telemetry_Init will be called at script engine creation and each time when new window of data 
--      will be analysed
local iP7_Context
local iP7_Max
local iP7_Min
local sP7_Name
local sReport
local uSamplesCount

-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process to initialize the script, provides important for analysis values
-- Parameters:
-- iContext: (int) calling context value, should be saved for further calls, like P7_Telemetry_Fotmat_Time(...)
-- sName   : (str) Counter name (ID)
-- iMin    : (double) expected minimum value of the counter's values
-- iMax    : (double) expected maximum value of the counter's values
------------
-- Return  : nothing
function P7_Telemetry_Init(iContext, sName, iMin, iMax)
    --Save context
    iP7_Context = iContext
    iP7_Max     = iMax
    iP7_Min     = iMin
    sP7_Name    = sName
    
    --Assign values for all global variables, this step has to be done each time when script is initializing or re-initializing 
    uSamplesCount = 0
end

-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process to add another sample for analysis
-- Parameters:
-- iTime  : (int) time of the sample (100ns accuracy, 1 tick = 100nanoseconds, 10 000 000 per second)
-- iValue : (double) sample's value
------------
-- Return : nothing
function P7_Telemetry_Push_Sample(iTime, iValue)
-- processing time & values here
   uSamplesCount = uSamplesCount + 1
end


-----------------------------------------------------------------------------------------------------------------------
--Function called by hosting process around 4 times per second to get analysis report
-- Return : (txt) report, HTML 4 tags are accepted
function P7_Telemetry_Get_Report()
    return [[Samples processed: ]] .. tostring(uSamplesCount)
end


-----------------------------------------------------------------------------------------------------------------------
-- Calling process provides function to convert sample time to text
-- N.B.: Function call is expensive from CPU cycles point of view, so it is good strategy to call it only when 
--       report has to be generated
-- Parameters:
-- iContext: (int) calling context value, provided by P7_Telemetry_Init(...) function call
-- iTime   : (int) sample time, provided by P7_Telemetry_Push_Sample(...) function call
------------
-- Return  : (txt) time in readable text form
-->> function P7_Telemetry_Fotmat_Time(iContext, iTime)
