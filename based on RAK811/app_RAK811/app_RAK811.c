#include "rui.h"
#include "board.h"

//join cnt
#define JOIN_MAX_CNT 6
static uint8_t JoinCnt=0;
static bool IsTxDone = false;   //Entry sleep flag
static RUI_DEVICE_STATUS_T app_device_status; //record device status 
static RUI_LORA_STATUS_T app_lora_status; //record lora status 

/*******************************************************************************************
 * The BSP user functions.
 * 
 * *****************************************************************************************/ 

const uint8_t level[2]={0,1};
#define low     &level[0]
#define high    &level[1]

RUI_I2C_ST I2c_1;
volatile static bool autosend_flag = false;    //auto send flag



void rui_lora_autosend_callback(void)  //auto_send timeout event callback
{
    autosend_flag = true;
}

void bsp_i2c_init(void)
{
    I2c_1.INSTANCE_ID = 1;
    I2c_1.PIN_SDA = I2C_SDA;
    I2c_1.PIN_SCL = I2C_SCL;
    I2c_1.FREQUENCY = RUI_I2C_FREQ_400K;

    rui_i2c_init(&I2c_1);

}
void bsp_init(void)
{
    bsp_i2c_init();
}

void app_loop(void)
{
    rui_lora_get_status(&app_lora_status);
    if(app_lora_status.IsJoined)  //if LoRaWAN is joined
    {
       /*****************************************************************************
             * user app loop code
        *****************************************************************************/
    }	
}

/*******************************************************************************************
 * LoRaMac callback functions
 * * void LoRaReceive_callback(RUI_RECEIVE_T* Receive_datapackage);//LoRaWAN callback if receive data 
 * * void LoRaP2PReceive_callback(RUI_LORAP2P_RECEIVE_T *Receive_P2Pdatapackage);//LoRaP2P callback if receive data 
 * * void LoRaWANJoined_callback(uint32_t status);//LoRaWAN callback after join server request
 * * void LoRaWANSendsucceed_callback(RUI_MCPS_T status);//LoRaWAN call back after send data complete
 * *****************************************************************************************/  
void LoRaReceive_callback(RUI_RECEIVE_T* Receive_datapackage)
{
    char hex_str[3] = {0}; 
    RUI_LOG_PRINTF("at+recv=%d,%d,%d,%d:", Receive_datapackage->Port, Receive_datapackage->Rssi, Receive_datapackage->Snr, Receive_datapackage->BufferSize);   
    
    if ((Receive_datapackage->Buffer != NULL) && Receive_datapackage->BufferSize) {
        for (int i = 0; i < Receive_datapackage->BufferSize; i++) {
            sprintf(hex_str, "%02x", Receive_datapackage->Buffer[i]);
            RUI_LOG_PRINTF("%s", hex_str); 
        }
    }
    RUI_LOG_PRINTF("\r\n");
}
void LoRaP2PReceive_callback(RUI_LORAP2P_RECEIVE_T *Receive_P2Pdatapackage)
{
    char hex_str[3]={0};
    RUI_LOG_PRINTF("at+recv=%d,%d,%d:", Receive_P2Pdatapackage -> Rssi, Receive_P2Pdatapackage -> Snr, Receive_P2Pdatapackage -> BufferSize); 
    for(int i=0;i < Receive_P2Pdatapackage -> BufferSize; i++)
    {
        sprintf(hex_str, "%02X", Receive_P2Pdatapackage -> Buffer[i]);
        RUI_LOG_PRINTF("%s",hex_str);
    }
    RUI_LOG_PRINTF("\r\n");    
}
void LoRaWANJoined_callback(uint32_t status)
{
    static int8_t dr; 
    if(status)  //Join Success
    {
        JoinCnt = 0;
        RUI_LOG_PRINTF("[LoRa]:Joined Successed!\r\n");
        rui_device_get_status(&app_device_status);//The query gets the current device status 
        if(app_device_status.autosend_status)
        {
            autosend_flag = true;  //set autosend_flag after join LoRaWAN succeeded 
        }       
    }else 
    {        
        if(JoinCnt<JOIN_MAX_CNT) // Join was not successful. Try to join again
        {
            JoinCnt++;
            RUI_LOG_PRINTF("[LoRa]:Join retry Cnt:%d\n",JoinCnt);
            rui_lora_get_status(&app_lora_status);
            if(app_lora_status.lora_dr > 0)
            {
                app_lora_status.lora_dr -= 1;
            }else app_lora_status.lora_dr = 0;
            rui_lora_set_dr(app_lora_status.lora_dr);
            rui_lora_join();                    
        }
        else   //Join failed
        {
            RUI_LOG_PRINTF("[LoRa]:Joined Failed! \r\n"); 
            JoinCnt=0;      
        }          
    }    
}
void LoRaWANSendsucceed_callback(RUI_MCPS_T status)
{
    switch( status )
    {
        case RUI_MCPS_UNCONFIRMED:
        {
            RUI_LOG_PRINTF("[LoRa]: Unconfirm data send OK\r\n");
            break;
        }
        case RUI_MCPS_CONFIRMED:
        {
            RUI_LOG_PRINTF("[LoRa]: Confirm data send OK\r\n");

            break;
        }
        case RUI_MCPS_PROPRIETARY:
        {
            RUI_LOG_PRINTF("[LoRa]: MCPS_PROPRIETARY\r\n");
            break;
        }
        case RUI_MCPS_MULTICAST:
        {
            RUI_LOG_PRINTF("[LoRa]: MCPS_PROPRIETARY\r\n");
            break;           
        }
        default:             
            break;
    }     
    rui_device_get_status(&app_device_status);//The query gets the current device status 
    if(app_device_status.autosend_status)
    {
        rui_lora_set_send_interval(1,app_lora_status.lorasend_interval);  //start autosend_timer after send success
        IsTxDone=true;  //Sleep flag set true
    } 
}

