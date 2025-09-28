#include "MPSSE.h"
#include <mutex>
#include <chrono>
#include <vector>

std::mutex _lockObject;

unsigned long get_millis(void)
{
	return 0;//(unsigned long)(clock() * 1000 / CLOCKS_PER_SEC);
}


int32_t UInt32ToInt32(uint32_t variable)
{
	return *(int32_t*)&variable;
}

uint32_t Int32ToUInt32(int32_t variable)
{
	return *(uint32_t*)&variable;
}

float UInt32ToFloat(uint32_t variable)
{
	return *(float*)&variable;
}

FT_STATUS  FTDI_SPI_INIT(const char* serial, FT_HANDLE* _ftHandle)
{
	FT_STATUS status = FT_OK;
	uint8_t DATA[9] = { 0x8A, 0x97, 0x8D, 0x85, 0x86, 0x0, 0x0, 0x87, 0x8A };

	DWORD	dwNumBytesSent = 0;
	DWORD	dwNumBytesRead = 0;
	DWORD	dwNumInputBuffer = 0;

	status = FT_OpenEx((PVOID)(serial), FT_OPEN_BY_SERIAL_NUMBER, _ftHandle);

	if (status == FT_OK)
	{
		status |= FT_ResetDevice(*_ftHandle);
		status |= FT_GetQueueStatus(*_ftHandle, &dwNumInputBuffer);

		if ((status == FT_OK) && (dwNumInputBuffer > 0))
		{
			BYTE InputBuffer[512];
			status |= FT_Read(*_ftHandle, InputBuffer, dwNumInputBuffer, &dwNumBytesRead);
		}

		status |= FT_SetUSBParameters(*_ftHandle, 65535, 65535);
		status |= FT_SetChars(*_ftHandle, false, 0, false, 0);
		status |= FT_SetTimeouts(*_ftHandle, 100, 100);
		status |= FT_SetLatencyTimer(*_ftHandle, 1);
		status |= FT_SetBitMode(*_ftHandle, 0, 0);
		status |= FT_SetBitMode(*_ftHandle, 0, 2);

		Sleep(50);

		status = FT_Write(*_ftHandle, DATA, sizeof(DATA), &dwNumBytesSent);

		if (status == FT_OK)
		{
			return status;
		}
		else
		{
			return status;
		}
	}
	else
	{
		return FT_OTHER_ERROR;
	}
}



FT_STATUS  FPGA_Read_U32(FT_HANDLE handle, BYTE Address, uint32_t* data)
{
	FT_STATUS _ftStatus = FT_OK;
	std::lock_guard<std::mutex> lock(_lockObject);

	DWORD W = 0;
	DWORD R = 0;

	unsigned char Buffer[17] = { 0x80, (std::time(nullptr) % 2) << 7, 0x8B,0x11,0,0, Address,0x31,3,0,0,0,0,0,0x80,8 | (std::time(nullptr) % 2) << 7, 0x8B };

	_ftStatus = FT_GetQueueStatus(handle, &W);

	if (_ftStatus == FT_OK)
	{
		if (W > 0)
		{
			DWORD R = 0;
			BYTE* queue_data = new BYTE[W];
			_ftStatus |= FT_Read(handle, queue_data, W, &R);
			delete[] queue_data;
		}
	}
	else
	{
		return _ftStatus;
	}
	_ftStatus |= FT_Write(handle, Buffer, 17, &W);
	_ftStatus |= FT_Read(handle, Buffer, 4, &R);
	*data = *(uint32_t*)&Buffer;
	return _ftStatus;
}

FT_STATUS  FPGA_Write_U32(FT_HANDLE handle, BYTE Address, uint32_t data)
{
	FT_STATUS _ftStatus = FT_OK;
	std::lock_guard<std::mutex> lock(_lockObject);
	DWORD W = 0;
	uint8_t  Buffer[17] = { 0x80, (std::time(nullptr) % 2) << 7, 0x8B,0x11,0,0, Address + 128,0x11,3,0,0,0,0,0,0x80,8 | (std::time(nullptr) % 2) << 7 , 0x8B };
	(*(uint32_t*)(Buffer + 10)) = data;
	_ftStatus = FT_Write(handle, Buffer, 17, &W);
	return _ftStatus;
}

