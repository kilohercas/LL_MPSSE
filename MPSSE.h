#include "stdint.h"
#include "ftd2xx.h"

unsigned long get_millis(void);

int32_t  UInt32ToInt32(uint32_t variable);
uint32_t Int32ToUInt32(int32_t  variable);
float    UInt32ToFloat(uint32_t variable);

FT_STATUS FTDI_SPI_INIT(const char* serial, FT_HANDLE* _ftHandle);


FT_STATUS  FPGA_Read_U32(FT_HANDLE handle, BYTE Address, uint32_t* data);
FT_STATUS  FPGA_Write_U32(FT_HANDLE handle, BYTE Address, uint32_t data);
FT_STATUS  FPGA_RW_U32_ARRAY(FT_HANDLE handle, uint16_t size, BYTE Address[], uint32_t* Data);
FT_STATUS  FPGA_W_U32_ARRAY(FT_HANDLE handle, uint16_t size, BYTE Address[], uint32_t Data[]);

FT_STATUS  FPGA_W_U32_ARRAY_TO_SINGLE_ADR(FT_HANDLE handle, uint32_t size, BYTE Address, uint32_t Data[]);

uint32_t CRC32(const uint32_t* u, uint32_t n);
uint32_t CRC32_u8(const uint8_t* u, int32_t n);

uint32_t CRC32_u8_Virtual(const uint8_t* u, uint32_t n, uint32_t total_size);


FT_STATUS FPGA_Read_Image_Compressed(FT_HANDLE handle, uint16_t* data, uint32_t Size_x, uint32_t Size_y);


FT_STATUS FTDI_UART_INIT(FT_HANDLE* handle, const char* serial, uint32_t boudrate);
FT_STATUS FTDI_UART_RXCNT(FT_HANDLE handle, uint32_t* rx_cnt);
FT_STATUS FTDI_UART_RX(FT_HANDLE handle, uint8_t* data, uint32_t rx_cnt, uint32_t offset);
FT_STATUS FTDI_UART_TX(FT_HANDLE handle, byte* data, uint16_t lenght, uint32_t timeout);
FT_STATUS FTDI_UART_PURGE(FT_HANDLE handle);
