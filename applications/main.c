/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-07-14     RT-Thread    first version
 */

#include <rtthread.h>
#include <string.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

#include "main.h"

#include "adc.h"
#include "dev.h"
#include "gun_info.h"
#include "src_gpio.h"
#include "src_usart.h"
#include "src_i2c.h"
#include "oled_code.h"
#include "bsp_i2c_oled.h"
#include "bsp_spiflash.h"

struct __Config{
    uint32_t ip , id;
    uint8_t ch;
    uint8_t pid;

    int i_shootcnt;
    uint8_t laser_stat , mot_stat , snd_stat , snd_typ , snd_vol , lang;

    int i_shoottemp;    //本轮开机击发的次数
}config;

struct __Flagbit{
    uint8_t oled_fin;   //一张画面完整显示完成
    uint8_t oled_scrn;  //屏幕画面
    uint8_t oled_bat_ani;   //充电时的动画效果
    uint8_t oled_shcnt_e;   //shoot count refresh 击发时，若界面在射击总数界面，此符号为Y，在电池检测线程里显示电池电量，同时可以改变射击次数的数字显示
    uint8_t stw_typec;  //start with type-c

    uint8_t mrs_loaded;
    uint8_t mrs_step;

    uint8_t p_cnt;
    uint8_t load_lk;    //上膛-未击发的锁，防止用户在未击发时松开，此时会发送100个release，而发送release期间用户又再度按压，导致错误
    uint8_t lgtoff;     //击发，采5点关不可见光
    uint8_t rls_cnt;    //确认松开后，持续SHOOT_RLS_CNT次发送release
    uint8_t rls_chk;    //按压松开后，RLS_CHK_PT次松开才算确认松开

    uint8_t butt_lk;    //上膛后，按钮上锁，不再有反应直到击发
    uint8_t key_on_stat;    //开机键状态

    uint8_t sav_set_flg;    //设置有改动，关机时需要保存

}fb;

rt_adc_device_t rt_adc_dev;
Thread adc_p_th , adc_b_th , mot_th;
GunInfo_Struct guninfo;

__IO int fd_oled , fd_spiflash , fd_mp3 , fd_gc24;
__IO uint8_t fs_rxbuf[10];

uint8_t but_snd[]={0x7e,0x05,0x41,0x00,0x01,0x45,0xef}; //按钮声音