FT_STATUS  FPGA_RW_U32_ARRAY(FT_HANDLE handle, uint16_t size, BYTE Address[], uint32_t* Data)
{
	FT_STATUS _ftStatus = FT_OK;
	DWORD W = 0;
	DWORD R = 0;
	_ftStatus = FT_GetQueueStatus(handle, &W);
	if (_ftStatus == FT_OK)
	{
		if (W > 0)
		{
			DWORD R = 0;
			BYTE* queue_data = new BYTE[W];
			_ftStatus |= FT_Read(handle, queue_data, W, &R);
			delete[] queue_data;
		}
	}
	else
	{
		return _ftStatus;
	}

	BYTE* array = new BYTE[size * 17];

	unsigned char Buffer[17] = { 0x80, (std::time(nullptr) % 2) << 7, 0x8B,0x11,0,0, 0x00,0x31,3,0,0x00,0x00,0x00,0x00,0x80,8 | (std::time(nullptr) % 2) << 7, 0x8B };

	for (int i = 0; i < size; i++)
	{
		Buffer[6] = Address[i];
		Buffer[10] = static_cast<unsigned char>(Data[i] & 0xFF);
		Buffer[11] = static_cast<unsigned char>((Data[i] >> 8) & 0xFF);
		Buffer[12] = static_cast<unsigned char>((Data[i] >> 16) & 0xFF);
		Buffer[13] = static_cast<unsigned char>((Data[i] >> 24) & 0xFF);
		std::memcpy(array + (i * 17), Buffer, 17);
	}
	_ftStatus |= FT_Write(handle, array, 17 * size, &W);

	_ftStatus |= FT_Read(handle, array, 4 * size, &R);

	for (int i = 0; i < size; i++)
	{
		if (Address[i] < 128)
		{
			Data[i] = *(uint32_t*)(array + (i * 4));
		}
	}

	delete[] array;

	return  _ftStatus;
}

FT_STATUS  FPGA_W_U32_ARRAY(FT_HANDLE handle, uint16_t size, BYTE Address[], uint32_t Data[])
{
	FT_STATUS _ftStatus = FT_OK;
	DWORD W = 0;
	DWORD R = 0;
	_ftStatus = FT_GetQueueStatus(handle, &W);
	if (_ftStatus == FT_OK)
	{
		if (W > 0)
		{
			DWORD R = 0;
			BYTE* queue_data = new BYTE[W];
			_ftStatus |= FT_Read(handle, queue_data, W, &R);
			delete[] queue_data;
		}
	}
	else
	{
		return _ftStatus;
	}

	unsigned char Buffer[17] = { 0x80, (std::time(nullptr) % 2) << 7, 0x8B,0x11,0,0, 0x00,0x11,3,0,0x00,0x00,0x00,0x00,0x80,8 | (std::time(nullptr) % 2) << 7, 0x8B };


	if (size < 4096)
	{
		BYTE* array = new BYTE[size * 17];

		for (int i = 0; i < size; i++)
		{
			Buffer[6] = Address[i];// +128;
			Buffer[10] = static_cast<unsigned char>(Data[i] & 0xFF);
			Buffer[11] = static_cast<unsigned char>((Data[i] >> 8) & 0xFF);
			Buffer[12] = static_cast<unsigned char>((Data[i] >> 16) & 0xFF);
			Buffer[13] = static_cast<unsigned char>((Data[i] >> 24) & 0xFF);
			std::memcpy(array + (i * 17), Buffer, 17);
		}
		_ftStatus |= FT_Write(handle, array, 17 * size, &W);

		delete[] array;
	}
	else
	{
		uint32_t data_to_send = 0;
		BYTE* array = new BYTE[1024 * 17];
		uint32_t data_send = 0;
		while (data_send < size)
		{
			if (data_send + 1024 < size)
			{
				data_to_send = 1024;
			}
			else
			{
				data_to_send = size - data_send;
			}

			for (int i = 0; i < data_to_send; i++)
			{
				Buffer[6] = Address[data_send + i]+ 128;
				Buffer[10] = static_cast<unsigned char>(Data[data_send+i] & 0xFF);
				Buffer[11] = static_cast<unsigned char>((Data[data_send + i] >> 8) & 0xFF);
				Buffer[12] = static_cast<unsigned char>((Data[data_send + i] >> 16) & 0xFF);
				Buffer[13] = static_cast<unsigned char>((Data[data_send + i] >> 24) & 0xFF);
				std::memcpy(array  + (i * 17), Buffer, 17);
			}
			_ftStatus |= FT_Write(handle, array, 17 * data_to_send, &W);
			data_send += data_to_send;
		}
	}

	return  _ftStatus;
}

