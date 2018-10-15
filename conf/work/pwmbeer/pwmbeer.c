
#include <stdint.h>
#include "pwmbeer.h"
#include <sys/mman.h> 
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


#define PWM_INDEX 0
#define MAP_SIZE  0xFF
#define CTL_BASE_PWM 0X40260000
#define REG_AON_CLK_PWM0_CFG 0x402D0000+0x002C
#define REG_AON_APB_APB_EB0 0X402E0000

#define PWM_PRESCALE (0x0000)
#define PWM_CNT (0x0004)
#define PWM_PAT_LOW (0x000C)
#define PWM_PAT_HIG (0x0010)

#define PWM_ENABLE (1 << 8)
#define PWM2_SCALE 0x0
#define PWM_REG_MSK 0xffff
#define PWM_MOD_MAX 0xff

#define REG32(addr)               (*(volatile unsigned int *) (addr))
#define __raw_writel(v,a)        ctrlreg(1,a,v)
#define __raw_readl(a)           ctrlreg(0,a,0)

typedef unsigned char uchar;

int c2i(char ch)
{
    if(ch>='0' && ch<='9')
            return ch - 48;

    if( ch < 'A' || (ch > 'F' && ch < 'a') || ch > 'z' )
            return -1;

    if(ch>='A' && ch<='F')
        return ch - 55;

    if(ch>='a' && ch<='f')
        return ch - 87;
    
    return -1;
}
 

int hex2dec(char *hex)
{
    int len;
    int num = 0;
    int temp;
    int bits;
    int i;
    
    len = strlen(hex);

    for (i=0, temp=0; i<len; i++, temp=0)
    {
            temp = c2i( *(hex + i) );
            bits = (len - i - 1) * 4;
            temp = temp << bits;

            num = num | temp;
    }

    return num;
}

unsigned int ctrlreg(int flag,unsigned int addr, unsigned int value)
{
    int fd1, fd2;
    char *dir = "/sys/kernel/debug/";
    char strPath1[256] = {0};
    char strPath2[256] = {0};
    char c[12 + 12] = {0};
    int n = 0;
    ssize_t cnt = 0;
    unsigned int regValue = 0;

    sprintf(strPath1, "%s%s", dir, "lookat/addr_rwpv");
    sprintf(strPath2, "%s%s", dir, "lookat/data");

    if((fd1 = open(strPath1, O_RDWR | O_SYNC)) < 0)
    {
        perror("open addr_rwpv");
         //exit(EXIT_FAILURE);
         return -1;
    }
    if((fd2 = open(strPath2, O_RDWR | O_SYNC)) < 0)
    {
         perror("open data");
         //exit(EXIT_FAILURE);
         return -1;
    }

    // read reg
    if(flag==0)
    {
          //¼Ä´æÆ÷¶Á²Ù×÷
        n = snprintf(c, 11,"0x%x", addr);
        write(fd1, c, n);
        memset(c, 0, 11);
        cnt = read(fd2, c, 10);
        if((cnt < 0) || (cnt > 10)){
            perror("read data");
            //exit(EXIT_FAILURE);
            return -1;
        }   
        regValue = hex2dec(&c[2]);
             
        //fprintf(stdout, "read reg:value %s addr:%x,int:0x%08x\n",c,addr,regValue);
        
    }
    // write reg
    else
    {  
        //¼Ä´æÆ÷Ð´²Ù×÷
        //addr |= 2;
        n = snprintf(c, 11,"0x%x", addr|2);
        write(fd1, c, n);
        n = snprintf(c, 11,"0x%x", value);
        write(fd2, c, n);

        //fprintf(stdout, "write reg addr:%x value:%x c:%s\n",addr|2,value,c);   
    }
        
    close(fd2);
    close(fd1);
    return regValue;
}


void __raw_bits_or(unsigned int v, unsigned int a)
{
    __raw_writel((__raw_readl(a) | v), a);
}

void __raw_bits_and(unsigned int v, unsigned int a)
{
    __raw_writel((__raw_readl(a) & v), a);
}


static inline uint32_t pwm_read(int index, uint32_t reg)
{
    return __raw_readl(CTL_BASE_PWM + index * 0x20 + reg);
}

static void pwm_write(int index, uint32_t value, uint32_t reg)
{
    __raw_writel(value, CTL_BASE_PWM + index * 0x20 + reg);
}

unsigned char scaleindex = 28;
void set_beer(uint32_t duty)
{
    __raw_bits_or((0x1 << 0), REG_AON_CLK_PWM0_CFG + PWM_INDEX * 4);//ext_26m select
    //__raw_bits_or((0x00 << 0), REG_AON_CLK_PWM0_CFG + index * 4);//ext_32k select

    if (0 == duty) {
        pwm_write(PWM_INDEX, 0, PWM_PRESCALE);
        //fprintf(stdout,"sprd backlight power off. pwm_index=%d  duty=%d\n", PWM_INDEX, duty);
    } else {
        __raw_bits_or((0x1 << (PWM_INDEX+4)), REG_AON_APB_APB_EB0); //PWMx EN
        //printf("REG_AON_APB_APB_EB0:%x REG_AON_CLK_PWM0_CFG:%x\n",REG_AON_APB_APB_EB0,
        //    REG_AON_CLK_PWM0_CFG);
        pwm_write(PWM_INDEX, PWM2_SCALE, PWM_PRESCALE);
        pwm_write(PWM_INDEX, (duty << 8) | PWM_MOD_MAX, PWM_CNT);
        pwm_write(PWM_INDEX, PWM_REG_MSK, PWM_PAT_LOW);
        pwm_write(PWM_INDEX, PWM_REG_MSK, PWM_PAT_HIG);

        //pwm_write(index, PWM_ENABLE, PWM_PRESCALE);
        pwm_write(PWM_INDEX, PWM_ENABLE|scaleindex, PWM_PRESCALE);  // 26M scale clock
        //fprintf(stdout,"sprd backlight power on. pwm_index=%d  duty=%d dutymod:%x scal:%x\n", PWM_INDEX, duty,
        //    (duty << 8) | PWM_MOD_MAX,PWM_ENABLE|4);
    }

    return;

}

static uchar s_InitPlayTone = 0;
int InitPlayTone()
{
    __raw_bits_or((0x1 << 0), REG_AON_CLK_PWM0_CFG + PWM_INDEX * 4);//ext_26m select    

    __raw_bits_or((0x1 << (PWM_INDEX+4)), REG_AON_APB_APB_EB0); //PWMx EN

    s_InitPlayTone = 1;
    return 0;
    
}

int PlayTone(int playInterVal,unsigned char scaleClock)
{
    if(s_InitPlayTone == 0)
    {
        return -1;
    }
    scaleindex = scaleClock;
    set_beer(128);  //128
    usleep(playInterVal*1000);
    set_beer(0);
    return 0;
}

int UnInitPlayTone()
{
    s_InitPlayTone = 0;
    set_beer(0);
    __raw_bits_and(~(0x1 << (PWM_INDEX+4)), REG_AON_APB_APB_EB0); //PWMx DisEN
    return 0;

}


int main(int ac, char *av[]) {

	if (ac != 3) {
		printf(" need two args\n");
		return -1;
	}
	int interval = atoi(av[1]);
	int scale = atoi(av[2]);
	InitPlayTone();
	PlayTone(interval, scale);
	UnInitPlayTone();
	return 0;
}