/* press value config */
float p_val_prm[3][2]={ {23,300 } ,  {37,400 } , { 67,700 } };
float a1,b1,a2,b2;
/* ADC */
void adc_b_th_entry(void*parameter){
    float f_adc_val , f_adc_tval;
    while(1){
        //检测电量
        fb.oled_fin=Y;
        rt_adc_enable(rt_adc_dev, 9);
        f_adc_val=rt_adc_read(rt_adc_dev, 9);
        f_adc_tval = (uint32_t)(f_adc_val*330/4096);
        rt_adc_disable(rt_adc_dev, 9);

        guninfo.bat_lv = get_bat_perc(f_adc_tval);

        if((fb.oled_scrn != SCRN_START)&&(fb.oled_scrn != SCRN_TURNOFF)){
            bat_disp(100,0,90,0 ,guninfo.bat_lv);

            if(fb.oled_shcnt_e==Y){
                shw_shootcnt(20,5,10,config.i_shootcnt);
                fb.oled_shcnt_e=N;
            }
        }
        fb.oled_fin=N;
        rt_thread_mdelay(500);
    }
}
void adc_p_th_entry(void*parameter){
    uint8_t load_snd[] = {0x7e,0x05,0x41,0x00,0x05, 0x41,0xef};
    float f_adc_val , f_adc_tval , f_p_raw , f_p_sav;
    while(1){
        rt_sem_take(&adc_p_th.sem, RT_WAITING_FOREVER); //ADC定时器来延时
        //检测压力
        if((fb.mrs_loaded==Y)&&(MRS_RELEASE)){    //击发
            if(config.mot_stat == MOT_E){
                MOT_ON();
            }
            if(config.snd_stat==SND_E){
                play_snd(config.snd_typ);
            }
            rt_timer_start(mot_th.tm);

            if(config.laser_stat==LASER_SHOOTON){
                REDL_ON();
            }

            fb.mrs_loaded=N;
            fb.mrs_step = MRS_SHOOT;

            //总射击次数增加
            if(config.i_shootcnt <9999999){
                config.i_shootcnt++;
            }
            else{
                config.i_shootcnt=0;
            }
            if(fb.oled_scrn == SCRN_SHOOTCNT){
                fb.oled_shcnt_e=Y;
            }
            //临时变量 射击次数增加 用于记录本次开机的射击次数
            config.i_shoottemp ++;
            guninfo.shootnum = config.i_shootcnt + config.i_shoottemp;
        }
        if((fb.mrs_loaded==N)&&(fb.mrs_step == MRS_NRM )&&(MRS_LOAD)){ //检测到上膛
            fb.mrs_step = MRS_LOADED;
            fb.mrs_loaded = Y;
            UVL_ON();

            if((config.snd_stat==SND_E)&&(config.snd_typ==SNDTYP_D)){
                Device_write(fd_mp3, load_snd, 7);
            }
            fb.p_cnt=0;
            fb.load_lk=N;
            fb.lgtoff=0;
            fb.rls_cnt =0;

            fb.butt_lk=Y;
        }

        if(fb.mrs_step == MRS_LOADED){
            rt_adc_enable(rt_adc_dev, 8);
            f_adc_val=rt_adc_read(rt_adc_dev, 8);
            f_adc_tval = (float)(f_adc_val*3.3/4096);
            f_p_raw = (f_adc_tval/11)*10.2*100; //得出原始未处理的压力值
            rt_adc_disable(rt_adc_dev, 8);

            guninfo.p_val = opt_p_val((int)f_p_raw);
            if((int)guninfo.p_val >= P_SEND_LIMVAL){

                Device_write(fd_gc24, &guninfo, SHOOTFLG_P);

                fb.rls_cnt =0;
                fb.rls_chk=0;
                f_p_sav = guninfo.p_val;

                if(config.laser_stat==LASER_TRIGGER){
                    REDL_ON();
                }
                continue;
            }
            else if((int)guninfo.p_val < P_SEND_LIMVAL){
                if((fb.rls_cnt>0)&&(fb.rls_cnt<=SHOOT_RLS_CNT)){
                    fb.rls_cnt++;
                    Device_write(fd_gc24, &guninfo, SHOOTFLG_RLS);
                    continue;
                }
                else if(fb.rls_cnt > SHOOT_RLS_CNT){
                    guninfo.shootflg = SHOOTFLG_NRM;
                    fb.rls_cnt=0;
                    continue;
                }
                if(guninfo.shootflg == SHOOTFLG_P){
                    if(fb.rls_chk < RLS_CHK_PT){
                        fb.rls_chk++;
                        Device_write(fd_gc24, &guninfo, SHOOTFLG_P);
                        fb.rls_cnt =0;
                        continue;
                    }
                    Device_write(fd_gc24, &guninfo, SHOOTFLG_RLS);
                    fb.rls_cnt++;

                    if(config.laser_stat!=LASER_ALWAYSON){
                        REDL_OFF();
                    }
                    continue;
                }
            }
        }
        /* 击发状态 */
        else if(fb.mrs_step == MRS_SHOOT){
            rt_adc_enable(rt_adc_dev, 8);
            f_adc_val=rt_adc_read(rt_adc_dev, 8);
            f_adc_tval = (float)(f_adc_val*3.3/4096);
            f_p_raw = (f_adc_tval/11)*10.2*100;
            rt_adc_disable(rt_adc_dev, 8);
            guninfo.p_val = opt_p_val((int)f_p_raw);
            //防止击发瞬间ADC可能有差距大的改变
            if(guninfo.p_val < f_p_sav){
                guninfo.p_val = f_p_sav;
            }
            else if(guninfo.p_val > P_LIMVAL){
                guninfo.p_val = f_p_sav;
            }

            Device_write(fd_gc24, &guninfo, SHOOTFLG_SHOOT);

            fb.mrs_step = MRS_END;

            fb.rls_cnt=0;
            fb.p_cnt=0;
            fb.load_lk = N;
            fb.lgtoff = 0;
            continue;
        }
        /* 结束状态 */
        else if(fb.mrs_step == MRS_END){
            rt_adc_enable(rt_adc_dev, 8);
            f_adc_val=rt_adc_read(rt_adc_dev, 8);
            f_adc_tval = (float)(f_adc_val*3.3/4096);
            f_p_raw = (f_adc_tval/11)*10.2*100;
            rt_adc_disable(rt_adc_dev, 8);
            guninfo.p_val = opt_p_val((int)f_p_raw);

            if((fb.rls_cnt>0)&&(fb.rls_cnt<=SHOOT_RLS_CNT)){
                fb.rls_cnt++;
                Device_write(fd_gc24, &guninfo, SHOOTFLG_RLS);
                continue;
            }
            else if(fb.rls_cnt>SHOOT_RLS_CNT){
                fb.rls_cnt=0;
                guninfo.shootflg=SHOOTFLG_NRM;
                fb.mrs_step = MRS_NRM;
                UVL_OFF();
                continue;
            }

            if((int)guninfo.p_val>=P_SEND_LIMVAL){
                Device_write(fd_gc24, &guninfo, SHOOTFLG_SHOOT);
                continue;
            }
            else{
                fb.rls_cnt++;
                Device_write(fd_gc24, &guninfo, SHOOTFLG_RLS);

                if(config.laser_stat!=LASER_ALWAYSON){
                    REDL_OFF();
                }

                fb.butt_lk = N;
                continue;
            }
        }
    }
}

/* TIM */
void adc_p_tm_callback(void*parameter)
{
    rt_sem_release(&adc_p_th.sem);
}
void mot_tm_callback(void*parameter){
    MOT_OFF();
    MY_IO1_HIGH();
    MY_IO2_HIGH();
    MY_IO3_HIGH();
    MY_IO4_HIGH();
    MY_IO5_HIGH();
}

