#define RX_Length 1000 //接收字节长度
// 共阴数字数组
// 0,1,2,3,4,5,6,7,8,9,A,B,C,D,E,F, .,全灭
u8 smg_num[] = {0xfc, 0x60, 0xda, 0xf2, 0x66, 0xb6, 0xbe, 0xe0, 0xfe, 0xf6, 0xee, 0x3e, 0x9c, 0x7a, 0x9e, 0x8e, 0x01, 0x00};
// 0.,1.,2.,3.,4.,5.,6.,7.,8.,9.,A.,B.,C.,D.,E.,F., .
u8 smg_num_1[] = {0xfd, 0x61, 0xdb, 0xf3, 0x67, 0xb7, 0xbf, 0xe1, 0xff, 0xf7, 0xef, 0x3f, 0x9d, 0x7b, 0x9f, 0x8f, 0x01};
u8 key = 0;       //定义按键初值
u8 key_event = 3; //按键事件 1:DHT11 2:RTC日期  3：RTC时间 4:波形
//按下KEY_UP进入change_mode=1模式，手动调节时钟日期和时间，再按一次退出调节模式
//LED0闪烁：dht11采集中；LED1闪烁：WAKE_UP中断处理；LED2：RTC时间工作指示灯；LED3闪烁：dht11没有工作；LED5闪烁：输出波形；LED6、7闪烁：报警
int temperature; //温度值（格式整数部分*100+小数部分）
int humidity;    //湿度值（格式整数部分*100+小数部分）
u8 LED = 0;
u16 times = 0;              //用于计数，每秒发送一次日期、时间、温湿度信息到电脑
int alarm_temperature = 38; //设置温度报警值
int alarm_humidity = 80;    //设置湿度报警值
u8 change_mode = 0;         //修改模式，change_mode=1时候修改
int mode_index = 0;
char buffer[200] = {0};
char Usart_ReceiveBuffer[RX_Length] = {0};
char USART1MESSAGE[RX_Length] = {0};
int USART_COUNTER = 0;
char END_STA = 0; //接收状态标记
char TempChar = 0;
int pwmval = 0;
u8 dir = 1;
float xx = 0; //a为角度制
//ADC变量
static u16 adcx = 0;
static u16 adcx1 = 0;
static float temp = 0;
static int boxing_val = 1; //默认波形为方波
u8 smg_wei = 0;            //数码管位选
u8 num = 0;                //数码管数值
u8 time = 0;               //时间值
u16 led_t = 0;             //led统计时间
u16 dht11_t = 0;           //dht11采样时间
u8 adc_t = 0;              //adc采样时间值
2)初始化
HAL_Init();                    	//初始化HAL库
Stm32_Clock_Init(96, 4, 2, 4);  //设置时钟,96Mhz
delay_init(96);                 //初始化延时函数
uart_init(115200);              //初始化串口115200
LED_Init();                     //初始化LED
LED_SMG_Init();                 //数码管初始化
TIM2_PWM_Init(2300 - 1, 0);     //不分频。PWM频率=96000/2300=41.7Khz
TIM3_Init(20 - 1, 9600 - 1);    //数码管2ms定时显示
usmart_dev.init(96);            //初始化USMART
KEY_Init();                     //初始化按键
BEEP_Init();                    //初始化蜂鸣器端口
Adc_Init();                     //ADC初始化
/*********************DHT11初始化*********************/
printf("NANO STM32\r\n");
printf("DHT11 TEST\r\n");
while (DHT11_Init()) //DHT11初始化
{
    printf("DHT11 Error\r\n");
    delay_ms(200);
    LED3 = !LED3; //LED3闪烁表示DHT11初始化失败
}
LED3 = 1;
printf("DHT11 OK\r\n");
/*****************************************************/
while (RTC_Init()) //RTC初始化，一定要初始化成功
{
    printf("RTC ERROR!\r\n");
    delay_ms(800);
    printf("RTC Trying...\r\n");
}
RTC_Set_WakeUp(RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0);            //配置WAKE UP中断,1秒钟中断一次
                                                              /******************************************************************************/
