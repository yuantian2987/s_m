// soft_mute.cpp : Defines the entry point for the console application.
//

#include "stdio.h"

FILE *fp_in,*fp_out,*fp_test,*fp_out_l,*fp_out_r;

typedef short                       INT16;
typedef int       			INT32;
typedef unsigned int               UINT32;
typedef void                        VOID;
typedef int                    BOOL;

typedef enum {
    APP_STATE_IDLE = 0,
    APP_STATE_SRC_SELECT ,
    APP_STATE_SRC_CONFIG,
    APP_STATE_DST_SELECT,
    APP_STATE_DST_CONFIG,
    APP_STATE_START,
    APP_STATE_STOP
} SERV_APP_STATE;

#define APP_INPUT_BUFF_SIZE  0x4000
#define APP_OUTPUT_BUFF_SIZE APP_INPUT_BUFF_SIZE*4
#define FADE_LATENCY_SAMPLE 24
#define FADE_STEP           0x2
#define MUTE_DATA_DELAY     0x200

UINT32 soft_mute_ctrl;
 SERV_APP_STATE state;

UINT32  av_ifc_buff_out[APP_OUTPUT_BUFF_SIZE];
UINT32 buff_size;
 
VOID serv_app_msg_soft_mute(BOOL on)
{
    //// TODO
    //// When 2 channel output , how to configure
    UINT32 curAddr;
    UINT32 * OutBuf;
    UINT32 fade_point,i,fade_sample_num,flag_l,flag_r;
    INT32 data_l,data_r;
    soft_mute_ctrl = on ? 0x400:0x10000;
    if( state == APP_STATE_START) {
        if(on) {
            //mute_gain=1024;
            OutBuf=&av_ifc_buff_out[0];
            curAddr =(UINT32 )&av_ifc_buff_out[128];// (dev_aio_ch_get_ifc_cur(g_app_cxt->aud_app_path.audio_ch_out)+0x8)&0xfffffff8; //insure fade process aligned to 8;
            fade_point=((UINT32 *)curAddr-OutBuf+FADE_LATENCY_SAMPLE*2);
            fade_sample_num = soft_mute_ctrl/FADE_STEP;
            for(i=0; i < (fade_sample_num + MUTE_DATA_DELAY); i++,fade_point +=2) {
                data_l=OutBuf[ fade_point & (buff_size/4 - 1) ];
                flag_l= data_l & (1<<31);
                data_l=(INT16)((data_l&(~(1<<31)))>>8);

                data_r=OutBuf[(fade_point +1) & (buff_size/4 - 1)];
                flag_r = data_r & (1<<31);
                data_r =(INT16)((data_r&(~(1<<31)))>>8);

                if(soft_mute_ctrl != 0) {
                    soft_mute_ctrl -= FADE_STEP;
                    data_l = (data_l*soft_mute_ctrl)>>(10);
                    data_r = (data_r*soft_mute_ctrl)>>(10);

                } else {
                    data_l = 0;
                    data_r = 0;
                }

                data_l = (data_l<<8)&(0x00ffffff);
                data_r = (data_r<<8)&(0x00ffffff);
                OutBuf[fade_point & (buff_size/4 - 1) ]=data_l|flag_l;
                OutBuf[(fade_point+1) & (buff_size/4 - 1) ]=data_r|flag_r;
            }
            //COS_Sleep(15);
        }
    }
   // serv_app_msg_dac_mute(on);
}


int main(int argc, char* argv[])
{
	int data_l,data_r,n,i;
	
	fp_in  = fopen("test.bin","rb");//48_1K_16bit.bin 44_1_1k_16bit.bin 96_1k_16bit.bin resampe_test_8K.bin 16k_16bit.bin Aa22050-l-channel.pcm
	if (fp_in == NULL)
	{
		printf("aaaaab\n");
		return -1;
	}
	fp_out  = fopen("48_1K_32bit_out.txt","w");//48_1K_16bit_out.bin 44_1K_16bit_out.bin 96_1K_16bit_out.bin 
	if (fp_out == NULL)
	{	
		printf("ddddd\n");
		return -2;
	}
	fp_out_l  = fopen("left.txt","w");
	fp_out_r  = fopen("right.txt","w");
	state=	APP_STATE_START;
	buff_size=APP_OUTPUT_BUFF_SIZE;
#ifdef MUTE_TEST	
	while(1)
	{
		if(fread(&data_l,1,4,fp_in )<=0)
			break;
		if(fread(&data_r,1,4,fp_in )<=0)
			break;
		data_l &=~(0x80000000);
		data_r &=~(0x80000000);
		if(0x800000&data_l)
		{
			data_l = data_l |(0xFF800000);
		}
		if(0x800000&data_r)
		{
			data_r= data_r |(0xFF800000);
		}

		fprintf(fp_out_l,"%08d\n",data_l);
		fprintf(fp_out_r,"%08d\n",data_r);
	}
#endif
	fread(&av_ifc_buff_out,4,APP_OUTPUT_BUFF_SIZE,fp_in);
	serv_app_msg_soft_mute(1);
	for(i=0;i<APP_OUTPUT_BUFF_SIZE;i+=2)
	{
		data_l=av_ifc_buff_out[i];
		data_l &=~(0x80000000);
		if(0x800000&data_l)
		{
			data_l = data_l |(0xFF800000);
		}
	
		fprintf(fp_out,"%08d\n",data_l);
	}
	fclose(fp_in);
	fclose(fp_out);
	fclose(fp_out_l);
	fclose(fp_out_r);
	printf("Hello World!\n");
	return 0;
}