int main(void)
{
    a1=(p_val_prm[1][1]-p_val_prm[0][1])/(p_val_prm[1][0]*p_val_prm[1][0]-p_val_prm[0][0]*p_val_prm[0][0]);
    b1=p_val_prm[1][1]-a1*p_val_prm[1][0]*p_val_prm[1][0];
    a2=(p_val_prm[2][1]-p_val_prm[1][1])/(p_val_prm[2][0]-p_val_prm[1][0]);
    b2=p_val_prm[2][1] - a2*p_val_prm[2][0];

    HAL_Init();
    MX_GPIO_Init();

    rt_sec_delay(1);    //延时，这样可实现长按开机
    SYS_ON();           //开启系统供电

    Device_register(&oled);
    Device_register(&spiflash);
    Device_register(&mp3);
    Device_register(&gc24);

    fd_oled = Device_open("oled",0);
    fd_spiflash = Device_open("spiflash",0);
    fd_mp3 = Device_open("mp3",0);
    fd_gc24 = Device_open("gc24",0);

    uint8_t pos[4];
    pos[0]=0;pos[1]=0;pos[2]=120;pos[3]=8;  //pos[]={0,0,120,8};
    Device_ioctl(fd_oled, pos, bmp_aocekeji);

    if(Device_read(fd_spiflash, (uint8_t*)fs_rxbuf, FS_CHK_ADD) != FS_CHK_VAL){
        //首次使用，初始化flash
        /* id、ip config */
        config.id=0x02000002;
        config.ip=0x0201;
        uint8_t chk = FS_CHK_VAL;
        uint32_t *padd , add;
        padd=&add;
        add = FS_ID_ADD;
        Device_ioctl(fd_spiflash, 0, padd);
        uint8_t *pid=(uint8_t*)&config.id;
        Device_write(fd_spiflash, pid, FS_ID_ADD);
        uint8_t *pip=(uint8_t*)&config.ip;
        Device_write(fd_spiflash, pip, FS_ID_ADD+4);

        add = FS_SHOOTCNT_ADD;
        Device_ioctl(fd_spiflash, 0, padd);
        uint8_t *pshc=(uint8_t*)&config.i_shootcnt;
        Device_write(fd_spiflash, pshc, FS_SHOOTCNT_ADD);

        add = FS_SET_ADD;
        Device_ioctl(fd_spiflash, 0, padd);
        config.snd_vol=30;
        uint8_t set_buf[6]={config.laser_stat , config.mot_stat , config.snd_stat ,
                config.snd_typ , config.snd_vol , config.lang};
        Device_write(fd_spiflash, set_buf, FS_SET_ADD);

        uint8_t *p=&chk;
        add = FS_CHK_ADD;
        Device_ioctl(fd_spiflash, 0, padd);
        Device_write(fd_spiflash, p, FS_CHK_ADD);
    }

    Device_read(fd_spiflash, (uint8_t*)fs_rxbuf, FS_ID_ADD);
    config.id=*(uint32_t*)fs_rxbuf;
    uint8_t *prx; prx=(uint8_t*)fs_rxbuf;
    config.ip=*(uint32_t*)(prx+4);

    Device_read(fd_spiflash, (uint8_t*)fs_rxbuf, FS_SHOOTCNT_ADD);
    config.i_shootcnt=*(uint32_t*)fs_rxbuf;

    Device_read(fd_spiflash, (uint8_t*)fs_rxbuf, FS_SET_ADD);
    config.laser_stat=*fs_rxbuf;
    config.mot_stat=*(fs_rxbuf+1);
    config.snd_stat=*(fs_rxbuf+2);
    config.snd_typ=*(fs_rxbuf+3);
    config.snd_vol=*(fs_rxbuf+4);
    config.lang=*(fs_rxbuf+5);

    guninfo.sernum = config.id;

    if(config.snd_stat == SND_E){
        SND_ON();
    }
    if(config.laser_stat==LASER_ALWAYSON){
        REDL_ON();
    }

    if(BAT_CHRG_Y){
        fb.stw_typec = Y;   //插着type-c线开机
    }

    /* 开启RTT - ADC设备 */
    rt_adc_dev=(rt_adc_device_t)rt_device_find("adc1");
    /* ADC线程 - 磁感应部分改为放在这里 */
    rt_sem_init(&adc_p_th.sem, "adc_p_sem", 0, RT_IPC_FLAG_FIFO);
    adc_p_th.tm = rt_timer_create("adc_p_tm", adc_p_tm_callback, NULL, TIMER_ADC_P_INTERVAL,RT_TIMER_FLAG_PERIODIC|RT_TIMER_FLAG_SOFT_TIMER);
    rt_timer_start(adc_p_th.tm);
    adc_p_th.th = rt_thread_create("adc_p_th", adc_p_th_entry , NULL, 2048, 10, 5);
    rt_thread_startup(adc_p_th.th);
    /* ADC线程 - 电池电量检测*/
    adc_b_th.th = rt_thread_create("adc_b_th", adc_b_th_entry, NULL, 2048, 10, 5);
    rt_thread_startup(adc_b_th.th);

    /* 振动停止定时器 */
    mot_th.tm = rt_timer_create("mot_tm", mot_tm_callback, NULL, TIMER_MOT_OFF_INTERVAL, RT_TIMER_FLAG_ONE_SHOT|RT_TIMER_FLAG_SOFT_TIMER);

    return RT_EOK;
}


