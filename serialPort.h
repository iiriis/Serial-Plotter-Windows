/**
 * @file serialPort.h
 * @brief API declarations for serial port operations.
 * 
 * This header file provides the declarations for handling serial port communication,
 * including functions for opening, closing, reading, and writing to serial ports.
 * 
 * @author iiriis
 * @date 2023 - 2024
 * @copyright
 * This program is licensed under the GNU General Public License v3.0.
 */

#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <errno.h>
#include <stdint.h>
#include <windows.h>

/**
 * @defgroup structs Structures
 * @brief Structures used for serial port communication.
 */

/**
 * @defgroup enums Enums
 * @brief Error codes for serial port functions.
 */

/**
 * @defgroup HL_functions High-Level Functions
 * @ingroup functions
 * @brief Functions for managing serial port operations.
 */

/**
 * @defgroup functions Functions
 * @brief Functions for managing serial port operations.
 */

/**
 * @struct serial_port_t
 * @brief Stores configuration and status of a serial port.
 * 
 * @ingroup structs
 */
typedef struct {
    HANDLE handle;          /**< File handle for the serial port. */
    const char *name;       /**< Name of the serial port (e.g., COM1). */
    uint8_t isOpen;            /**< Indicates if the port is open. */
    uint64_t baud;          /**< Baud rate of the port. */
    uint32_t readTimeout;   /**< Read timeout in milliseconds. */
    uint32_t writeTimeout;  /**< Write timeout in milliseconds. */
    void (*serialEventHandler)(char*, int); /**< Callback for received data events. */
} serial_port_t;

/**
 * @enum serial_port_err_t
 * @brief Error codes for serial port operations.
 * 
 * @ingroup enums
 */
typedef enum {
    SERIAL_ERR_OK,                 /**< No error. */
    SERIAL_ERR_OPEN,               /**< Error opening the serial port. */
    SERIAL_ERR_CLOSE,              /**< Error closing the serial port. */
    SERIAL_ERR_UNKNOWN,            /**< Unknown error occurred. */
    SERIAL_ERR_READ_UNKNOWN,       /**< Unknown error during read operation. */
    SERIAL_ERR_READ_SIZE_MISMATCH, /**< Bytes read do not match expected size. */
    SERIAL_ERR_WRITE_UNKNOWN,      /**< Unknown error during write operation. */
    SERIAL_ERR_WRITE_SIZE_MISMATCH /**< Bytes written do not match buffer size. */
} serial_port_err_t;

/**
 * @brief Opens a serial port and initializes the handle.
 *
 * @param[in] port Pointer to the serial port structure.
 * @param[in] name String containing the name of the serial port.
 * @param[in] baud Baud rate for the serial port.
 * @param[in] readTimeout Read timeout in milliseconds.
 * @param[in] writeTimeout Write timeout in milliseconds.
 * 
 * @return SERIAL_ERR_OK if successful, otherwise an appropriate error code.
 *
 * @ingroup HL_functions
 */
serial_port_err_t serialPortOpen(serial_port_t* port, const char* name, uint64_t baud, uint32_t readTimeout, uint32_t writeTimeout);

/**
 * @brief Closes the serial port.
 * 
 * @param[in] port Pointer to a serial_port_t structure.
 * 
 * @return SERIAL_ERR_OK if successful, otherwise SERIAL_ERR_CLOSE.
 *
 * @ingroup HL_functions
 */
serial_port_err_t serialPortClose(serial_port_t* port);

/**
 * @brief Reads data from the serial port.
 * 
 * @param[in] port Pointer to the serial port structure.
 * @param[out] buf Buffer to store the read data.
 * @param[in] size Number of bytes to read.
 * 
 * @return SERIAL_ERR_OK if successful, otherwise an appropriate error code.
 *
 * @ingroup HL_functions
 */
serial_port_err_t serialPortRead(serial_port_t* port, char *buf, uint64_t size);

/**
 * @brief Writes data to the serial port.
 * 
 * @param[in] port Pointer to the serial port structure.
 * @param[in] buf Buffer containing the data to write.
 * @param[in] size Size of the data in the buffer.
 * 
 * @return SERIAL_ERR_OK if successful, otherwise an appropriate error code.
 *
 * @ingroup HL_functions
 */
serial_port_err_t serialPortWrite(serial_port_t* port, uint8_t *buf, uint64_t size);

/**
 * @brief Sets the baud rate for the serial port.
 * 
 * @param[in] port Pointer to the serial port structure.
 * @param[in] baudRate Desired baud rate.
 * 
 * @return SERIAL_ERR_OK if successful, otherwise SERIAL_ERR_UNKNOWN.
 *
 * @ingroup HL_functions
 */
serial_port_err_t setBaud(serial_port_t* port, uint64_t baudRate);

/**
 * @brief Configures the read and write timeouts for the serial port.
 * 
 * @param[in] port Pointer to the serial port structure.
 * @param[in] readTimeout Read timeout in milliseconds.
 * @param[in] writeTimeout Write timeout in milliseconds.
 * 
 * @return SERIAL_ERR_OK if successful, otherwise SERIAL_ERR_UNKNOWN.
 *
 * @ingroup HL_functions
 */
serial_port_err_t setTimeouts(serial_port_t* port, uint64_t readTimeout, uint64_t writeTimeout);


/**
 * @brief Returns the number of bytes available to read from the serial port.
 * 
 * @param[in] hSerial Pointer to a serial_port_t structure.
 * 
 * @return Number of bytes available, or -1 if an error occurred.
 * 
 * @ingroup HL_functions
 */
int bytesAvailable(serial_port_t *hSerial);

/**
 * @brief Registers a callback function to handle serial port data reception and starts a monitoring thread.
 * 
 * This function associates a callback handler to manage data received from the serial port.
 * It spawns a dedicated thread that monitors the port for incoming data without causing CPU load due to polling.
 * The thread invokes the callback whenever new data is available.
 * 
 * @param[in] hSerial Pointer to a serial_port_t structure.
 * @param[in] event_handler     Callback function to handle received data, called with the buffer and number of bytes received.
 *                          The callback function should have the following signature:
 *                          **`void event_handler(char* buffer, int bytes);`**
 *                          - `buffer` contains the data received from the serial port.
 *                          - `bytes` is the number of bytes in the buffer.
 * 
 * @return 0 if successful, otherwise -1 if an event handler is already registered or on error.
 * 
 * @ingroup HL_functions
 */
int enableSerialEvent(serial_port_t *hSerial, void (*event_handler)(char* buffer, int bytes));

#endif
