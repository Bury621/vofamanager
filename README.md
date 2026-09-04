# vofamanager

一个给 VOFA+ 或其它串口上位机调参使用的 C 中间层。它把串口中断收到的逐字节数据组装成 `参数名:数值\n` 文本包，解析后通过弱回调交给你的业务代码，方便在上位机里修改 PID、限幅、标志位等运行参数，而不需要把串口解析逻辑散落在主程序里。

## 一、工作方式

```text
上位机 / 串口工具                 UART RX 中断                   业务层
"Kp:12.34\n"  ---------------->  VOFA_Recevice_Callback()  -->  VOFA_Get_Package_Callback()
                                  逐字节解析状态机                   修改 PID 等参数
```

约定的一帧格式是：

```text
参数名:数值\n
```

例如：

```text
Kp:12.34
Ki:0.02
Kd:8
Enable:1
```

收到的完整帧会自动调用 `VOFA_Get_Package_Callback()`，参数为标题字符串和已转换为 `float` 的数值。

## 二、文件组成

只需加入这两个文件：

- `vofamanager.h`
- `vofamanager.c`

解析器本身不依赖 STM32 HAL，也不绑定具体串口句柄。`vofamanager.h` 只提供接收状态机；任何串口驱动只要把收到的字节传给 `VOFA_Recevice_Callback()` 就能工作。接入时保证工程能提供 `uint8_t` 定义即可。

## 三、快速接入

### 1. 创建上下文并初始化

在应用层定义：

```c
vofamanager_csx vofa_csx;
uint8_t vofa_rx_byte;
```

在 `main()` 串口初始化完成之后：

```c
VOFA_Init(&vofa_csx);
HAL_UART_Receive_IT(&huart1, &vofa_rx_byte, 1);
```

### 2. 在 UART 接收完成回调中喂给中间层

下面是 STM32 HAL 工程的一种接入方式；如果使用其它串口驱动，只要在每收到一个字节时调用 `VOFA_Recevice_Callback()`：

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        VOFA_Recevice_Callback(&vofa_csx, vofa_rx_byte);
        HAL_UART_Receive_IT(&huart1, &vofa_rx_byte, 1);
    }
}
```

这里的关键是：每收到 1 个字节就调用一次 `VOFA_Recevice_Callback()`，再重新启动下一次接收。

### 3. 实现数据包回调

库中的 `VOFA_Get_Package_Callback()` 是弱定义。在应用代码中写同名函数即可覆盖，不需要改库文件：

```c
#include <string.h>

void VOFA_Get_Package_Callback(vofamanager_csx *csx, char *sign, float data)
{
    (void)csx;

    if (strcmp(sign, "Kp") == 0)
    {
        pid_controller.kp = data;
    }
    else if (strcmp(sign, "Ki") == 0)
    {
        pid_controller.ki = data;
    }
    else if (strcmp(sign, "Enable") == 0)
    {
        motor_enable = (data != 0.0f);
    }
}
```

## 四、VOFA+ / 上位机侧发送方式

不需要用 JustFloat 那种二进制波形通道来发参数。让上位机按文本格式发送单行即可：

```text
Kp:2.5
```

也就是说，发送端最终给串口的原始内容是：

```text
Kp:2.5\n
```

如果上位机一次发来多个参数，请分成多行发送。本模块按“一行一个参数”解析，不处理 `Kp:1.0,Ki:2.0` 这种格式。

## 五、常用配置项

在 `vofamanager.h` 中：

| 宏 | 默认值 | 含义 |
| --- | --- | --- |
| `VOFA_RX_SIGN_BUFFER_SIZE` | `64` | 参数名缓冲区大小 |
| `VOFA_RX_DATA_BUFFER_SIZE` | `64` | 数值缓冲区大小 |

缓冲区大小为 `64` 时，参数名最多 63 个字符，数值部分最多 63 个字符。一般调参名和数值远小于这个长度，不需要改。

## 六、中断与实时性注意事项

- `VOFA_Get_Package_Callback()` 在 UART 中断上下文里被调用，不要在回调里做阻塞操作、长延时或慢速打印。
- 在回调里只做“收到新值后更新变量”这类轻量动作。
- 如果多个任务会同时读被修改的参数，建议按工程现有的并发方式处理，例如加临界区，或用“更新标志 + 主循环统一应用”的模式。
- 当前接收方式是单字节中断接收。追求低 CPU 占用时可自行改成 DMA + IDLE 收包，再把每一帧一次性喂给解析逻辑，但需要额外维护自己的串口帧层。

## 七、已知限制

- 当前解析器没有缓冲区溢出保护。上位机发送的参数名或数值如果超过缓冲区大小，会继续写越界内存，接入前应在上位机侧限制发送长度，或自行给状态机增加长度检查。
- 以 `\n` 作为一帧结束符。`\r\n` 一般也能工作，因为 `atof()` 解析到 `\r` 会停止。
- 一个空行会触发一次回调，此时 `sign` 为空、`data` 为 `0`，应用侧应忽略未知参数名。
- 每个数据包只解析一个 `float`。需要解析整数、布尔值或数组时，可以在你的回调里基于 `data` 或字符串再处理。
- 若换用非 GCC/ARMCC 编译器，`__attribute__((weak))` 可能需要替换成对应工具链的弱符号写法。

## 八、典型用途

1. 上位机在线调 PID，改完立即生效，不需要重新烧录。
2. 通过 VOFA+ 或串口助手修改限幅、目标速度、使能开关等标量参数。
3. 把参数下发逻辑统一收口，主程序只关心业务层的“参数名 -> 动作”映射。
