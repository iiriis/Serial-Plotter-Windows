#include "serialPort.h"
#include "plot.h"


#include <windows.h>
#include <stdio.h>


serial_port_t serial;

HANDLE data_ready;

char input_buffer[1024];
short input_buffer_idx;

void serialIRQ(char* buffer, int bytes);

int main(){


    if(serialPortOpen(&serial, "\\\\.\\COM9", 115200, 1000, 1000) != SERIAL_ERR_OK)
    {
        printf("Serial Port unavailable\n");
        return -1;
    }

    data_ready = CreateSemaphore(NULL, 0, 1, NULL);

    enableSerialEvent(&serial, serialIRQ);

    startOpenGL();

    // while(1){
    //     Sleep(100000);
    // }


    return 0;
}



void serialIRQ(char* buffer, int bytes){

    int a0, a1, a2;

    for(int i=0; i<bytes; i++)
    {
        input_buffer[input_buffer_idx++] = buffer[i];
        if(buffer[i] == '\n')
        {
            sscanf(input_buffer, "%d %d %d\n", &a0, &a1, &a2);
            input_buffer_idx = 0;

            ReleaseSemaphore(data_ready, 1, NULL);
        }

    }

    // printf("%*s", bytes, buffer);

    push_data(4, (float)a0, (float)a1, (float)a2, (float)(rand()%255));

    printf("%d %d %d\n", a0, a1, a2);

}