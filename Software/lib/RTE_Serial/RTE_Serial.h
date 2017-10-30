/*
  RTE_Serial.h - Library for Arduino Serial comms.
  Created by Collin Matthews, 06-NOV-2014.
*/
#ifndef RET_Serial_h
#define RET_Serial_h

#include "Arduino.h"

#define RX_BUFFER 128	//byte[#] RX buffer (commands ~15-30 bytes)
#define BYTE_SOM 254
#define BYTE_EOM 250
#define BYTE_ACK 5
#define BYTE_NAK 6

//#define CMD_BUFFER 8


class RTE_Serial
{
  public:
    //vars
    uint8_t rx_buffer[RX_BUFFER];
    boolean valid_data;
	//Constant Defines
	int MSG_INFO_CMD = 0;
	int MSG_INFO_LEN = 1;
	int MSG_INFO_CRC_ERR = 2;
	int MSG_INFO_CRC = 3;
	
	
    //functions
    RTE_Serial(void);
    RTE_Serial( usb_serial_class  &print );
	void readNewData(void);
	uint8_t crc8(const uint8_t *addr, uint8_t len);
	void sendACK(uint8_t crc);
	void sendNAK(uint8_t crc);
	void getAndValadateCommand(uint8_t msg[], uint8_t msg_info[]);
	uint8_t peek();
	void transmitDataSmall(uint8_t cmd, uint8_t data[], uint8_t data_len);
	

  private:
    //char serial_raw_rx;
    byte rx_buf_w_pnt;
    byte rx_buf_r_pnt;
    usb_serial_class * printer;
	
	byte getByteFromBuffer(void);
	boolean findByteInBuffer(byte match_byte);
	
	
};

#endif