/*
 * 按键中断回调函数
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint8_t pos[4];
    uint32_t *padd , add;
    if(GPIO_Pin==KEY_ON_PIN){
        rt_thread_mdelay(10);
        if(KEY_ON_PRESS){
            if(fb.key_on_stat == KEY_ON_DIS){
                fb.key_on_stat = KEY_ON_NRM;
                __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
                return;
            }
            else if(fb.key_on_stat == KEY_ON_E){
                if(fb.butt_lk==Y){
                    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
                    return;
                }

                /* 关机 */
                //声音
                if(config.snd_stat == SND_E){
                    Device_write(fd_mp3, but_snd, 7);
                }
                fb.key_on_stat = KEY_ON_END;

                /* 保存射击次数　设置*/
                /* 满30次记录击发数 */
                if(config.i_shoottemp > 30){
                    OLED_ShowStr(80, 6,(uint8_t*)&"...", 2);
                    uint8_t *pshc=(uint8_t*)&config.i_shootcnt;
                    padd=&add;
                    add = FS_SHOOTCNT_ADD;
                    Device_ioctl(fd_spiflash, 0, padd);
                    Device_write(fd_spiflash, pshc, FS_SHOOTCNT_ADD);
                }
                if(fb.sav_set_flg==Y){
                    OLED_ShowStr(80, 6,(uint8_t*)&"...", 2);
                    padd=&add;
                    add = FS_SET_ADD;
                    Device_ioctl(fd_spiflash, 0, padd);
                    uint8_t set_buf[6]={config.laser_stat , config.mot_stat , config.snd_stat ,
                            config.snd_typ , config.snd_vol , config.lang};
                    Device_write(fd_spiflash, set_buf, FS_SET_ADD);
                }
                SYS_OFF();
                __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
                return;
            }

            else if(fb.key_on_stat == KEY_ON_NRM){
                if(fb.butt_lk==Y){
                    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
                    return;
                }

                if(fb.oled_fin == Y){
                    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
                    return;
                }

                /* 红点激光设置 */
                if(fb.oled_scrn == SCRN_LASER){
                    //声音
                    if(config.snd_stat == SND_E){
                        Device_write(fd_mp3, but_snd, 7);
                    }
                    if(config.laser_stat < 3){
                        config.laser_stat++;
                    }
                    else{
                        config.laser_stat = LASER_ALWAYSOFF;
                    }

                    set_opt(20,5,SCRN_LASER,config.laser_stat,config.lang);
                    fb.sav_set_flg=Y;

                    if(config.laser_stat==LASER_ALWAYSON){
                        REDL_ON();
                    }
                    else{
                        REDL_OFF();
                    }
                }
                else if(fb.oled_scrn == SCRN_SHOCK){
                    //声音
                    if(config.snd_stat == SND_E){
                        Device_write(fd_mp3, but_snd, 7);
                    }
                    if(config.mot_stat==MOT_DIS){
                        config.mot_stat=MOT_E;
                    }
                    else if(config.mot_stat==MOT_E){
                        config.mot_stat=MOT_DIS;
                    }
                    set_opt(20,5,SCRN_SHOCK,config.mot_stat,config.lang);
                    fb.sav_set_flg=Y;
                }
                else if(fb.oled_scrn == SCRN_SND){
                    //声音
                    if(config.snd_stat == SND_E){
                        Device_write(fd_mp3, but_snd, 7);
                    }
                    if(config.snd_stat==SND_DIS){
                        config.snd_stat=SND_E;
                        SND_ON();
                    }
                    else if(config.snd_stat==SND_E){
                        config.snd_stat=SND_DIS;
                        SND_OFF();
                    }
                    set_opt(20,5,SCRN_SND,config.snd_stat,config.lang);
                    fb.sav_set_flg=Y;  //设置有变过
                }
                else if(fb.oled_scrn == SCRN_SNDTYP){
                    //声音
                    if(config.snd_stat == SND_E){
                        Device_write(fd_mp3, but_snd, 7);
                    }
                    if(config.snd_typ<3){
                        config.snd_typ++;
                    }
                    else{
                        config.snd_typ=SNDTYP_A;
                    }
                    set_opt(20,5,SCRN_SNDTYP,config.snd_typ,config.lang);
                    fb.sav_set_flg=Y;
                }
                else if(fb.oled_scrn==SCRN_VOL){
                    if(config.snd_stat==SND_E){
                        if(config.snd_vol<30){
                            config.snd_vol++;
                        }
                        else{
                            config.snd_vol=0;
                        }
                        pos[0]=10;  pos[1]=4; pos[2]=76; pos[3]=7;
                        Device_ioctl(fd_oled, pos, bmp_volume[30-config.snd_vol]);
                        shw_volnum(90,4,10,config.snd_vol);

                        //声音
                        set_vol(config.snd_vol);
                        fb.sav_set_flg=Y;  //设置有变过
                    }
                }
                else if(fb.oled_scrn == SCRN_LANG){
                    //声音
                    if(config.snd_stat==SND_E){
                        Device_write(fd_mp3, but_snd, 7);
                    }
                    if(config.lang<2){
                        config.lang++;
                    }
                    else{
                        config.lang=LANG_ENG;
                    }
                    set_opt(20,5,SCRN_LANG,config.lang,config.lang);
                    fb.sav_set_flg=Y;  //设置有变过
                }

            }
            __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);

            return;
        }
    }

    else if(GPIO_Pin==KEY2_PIN){
        rt_thread_mdelay(10);
        if(KEY2_PRESS){
            if(fb.oled_fin==Y){
                __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
                return;
            }

            //声音
            if(config.snd_stat==SND_E){
                Device_write(fd_mp3, but_snd, 7);
            }

            if(fb.oled_scrn == SCRN_START){
                /* 进入IP ID显示界面 */
                Device_write(fd_oled, 0, 0);
                pos[0]=0;  pos[1]=0; pos[2]=30; pos[3]=3;
                Device_ioctl(fd_oled, pos, bmp_word[1]);
                pos[0]=0;  pos[1]=6; pos[2]=30; pos[3]=8;
                Device_ioctl(fd_oled, pos, bmp_word[0]);

                shw_msg(5,3,10,config.id,30,6,10,config.ip);
                fb.oled_scrn = SCRN_MSG;
            }

            else if(fb.oled_scrn == SCRN_MSG){
                /* 进入射击总次数统计界面  */
                fb.oled_scrn = SCRN_SHOOTCNT;
                clr_bmp();

                shw_menutit(0,1,SCRN_SHOOTCNT,config.lang);
                shw_shootcnt(20,5,10,config.i_shootcnt);
            }

            else if(fb.oled_scrn == SCRN_SHOOTCNT){
                /* 进入红点激光设置界面  */
                fb.oled_scrn = SCRN_LASER;
                clr_bmp();
                shw_menutit(0,1,SCRN_LASER,config.lang);
                set_opt(20,5,SCRN_LASER,config.laser_stat,config.lang);
            }

            else if(fb.oled_scrn == SCRN_LASER){
                /* 进入振动设置界面  */
                fb.oled_scrn = SCRN_SHOCK;
                clr_bmp();
                shw_menutit(0,1,SCRN_SHOCK,config.lang);
                set_opt(20,5,SCRN_SHOCK,config.mot_stat,config.lang);
            }

            else if(fb.oled_scrn == SCRN_SHOCK){
                /* 进入声音设置界面 */
                fb.oled_scrn = SCRN_SND;
                clr_bmp();
                shw_menutit(0,1,SCRN_SND,config.lang);
                set_opt(20,5,SCRN_SND,config.snd_stat,config.lang);
            }
            else if(fb.oled_scrn == SCRN_SND){
                /* 进入声音效果设置页面 */
                fb.oled_scrn = SCRN_SNDTYP;
                clr_bmp();
                shw_menutit(0,1,SCRN_SNDTYP,config.lang);
                set_opt(20,5,SCRN_SNDTYP,config.snd_typ,config.lang);
            }

            else if(fb.oled_scrn == SCRN_SNDTYP){
                /* 进入音量页面 */
                fb.oled_scrn = SCRN_VOL;
                clr_bmp();
                shw_menutit(0,1,SCRN_VOL,config.lang);
                pos[0]=10;  pos[1]=4; pos[2]=76; pos[3]=7;
                Device_ioctl(fd_oled, pos, bmp_volume[30-config.snd_vol]);
                shw_volnum(90,4,10,config.snd_vol);
            }

            else if(fb.oled_scrn == SCRN_VOL){
                /* 进入语言设置页面 */
                fb.oled_scrn = SCRN_LANG;
                clr_bmp();
                set_opt(20,5,SCRN_LANG,config.lang,config.lang);
            }

            else if(fb.oled_scrn == SCRN_LANG){
                /* 进入关机页面 */
                fb.oled_scrn = SCRN_TURNOFF;
                fb.key_on_stat = KEY_ON_E;
                clr_bmp();
                shw_menutit(20,5,SCRN_TURNOFF,config.lang);
                pos[0]=100;  pos[1]=0; pos[2]=120; pos[3]=3;
                Device_ioctl(fd_oled, pos, bmp_batt_clear);
            }
            else if(fb.oled_scrn == SCRN_TURNOFF){
                fb.key_on_stat = KEY_ON_NRM;
                fb.oled_scrn = SCRN_MSG;
                clr_bmp();
                pos[0]=0;  pos[1]=0; pos[2]=30; pos[3]=3;
                Device_ioctl(fd_oled, pos, bmp_word[1]);
                pos[0]=0;  pos[1]=6; pos[2]=30; pos[3]=8;
                Device_ioctl(fd_oled, pos, bmp_word[0]);
                shw_msg(5,3,10,config.id,30,6,10,config.ip);
            }
            __HAL_GPIO_EXTI_CLEAR_IT(GPIO_Pin);
            return;
        }

    }
}
/*
 * 菜单中清除上一张画面的菜单内容
 */