FT_STATUS  FPGA_W_U32_ARRAY_TO_SINGLE_ADR(FT_HANDLE handle, uint32_t size, BYTE Address, uint32_t Data[])
{
	FT_STATUS _ftStatus = FT_OK;
	DWORD W = 0;
	DWORD R = 0;
	_ftStatus = FT_GetQueueStatus(handle, &W);
	if (_ftStatus == FT_OK)
	{
		if (W > 0)
		{
			DWORD R = 0;
			BYTE* queue_data = new BYTE[W];
			_ftStatus |= FT_Read(handle, queue_data, W, &R);
			delete[] queue_data;
		}
	}
	else
	{
		return _ftStatus;
	}

	unsigned char Buffer[17] = { 0x80, (std::time(nullptr) % 2) << 7, 0x8B,0x11,0,0, 0x00,0x11,3,0,0x00,0x00,0x00,0x00,0x80,8 | (std::time(nullptr) % 2) << 7, 0x8B };


	if (size < 4096)
	{
		BYTE* array = new BYTE[size * 17];

		for (int i = 0; i < size; i++)
		{
			Buffer[6] = Address + 128;
			Buffer[10] = static_cast<unsigned char>(Data[i] & 0xFF);
			Buffer[11] = static_cast<unsigned char>((Data[i] >> 8) & 0xFF);
			Buffer[12] = static_cast<unsigned char>((Data[i] >> 16) & 0xFF);
			Buffer[13] = static_cast<unsigned char>((Data[i] >> 24) & 0xFF);
			std::memcpy(array + (i * 17), Buffer, 17);
		}
		_ftStatus |= FT_Write(handle, array, 17 * size, &W);

		delete[] array;
	}
	else
	{
		uint32_t data_to_send = 0;
		BYTE* array = new BYTE[1024 * 17];
		uint32_t data_send = 0;
		while (data_send < size)
		{
			if (data_send + 1024 < size)
			{
				data_to_send = 1024;
			}
			else
			{
				data_to_send = size - data_send;
			}

			for (int i = 0; i < data_to_send; i++)
			{
				Buffer[6] = Address + 128;
				Buffer[10] = static_cast<unsigned char>(Data[data_send + i] & 0xFF);
				Buffer[11] = static_cast<unsigned char>((Data[data_send + i] >> 8) & 0xFF);
				Buffer[12] = static_cast<unsigned char>((Data[data_send + i] >> 16) & 0xFF);
				Buffer[13] = static_cast<unsigned char>((Data[data_send + i] >> 24) & 0xFF);
				std::memcpy(array + (i * 17), Buffer, 17);
			}
			_ftStatus |= FT_Write(handle, array, 17 * data_to_send, &W);
			data_send += data_to_send;
		}
		delete[] array;
	}

	return  _ftStatus;
}

