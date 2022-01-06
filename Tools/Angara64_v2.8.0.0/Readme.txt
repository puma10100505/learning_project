www.baical.net
Angara is utility for redirecting Event Tracing real-time logs from your 
driver to Baical server. 

****************************************************************************
N.B.: Be sure that Angara is launched from fast enought HDD with few  Gb  of
      free space.
      Angara uses Event Trace for Windows (ETW) engine  to  receive  traces.
      ETW uses intermediate file for traces processing,  this  file  may  be
      very large (a few Gb). 
      This file will be created inside "<Angara>\Temp" folder  and  will  be
      automatically removed on application exit.
****************************************************************************

List of supported arguments types (TMF):
  ItemLongLongX  
  ItemLongLongXX  
  ItemLongLongO  
  ItemLongLong  
  ItemChar     
  ItemUChar 
  ItemShort 
  ItemLong 
  ItemWString
  ItemString
  ItemPtr
  ItemDouble 
  ItemNTSTATUS
  ItemWINERROR
  ItemHRESULT 
  ItemIPAddr    - displays HEX value (uint32)
  ItemPort      
  ItemTimestamp - displays raw value (uint64)
  ItemWaitTime  - displays raw value (uint64)
  ItemTimeDelta - displays raw value (uint64)

Angara doesn't yet support next complex arguments types:
  ItemListByte
  ItemSetByte
  ItemListShort
  ItemSetShort 
  ItemListLong  
  ItemSetLong   
  ItemEnum      
  ItemIPV6Addr  
  ItemRString,
  ItemRWString, 
  ItemPString, 
  ItemPWString, 
  ItemPString, 
  ItemCLSID
  ItemLIBID
  ItemIID 
  ItemHEXDump 