void clr_bmp(void)
{
    uint8_t pos1[]={0,0,85,3};
    uint8_t pos2[]={0,3,120,8};
    Device_ioctl(fd_oled, pos1, bmp_clear1);
    Device_ioctl(fd_oled, pos2, bmp_clear2);
}

/*
 *  显示IP ID信息
 */
void shw_msg(uint8_t x,uint8_t y,uint8_t itl,uint32_t msg ,uint8_t x2,uint8_t y2,uint8_t itl2, uint32_t msg2)
{
    uint8_t val;

    val=(uint8_t)((msg&0xF0000000)>>28);
    shw_bmpNum(x,y,val);
    val=(uint8_t)((msg&0x0F000000)>>24);
    shw_bmpNum(x+itl,y,val);
    val=(uint8_t)((msg&0x00F00000)>>20);
    shw_bmpNum(x+itl*2,y,val);
    val=(uint8_t)((msg&0x000F0000)>>16);
    shw_bmpNum(x+itl*3,y,val);

    val=(uint8_t)((msg&0x0000F000)>>12);
    shw_bmpNum(x+itl*4,y,val);
    val=(uint8_t)((msg&0x00000F00)>>8);
    shw_bmpNum(x+itl*5,y,val);
    val=(uint8_t)((msg&0x000000F0)>>4);
    shw_bmpNum(x+itl*6,y,val);
    val=(uint8_t)((msg&0x0000000F)>>0);
    shw_bmpNum(x+itl*7,y,val);

    val=(uint8_t)((msg2&0x0000F000)>>12);
    shw_bmpNum(x2,y2,val);
    val=(uint8_t)((msg2&0x00000F00)>>8);
    shw_bmpNum(x2+itl2,y2,val);
    val=(uint8_t)((msg2&0x000000F0)>>4);
    shw_bmpNum(x2+itl2*2,y2,val);
    val=(uint8_t)((msg2&0x0000000F)>>0);
    shw_bmpNum(x2+itl2*3,y2,val);
}
/*
 * 菜单里可变动的设置项
 */
