#ifndef __VOFAMANAGER_H
#define __VOFAMANAGER_H

#include "main.h"
#include <stdlib.h>

#define VOFA_UART huart1 //配置vofa连接的串口
#define VOFA_RX_SIGN_BUFFER_SIZE 64 //标题接收缓冲区大小
#define VOFA_RX_DATA_BUFFER_SIZE 64 //数据接收缓冲区大小

extern UART_HandleTypeDef VOFA_UART;

//接收状态机
typedef enum{
    VOFAMANAGER_STATUS_WAIT_SIGN, //接收冒号之前的数据的时候的状态
    VOFAMANAGER_STATUS_WAIT_DATA, //接收冒号之后的数据的时候的状态
}VOFAMANAGER_STATUS;

//vofamanager句柄
typedef struct
{
    char rx_sign_buffer[VOFA_RX_SIGN_BUFFER_SIZE]; //标题缓冲区
    char rx_data_buffer[VOFA_RX_DATA_BUFFER_SIZE]; //数据缓冲区
    char *rx_sign_p; //写入标题的指针
    char *rx_data_p; //写入数据的指针
    VOFAMANAGER_STATUS status; //接收状态机
}vofamanager_csx;

void VOFA_Init(vofamanager_csx *csx);
void VOFA_Recevice_Callback(vofamanager_csx *csx,uint8_t dat);
__attribute__((weak)) void VOFA_Get_Package_Callback(vofamanager_csx *csx,char *sign,float data);
#endif