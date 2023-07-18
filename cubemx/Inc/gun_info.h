#ifndef __GUN_INFO_H__
#define __GUN_INFO_H__

#include "main.h"

typedef struct{
    uint32_t sernum;
    uint32_t shootnum;
    float p_val;
    uint8_t bat_lv;
    uint8_t gun_typ;
    uint8_t shootflg;
    uint32_t uwtick;
}GunInfo_Struct;

enum{
    SHOOTFLG_NRM = 0,
    SHOOTFLG_P,
    SHOOTFLG_SHOOT,
    SHOOTFLG_RLS = 4,
    SHOOTFLG_LGTOFF = 8
};

extern uint8_t tmp_buf[32];

void guninfo_send(GunInfo_Struct*info);


#endif