void set_opt(uint8_t x,uint8_t y,uint8_t type,uint8_t set,uint8_t lang)
{
    uint8_t pos[4]={x,y,x+80,y+3};
    if(type==SCRN_LASER){
        if(lang==LANG_ENG){
            Device_ioctl(fd_oled, pos, bmp_laserset[set]);
        }
        else if(lang==LANG_CHS){
            Device_ioctl(fd_oled, pos, bmp_laserset_chs[set]);
        }
        else if(lang==LANG_TC){
            Device_ioctl(fd_oled, pos, bmp_laserset_tc[set]);
        }
        return;
    }
    else if((type==SCRN_SHOCK)||(type==SCRN_SND)){
        if(lang==LANG_ENG){
            Device_ioctl(fd_oled, pos, bmp_switchset[set]);
        }
        else if(lang==LANG_CHS){
            Device_ioctl(fd_oled, pos, bmp_switchset_chs[set]);
        }
        else if(lang==LANG_TC){
            Device_ioctl(fd_oled, pos, bmp_switchset_tc[set]);
        }
        return;
    }
    else if(type==SCRN_SNDTYP){
        if(lang==LANG_ENG){
            Device_ioctl(fd_oled, pos, bmp_soundtype[set]);
        }
        else if((lang==LANG_CHS)||(lang==LANG_TC)){
            Device_ioctl(fd_oled, pos, bmp_soundtype_chs[set]);
        }
        return;
    }

    //语言选项 - 特殊
    else if(type==SCRN_LANG){
        shw_menutit(0,1,SCRN_LANG,lang);
        Device_ioctl(fd_oled, pos, bmp_langset[set]);
        return;
    }
}
/*
 * 菜单标题显示
 */
void shw_menutit(uint8_t x,uint8_t y,uint8_t type,uint8_t lang)
{
    uint8_t pos[4]={x,y,x+80,y+3};
    if(lang==LANG_ENG){
        Device_ioctl(fd_oled, pos, bmp_menu_ENG[type-2]);
        return;
    }
    else if(lang==LANG_CHS){
        Device_ioctl(fd_oled, pos, bmp_menu_CHN[type-2]);
        return;
    }
    else if(lang==LANG_TC){
        Device_ioctl(fd_oled, pos, bmp_menu_TC[type-2]);
        return;
    }
}
/*
 * 音量调节界面，显示音量的数值
 */
void shw_volnum(uint8_t x,uint8_t y,uint8_t itl,uint32_t vol)
{
    uint8_t val;
    val=vol/10%10;
    shw_bmpNum(x,y,val);
    val=vol/1%10;
    shw_bmpNum(x+itl,y,val);
}

/*
 * 原始压力值进行精细处理
 */
float opt_p_val(int raw_val)
{
    //分段：0  300 400 700
    if(  raw_val<=p_val_prm[0][0]){
//        return 13.0434f*raw_val;
        return (p_val_prm[0][1]/p_val_prm[0][0])*raw_val;
    }
    else if(  (raw_val>p_val_prm[0][0]) && (raw_val<=p_val_prm[1][0])){
//        return  0.1190f*raw_val*raw_val+237.0890f;
        return a1*raw_val*raw_val+b1;
    }
    else if(  (raw_val>p_val_prm[1][0]) && (raw_val<=p_val_prm[2][0])){
//        return  10.0f*raw_val+30.0f;
        return a2*raw_val+b2;
    }
    else if(  raw_val>p_val_prm[2][0]){
        return  p_val_prm[2][1];
    }
    return 0.0;
}