HAL_UART_IRQHandler(&UART1_Handler);                          //串口中断函数
HAL_UART_Receive_IT(&UART1_Handler, (uint8_t *)&TempChar, 1); //接收中断函数
/******************************************************************************/
3)被调函数
int* get_alarm_temperature()
{ //获取报警温度
    return &alarm_temperature;
}
int *get_alarm_humidity()
{ //获取报警湿度
    return &alarm_humidity;
}
RTC_DateTypeDef get_date()
{ //获取日期
    return RTC_DateStruct;
}
RTC_TimeTypeDef get_time()
{ //获取时间
    return RTC_TimeStruct;
}
u8 str_len(char *s)
{ //读取字符串长度
    int i = 0;
    while (s[i++] != '\0')
        ;
    return i;
}
/*******************************************************/
int cmd_is(char *cmd_recv, char *cmd)
{ //判断串口发送命令
    for (int i = 0;; i++)
    {
        if (cmd[i] != 0)
        {
            if (cmd_recv[i] != cmd[i])
                return 0;
        }
        else
            return 1;
    }
}
/*******************************************************/
//库函数 int atoi(const char *str) 把参数 str 所指向的字符串转换为一个整数（类型为 int 型）。
int get_value_int(char *cmd_recv)
{ //通过判断“=”读取输入命令值的大小
    for (int i = 0;; i++)
    {
        if (cmd_recv[i] == '=')
        {
            return (atoi(cmd_recv + i + 1));
        }
    }
}
int get_value_date(char *cmd_recv, uint8_t *year, uint8_t *mounth, uint8_t *date, uint8_t *week)
{ //从串口读取日期
    for (int i = 0;; i++)
    {
        if (cmd_recv[i] == '=')
        {
            char *datelp = cmd_recv + i + 1;
            char buf[10] = {0};
            int index = 0;
            int date_list_index = 0;
            for (int j = 0; datelp[j]; j++)
            {
                buf[index++] = datelp[j];
                if (datelp[j] == ',' || datelp[j] == ';')
                {
                    if (date_list_index == 0)
                        *year = (uint8_t)atoi(buf);
                    if (date_list_index == 1)
                        *mounth = (uint8_t)atoi(buf);
                    if (date_list_index == 2)
                        *date = (uint8_t)atoi(buf);
                    if (date_list_index == 3)
                        *week = (uint8_t)atoi(buf);
                    date_list_index++;
                    index = 0;
                    for (int k = 0; k < 8; k++)
                    {
                        buf[k] = 0;
                    }
                    if (datelp[j] == ';')
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
    }
}
int get_value_time(char *cmd_recv, uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *ampm)
{ //从串口读取输入时间的值
    for (int i = 0;; i++)
    {
        if (cmd_recv[i] == '=')
        {
            char *datelp = cmd_recv + i + 1;
            char buf[10] = {0};
            int index = 0;
            int date_list_index = 0;
            for (int j = 0; datelp[j]; j++)
            {
                buf[index++] = datelp[j];
                if (datelp[j] == ',' || datelp[j] == ';')
                {
                    if (date_list_index == 0)
                        *hour = (uint8_t)atoi(buf);
                    if (date_list_index == 1)
                        *minute = (uint8_t)atoi(buf);
                    if (date_list_index == 2)
                        *second = (uint8_t)atoi(buf);
                    if (date_list_index == 3)
                        *ampm = (uint8_t)atoi(buf);
                    date_list_index++;
                    index = 0;
                    for (int k = 0; k < 8; k++)
                    {
                        buf[k] = 0;
                    }
                    if (datelp[j] == ';')
                    {
                        return 1;
                    }
                }
            }
            return 0;
        }
    }
}
4)按键修改操作
if (change_mode == 0) //0模式，正常读取日期和时间
{
    HAL_RTC_GetDate(&RTC_Handler, &RTC_DateStruct, RTC_FORMAT_BIN);
    RTC_DateStruct = old_RTC_DateStruct;
    HAL_RTC_GetTime(&RTC_Handler, &RTC_TimeStruct, RTC_FORMAT_BIN);
    RTC_TimeStruct = old_RTC_TimeStruct;
}
key = KEY_Scan(0); //得到键值
if (key)
{
    sprintf(buffer, "%d key down;\n", key);
    HAL_UART_Transmit(&UART1_Handler, (uint8_t *)buffer, str_len(buffer), 1000);
    if (key == WKUP_PRES)
    {
        if (change_mode == 1)
        {
            RTC_DateStruct = old_RTC_DateStruct;
            RTC_Set_Date(RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date,
                         RTC_DateStruct.WeekDay);
            RTC_TimeStruct = old_RTC_TimeStruct;
            RTC_Set_Time(RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes,
                         RTC_TimeStruct.Seconds, RTC_TimeStruct.TimeFormat);
        }
        change_mode = !change_mode;
    }
    if (change_mode == 0)
    {
        switch (key)
        {
        case KEY0_PRES: //KEY0用于在显示波形的时候切换到三角波
            if (key_event == 4)
                boxing_val = 0;
            break;
        case KEY1_PRES: //KEY1用于切换到温湿度值、日期、时间、波形
            key_event++;
            if (key_event == 5)
                key_event = 1;
            break;
        case KEY2_PRES: //KEY2用于在显示波形的时候切换到正弦波
            if (key_event == 4)
                boxing_val = 2;
            break;
        default:;
        }
    }
    else //进入修改模式
    {
        switch (key_event)
        {
        case 1: //温湿度模块
            switch (key)
            {
            case KEY0_PRES:
            {
                break;
            }
            case KEY1_PRES:
            {
                break;
            }
            case KEY2_PRES:
                break;
            }
            break;
        case 2: //RTC日期修改
            switch (key)
            {
            case KEY0_PRES: //-1
            {
                switch (mode_index)
                {
                case 0:
                    old_RTC_DateStruct.Year--;
                    break;
                case 1:
                    RTC_DateStruct.Month = old_RTC_DateStruct.Month--;
                    if (old_RTC_DateStruct.Month == 0)
                    {
                        old_RTC_DateStruct.Month = 12;
                    }
                    break;
                case 2:
                    RTC_DateStruct.Date = old_RTC_DateStruct.Date--;
                    if (old_RTC_DateStruct.Date == 0)
                    {
                        old_RTC_DateStruct.Date = 31;
                    }
                    break;
                }
                break;
            }
            case KEY1_PRES: //切换位置
            {
                mode_index++;
                if (mode_index == 3)
                {
                    mode_index = 0;
                }
                break;
            }
            case KEY2_PRES: //+1
            {
                switch (mode_index)
                {
                case 0:
                    old_RTC_DateStruct.Year++;
                    break;
                case 1:
                    RTC_DateStruct.Month = old_RTC_DateStruct.Month++;
                    if (old_RTC_DateStruct.Month == 13)
                    {
                        old_RTC_DateStruct.Month = 1;
                    }
                    break;
                case 2:
                    RTC_DateStruct.Date = old_RTC_DateStruct.Date++;
                    if (old_RTC_DateStruct.Date == 31)
                    {
                        old_RTC_DateStruct.Date = 1;
                    }
                    break;
                }
                break;
            }
            }
            switch (mode_index)
            {
            case 0:
                RTC_DateStruct.Year = old_RTC_DateStruct.Year;
                RTC_DateStruct.Month = 100;
                RTC_DateStruct.Date = 100;
                break;
            case 1:
                RTC_DateStruct.Year = 100;
                RTC_DateStruct.Month = old_RTC_DateStruct.Month;
                RTC_DateStruct.Date = 100;
                break;
            case 2:
                RTC_DateStruct.Year = 100;
                RTC_DateStruct.Month = 100;
                RTC_DateStruct.Date = old_RTC_DateStruct.Date;
                break;
            }
            break;
        case 3: //RTC时间修改
            switch (key)
            {
            case KEY0_PRES: //-1
            {
                switch (mode_index)
                {
                case 0:
                    old_RTC_TimeStruct.Hours--;
                    if (old_RTC_TimeStruct.Hours == 0)
                    {
                        old_RTC_TimeStruct.Hours = 24;
                    }
                    break;
                case 1:
                    RTC_TimeStruct.Minutes = old_RTC_TimeStruct.Minutes--;
                    if (old_RTC_TimeStruct.Minutes == 0)
                    {
                        old_RTC_TimeStruct.Minutes = 59;
                    }
                    break;
                case 2:
                    RTC_TimeStruct.Seconds = old_RTC_TimeStruct.Seconds--;
                    if (old_RTC_TimeStruct.Seconds == 0)
                    {
                        old_RTC_TimeStruct.Seconds = 59;
                    }
                    break;
                }
                break;
            }
            case KEY1_PRES: //切换位置
            {
                mode_index++;
                if (mode_index == 3)
                {
                    mode_index = 0;
                }
                break;
            }
            case KEY2_PRES: //+1
            {
                switch (mode_index)
                {
                case 0:
                    old_RTC_TimeStruct.Hours++;
                    if (old_RTC_TimeStruct.Hours == 24)
                    {
                        old_RTC_TimeStruct.Hours = 0;
                    }
                    break;
                case 1:
                    RTC_TimeStruct.Minutes = old_RTC_TimeStruct.Minutes++;
                    if (old_RTC_TimeStruct.Minutes == 60)
                    {
                        old_RTC_TimeStruct.Minutes = 0;
                    }
                    break;
                case 2:
                    RTC_TimeStruct.Seconds = old_RTC_TimeStruct.Seconds++;
                    if (old_RTC_TimeStruct.Seconds == 60)
                    {
                        old_RTC_TimeStruct.Seconds = 0;
                    }
                    break;
                }
                break;
            }
            }
            switch (mode_index)
            {
            case 0:
                RTC_TimeStruct.Hours = old_RTC_TimeStruct.Hours;
                RTC_TimeStruct.Minutes = 100;
                RTC_TimeStruct.Seconds = 100;
                break;
            case 1:
                RTC_TimeStruct.Hours = 100;
                RTC_TimeStruct.Minutes = old_RTC_TimeStruct.Minutes;
                RTC_TimeStruct.Seconds = 100;
                break;
            case 2:
                RTC_TimeStruct.Hours = 100;
                RTC_TimeStruct.Minutes = 100;
                RTC_TimeStruct.Seconds = old_RTC_TimeStruct.Seconds;
                break;
            };
            break;
        default:;
        }
    }
}
/******************************************************************************************/
5)波形切换
/******************************************************************************************/
			if(key_event==4)
{
    if (boxing_val == 0) /***********三角波********/
    {
        if (dir)
            pwmval = pwmval + 1;
        else
            pwmval = 0;
        if (pwmval > 600)
            dir = 0;
        if (pwmval == 0)
            dir = 1;
        TIM_SetTIM2Compare1(pwmval); //修改比较值，修改占空比
        adcx = Get_Adc_Average(ADC_CHANNEL_9, 50);
        printf("电压幅值为：%d\r\n", adcx);
    }
    if (boxing_val == 1) /*********方波************/
    {
        pwmval = 2300;
        TIM_SetTIM2Compare1(pwmval);
        adcx = Get_Adc_Average(ADC_CHANNEL_9, 50);
        printf("ADC初值为：%d\r\n", adcx);
        pwmval = 0;
        TIM_SetTIM2Compare1(pwmval);
        adcx = Get_Adc_Average(ADC_CHANNEL_9, 50);
        printf("ADC初值为：%d\r\n", adcx);
    }
    if (boxing_val == 2) /**********正弦波*********/
    {
        xx = xx + 0.1;
        pwmval = 1150 + 1150 * sin(xx);
        TIM_SetTIM2Compare1(pwmval);
        adcx = Get_Adc_Average(ADC_CHANNEL_9, 50);
        printf("ADC初值为：%d\r\n", adcx);
    }
}
/******************************************************************************************/
6)串口通讯
/******************************************************************************************/  
		if(times==500)//times每加500次(1s)读取一次温度、湿度、日期、时间
{
    times = 0;
    DHT11_Read_Data(&temperature, &humidity);
    printf("温度:");
    sprintf(buffer, "%d%c%dC\r\n", temperature / 100, '.', temperature % 100 / 10);
    HAL_UART_Transmit(&UART1_Handler, (uint8_t *)buffer, str_len(buffer), 1000);
    printf("湿度:");
    sprintf(buffer, "%d%c%drh\r\n", humidity / 100, '.', humidity % 100 / 10);
    HAL_UART_Transmit(&UART1_Handler, (uint8_t *)buffer, str_len(buffer), 1000);
    HAL_RTC_GetDate(&RTC_Handler, &RTC_DateStructure, RTC_FORMAT_BIN); //读取当前RTC日期
    HAL_RTC_GetTime(&RTC_Handler, &RTC_TimeStructure, RTC_FORMAT_BIN); //读取当前RTC时间
    printf("日期：");
    sprintf(buffer, "%d.%d.%d\r\n", RTC_DateStructure.Year, RTC_DateStructure.Month,
            RTC_DateStructure.Date);
    HAL_UART_Transmit(&UART1_Handler, (uint8_t *)buffer, str_len(buffer), 1000);
    printf("星期");
    sprintf(buffer, "%d\r\n", RTC_DateStructure.WeekDay);
    HAL_UART_Transmit(&UART1_Handler, (uint8_t *)buffer, str_len(buffer), 1000);
    printf("时间：");
    sprintf(buffer, "%d:%d:%d\r\n", RTC_TimeStructure.Hours, RTC_TimeStructure.Minutes,
            RTC_TimeStructure.Seconds);
    HAL_UART_Transmit(&UART1_Handler, (uint8_t *)buffer, str_len(buffer), 1000);
    printf("\r嵌入式实习作品\r\n——陈雨\r\n");
}
7)温度异常报警
/***********************************************************************************************/
		  if((temperature>alarm_temperature*100)||(humidity>alarm_humidity*100))
{
    LED = 0xc0; //LED6、7亮
    HAL_GPIO_WritePin(GPIOC, LED, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET); //BEEP引脚拉低，响，等同BEEP=0;
    delay_ms(100);
    LED = 0xc0; //LED6、7灭
    HAL_GPIO_WritePin(GPIOC, LED, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET); //BEEP引脚拉高，不响，等同BEEP=1;
    delay_ms(100);
}
/***********************************************************************************************/
8)定时器中断回调函数
//回调函数，定时器中断服务函数调用
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == (&TIM3_Handler))
    {
        times++;
        if (key_event == 1)
        {
            LED5 = 1;
            dht11_t++;
            if (dht11_t == 500) //DHT11 1S采样
            {
                dht11_t = 0;
                DHT11_Read_Data(&temperature, &humidity); //读取温湿度值
            }
            switch (smg_wei)
            {
            case 0:
                num = smg_num[temperature / 1000];
                break; //温度整数部分高位值
            case 1:
                num = smg_num_1[temperature % 1000 / 100];
                break; //温度整数部分低位值
            case 2:
                num = smg_num[temperature % 100 / 10];
                break; //温度小数部分第一位值
            case 3:
                break;
            case 4:
                break;
            case 5:
                num = smg_num[humidity / 1000];
                break; //湿度整数部分高位值
            case 6:
                num = smg_num_1[humidity % 1000 / 100];
                break; //湿度整数部分低位值
            case 7:
                num = smg_num[humidity % 100 / 10];
                break; //湿度小数部分第一位值
            }
            LED_Write_Data(num, smg_wei); //写数据到数码管
            LED_Refresh();                //更新显示
            smg_wei++;
            if (smg_wei == 3)
                smg_wei = 5;
            if (smg_wei == 8)
                smg_wei = 0;
            led_t++;
            if (led_t == 250) //LED0每500ms闪烁
            {
                led_t = 0;
                LED0 = !LED0;
            }
        }
        if (key_event == 2)
        {
            LED5 = 1;
            if (change_mode == 1)
            {
                HAL_RTC_GetTime(&RTC_Handler, &RTC_TimeStruct, RTC_FORMAT_BIN);
                RTC_DateTypeDef RTC_DateStruct = get_date();
                switch (smg_wei)
                {
                case 0:
                    num = RTC_DateStruct.Year == 100 ? 0x02 : smg_num[RTC_DateStruct.Year / 10];
                    break; //年
                case 1:
                    num = RTC_DateStruct.Year == 100 ? 0x02 : smg_num[RTC_DateStruct.Year % 10];
                    break;
                case 2:
                    num = 0x02;
                    break;
                case 5:
                    num = 0x02;
                    break;
                case 3:
                    num = RTC_DateStruct.Month == 100 ? 0x02 : smg_num[RTC_DateStruct.Month / 10];
                    break; //月
                case 4:
                    num = RTC_DateStruct.Month == 100 ? 0x02 : smg_num[RTC_DateStruct.Month % 10];
                    break;
                case 6:
                    num = RTC_DateStruct.Date == 100 ? 0x02 : smg_num[RTC_DateStruct.Date / 10];
                    break; //日
                case 7:
                    num = RTC_DateStruct.Date == 100 ? 0x02 : smg_num[RTC_DateStruct.Date % 10];
                    break;
                }
                LED_Write_Data(num, smg_wei); //写数据到数码管
                LED_Refresh();                //更新显示
                smg_wei++;
                if (smg_wei == 8)
                    smg_wei = 0;
            }
            else
            {
                HAL_RTC_GetDate(&RTC_Handler, &RTC_DateStruct, RTC_FORMAT_BIN);
                switch (smg_wei)
                {
                case 0:
                    num = RTC_DateStruct.Year == 100 ? 0x02 : smg_num[RTC_DateStruct.Year / 10];
                    break; //年
                case 1:
                    num = RTC_DateStruct.Year == 100 ? 0x02 : smg_num[RTC_DateStruct.Year % 10];
                    break;
                case 2:
                    num = 0x02;
                    break;
                case 5:
                    num = 0x02;
                    break;
                case 3:
                    num = RTC_DateStruct.Month == 100 ? 0x02 : smg_num[RTC_DateStruct.Month / 10];
                    break; //月
                case 4:
                    num = RTC_DateStruct.Month == 100 ? 0x02 : smg_num[RTC_DateStruct.Month % 10];
                    break;
                case 6:
                    num = RTC_DateStruct.Date == 100 ? 0x02 : smg_num[RTC_DateStruct.Date / 10];
                    break; //日
                case 7:
                    num = RTC_DateStruct.Date == 100 ? 0x02 : smg_num[RTC_DateStruct.Date % 10];
                    break;
                }
                LED_Write_Data(num, smg_wei); //写数据到数码管
                LED_Refresh();                //更新显示
                smg_wei++;
                if (smg_wei == 8)
                    smg_wei = 0;
            }
        }
        if (key_event == 3)
        {
            LED5 = 1;
            if (change_mode == 1)
            {
                HAL_RTC_GetDate(&RTC_Handler, &RTC_DateStruct, RTC_FORMAT_BIN);
                RTC_TimeTypeDef RTC_TimeStruct;
                RTC_TimeStruct = get_time();
                switch (smg_wei)
                {
                case 0:
                    num = RTC_TimeStruct.Hours == 100 ? 0x02 : smg_num[RTC_TimeStruct.Hours / 10];
                    break; //时
                case 1:
                    num = RTC_TimeStruct.Hours == 100 ? 0x02 : smg_num[RTC_TimeStruct.Hours % 10];
                    break;
                case 2:
                    num = 0x02;
                    break;
                case 5:
                    num = 0x02;
                    break;
                case 3:
                    num = RTC_TimeStruct.Minutes == 100 ? 0x02 : smg_num[RTC_TimeStruct.Minutes / 10];
                    break; //分
                case 4:
                    num = RTC_TimeStruct.Minutes == 100 ? 0x02 : smg_num[RTC_TimeStruct.Minutes % 10];
                    break;
                case 6:
                    num = RTC_TimeStruct.Seconds == 100 ? 0x02 : smg_num[RTC_TimeStruct.Seconds / 10];
                    break; //秒
                case 7:
                    num = RTC_TimeStruct.Seconds == 100 ? 0x02 : smg_num[RTC_TimeStruct.Seconds % 10];
                    break;
                }
                LED_Write_Data(num, smg_wei); //写数据到数码管
                LED_Refresh();                //更新显示
                smg_wei++;
                if (smg_wei == 8)
                    smg_wei = 0;
            }
            else
            {
                HAL_RTC_GetTime(&RTC_Handler, &RTC_TimeStruct, RTC_FORMAT_BIN);
                switch (smg_wei)
                {
                case 0:
                    num = RTC_TimeStruct.Hours == 100 ? 0x02 : smg_num[RTC_TimeStruct.Hours / 10];
                    break; //时
                case 1:
                    num = RTC_TimeStruct.Hours == 100 ? 0x02 : smg_num[RTC_TimeStruct.Hours % 10];
                    break;
                case 2:
                    num = 0x02;
                    break;
                case 5:
                    num = 0x02;
                    break;
                case 3:
                    num = RTC_TimeStruct.Minutes == 100 ? 0x02 : smg_num[RTC_TimeStruct.Minutes / 10];
                    break; //分
                case 4:
                    num = RTC_TimeStruct.Minutes == 100 ? 0x02 : smg_num[RTC_TimeStruct.Minutes % 10];
                    break;
                case 6:
                    num = RTC_TimeStruct.Seconds == 100 ? 0x02 : smg_num[RTC_TimeStruct.Seconds / 10];
                    break; //秒
                case 7:
                    num = RTC_TimeStruct.Seconds == 100 ? 0x02 : smg_num[RTC_TimeStruct.Seconds % 10];
                    break;
                }
                if (time != RTC_TimeStructure.Seconds) //LED0每秒闪烁
                {
                    time = RTC_TimeStructure.Seconds;
                    LED2 = !LED2;
                }
                LED_Write_Data(num, smg_wei); //写数据到数码管
                LED_Refresh();                //更新显示
                smg_wei++;
                if (smg_wei == 8)
                    smg_wei = 0;
            }
        }
        if (key_event == 4)
        {
            adc_t++;
            if (adc_t == 1) //ADC 2ms采样
            {
                adc_t = 0;
                adcx = Get_Adc_Average(ADC_CHANNEL_9, 50); //ADC原始值
                temp = (float)adcx * (3.3 / 4096);         //ADC电压值
                adcx1 = temp;
                temp -= adcx1;
                temp *= 1000;
            }
            switch (smg_wei)
            {
            case 0: //ADC采样值
                if (adcx > 1000)
                    num = smg_num[adcx / 1000];
                else
                    num = 0x00;
                break;
            case 1:
                if (adcx > 100)
                    num = smg_num[adcx % 1000 / 100];
                else
                    num = 0x00;
                break;
            case 2:
                if (adcx > 10)
                    num = smg_num[adcx % 100 / 10];
                else
                    num = 0x00;
                break;
            case 3:
                num = smg_num[adcx % 10];
                break;
            case 4:
                num = 0x00;
                break;
            case 5:
                num = smg_num[adcx1] | 0x01;
                break; //ADC换算后的电压值
            case 6:
                num = smg_num[(u16)temp / 100];
                break;
            case 7:
                num = smg_num[(u16)temp % 100 / 10];
                break;
            }
            LED_Write_Data(num, smg_wei); //写数据到数码管
            LED_Refresh();                //更新显示
            smg_wei++;
            if (smg_wei == 8)
                smg_wei = 0;
            led_t++;
            if (led_t == 250) //500ms闪烁
            {
                led_t = 0;
                LED5 = !LED5;
            }
        }
    }
