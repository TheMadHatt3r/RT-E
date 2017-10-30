/*
  RTE_Serial.cpp - Library for Arduino Serial comms.
  Created by Collin Matthews, 27-JUL-2015.
  Uses circular read buffer to prevent loss of multiple quick commands from VB.
  This library should allow basic serial between vb.net and Arduino.
  There is a complementary VB.Net library I also made to work with this one.
*/

#include "Arduino.h"
#include "RTE_Serial.h"


//TODO**********************
//initialize all variables in constructor.


//READING COMMANDS
	//1)check buffer for end of command char (peek buffer)
	//2)read from read_pt to end of command char
			//done with loop and check each char returned for EOC
			//If EOC, break loop, if write_pt=read_pt, break loop with error
	//3)with new single command byte array, check CRC
	//4)IF CRC valid, then send to command processor, ELSE break with error.
	//5)CMD processor validates command and sends ACK, else appropriate ERR ACK.
	//6)CMD is executed.
	//7)Whole process is repeated if another EOC is in buffer, else exit function and do work.
	
//SENDING COMMANDS - BASIC
	//1)create byte array of CMD and DATA
	//2)pass array to CRC engine and have it return CMD+DATA+CRC
	//3)Add on EOC and such, then load into transmit buffer??
	
//SENDING COMMANDS - FILE
	//0)Read 1 line of file at time, for each line do the following
		//a)create byte array of CMD and DATA
		//b)pass array to CRC engine and have it return CMD+DATA+CRC
		//c)Add on EOC and such, then load into transmit buffer??
	//1)Send final "line" containing the number of lines in the file as a simple check that everything made it?



//#define ERR_CRC 0xFC


/**********************************************************************************
********************************* Constructors ************************************
***********************************************************************************/


RTE_Serial::RTE_Serial(){
}


RTE_Serial::RTE_Serial( usb_serial_class  &print ){
  rx_buf_w_pnt = 0;
  rx_buf_r_pnt = 0;
  valid_data = false; //NOT USED!
  printer = &print; //operate on the address of print
}



/**********************************************************************************
************************ Command & High Level Functions ***************************
***********************************************************************************/


/*
Function loads RX soft buffer with bytes from UART.
	Call often to feed this class!
*/
void RTE_Serial::readNewData(){
	while (printer->available()) {
		rx_buffer[rx_buf_w_pnt] = printer->read();
		//DEBUG!!!!!!!!!!!!!
		//printer->write(rx_buffer[rx_buf_w_pnt]);
		if (rx_buf_w_pnt<RX_BUFFER-1)
			rx_buf_w_pnt++;
		else
			rx_buf_w_pnt=0;
    }
}


/*
Function Gets 1 message from serial buffer, requires msg[] buffer and 2 position info array.
	This function assumes an EOM in buffer means there is a valid message.
	This function is called to also check if a command is available.
	'SOM + Crc_8 + Len_8 + Cmd_8 + data_8... + EOM
*/
void RTE_Serial::getAndValadateCommand(uint8_t msg[], uint8_t msg_info[]){
	uint8_t crc_recived = 0x00;
	int message_length = 0;
	//Array for return
	msg_info[MSG_INFO_CMD] = 0;
	msg_info[MSG_INFO_CRC] = 0;
	msg_info[MSG_INFO_CRC_ERR] = 0; // 1 on fail
	msg_info[MSG_INFO_LEN] = 0;		// 0 default/error

	//If Start and End of message found in buffer (Assumes valid message, CRC will ensure)
	if(!findByteInBuffer(BYTE_EOM) | !findByteInBuffer(BYTE_SOM))
		return;
	
	//Make sure first byte is SOM
	while(peek() != BYTE_SOM)
		getByteFromBuffer();
	
	//Recheck EOM
	if(!findByteInBuffer(BYTE_EOM))
		return;
	//Eat SOM
	getByteFromBuffer();
	//Get CRC
	crc_recived = getByteFromBuffer();
	//Get Length of data
	message_length = getByteFromBuffer();
	//Get CMD
	msg_info[MSG_INFO_CMD] = getByteFromBuffer();
	//For at most length of buffer, or until end of message.
	for (int cnt = 0; cnt<RX_BUFFER;cnt++){
		msg[cnt] = getByteFromBuffer();
		message_length++;
		if(msg[cnt] == BYTE_EOM)
			break;
	}
	
	//Perform CRC on DATA and CMD
	//crc8()
	
	
	//NOTE REMOVED TO DEBUG!!!!!!!!!!
	//Now have what is assumed to be a complete message.
	/*
	msg_info[MSG_INFO_LEN] = message_length;
	crc_recived = msg[message_length-1];
	if (crc_recived != crc8(msg, message_length)){
		msg_info[MSG_INFO_CRC_ERR] = 1;
		sendNAK(0x00);
		return;
	}
	
	//Valid Data. Return ACK.
	sendACK(crc_recived);
	*/
}