/*
 * 显示射击总数 - 十进制
 */
void shw_shootcnt(uint8_t x,uint8_t y,uint8_t itl,uint32_t msg)
{
    uint8_t val;
    val=msg/10000000%10;
    shw_bmpNum(x,y,val);
    val=msg/1000000%10;
    shw_bmpNum(x+itl,y,val);
    val=msg/100000%10;
    shw_bmpNum(x+itl*2,y,val);
    val=msg/10000%10;
    shw_bmpNum(x+itl*3,y,val);
    val=msg/1000%10;
    shw_bmpNum(x+itl*4,y,val);
    val=msg/100%10;
    shw_bmpNum(x+itl*5,y,val);
    val=msg/10%10;
    shw_bmpNum(x+itl*6,y,val);
    val=msg/1%10;
    shw_bmpNum(x+itl*7,y,val);
}
/*
 *  显示单个数字 显示范围从0到F
 */
void shw_bmpNum(uint8_t x,uint8_t y,uint8_t val)
{
    uint8_t pos[4];
    pos[0]=x;  pos[1]=y;  pos[2]=x+10;    pos[3]=y+2;
    Device_ioctl(fd_oled, pos, bmp_num[val]);
}

/*
 *      根据电池电量，获取百分比
 */
uint32_t get_bat_perc(uint32_t qty)
{
    if(qty>=103){return 100;}
    else if((qty>=101)&&(qty<103)){return 90;}
    else if((qty>=100)&&(qty<101)){return 85;}
    else if((qty>=99)&&(qty<100)){return 80;}
    else if((qty>=98)&&(qty<99)){return 75;}
    else if((qty>=97)&&(qty<98)){return 70;}
    else if((qty>=96)&&(qty<97)){return 65;}
    else if((qty>=95)&&(qty<96)){return 60;}
    else if((qty>=94)&&(qty<95)){return 55;}
    else if((qty>=93)&&(qty<94)){return 50;}
    else if((qty>=92)&&(qty<93)){return 45;}
    else if((qty>=91)&&(qty<92)){return 40;}
    else if((qty>=90)&&(qty<91)){return 35;}
    else if((qty>=89)&&(qty<90)){return 20;}
    else if((qty>=87)&&(qty<89)){return 10;}
    else if(qty<87){return 5;}
    return 0;
}
/*
  * 根据ADC获取到的电池电量，改变电池电量OLED显示
 */
void bat_disp(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2 ,uint32_t per)
{
    uint8_t pos[4];
    //充电
    if(BAT_CHRG_Y){
        if(fb.oled_bat_ani==0){
            pos[0]=x2;  pos[1]=y2;  pos[2]=x2+5;    pos[3]=y2+3;
            Device_ioctl(fd_oled, pos, bmp_bat_charg[0]);
            fb.oled_bat_ani=1; //启动电池充电动画
        }

        /* 若是插了typec开机 ，首次开启电池充电动画需要这样处理，才能正常有充电动画*/
        if(fb.stw_typec==Y){
            fb.stw_typec=N;
            fb.oled_bat_ani=1;
            pos[0]=x1;  pos[1]=y1;  pos[2]=x1+20; pos[3]=y1+3;
            Device_ioctl(fd_oled, pos, bmp_batt[16-fb.oled_bat_ani]);
            return;
        }

        /* 正常充电中 */
        if(BAT_DONE_N){
            if(fb.oled_bat_ani==17){
                fb.oled_bat_ani=1;
            }
            pos[0]=x1;  pos[1]=y1;  pos[2]=x1+20; pos[3]=y1+3;
            Device_ioctl(fd_oled, pos, bmp_batt[16-fb.oled_bat_ani]);
            fb.oled_bat_ani++;
            return;
        }
        else if(BAT_DONE_Y){
            pos[0]=x1;  pos[1]=y1;  pos[2]=x1+20; pos[3]=y1+3;
            Device_ioctl(fd_oled, pos, bmp_batt[0]);
            return;
        }
    }

    //平常状态
    if(fb.oled_bat_ani!=0){    //刚拔掉充电线
        pos[0]=x2;  pos[1]=y2;  pos[2]=x2+5;    pos[3]=y2+3;
        Device_ioctl(fd_oled, pos, bmp_bat_charg[1]);
        fb.oled_bat_ani=0;
    }
    pos[0]=x1;  pos[1]=y1;  pos[2]=x1+20; pos[3]=y1+3;
    if(per==100){
        Device_ioctl(fd_oled, pos, bmp_batt[0]);
        return;
    }
    else if(per==90){
        Device_ioctl(fd_oled, pos, bmp_batt[1]);
        return;
    }
    else if(per==85){
        Device_ioctl(fd_oled, pos, bmp_batt[2]);
        return;
    }
    else if(per==80){
        Device_ioctl(fd_oled, pos, bmp_batt[3]);
        return;
    }
    else if(per==75){
        Device_ioctl(fd_oled, pos, bmp_batt[4]);
        return;
    }
    else if(per==70){
        Device_ioctl(fd_oled, pos, bmp_batt[5]);
        return;
    }
    else if(per==65){
        Device_ioctl(fd_oled, pos, bmp_batt[6]);
        return;
    }
    else if(per==60){
        Device_ioctl(fd_oled, pos, bmp_batt[7]);
        return;
    }
    else if(per==55){
        Device_ioctl(fd_oled, pos, bmp_batt[8]);
        return;
    }
    else if(per==50){
        Device_ioctl(fd_oled, pos, bmp_batt[9]);
        return;
    }
    else if(per==45){
        Device_ioctl(fd_oled, pos, bmp_batt[10]);
        return;
    }
    else if(per==40){
        Device_ioctl(fd_oled, pos, bmp_batt[11]);
        return;
    }
    else if(per==35){
        Device_ioctl(fd_oled, pos, bmp_batt[12]);
        return;
    }
    else if(per==20){
        Device_ioctl(fd_oled, pos, bmp_batt[13]);
        return;
    }
    else if(per==10){
        Device_ioctl(fd_oled, pos, bmp_batt[14]);
        return;
    }
    else if(per==5){
        Device_ioctl(fd_oled, pos, bmp_batt[15]);
        return;
    }
}