void ARRAY_ASSEMBLE(byte* data, byte address, uint32_t write, uint32_t offset)
{
	data[0 + offset] = 0x80;
	data[1 + offset] = (std::time(nullptr) % 2) << 7;
	data[2 + offset] = 139;

	data[3 + offset] = 0x31;
	data[4 + offset] = 0;
	data[5 + offset] = 0;
	data[6 + offset] = address;

	data[7 + offset] = 0x31;
	data[8 + offset] = 3;
	data[9 + offset] = 0;
	data[10 + offset] = write & 0xFF;
	data[11 + offset] = (write >> 8) & 0xFF;
	data[12 + offset] = (write >> 16) & 0xFF;
	data[13 + offset] = (write >> 24) & 0xFF;

	data[14 + offset] = 0x80;
	data[15 + offset] = (byte)(8 | data[1 + offset]);
	data[16 + offset] = 0x8B;
}

FT_STATUS FPGA_Read_Image_Compressed(FT_HANDLE handle, uint16_t* data, uint32_t Size_x, uint32_t Size_y)
{
	FT_STATUS ftstatus = FT_OK;
	ftstatus = FPGA_Write_U32(handle, 42, 1);

	DWORD w = 0;
	DWORD r = 0;
	byte Buffer[65535] = { 0 };
	uint32_t index = 0;
	uint32_t i = 0;
	uint32_t image = 0;
	DWORD size = 1000;

	uint32_t transfer_size = ((Size_x * Size_y * 10) / 32);
	uint32_t wordIndex = 0; // Raw array index
	uint32_t  decode_index = 0;
	uint32_t w0 = 0;
	uint32_t w1 = 0;
	uint32_t w2 = 0;
	uint32_t w3 = 0;
	uint32_t w4 = 0;
	while (image < transfer_size)
	{
		i = 0;
		index = 0;

		if (transfer_size < 1000)
		{
			size = transfer_size;
		}
		else if ((image + size) < transfer_size)
		{
			size = 1000;
		}
		else
		{
			size = transfer_size - image;
		}

		while (i < size)
		{
			ARRAY_ASSEMBLE(Buffer, 42, 0, index);
			index += 17;
			i++;
		}

		ftstatus |= FT_Write(handle, Buffer, size * 17, &w);
		ftstatus |= FT_Read(handle, Buffer, size * 5, &r);
		i = 0;
		while (i < (size * 5))
		{
			w0 = static_cast<uint32_t>(Buffer[i + 1]) | (static_cast<uint32_t>(Buffer[i + 2]) << 8) | (static_cast<uint32_t>(Buffer[i + 3]) << 16) | (static_cast<uint32_t>(Buffer[i + 4]) << 24);
			i += 5;
			w1 = static_cast<uint32_t>(Buffer[i + 1]) | (static_cast<uint32_t>(Buffer[i + 2]) << 8) | (static_cast<uint32_t>(Buffer[i + 3]) << 16) | (static_cast<uint32_t>(Buffer[i + 4]) << 24);
			i += 5;
			w2 = static_cast<uint32_t>(Buffer[i + 1]) | (static_cast<uint32_t>(Buffer[i + 2]) << 8) | (static_cast<uint32_t>(Buffer[i + 3]) << 16) | (static_cast<uint32_t>(Buffer[i + 4]) << 24);
			i += 5;
			w3 = static_cast<uint32_t>(Buffer[i + 1]) | (static_cast<uint32_t>(Buffer[i + 2]) << 8) | (static_cast<uint32_t>(Buffer[i + 3]) << 16) | (static_cast<uint32_t>(Buffer[i + 4]) << 24);
			i += 5;
			w4 = static_cast<uint32_t>(Buffer[i + 1]) | (static_cast<uint32_t>(Buffer[i + 2]) << 8) | (static_cast<uint32_t>(Buffer[i + 3]) << 16) | (static_cast<uint32_t>(Buffer[i + 4]) << 24);
			i += 5;
			image += 5;

			data[decode_index++] = (uint16_t)((w0 >> 24) & 0xFF) + (uint16_t)(((w1 >> 30) & 0x3) << 8);    // 1st pixel w0[31..24]
			data[decode_index++] = (uint16_t)((w0 >> 16) & 0xFF) + (uint16_t)(((w1 >> 28) & 0x3) << 8);   // 2nd pixel w0[23..16]
			data[decode_index++] = (uint16_t)((w0 >> 8) & 0xFF) + (uint16_t)(((w1 >> 26) & 0x3) << 8);    // 3rd pixel w0[15..8]
			data[decode_index++] = (uint16_t)((w0 >> 0) & 0xFF) + (uint16_t)(((w1 >> 24) & 0x3) << 8);    // 3rd pixel w0[7..0]    w1[31..24]

			data[decode_index++] = (uint16_t)((w1 >> 16) & 0xFF) + (uint16_t)(((w2 >> 22) & 0x3) << 8); // 1st pixel  w1[23..16]
			data[decode_index++] = (uint16_t)((w1 >> 8) & 0xFF) + (uint16_t)(((w2 >> 20) & 0x3) << 8);  // 2nd pixel  w1[15..8]
			data[decode_index++] = (uint16_t)((w1 >> 0) & 0xFF) + (uint16_t)(((w2 >> 18) & 0x3) << 8);  // 3rd pixel  w1[7..0]
			data[decode_index++] = (uint16_t)((w2 >> 24) & 0xFF) + (uint16_t)(((w2 >> 16) & 0x3) << 8); // 3rd pixel  w2[31..24]  w2[23..16]

			data[decode_index++] = (uint16_t)((w2 >> 8) & 0xFF) + (uint16_t)(((w3 >> 14) & 0x3) << 8);  // 1st pixel  w2[15..8]
			data[decode_index++] = (uint16_t)((w2 >> 0) & 0xFF) + (uint16_t)(((w3 >> 12) & 0x3) << 8);  // 2nd pixel  w2[7..0]
			data[decode_index++] = (uint16_t)((w3 >> 24) & 0xFF) + (uint16_t)(((w3 >> 10) & 0x3) << 8);   // 3rd pixel  w3[31..24]
			data[decode_index++] = (uint16_t)((w3 >> 16) & 0xFF) + (uint16_t)(((w3 >> 8) & 0x3) << 8);   // 3rd pixel  w3[23..16] w3[15..8] 

			data[decode_index++] = (uint16_t)((w3 >> 0) & 0xFF) + (uint16_t)(((w4 >> 6) & 0x3) << 8);  // 1st pixel w3[7..0]
			data[decode_index++] = (uint16_t)((w4 >> 24) & 0xFF) + (uint16_t)(((w4 >> 4) & 0x3) << 8);  // 2nd pixel w4[31..24]
			data[decode_index++] = (uint16_t)((w4 >> 16) & 0xFF) + (uint16_t)(((w4 >> 2) & 0x3) << 8);   // 3rd pixel w4[23..16]
			data[decode_index++] = (uint16_t)((w4 >> 8) & 0xFF) + (uint16_t)(((w4 >> 0) & 0x3) << 8);   // 3rd pixel w4[15..8]   w4[7..0]
		}
	}
	ftstatus += FPGA_Write_U32(handle, 42, 0);
	return ftstatus;
}


