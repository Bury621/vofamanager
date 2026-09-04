#include "vofamanager.h"

/**
 * @brief 初始化
 * 
 * @param csx 传入的上下文指针
 */
void VOFA_Init(vofamanager_csx *csx)
{
    csx->status=VOFAMANAGER_STATUS_WAIT_SIGN; //输入标题中
    csx->rx_sign_p=csx->rx_sign_buffer; //操作指针复位
    csx->rx_data_p=csx->rx_data_buffer;
}

/**
 * @brief 在串口的中断函数里面调用
 * 
 * @param csx 回调的上下文
 * @param dat 接收到的字节数据
 */
void VOFA_Recevice_Callback(vofamanager_csx *csx,uint8_t dat)
{
    if(dat == ':')
    {
        csx->status=VOFAMANAGER_STATUS_WAIT_DATA; //切换状态机成接收数据
        *(csx->rx_sign_p)='\0'; //字符串收尾
        csx->rx_sign_p=csx->rx_sign_buffer; //操作标题指针复位
        csx->rx_data_p = csx->rx_data_buffer;  //复位数据指针
    }
    else if(dat == '\n')
    {
        csx->status=VOFAMANAGER_STATUS_WAIT_SIGN; //输入标题中
        *(csx->rx_data_p)='\0'; //字符串收尾
        csx->rx_data_p=csx->rx_data_buffer; //复位数据指针
        VOFA_Get_Package_Callback(csx,csx->rx_sign_buffer,atof(csx->rx_data_buffer)); //调用处理数据包函数
    }
    else
    {
        switch(csx->status)
        {
            case VOFAMANAGER_STATUS_WAIT_SIGN: //输入标题中
                *(csx->rx_sign_p)=dat;
                csx->rx_sign_p++;
                break;
            case VOFAMANAGER_STATUS_WAIT_DATA:
                *(csx->rx_data_p)=dat;
                csx->rx_data_p++;
                break;
        }
    }
}

/**
 * @brief 收到一套完整的数据包之后自动回调，用户实现这个函数并且在里面处理收到的数据
 * 
 * @param csx 触发回调的上下文
 * @param sign 收到的标题字符串首指针
 * @param data 收到的浮点型数据
 * @return __weak 
 */
__attribute__((weak)) void VOFA_Get_Package_Callback(vofamanager_csx *csx,char *sign,float data)
{
    //重写这个函数,让数据包有具体的功能
}