//TODO, ensure msg is less then 250 len.
void RTE_Serial::transmitDataSmall(uint8_t cmd, uint8_t data[], uint8_t data_len){
	//Full array size
	uint8_t full_msg_arr_len = data_len + 5; //SOM + Crc_8 + Len_8 + Cmd_8 + data_8... + EOM
	uint8_t full_msg[full_msg_arr_len];		 //SOM + Crc_8 + Len_8 + Cmd_8 + data_8... + EOM
	full_msg[0] = BYTE_SOM;
	//CRC goes here
	full_msg[2] = data_len;
	full_msg[3] = cmd;
	//Data goes here
	full_msg[full_msg_arr_len - 1] = BYTE_EOM;
	
	//Build [LEN + CMD + DATA] for CRC IN THIS ORDER!
	uint8_t tmp_crc_arr[data_len + 2];
	tmp_crc_arr[0] = data_len;
	tmp_crc_arr[1] = cmd;
	for (int cnt = 0; cnt < data_len; cnt++){
		tmp_crc_arr[cnt+2] = data[cnt];
		full_msg[cnt + 4] = data[cnt];
	}
	
	uint8_t crc = crc8(tmp_crc_arr, data_len + 2 );
	full_msg[1] = crc;
	
	//Full message constructed
	for (int cnt = 0; cnt < full_msg_arr_len; cnt++) 
		printer->write(full_msg[cnt]);

}





/**********************************************************************************
***************************** Ring Buffer Functions *******************************
***********************************************************************************/


/*
Function gets single byte from RX buffer, or returns 0 on empty.
*/
byte RTE_Serial::getByteFromBuffer(){
	byte result;
	if(rx_buf_r_pnt != rx_buf_w_pnt){
		result = rx_buffer[rx_buf_r_pnt];
		//handle pointer inc logic
		if (rx_buf_r_pnt<RX_BUFFER-1)
			rx_buf_r_pnt++;
		else
			rx_buf_r_pnt=0;
	}
	else
		result = 0x00; //Nul char. in this case, empty buffer
   return result;
}

/*
Function searches through buffer looking for a char, returns true if found
*/
boolean RTE_Serial::findByteInBuffer(byte match_byte){
	int local_rx_r_pnt = rx_buf_r_pnt;

    while (local_rx_r_pnt != rx_buf_w_pnt){
		//on match return true
		if(match_byte == rx_buffer[local_rx_r_pnt]){
			return true;
		}
		//increment buf pointer local
		if (local_rx_r_pnt<RX_BUFFER-1)
			local_rx_r_pnt++;
		else
			local_rx_r_pnt=0;
	}
   return false;
}

/*
  Gets next char in buffer but does not consume
*/
uint8_t RTE_Serial::peek(){
    if(rx_buf_r_pnt != rx_buf_w_pnt)
		return  rx_buffer[rx_buf_r_pnt];
	return 0;
}


/**********************************************************************************
******************************* Misc. Functions ***********************************
***********************************************************************************/

/*
Function returns 8 bit CRC of array
Pass array and length of array.
*/
uint8_t RTE_Serial::crc8(const uint8_t *addr, uint8_t len)
{
    uint8_t crc = 0;

    while (len--) {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

/*
  Function ACK and also sends crc of last message.
  'SOM + Crc_8 + Len_8 + Cmd_8 + data_8... + EOM
*/
void RTE_Serial::sendACK(uint8_t crc){
	printer->write(BYTE_SOM);
	printer->write(BYTE_ACK);
	printer->write(crc);
	printer->write(BYTE_EOM);
}

/*
  Function NAK and also sends crc of last message.
  'SOM + Crc_8 + Len_8 + Cmd_8 + data_8... + EOM
*/
void RTE_Serial::sendNAK(uint8_t crc){
	printer->write(BYTE_SOM);
	printer->write(crc);
	printer->write(BYTE_NAK);
	
	printer->write(BYTE_EOM);
}



/*
  Called to send data from Arduino to host.
*/
/*
void RTE_Serial::transmitString(String str){
  printer->print(str);
}
*/