uint32_t CRC32(const uint32_t* u, uint32_t n)
{
	uint32_t crc = 0xAAAAAAAA;
	for (uint32_t i = 0; i < n; i++)
	{
		crc ^= u[i];
		for (uint32_t j = 0; j < 32; j++)
		{
			if (crc & 0x80000000)
			{
				crc = (crc << 1) ^ 0x04C11DB7U;
			}
			else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

uint32_t CRC32_u8(const uint8_t* u, int32_t n)
{
	uint32_t crc = 0xAAAAAAAA;
	for (uint32_t i = 0; i < n; i += 4)
	{
		uint32_t value = ((uint32_t)u[i]) | (((uint32_t)u[i + 1]) << 8) | (((uint32_t)u[i + 2]) << 16) | (((uint32_t)u[i + 3]) << 24);
		crc ^= value;
		for (int j = 0; j < 32; j++)
		{
			if (crc & 0x80000000)
			{
				crc = (crc << 1) ^ 0x04C11DB7U;
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}



uint32_t CRC32_u8_Virtual(const uint8_t* u, uint32_t n, uint32_t total_size)
{
	uint32_t crc = 0xAAAAAAAA;
	uint32_t value = 0;
	uint32_t size = 0;

	for (uint32_t i = 0; i < total_size; i += 4)
	{

		if (size < n)
		{
			value = ((uint32_t)u[size]);
		}
		else
		{
			value = (uint32_t)0xFF;
		}
		size++;

		if (size < n)
		{
			value += (((uint32_t)u[size]) << 8);
		}
		else
		{
			value += (uint32_t)0xFF << 8;
		}
		size++;

		if (size < n)
		{
			value += (((uint32_t)u[size]) << 16);
		}
		else
		{
			value += (uint32_t)0xFF << 16;
		}
		size++;

		if (size < n)
		{
			value += (((uint32_t)u[size]) << 24);
		}
		else
		{
			value += (uint32_t)0xFF << 24;
		}
		size++;

		crc ^= value;
		for (int j = 0; j < 32; j++)
		{
			if (crc & 0x80000000)
			{
				crc = (crc << 1) ^ 0x04C11DB7U;
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}



//uint8_t uart_array[65536] = { 0 };
FT_STATUS FTDI_UART_RXCNT(FT_HANDLE handle, uint32_t* rx_cnt)
{
	FT_STATUS ftstatus = FT_OK;
	ftstatus = FT_GetQueueStatus(handle, (DWORD*)rx_cnt);
	return ftstatus;
}

FT_STATUS FTDI_UART_RX(FT_HANDLE handle, uint8_t* data, uint32_t rx_cnt, uint32_t offset)
{
	FT_STATUS ftstatus = FT_OK;
	DWORD  read_size = 0;
	DWORD  read = 0;


	uint8_t uart[512];
	ftstatus = FT_GetQueueStatus(handle, &read_size);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}
	else
	{
		if (read_size >= rx_cnt)
		{
			ftstatus = FT_Read(handle, uart, rx_cnt, &read);
			if (ftstatus != FT_OK)
			{
				return ftstatus;
			}
			else
			{
				if (read > 0)
				{
					try
					{
						memcpy(data + offset, uart, read);
					}
					catch (...)
					{
						return FT_OTHER_ERROR;
					}
				}
			}
		}
	}
	return ftstatus;
}

FT_STATUS FTDI_UART_PURGE(FT_HANDLE handle)
{
	return FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
}


FT_STATUS FTDI_UART_TX(FT_HANDLE handle, byte* data, uint16_t lenght, uint32_t timeout)
{
	FT_STATUS ftstatus = FT_OK;
	DWORD writen = 0;
	ftstatus = FT_Write(handle, data, lenght, &writen);

	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}
	else
	{
		if (timeout == 0)
		{
			return ftstatus;
		}
		else
		{
			unsigned long start_time = get_millis();
			unsigned long elapsed = 0;
			DWORD rx = 0;
			DWORD tx = 0;
			DWORD event = 0;
			do
			{
				Sleep(1);
				ftstatus = FT_GetStatus(handle, &rx, &tx, &event);
				if (ftstatus != FT_OK)
				{
					return ftstatus;
				}
				elapsed = get_millis() - start_time;
				if (elapsed > timeout)
				{
					break;
				}

			} while (tx != 0);
		}
	}
	return ftstatus;
}


FT_STATUS FTDI_UART_INIT(FT_HANDLE* handle, const char* serial, uint32_t boudrate)
{
	FT_STATUS ftstatus = FT_OK;

	ftstatus = FT_OpenEx((PVOID)serial, FT_OPEN_BY_SERIAL_NUMBER, handle);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_ResetDevice(*handle);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_ResetPort(*handle);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_SetLatencyTimer(*handle, 0);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_Purge(*handle, FT_PURGE_RX | FT_PURGE_TX);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_SetBaudRate(*handle, boudrate);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_SetDataCharacteristics(*handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_EVEN);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_SetFlowControl(*handle, FT_FLOW_NONE, 0, 0);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}

	ftstatus = FT_SetTimeouts(*handle, 500, 500);
	if (ftstatus != FT_OK)
	{
		return ftstatus;
	}


	return ftstatus;
}

