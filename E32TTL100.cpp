/**
 * E32-TTL-100 Transceiver Interface
 *
 * @author Bob Chen (bob-0505@gotmail.com)
 * @Followed By Vipul Garg (vipul.bigevil@gmail.com)
 * @date 6 January 2019
 * https://github.com/Bob0505/E32-100
 * Analog Pin Mapping
 * A0 = 14;
 * A1 = 15;
 * A2 = 16;
 * A3 = 17;
 * A4 = 18;
 * A5 = 19;
 * A6 = 20;
 * A7 = 21;
 */
#if defined(ARDUINO) && ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include "E32TTL100.h"

E32TTL100::E32TTL100(uint8_t M0, uint8_t M1, uint8_t AUX, uint8_t addr_h, uint8_t addr_l):
    
    M0_PIN(M0),
    M1_PIN(M1),
    AUX_PIN(AUX),
    ADDR_H(addr_h),
    ADDR_L(addr_l)
{}

void E32TTL100::begin(Stream *ser,Stream *ds){
    //*serial = SoftwareSerial(RX, TX);
    //serial->begin(9600);
    //while (!serial) ;
    serial = ser;
    debugSerial=ds;
    ds->println("YO I HAve Done it");
    RET_STATUS STATUS = RET_SUCCESS;
    struct CFGstruct CFG;
    struct MVerstruct MVer;
    
    pinMode(M0_PIN, OUTPUT);
    pinMode(M1_PIN, OUTPUT);
    pinMode(AUX_PIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    
    //serial->begin(9600);
    //Serial.begin(9600);
    
    STATUS = SleepModeCmd(R_CFG, (void* )&CFG);
    STATUS = SettingModule(&CFG);
    
    STATUS = SleepModeCmd(R_MODULE_VERSION, (void* )&MVer);
    
    // Mode 0 | normal operation
    SwitchMode(MODE_0_NORMAL);
    
    //self-check initialization.
    WaitAUX_H();
    delay(10);
    
    if(STATUS == RET_SUCCESS);
    debugSerial->println("Setup init OK!!");
}

//=== AUX ===========================================+

bool E32TTL100::ReadAUX()
{
    int val = analogRead(AUX_PIN);
    
    if(val<50)
    {
        AUX_HL = LOW;
    }else {
        AUX_HL = HIGH;
    }
    
    return AUX_HL;
}

//return default status
RET_STATUS E32TTL100::WaitAUX_H(void)
{
    RET_STATUS STATUS = RET_SUCCESS;
    
    uint8_t cnt = 0;
    uint8_t data_buf[100], data_len;
    
    while((ReadAUX()==LOW) && (cnt++<TIME_OUT_CNT))
    {
        //Serial.print(".");
        delay(100);
    }
    
    if(cnt==0)
    {
    }
    else if(cnt>=TIME_OUT_CNT)
    {
        STATUS = RET_TIMEOUT;
        //Serial.println(" TimeOut");
    }
    else
    {
        //Serial.println("");
    }
    
    return STATUS;
}
//=== AUX ===========================================-
//=== Mode Select ===================================+
bool E32TTL100::chkModeSame(MODE_TYPE mode)
{
    static MODE_TYPE pre_mode = MODE_INIT;
    
    if(pre_mode == mode)
    {
        //Serial.print("SwitchMode: (no need to switch) ");  Serial.println(mode, HEX);
        return true;
    }
    else
    {
       // Serial.print("SwitchMode: from ");  Serial.print(pre_mode, HEX);  Serial.print(" to ");  //Serial.println(mode, HEX);
        pre_mode = mode;
        return false;
    }
}

void E32TTL100::SwitchMode(MODE_TYPE mode)
{
    if(!chkModeSame(mode))
    {
        WaitAUX_H();
        
        switch (mode)
        {
            case MODE_0_NORMAL:
                // Mode 0 | normal operation
                digitalWrite(M0_PIN, LOW);
                digitalWrite(M1_PIN, LOW);
                break;
            case MODE_1_WAKE_UP:
                digitalWrite(M0_PIN, HIGH);
                digitalWrite(M1_PIN, LOW);
                break;
            case MODE_2_POWER_SAVIN:
                digitalWrite(M0_PIN, LOW);
                digitalWrite(M1_PIN, HIGH);
                break;
            case MODE_3_SLEEP:
                // Mode 3 | Setting operation
                digitalWrite(M0_PIN, HIGH);
                digitalWrite(M1_PIN, HIGH);
                break;
            default:
                return ;
        }
        
        WaitAUX_H();
        delay(10);
    }
}
//=== Mode Select ===================================-
//=== Basic cmd =====================================+
void E32TTL100::cleanUARTBuf(void)
{
    bool IsNull = true;
    
    while (serial->available())
    {
        IsNull = false;
        
        serial->read();
    }
}

void E32TTL100::triple_cmd(SLEEP_MODE_CMD_TYPE Tcmd)
{
    uint8_t CMD[3] = {Tcmd, Tcmd, Tcmd};
    serial->write(CMD, 3);
    delay(50);  //need ti check
}

RET_STATUS E32TTL100::Module_info(uint8_t* pReadbuf, uint8_t buf_len)
{
    RET_STATUS STATUS = RET_SUCCESS;
    uint8_t Readcnt, idx;
    
    Readcnt = serial->available();
    //Serial.print("serial->available(): ");  Serial.print(Readcnt);  Serial.println(" bytes.");
    if (Readcnt == buf_len)
    {
        for(idx=0;idx<buf_len;idx++)
        {
            *(pReadbuf+idx) = serial->read();
            //Serial.print(" 0x");
            //Serial.print(0xFF & *(pReadbuf+idx), HEX);    // print as an ASCII-encoded hexadecimal
        } //Serial.println("");
    }
    else
    {
        STATUS = RET_DATA_SIZE_NOT_MATCH;
        //Serial.print("  RET_DATA_SIZE_NOT_MATCH - Readcnt: ");  Serial.println(Readcnt);
        cleanUARTBuf();
    }
    
    return STATUS;
}
//=== Basic cmd =====================================-
//=== Sleep mode cmd ================================+
RET_STATUS E32TTL100::Write_CFG_PDS(struct CFGstruct* pCFG)
{
    serial->write((uint8_t *)pCFG, 6);
    
    WaitAUX_H();
    delay(1200);  //need ti check
    
    return RET_SUCCESS;
}

RET_STATUS E32TTL100::Read_CFG(struct CFGstruct* pCFG)
{
    RET_STATUS STATUS = RET_SUCCESS;
    
    //1. read UART buffer.
    cleanUARTBuf();
    
    //2. send CMD
    triple_cmd(R_CFG);
    
    //3. Receive configure
    STATUS = Module_info((uint8_t *)pCFG, sizeof(CFGstruct));
    if(STATUS == RET_SUCCESS)
    {
        //Serial.print("  HEAD:     ");  Serial.println(pCFG->HEAD, HEX);
        //Serial.print("  ADDH:     ");  Serial.println(pCFG->ADDH, HEX);
        //Serial.print("  ADDL:     ");  Serial.println(pCFG->ADDL, HEX);
        
        //Serial.print("  CHAN:     ");  Serial.println(pCFG->CHAN, HEX);
    }
    
    return STATUS;
}

RET_STATUS E32TTL100::Read_module_version(struct MVerstruct* MVer)
{
    RET_STATUS STATUS = RET_SUCCESS;
    
    //1. read UART buffer.
    cleanUARTBuf();
    
    //2. send CMD
    triple_cmd(R_MODULE_VERSION);
    
    //3. Receive configure
    STATUS = Module_info((uint8_t *)MVer, sizeof(MVerstruct));
    if(STATUS == RET_SUCCESS)
    {
        //Serial.print("  HEAD:     0x");  Serial.println(MVer->HEAD, HEX);
        //Serial.print("  Model:    0x");  Serial.println(MVer->Model, HEX);
        //Serial.print("  Version:  0x");  Serial.println(MVer->Version, HEX);
        //Serial.print("  features: 0x");  Serial.println(MVer->features, HEX);
    }
    
    return RET_SUCCESS;
}

void E32TTL100::Reset_module(void)
{
    triple_cmd(W_RESET_MODULE);
    
    WaitAUX_H();
    delay(1000);
}

RET_STATUS E32TTL100::SleepModeCmd(uint8_t CMD, void* pBuff)
{
    RET_STATUS STATUS = RET_SUCCESS;
    
    //Serial.print("SleepModeCmd: 0x");  Serial.println(CMD, HEX);
    WaitAUX_H();
    
    SwitchMode(MODE_3_SLEEP);
    
    switch (CMD)
    {
        case W_CFG_PWR_DWN_SAVE:
            STATUS = Write_CFG_PDS((struct CFGstruct* )pBuff);
            break;
        case R_CFG:
            STATUS = Read_CFG((struct CFGstruct* )pBuff);
            break;
        case W_CFG_PWR_DWN_LOSE:
            
            break;
        case R_MODULE_VERSION:
            Read_module_version((struct MVerstruct* )pBuff);
            break;
        case W_RESET_MODULE:
            Reset_module();
            break;
            
        default:
            return RET_INVALID_PARAM;
    }
    
    WaitAUX_H();
    return STATUS;
}
//=== Sleep mode cmd ================================-

RET_STATUS E32TTL100::SettingModule(struct CFGstruct *pCFG)
{
    RET_STATUS STATUS = RET_SUCCESS;
    
    // Device Address
    pCFG->ADDH = ADDR_H;
    pCFG->ADDL = ADDR_L;
    
    pCFG->OPTION_bits.trsm_mode =TRSM_FP_MODE;
    pCFG->OPTION_bits.tsmt_pwr = TSMT_PWR_10DB;
    
    STATUS = SleepModeCmd(W_CFG_PWR_DWN_SAVE, (void* )pCFG);
    
    SleepModeCmd(W_RESET_MODULE, NULL);
    
    STATUS = SleepModeCmd(R_CFG, (void* )pCFG);
    
    return STATUS;
}

RET_STATUS E32TTL100::ReceiveMsg(uint8_t *pdatabuf, uint8_t *data_len)
{
    
    RET_STATUS STATUS = RET_SUCCESS;
    uint8_t idx;
    
    SwitchMode(MODE_0_NORMAL);
    *data_len = serial->available();
    
    if (*data_len > 0)
    {
        //Serial.print("ReceiveMsg: ");  Serial.print(*data_len);  Serial.println(" bytes.");
        
        for(idx=0;idx<*data_len;idx++)
            *(pdatabuf+idx) = serial->read();
        
        //for(idx=0;idx<*data_len;idx++)
        //{
            //Serial.print(" 0x");
            //Serial.print(0xFF & *(pdatabuf+idx),HEX);    // print as an ASCII-encoded hexadecimal
        //} //Serial.println("");
    }
    else
    {
        STATUS = RET_NOT_IMPLEMENT;
    }
    
    return STATUS;
}

RET_STATUS E32TTL100::SendMsg(String data)
{
    RET_STATUS STATUS = RET_SUCCESS;
    
    SwitchMode(MODE_0_NORMAL);
    
    if(ReadAUX()!=HIGH)
    {
        return RET_NOT_IMPLEMENT;
    }
    delay(10);
    if(ReadAUX()!=HIGH)
    {
        return RET_NOT_IMPLEMENT;
    }
    char stringlength = data.length() +1 ;
    
    char dataToSend[stringlength];
    data.toCharArray(dataToSend, stringlength);
    
    //debugSerial->println(data);
    //debugSerial->println("sent");
    //TRSM_FP_MODE
    //Send format : ADDH ADDL CHAN DATA_0 DATA_1 DATA_2 ...
    char SendBuf[100] = { ADDR_H, ADDR_L, 0x17};    //for A
    strcat(SendBuf,dataToSend);
    
    serial->write(SendBuf, stringlength+3);
    
    return STATUS;
}