/*******************************************************************************************
 * The RUI is used to receive data from uart.
 * 
 * *****************************************************************************************/ 
void rui_uart_recv(RUI_UART_DEF uart_def, uint8_t *pdata, uint16_t len)
{
    switch(uart_def)
    {
        case RUI_UART1://process code if RUI_UART1 work at RUI_UART_UNVARNISHED
            rui_lora_send(8,pdata,len);  
            break;
        case RUI_UART3://process code if RUI_UART3 received data ,the len is always 1
            /*****************************************************************************
             * user code 
            ******************************************************************************/
            break;
        default:break;
    }
}

/*******************************************************************************************
 * the app_main function
 * *****************************************************************************************/ 
void main(void)
{
    static bool autosendtemp_status;

    rui_init();
    bsp_init();

/*******************************************************************************************
 * Register LoRaMac callback function
 * 
 * *****************************************************************************************/
    rui_lora_register_recv_callback(LoRaReceive_callback);  
    rui_lorap2p_register_recv_callback(LoRaP2PReceive_callback);
    rui_lorajoin_register_callback(LoRaWANJoined_callback); 
    rui_lorasend_complete_register_callback(LoRaWANSendsucceed_callback); 


/*******************************************************************************************    
 *The query gets the current device and lora status 
 * 
 * *****************************************************************************************/    
    rui_device_get_status(&app_device_status);
    rui_lora_get_status(&app_lora_status);
	
	if(app_device_status.autosend_status)RUI_LOG_PRINTF("autosend_interval: %us\r\n", app_lora_status.lorasend_interval);
/*******************************************************************************************    
 *Init OK ,print board status and auto join LoRaWAN
 * 
 * *****************************************************************************************/  
    switch(app_device_status.uart_mode)
    {
        case RUI_UART_NORAMAL: RUI_LOG_PRINTF("Initialization OK,AT Uart work mode:normal mode, "); 
            break;
        case RUI_UART_UNVARNISHED:RUI_LOG_PRINTF("Initialization OK,AT Uart work mode:unvarnished transmit mode, ");
            break;   
    }   

    switch(app_lora_status.work_mode)
	{
		case RUI_LORAWAN:
            if(app_lora_status.join_mode == RUI_OTAA)
            {
                switch(app_lora_status.class_status)
                {
                    case RUI_CLASS_A:RUI_LOG_PRINTF("Current work_mode:LoRaWAN, join_mode:OTAA, Class: A\r\n");
                        break;
                    case RUI_CLASS_B:RUI_LOG_PRINTF("Current work_mode:LoRaWAN, join_mode:OTAA, Class: B\r\n");
                        break;
                    case RUI_CLASS_C:RUI_LOG_PRINTF("Current work_mode:LoRaWAN, join_mode:OTAA, Class: C\r\n");
                        break;
                    default:break;
                }                
                rui_lora_join();  //join LoRaWAN by OTAA mode
            }else if(app_lora_status.join_mode == RUI_ABP)
            {
                switch(app_lora_status.class_status)
                {
                    case RUI_CLASS_A:RUI_LOG_PRINTF("Current work_mode:LoRaWAN, join_mode:ABP, Class: A\r\n");
                        break;
                    case RUI_CLASS_B:RUI_LOG_PRINTF("Current work_mode:LoRaWAN, join_mode:ABP, Class: B\r\n");
                        break;
                    case RUI_CLASS_C:RUI_LOG_PRINTF("Current work_mode:LoRaWAN, join_mode:ABP, Class: C\r\n");
                        break;
                    default:break;
                }
                if(rui_lora_join() == 0)//join LoRaWAN by ABP mode
                {
                    LoRaWANJoined_callback(1);  //ABP mode join success
                }
            }
			break;
		case RUI_P2P:RUI_LOG_PRINTF("Current work_mode:P2P\r\n");
			break;
		default: break;
	}   
  
    while(1)
    {       
        rui_device_get_status(&app_device_status);//The query gets the current device status 
        rui_lora_get_status(&app_lora_status);//The query gets the current lora status 
        rui_running();
        switch(app_lora_status.work_mode)
        {
            case RUI_LORAWAN:
                if(autosendtemp_status != app_device_status.autosend_status) 
                {
                    autosendtemp_status = app_device_status.autosend_status;
                    if(autosendtemp_status == false)
                    {
                        autosendtemp_status = app_device_status.autosend_status;
                        rui_lora_set_send_interval(0,0);  //stop auto send data 
                        autosend_flag=false;
                    }else
                    {
                        autosend_flag=true;    
                    }          
                }

                if(IsTxDone)
                {
                    IsTxDone=false;   
                    rui_device_sleep(1);               
                }  

                app_loop();  

                break;
            case RUI_P2P:
                /*************************************************************************************
                 * user code at LoRa P2P mode
                *************************************************************************************/
                break;
            default :break;
        }
    }
}