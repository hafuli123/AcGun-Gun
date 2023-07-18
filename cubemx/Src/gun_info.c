#include "gun_info.h"
//#include "bsp_nrf24l01.h"
#include "src_usart.h"

uint8_t tmp_buf[32];
//extern __IO int fd_gc24;

void guninfo_send(GunInfo_Struct*info)
{
    uint8_t *pBuf , *pSrc;

    tmp_buf[0 +5]='g';tmp_buf[1 +5]='i';tmp_buf[2 +5]='{';

    pBuf=&tmp_buf[3 +5];
    pSrc=(uint8_t*)&info->sernum ;
    for(int i = 0; i < 4; i++) {
        pBuf[i] = pSrc[i];
    }

    pBuf=&tmp_buf[7 +5];
    pSrc=(uint8_t*)&info->shootnum;
    for(int i = 0; i < 4; i++) {
        pBuf[i] = pSrc[i];
    }

    pBuf=&tmp_buf[11 +5];
    if(info->p_val<0){
        info->p_val=0;
    }
    pSrc=(uint8_t*)&info->p_val;
    for(int i = 0; i < 4; i++) {
        pBuf[i] = pSrc[i];
    }

    tmp_buf[15 +5]=(uint8_t)info->bat_lv;
    tmp_buf[17 +5] = info->shootflg;

    info->uwtick = HAL_GetTick();
    pBuf=&tmp_buf[18 +5];
    pSrc=(uint8_t*)&info->uwtick;
    for(int i = 0; i < 4; i++) {
        pBuf[i] = pSrc[i];
    }

    info->gun_typ = GUN_TYPE;
    tmp_buf[16 +5]=info->gun_typ;

    tmp_buf[22 +5]='}';
//    NRF24L01_TxPacket(tmp_buf);
    HAL_UART_Transmit(&husartx,tmp_buf,32,3);

}