9)串口中断及回调函数
/*************************以下是串口中断函数及中断回调函数********************************/
void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&UART1_Handler); //串口中断
    HAL_UART_Receive_IT(&UART1_Handler, (uint8_t *)&TempChar, 1);
    接收中断
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef * huart) //串口回调函数
{
    if (huart->Instance == USART1)
    {
        if (!END_STA) //接收未完成
        {
            if (TempChar != ';')
            {
                Usart_ReceiveBuffer[USART_COUNTER++] = TempChar;
            }
            else
            { //0xxd 0x00 结束
                Usart_ReceiveBuffer[USART_COUNTER++] = TempChar;
                if (Usart_ReceiveBuffer[USART_COUNTER] == 0)
                {
                    END_STA = 1;
                } //接收完成
            }
        }
        if (END_STA)
        { //0x00 进入
            int temp = USART_COUNTER;
            Usart_ReceiveBuffer[USART_COUNTER++] = TempChar;
            END_STA = 0;
            USART_COUNTER = 0;
            for (int i = 0; i < temp; i++)
                USART1MESSAGE[i] = Usart_ReceiveBuffer[i];
            for (int i = 0; i < RX_Length; i++)
                Usart_ReceiveBuffer[i] = 0;
            char *cmd_recv = USART1MESSAGE;
            if (cmd_is(cmd_recv, "alarm_temperature"))
            {
                alarm_temperature = get_value_int(cmd_recv);
            }
            else if (cmd_is(cmd_recv, "alarm_humidity"))
            {
                alarm_humidity = get_value_int(cmd_recv);
            }
            else if (cmd_is(cmd_recv, "date"))
            {
                uint8_t y, m, d, w;
                if (get_value_date(cmd_recv, &y, &m, &d, &w))
                {
                    RTC_Set_Date(y, m, d, w);
                }
            }
            else if (cmd_is(cmd_recv, "time"))
            {
                uint8_t h, m, s, ampm;
                //RTC_HOURFORMAT12_AM:(uint8_t)0x00;
                //RTC_HOURFORMAT12_PM:(uint8_t)0x40
                if (get_value_time(cmd_recv, &h, &m, &s, &ampm))
                {
                    RTC_Set_Time(h, m, s, ampm);
                }
            }
            HAL_UART_Transmit_IT(&UART1_Handler, (uint8_t *)USART1MESSAGE, temp);
        }
    }
    HAL_UART_Receive_IT(&UART1_Handler, (uint8_t *)&TempChar, 1);
}
DHT11温湿度模块读取函数
//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//buf[0];湿度整数部分;buf[1]；湿度小数部分；buf[2];温度整数部分；buf[3];温度小数部分
//返回值：0,正常;1,读取失败
int DHT11_Read_Data(int *temp, int *humi)
{
    u8 buf[5];
    u8 i;
    DHT11_Rst();
    if (DHT11_Check() == 0)
    {
        for (i = 0; i < 5; i++) //读取40位数据
        {
            buf[i] = DHT11_Read_Byte();
        }
        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0] * 100 + buf[1]; //湿度部分（格式整数部分*100+小数部分）
            *temp = buf[2] * 100 + buf[3]; //温度部分（格式整数部分*100+小数部分）
        }
    }
    else
        return 1;
    return 0;
}
ADC采集读取函数
//获取指定通道的转换值，取times次,然后平均
//times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc_Average(u32 ch, u8 times)
{
    u32 temp_val = 0;
    u8 t;
    for (t = 0; t < times; t++)
    {
        temp_val += Get_Adc(ch);
    }
    return temp_val / times;
}