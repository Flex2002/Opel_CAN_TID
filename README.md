# Opel_CAN_TID
Arduino Nano Project to receive and send can-bus messages at the mid-speed of a Opel Corsa D to implement a water temperature gauge in the TID (tiple information display). After a radio swap, the TID only show the current date. That’s why I decided to retrofit a coolant temperature display, because GM hat decided to reduce the productions costs and eliminated this in the instrument display.


Milestones

	  Analysing CAN-Messages
	  
	  Verify that TID, GID and CID communication use similar protocol
	  
	  Send a constant message via CAN to Display
	  
	  Request temperature via CAN
	  
	  Send dynamic messages to Display


Development Hardware:

	  Opel Corsa D
	
	  Arduino Uno/Nano 
	
	  MCP2515 (8Mhz) for 95.238 communication | Register CFG1 (0x01) | Register CFG2 (0XBB) | Register CFG3 (0X07)


Thanks to the following projects:

	  SeppHansen/SLSS-CANAnalyser 
	  
	  megadrifter/Opel-Astra-H
	 
	  PNKP237/EHU32