/*
 * 枪声播放
 */
void play_snd(uint8_t type)
{
    if(type==SNDTYP_A){MY_IO2_LOW();return;}
    else if(type==SNDTYP_B){MY_IO3_LOW();return;}
    else if(type==SNDTYP_C){MY_IO4_LOW();return;}
    else if(type==SNDTYP_D){MY_IO2_LOW();return;}
}

void rt_sec_delay(int sec)
{
    for(int i=0;i<sec;i++){
        rt_thread_mdelay(1000);
    }
}
/*
 * 音量设置
 */
void set_vol(uint32_t vol)
{
    fb.oled_fin=Y;
    switch(vol){
    case 0:{uint8_t tx[6]={0x7e,0x04,0x31,0x00,0x35,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 1:{uint8_t tx[6]={0x7e,0x04,0x31,0x01,0x34,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 2:{uint8_t tx[6]={0x7e,0x04,0x31,0x02,0x37,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 3:{uint8_t tx[6]={0x7e,0x04,0x31,0x03,0x36,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 4:{uint8_t tx[6]={0x7e,0x04,0x31,0x04,0x31,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 5:{uint8_t tx[6]={0x7e,0x04,0x31,0x05,0x30,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 6:{uint8_t tx[6]={0x7e,0x04,0x31,0x06,0x33,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 7:{uint8_t tx[6]={0x7e,0x04,0x31,0x07,0x32,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 8:{uint8_t tx[6]={0x7e,0x04,0x31,0x08,0x3d,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 9:{uint8_t tx[6]={0x7e,0x04,0x31,0x09,0x3c,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 10:{uint8_t tx[6]={0x7e,0x04,0x31,0x0a,0x3f,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 11:{uint8_t tx[6]={0x7e,0x04,0x31,0x0b,0x3e,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 12:{uint8_t tx[6]={0x7e,0x04,0x31,0x0c,0x39,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 13:{uint8_t tx[6]={0x7e,0x04,0x31,0x0d,0x38,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 14:{uint8_t tx[6]={0x7e,0x04,0x31,0x0e,0x3b,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 15:{uint8_t tx[6]={0x7e,0x04,0x31,0x0f,0x3a,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 16:{uint8_t tx[6]={0x7e,0x04,0x31,0x10,0x25,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 17:{uint8_t tx[6]={0x7e,0x04,0x31,0x11,0x24,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 18:{uint8_t tx[6]={0x7e,0x04,0x31,0x12,0x27,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 19:{uint8_t tx[6]={0x7e,0x04,0x31,0x13,0x26,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 20:{uint8_t tx[6]={0x7e,0x04,0x31,0x14,0x21,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 21:{uint8_t tx[6]={0x7e,0x04,0x31,0x15,0x20,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 22:{uint8_t tx[6]={0x7e,0x04,0x31,0x16,0x23,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 23:{uint8_t tx[6]={0x7e,0x04,0x31,0x17,0x22,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 24:{uint8_t tx[6]={0x7e,0x04,0x31,0x18,0x2d,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 25:{uint8_t tx[6]={0x7e,0x04,0x31,0x19,0x2c,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 26:{uint8_t tx[6]={0x7e,0x04,0x31,0x1a,0x2f,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 27:{uint8_t tx[6]={0x7e,0x04,0x31,0x1b,0x2e,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 28:{uint8_t tx[6]={0x7e,0x04,0x31,0x1c,0x29,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 29:{uint8_t tx[6]={0x7e,0x04,0x31,0x1d,0x28,0xef};Device_write(fd_mp3, tx, 6);break;}
    case 30:{uint8_t tx[6]={0x7e,0x04,0x31,0x1e,0x2b,0xef};Device_write(fd_mp3, tx, 6);break;}
    }
    if(config.snd_stat==SND_E){
        Device_write(fd_mp3, but_snd, 7);
    }
    fb.oled_fin = N;
}
