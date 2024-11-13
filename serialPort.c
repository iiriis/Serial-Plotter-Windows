

/*
 * Copyright (C) 2023 Avijit Das <avijitdasxp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include "serialPort.h"
#include <windows.h>
#include <errno.h>


#define FILE_NO_SHARED_ACCESS   0
#define FILE_RW_MODE            (FILE_GENERIC_READ | FILE_GENERIC_WRITE)

DWORD WINAPI MonitorSerialRX(LPVOID lpParam);
static char input_buf[4096];    


serial_port_err_t setTimeouts(serial_port_t* port, uint64_t readTimeout, uint64_t writeTimeout)
{
    /* create a COMMTIMEOUTS structure and set the timeout values */
    COMMTIMEOUTS timeouts;
    timeouts.ReadIntervalTimeout = 0;           /* No timeout between subsequent reads */
    timeouts.ReadTotalTimeoutConstant = readTimeout;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = writeTimeout;
    timeouts.WriteTotalTimeoutMultiplier = 0;

    /* store the timeout values in the serial port handle */
    port->readTimeout = readTimeout;
    port->writeTimeout = writeTimeout;

    /* set the timeouts and check for error */
    if (!SetCommTimeouts(port->handle, &timeouts)) 
        return SERIAL_ERR_UNKNOWN;
    
    /* return OK */
    return SERIAL_ERR_OK;
}


serial_port_err_t setBaud(serial_port_t* port, uint64_t baudRate)
{
    /* create a DCB structure and set the Baudrate */
    DCB dcb = {0};

    /* set the size of DCB length to the size of the structure itself */
    dcb.DCBlength = sizeof(DCB);
    
    /* get the current DCB state */
    if (!GetCommState(port->handle, &dcb))
        return SERIAL_ERR_UNKNOWN;

    dcb.BaudRate = baudRate;
    port->baud = baudRate;

    /* set the new DCB values for the serial port */
    if (!SetCommState(port->handle, &dcb))
        return SERIAL_ERR_UNKNOWN;

    /* return OK */
    return SERIAL_ERR_OK;
}


serial_port_err_t serialPortOpen(serial_port_t* port, const char* name, uint64_t baud, uint32_t readTimeout, uint32_t writeTimeout) 
{

    /* Cosmetic colorisation of the terminal output by enabling Virtual Terminal */
    DWORD currentConsoleMode;
    /* get the current console attributes and OR with virtual terminal flag */
    GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &currentConsoleMode);
    SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), currentConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );

    /* initialise the port structure with the arguments */
    port->name = name;
    port->baud = baud;
    port->isOpen = FALSE;
    port->readTimeout = readTimeout;
    port->writeTimeout = writeTimeout;
    
    /* open the serial port by opening it as a file with the following attributes */
    port->handle = CreateFileA(port->name, FILE_RW_MODE, FILE_NO_SHARED_ACCESS, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    /* set the baud rate and the timeouts */
    setBaud(port, baud);
    setTimeouts(port, readTimeout, writeTimeout);
    
    /* check whether the port handle is invalid */
    if(port->handle == INVALID_HANDLE_VALUE){
        
        return SERIAL_ERR_OPEN;
    }

    /* set the port is open to TRUE */
    port->isOpen = TRUE;

    port->serialEventHandler = NULL;

    /* return OK */
    return SERIAL_ERR_OK;
}


serial_port_err_t serialPortClose(serial_port_t* port)
{
    /* Close the port handle and set the isOpen to FALSE upon success*/
    if (CloseHandle(port->handle))
    {
        port->isOpen = FALSE;
        /* return OK */
        return SERIAL_ERR_OK;
    }

    

    /* return error */
    return SERIAL_ERR_CLOSE;
}


serial_port_err_t serialPortRead(serial_port_t* port, char *buf, uint64_t size)
{
    /* to store the actual bytes read */
    uint64_t bytesRead;

    /* read from the serial port and check if it's a successful read */
    if(ReadFile(port->handle, buf, size, (LPDWORD)&bytesRead, NULL) != TRUE)
    {
        
        return SERIAL_ERR_READ_UNKNOWN;
    }

    /* if the actual bytes read and the size requested are not same; return error */
    if(bytesRead != size)
        return SERIAL_ERR_READ_SIZE_MISMATCH;

    /* return OK */
    return SERIAL_ERR_OK;
}


serial_port_err_t serialPortWrite(serial_port_t* port, uint8_t *buf, uint64_t size)
{
    /* to store the actual bytes written */
    uint64_t bytesWrite;

    /* write to the serial port and check if it's a successful write */
    if(WriteFile(port->handle, buf, size, (LPDWORD)&bytesWrite, NULL) != TRUE)
    {
        
        return SERIAL_ERR_WRITE_UNKNOWN;
    }

    /* if the actual bytes written and the size of buffer are not same; return error */
    if(bytesWrite != size)
        return SERIAL_ERR_WRITE_SIZE_MISMATCH;

    /* return OK */
    return SERIAL_ERR_OK;
}


int bytesAvailable(serial_port_t *hSerial) {
    COMSTAT comStat;
    DWORD errors;

    // Clear any communication errors and get the current status of the serial port
    if (ClearCommError(hSerial->handle, &errors, &comStat)) {
        // Return the number of bytes available in the input buffer
        return comStat.cbInQue;
    } else {
        // If there's an error, return -1 to indicate a failure
        return -1;
    }
}


int isDataAvailable(serial_port_t *hSerial) {
    DWORD eventMask;
    if (!SetCommMask(hSerial->handle, EV_RXCHAR)) {
        return -1;
    }

    // Wait for an event to occur (like receiving a character)
    if (WaitCommEvent(hSerial->handle, &eventMask, NULL)) {
        if (eventMask & EV_RXCHAR) {
            return 1;
        }
    }
    return 0;  // No data received
}


int enableSerialEvent(serial_port_t *hSerial, void (*event_handler)(char*, int)){
    
    if(event_handler == NULL)
        return -1;

    if(hSerial->serialEventHandler == NULL){
        hSerial->serialEventHandler = event_handler;

        // Create a thread
        HANDLE hThread = CreateThread(
            NULL,              // Default security attributes
            0,                 // Default stack size
            MonitorSerialRX,    // Function to be executed
            hSerial,       // Parameter to pass to the thread function
            0,                 // Start the thread immediately
            NULL               // No need for the thread ID
        );

        return 0;
    }

    return -1;  // already an IRQ handler is present

}

DWORD WINAPI MonitorSerialRX(LPVOID lpParam) {

    serial_port_t *serial = (serial_port_t*)(lpParam);

    while (1)
    {
        // blocking event until a new character is received and this does not load the CPU :)
        isDataAvailable(serial);
        int bytes = bytesAvailable(serial);
        serialPortRead(serial, input_buf, bytes);

        // Call the event Handler function and pass the received bytes
        serial->serialEventHandler(input_buf, bytes);
    }
    
    return 0;
}