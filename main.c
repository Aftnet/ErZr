#include "gba.h"
#include "sprites.h"
#include "lv1.h"
#include "lv2.h"
#include "lv3.h"
#include "lv4.h"
#include "lv5.h"
#include "lv6.h"
#include "lv7.h"
#include "lv8.h"
#include "lv9.h"
#include "lv10.h"
#include "lv11.h"
#include "lv12.h"
#include "lv13.h"
#include "lv14.h"
#include "lv15.h"


//Globals

u8 grid [10] [16];
u8 gridstat [10] [16];
u8 cb [4];
u8 cbx=7;
u8 cby=0;
u8 cbpx=7;
u8 cbpy=0;
u8 nextb1 [4];
u8 nextb2 [4];
u8 nextb3 [4];
u8 swpbrpos=0;
u8 swpbrcoor=0;
u8 swpbrprevcoor=0;
u8 mbdatarebuildflag=0;


u32 score=0;
u16 level=0;
u16 deletes=0;
u8 sweepdels=0;


//Mcroblock strucures
struct  mblk {u8 type; u8 size; u8 delcoordinate; u8 startcoordinate; u8 stdelcoor; u8 normalflag; u8 data [10] [16]; struct mblk *prev; struct mblk *next;}; 
typedef struct mblk macroblock;
typedef struct mblk * mbpointer;
#define NULL 0x0000
macroblock fmblock;



//timers and counters:these allow to have different game functions to occur every period defined frames
//es fnperiod=3 --> fn will occur once every 3 frames.

//gravity in grid period
#define GRAVPER 1
u8 gravtmr=0;
u8 dropper=110;
u8 droptmr=0;
//player movement(left or right) period
#define PMOVPER 10
#define SLIDEPER 1
#define TTSLIDE 30
#define PROTPER 10
u8 presstime=0;
u8 pmovper=PMOVPER;
u16 pmovtmr=0;
u16 prottmr=0;
u8 breaker=0;
//sweep line speed management
u8 swpbrper=2;
u8 swpbrtmr=0;
u8 swpbrrate=1;
//bonus signal timer
u8 bsigtmr=0;
//level timer and duration
#define LVDUR 3600
u32 lvtimer=0;
u8 lselector=0;




u16* OAM = (u16*)OAMmem;
OAMEntry sprites[128];

//Functions

void DMAFastCopy(void* source, void* dest, unsigned int count, unsigned int mode)
{
    if (mode == DMA_16NOW || mode == DMA_32NOW)
    {
        REG_DMA3SAD = (unsigned int)source;
        REG_DMA3DAD = (unsigned int)dest;
        REG_DMA3CNT = count | mode;
    }
}

void copyOAM()
{
    DMAFastCopy(sprites, (void*)OAM, 512, DMA_16NOW);
}

void loadlevel(void *bgpalette, void *bgdata, u16 sprcolor1, u16 sprcolor2)
{
     u16 i=0;
     u16 x=0;
     u16* tmp;
     //load background palette
     DMAFastCopy(bgpalette, (void*)BGPaletteMem, 256, DMA_16NOW);
     //load background tile data
     DMAFastCopy(bgdata, (void*)CharBaseBlock(0), 19360, DMA_16NOW);
     //trasparency correction for background tile data
     tmp = (u16*)CharBaseBlock(0);
     for(i=0; i<32; i++)
     {
          *(tmp+(19360+i)) = 0;
     }
     for(i=1; i<7; i++)
     {
          x = *(tmp+(19200+4*i));
          *(tmp+(19200+4*i))=(x & 0x00FF);
          *(tmp+(19200+4*i+1)) = 0;
          *(tmp+(19200+4*i+2)) = 0;
          x = *(tmp+(19200+4*i+3));
          *(tmp+(19200+4*i+3))=(x & 0xFF00);
          x=0;
     }
     //generation of external to grid tiles
     for(i=0; i<64; i++)
     {
          *(tmp+(19392+i)) = *(tmp+(19232+i));
     }
     for(i=0; i<4; i++)
     {
          *(tmp+(19392+i)) = 0;
     }
     for(i=1; i<7; i++)
     {
          x = *(tmp+(19392+4*i));
          *(tmp+(19392+4*i)) = (x & 0xFF00);
          x = *(tmp+(19392+4*i+3));
          *(tmp+(19392+4*i+3)) = (x & 0x00FF);
     }
     for(i=28; i<32; i++)
     {
          *(tmp+(19392+i)) = 0;
     }
     for(i=0; i<4; i++)
     {
          *(tmp+(19424+i)) = 0;
     }
     for(i=1; i<7; i++)
     {
          x = *(tmp+(19424+4*i));
          *(tmp+(19424+4*i)) = (x & 0xFF00);
          x = *(tmp+(19424+4*i+3));
          *(tmp+(19424+4*i+3)) = (x & 0x00FF);
     }
     for(i=28; i<32; i++)
     {
          *(tmp+(19424+i)) = 0;
     }
     tmp=(u16*)OBJPaletteMem;
     *(tmp+4)=sprcolor1;
     *(tmp+3)=sprcolor2;
     level++;
}

void drawsweepbar()
{
    //Sweep line digits sprites 
    sprites[1].attribute0 = COLOR_256 | SQUARE | 40;
	sprites[1].attribute1 = SIZE_8 | (47 + swpbrpos);
	sprites[2].attribute0 = COLOR_256 | SQUARE | 40;
	sprites[2].attribute1 = SIZE_8 | (39 + swpbrpos);
    //Sweep line sprites
    sprites[3].attribute0 = COLOR_256 | SQUARE | 40;
	sprites[3].attribute1 = SIZE_8 | (39 + swpbrpos);;
	sprites[3].attribute2 = 48;
    sprites[4].attribute0 = COLOR_256 | SQUARE | 40;
	sprites[4].attribute1 = SIZE_8 | (47 + swpbrpos);
	sprites[4].attribute2 = 50;
    sprites[5].attribute0 = COLOR_256 | SQUARE | 40;
	sprites[5].attribute1 = SIZE_8 | (55 + swpbrpos);
	sprites[5].attribute2 = 52;
    sprites[6].attribute0 = COLOR_256 | SQUARE | 48;
	sprites[6].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[6].attribute2 = 46;	        
    sprites[7].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[7].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[7].attribute2 = 46;	        
    sprites[8].attribute0 = COLOR_256 | SQUARE | 64;
	sprites[8].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[8].attribute2 = 46;	        
    sprites[9].attribute0 = COLOR_256 | SQUARE | 72;
	sprites[9].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[9].attribute2 = 46;
    sprites[10].attribute0 = COLOR_256 | SQUARE | 80;
	sprites[10].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[10].attribute2 = 46;	   
    sprites[11].attribute0 = COLOR_256 | SQUARE | 88;
	sprites[11].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[11].attribute2 = 46;	   
    sprites[12].attribute0 = COLOR_256 | SQUARE | 96;
	sprites[12].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[12].attribute2 = 46;	   
    sprites[13].attribute0 = COLOR_256 | SQUARE | 104;
	sprites[13].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[13].attribute2 = 46;	   
    sprites[14].attribute0 = COLOR_256 | SQUARE | 112;
	sprites[14].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[14].attribute2 = 46;	 
    sprites[15].attribute0 = COLOR_256 | SQUARE | 120;
	sprites[15].attribute1 = SIZE_8 | (49 + swpbrpos);
	sprites[15].attribute2 = 46;
}

void spriteinit()
{
	//"score" sprites
	sprites[16].attribute0 = COLOR_256 | SQUARE | 48;
	sprites[16].attribute1 = SIZE_8 | 200;
	sprites[16].attribute2 = 16;
	sprites[17].attribute0 = COLOR_256 | SQUARE | 48;
	sprites[17].attribute1 = SIZE_8 | 208;
	sprites[17].attribute2 = 18;
	sprites[18].attribute0 = COLOR_256 | SQUARE | 48;
	sprites[18].attribute1 = SIZE_8 | 216;
	sprites[18].attribute2 = 20;
	sprites[19].attribute0 = COLOR_256 | SQUARE | 48;
	sprites[19].attribute1 = SIZE_8 | 224;
	sprites[19].attribute2 = 22;
	//"level" sprites
	sprites[20].attribute0 = COLOR_256 | SQUARE | 72;
	sprites[20].attribute1 = SIZE_8 | 200;
	sprites[20].attribute2 = 0;
	sprites[21].attribute0 = COLOR_256 | SQUARE | 72;
	sprites[21].attribute1 = SIZE_8 | 208;
	sprites[21].attribute2 = 2;
	sprites[22].attribute0 = COLOR_256 | SQUARE | 72;
	sprites[22].attribute1 = SIZE_8 | 216;
	sprites[22].attribute2 = 4;
	sprites[23].attribute0 = COLOR_256 | SQUARE | 72;
	sprites[23].attribute1 = SIZE_8 | 224;
	sprites[23].attribute2 = 6;
	//"dels " sprites
	sprites[24].attribute0 = COLOR_256 | SQUARE | 96;
	sprites[24].attribute1 = SIZE_8 | 200;
	sprites[24].attribute2 = 8;
	sprites[25].attribute0 = COLOR_256 | SQUARE | 96;
	sprites[25].attribute1 = SIZE_8 | 208;
	sprites[25].attribute2 = 10;
	sprites[26].attribute0 = COLOR_256 | SQUARE | 96;
	sprites[26].attribute1 = SIZE_8 | 216;
	sprites[26].attribute2 = 12;
	sprites[27].attribute0 = COLOR_256 | SQUARE | 96;
	sprites[27].attribute1 = SIZE_8 | 224;
	sprites[27].attribute2 = 14;
	
	//score digit sprites
	sprites[28].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[28].attribute1 = SIZE_8 | 224;
	sprites[28].attribute2 = 44;
	sprites[29].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[29].attribute1 = SIZE_8 | 216;
	sprites[29].attribute2 = 44;
	sprites[30].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[30].attribute1 = SIZE_8 | 208;
	sprites[30].attribute2 = 44;
	sprites[31].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[31].attribute1 = SIZE_8 | 200;
	sprites[31].attribute2 = 44;
	sprites[32].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[32].attribute1 = SIZE_8 | 192;
	sprites[32].attribute2 = 44;
	sprites[33].attribute0 = COLOR_256 | SQUARE | 56;
	sprites[33].attribute1 = SIZE_8 | 184;
	sprites[33].attribute2 = 44;
	
	//level digits sprites
	sprites[34].attribute0 = COLOR_256 | SQUARE | 80;
	sprites[34].attribute1 = SIZE_8 | 224;
	sprites[34].attribute2 = 44;
	sprites[35].attribute0 = COLOR_256 | SQUARE | 80;
	sprites[35].attribute1 = SIZE_8 | 216;
	sprites[35].attribute2 = 44;
	sprites[36].attribute0 = COLOR_256 | SQUARE | 80;
	sprites[36].attribute1 = SIZE_8 | 208;
	sprites[36].attribute2 = 44;
	sprites[37].attribute0 = COLOR_256 | SQUARE | 80;
	sprites[37].attribute1 = SIZE_8 | 200;
	sprites[37].attribute2 = 44;
	
	//deletes digits sprites
	sprites[38].attribute0 = COLOR_256 | SQUARE | 104;
	sprites[38].attribute1 = SIZE_8 | 224;
	sprites[38].attribute2 = 44;
	sprites[39].attribute0 = COLOR_256 | SQUARE | 104;
	sprites[39].attribute1 = SIZE_8 | 216;
	sprites[39].attribute2 = 44;
	sprites[40].attribute0 = COLOR_256 | SQUARE | 104;
	sprites[40].attribute1 = SIZE_8 | 208;
	sprites[40].attribute2 = 44;
	sprites[41].attribute0 = COLOR_256 | SQUARE | 104;
	sprites[41].attribute1 = SIZE_8 | 200;
	sprites[41].attribute2 = 44;
	
	//other sprites
	sprites[42].attribute0 = COLOR_256 | SQUARE | 160;
	sprites[42].attribute1 = SIZE_16 | 240;
	sprites[42].attribute2 = 76;
	sprites[0].attribute0 = COLOR_256 | TALL | 160;
	sprites[0].attribute1 = SIZE_64 | 240;
	sprites[0].attribute2 = 84;
}

void displayupdate()
{
    u8 i;
    //score display update
    u32 temp=score;
    u16 tmp;
    tmp=temp%10;
    temp=temp/10;
    sprites[28].attribute2=(2*tmp+24);
    tmp=temp%10;
    temp=temp/10;
    sprites[29].attribute2=(2*tmp+24);
    for(i=0; i<4; i++)
    {
        tmp=temp%10;
        temp=temp/10;
        if (tmp==0) 
        {
            if(temp>0) {sprites[(30+i)].attribute2=(2*tmp+24);} else {sprites[(30+i)].attribute2=44;}
        }
        else
        {
            sprites[(30+i)].attribute2=(2*tmp+24);
        }
    }
    //level display update
    temp=level;
    tmp=temp%10;
    temp=temp/10;
    sprites[34].attribute2=(2*tmp+24);
    tmp=temp%10;
    temp=temp/10;
    sprites[35].attribute2=(2*tmp+24);
    for(i=0; i<2; i++)
    {
        tmp=temp%10;
        temp=temp/10;
        if (tmp==0) 
        {
            if(temp>0) {sprites[(36+i)].attribute2=(2*tmp+24);} else {sprites[(36+i)].attribute2=44;}
        }
        else
        {
            sprites[(36+i)].attribute2=(2*tmp+24);
        }
    }
    //deletes display update
    temp=deletes;
    tmp=temp%10;
    temp=temp/10;
    sprites[38].attribute2=(2*tmp+24);
    tmp=temp%10;
    temp=temp/10;
    sprites[39].attribute2=(2*tmp+24);
    for(i=0; i<2; i++)
    {
        tmp=temp%10;
        temp=temp/10;
        if (tmp==0) 
        {
            if(temp>0) {sprites[(40+i)].attribute2=(2*tmp+24);} else {sprites[(40+i)].attribute2=44;}
        }
        else
        {
            sprites[(40+i)].attribute2=(2*tmp+24);
        }
    }
    //sweep line deletes display update
    temp=sweepdels;
    tmp=temp%10;
    temp=temp/10;
    sprites[1].attribute2=(2*tmp+54);
    if (temp==0) {sprites[2].attribute2=74;} else {sprites[2].attribute2=(2*temp+54);}
}


void gridtranslate()
{
    u16 i,j;
    u16 *tmp=(u16 *)ScreenBaseBlock(30);
    for(i=0; i<10; i++)
    {
         for(j=0; j<16; j++)
         {
              *(tmp+(32*i)+j+199) = (grid[i][j]+600+2*(gridstat[i][j]==1));
         }
    }                      
}


void dnblocks()
{
    u16 *tmp=(u16 *)ScreenBaseBlock(30);
    *(tmp+131)= nextb1[0]+605;
    *(tmp+132)= nextb1[1]+605;
    *(tmp+163)= nextb1[3]+605;
    *(tmp+164)= nextb1[2]+605;
    *(tmp+227)= nextb2[0]+605;
    *(tmp+228)= nextb2[1]+605;
    *(tmp+259)= nextb2[3]+605;
    *(tmp+260)= nextb2[2]+605;
    *(tmp+323)= nextb3[0]+605;
    *(tmp+324)= nextb3[1]+605;
    *(tmp+355)= nextb3[3]+605;
    *(tmp+356)= nextb3[2]+605;                      
}

void dcblock()
{
    u16 *tmp=(u16 *)ScreenBaseBlock(29);
    *(tmp+(167+32*cbpy+cbpx))=605;
    *(tmp+(168+32*cbpy+cbpx))=605;
    *(tmp+(135+32*cbpy+cbpx))=605;
    *(tmp+(136+32*cbpy+cbpx))=605;
    *(tmp+(167+32*cby+cbx))=cb[3]+605;
    *(tmp+(168+32*cby+cbx))=cb[2]+605;
    *(tmp+(135+32*cby+cbx))=cb[0]+605;
    *(tmp+(136+32*cby+cbx))=cb[1]+605;    
}

void mblockinit(mbpointer mbel)
{
     u8 p;
     u8 q;
     mbel->prev=NULL;
     mbel->next=NULL;
     mbel->size=0;
     mbel->type=0;
     mbel->delcoordinate=19;
     mbel->startcoordinate=0; 
     mbel->stdelcoor=0;
     mbel->normalflag=1;
     for(p=0; p<10; p++)
     {
         for(q=0; q<16; q++)
         {
             mbel->data[p][q]=0;
         }
     }    
}

void adddelete(u8 a, u8 b)   //deletes: check if they belong to an existing macroblock, if not create one
{               
     mbpointer temp;
     mbpointer temp2;
     mbpointer temp3;
     u8 foundflag=0;
     temp = fmblock.next;
     while (temp!=NULL)  //checks if delete is already assigned to a macroblock: if so it stops the assigning procedure
     {
           if(temp->data[a][b]==1 && temp->data[a+1][b]==1 && temp->data[a][b+1]==1 && temp->data[a+1][b+1]==1)
           {foundflag=1;}
           temp=temp->next;
     }
     if(foundflag==0)
     {
         temp = fmblock.next;
         temp2= &fmblock;
         u8 attributed=0;
         while (temp!=NULL)
         {
              if(grid[a][b]==temp->type)
              {
                   if(temp->data[a][b]==1 || temp->data[a+1][b]==1 || temp->data[a][b+1]==1 || temp->data[a+1][b+1]==1)
                   {
                        if(attributed==0)
                        {
                            temp->data[a][b]=1; 
                            temp->data[a+1][b]=1; 
                            temp->data[a][b+1]=1; 
                            temp->data[a+1][b+1]=1;
                            temp->size++;
                            if ((b+2)>temp->delcoordinate) 
                            {temp->delcoordinate=(b+2);}
                            if (b<temp->startcoordinate)
                            {temp->startcoordinate=b;}
                            if(temp->normalflag==1)
                            {temp->stdelcoor=temp->startcoordinate;}
                            if(mbdatarebuildflag!=1)
                            {
                                if(swpbrcoor>temp->startcoordinate && swpbrcoor<temp->delcoordinate && b<swpbrcoor)
                                {temp->stdelcoor=swpbrcoor; temp->normalflag=0;}
                            }
                            attributed=1;
                        }
                        else  //when a delete can be attributed to two macroblocks, the macroblocks get merged
                        {
                            temp2->size=(temp2->size+temp->size);
                            if(temp->delcoordinate>temp2->delcoordinate) 
                            {temp2->delcoordinate=temp->delcoordinate;}
                            if(temp->startcoordinate<temp2->startcoordinate) 
                            {temp2->startcoordinate=temp->startcoordinate;}
                            if(temp->normalflag==1 && temp2->normalflag==1)
                            {temp2->stdelcoor=temp2->startcoordinate;}
                            else
                            {
                                temp2->normalflag=0;
                                if(temp->stdelcoor>temp2->stdelcoor)
                                {temp2->stdelcoor=temp->stdelcoor;}
                            }
                            u8 xlc;
                            u8 ylc;
                            for(xlc=0; xlc<10; xlc++)
                            {
                                for(ylc=0; ylc<16; ylc++)
                                {
                                    if(temp->data[xlc][ylc]==1) {temp2->data[xlc][ylc]=1;}
                                }
                            }
                            temp3=temp->prev;
                            temp3->next=temp->next;
                            temp3=temp->next;
                            temp3->prev=temp->prev;
                            free(temp);                            
                        }
                   }
              }
              temp2=temp;
              temp=temp->next; 
         }
         if(attributed==0)
         {
              temp = (macroblock*) malloc (sizeof(macroblock));
              mblockinit(temp);
              temp->prev=temp2;
              temp2->next=temp;
              temp->next=NULL;
              temp->type=grid[a][b];
              temp->size=1;
              temp->data[a][b]=1;
              temp->data[a+1][b]=1;
              temp->data[a][b+1]=1;
              temp->data[a+1][b+1]=1;
              temp->delcoordinate=(b+2);
              temp->startcoordinate=b;
              if(temp->normalflag==1)
              {temp->stdelcoor=temp->startcoordinate;}
              if(mbdatarebuildflag!=1)
              {
                  if(swpbrcoor>temp->startcoordinate && swpbrcoor<temp->delcoordinate && b<swpbrcoor)
                  {temp->stdelcoor=swpbrcoor; temp->normalflag=0;}
              }
         }
     }     
}

void macrodel()
{
     mbpointer temp;
     mbpointer temp2;
     temp=fmblock.next;
     u8 h;
     u8 k;
     u8 deleted=0;
     while(temp!=NULL)
     {
          if (temp->delcoordinate==swpbrcoor)
          {
               deleted=1;
               sweepdels+=temp->size;
               for(h=0; h<10; h++)
               {
                    for(k=temp->stdelcoor; k<16; k++)
                    {
                         if(temp->data[h][k]==1)
                         {
                              grid[h][k]=0;
                              gridstat[h][k]=0;
                         }
                    }
               }                              
          }
     temp=temp->next;
     }
     if(deleted==1)
     {
          temp=fmblock.next;
          while(temp!=NULL)
          {
               temp2=temp;
               temp=temp->next;
               free(temp2);
          }
          fmblock.next=NULL;
          mbdatarebuildflag=1;          
     }
}

void gamereset()
{
    cbx=7;
    cby=0;
    cbpx=7;
    cbpy=0;
    swpbrpos=0;
    swpbrcoor=0;
    swpbrprevcoor=0;
    mbdatarebuildflag=0;
    score=0;
    level=0;
    deletes=0;
    sweepdels=0;
    gravtmr=0;
    dropper=110;
    droptmr=0;
    presstime=0;
    pmovtmr=0;
    prottmr=0;
    breaker=0;
    swpbrtmr=0;
    swpbrrate=1;
    bsigtmr=0;
    lvtimer=0;
    mbpointer temp;
    mbpointer temp2;
    temp=fmblock.next;
    while(temp!=NULL)
    {
       temp2=temp;
       temp=temp->next;
       free(temp2);
    }
    fmblock.next=NULL;
}

int main()
{
     u32 i, j, x, y;
     REG_BG0CNT = (u16)0x1F83;  //0001 1111 1000 0011
     REG_BG1CNT = (u16)0x1E82;  //0001 1110 1000 0010
     REG_BG2CNT = (u16)0x1D81;  //0001 1101 1000 0001
     SetMode(MODE_0 | BG0_ENABLE | BG1_ENABLE | BG2_ENABLE |OBJ_ENABLE | OBJ_MAP_1D);
     //first sprite initialization     
     for (i=0; i<128; i++)
 	 {
		sprites[i].attribute0 = 160;	//y > 159
		sprites[i].attribute1 = 240;	//x > 239
	 }
     resetlabel:
	 spriteinit();
     copyOAM();
     //load sprites palette and tile data
     DMAFastCopy((void*)sprpal, (void*)OBJPaletteMem, 256, DMA_16NOW);
     DMAFastCopy((void*)sprdata, (void*)OAMdata, 74*32, DMA_16NOW);  
     //load level 1 data
     loadlevel((void *)lv1pal, (void *)lv1data, 0x03ff, 0x7e20); 
     //initialize background maps
     //background 1
     u16 *tmp= (u16*)ScreenBaseBlock(31);
     for(i=0; i<1024; i++)
     {*(tmp+i) = 0;}
     x=0;
     for(i=0; i<640; i+=32)
     {
         for(j=0; j<30; j++)
         {
                  *(tmp+(i+j)) = x;
                  x++;
         }
         for(j=30; j<32; j++)
         {
                  *(tmp+(i+j)) = 0;
         }
     }
     //background 2
     //tiles: 600 blank grid, 601 tilecolor1, 602 tilecolor2, 603 deletecolor1, 604 deletecolor2, 605 blankoffgrid, 606 tile1offgrid, 606 tile2offgrid, 
     tmp = (u16*)ScreenBaseBlock(30);
     for(i=0; i<32*6; i++)
     {
              *(tmp+i) = 605;
     }
     for(i=6; i<16; i++)
     {
         for(j=0; j<7; j++)
         {
                  *(tmp+(32*i)+j) = 605;
         }
         for(j=7; j<23; j++)
         {
                  *(tmp+(32*i)+j) = 600;
         }
         for(j=23; j<32; j++)
         {
                  *(tmp+(32*i)+j) = 605;
         }       
     }
     for(i=32*16; i<32*32; i++)
     {
         *(tmp+i) = 605;
     }
    //background3
    tmp = (u16*)ScreenBaseBlock(29);
    for(i=0; i<1024; i++)
     {
              *(tmp+i) = 605;
     }
     
     //Initialize grid and gridstat
     for(i=0; i<10; i++)
     {
        for(j=0; j<16; j++)
        {
                 grid[i][j]=0;
                 gridstat[i][j]=0;
        }
     }
     
     //initalize player blocks
     cb[0] = (rand()%2+1);
     cb[1] = (rand()%2+1);
     cb[2] = (rand()%2+1);
     cb[3] = (rand()%2+1);
     nextb1[0] = (rand()%2+1);
     nextb1[1] = (rand()%2+1);
     nextb1[2] = (rand()%2+1);
     nextb1[3] = (rand()%2+1);
     nextb2[0] = (rand()%2+1);
     nextb2[1] = (rand()%2+1);
     nextb2[2] = (rand()%2+1);
     nextb2[3] = (rand()%2+1);
     nextb3[0] = (rand()%2+1);
     nextb3[1] = (rand()%2+1);
     nextb3[2] = (rand()%2+1);
     nextb3[3] = (rand()%2+1);
     dnblocks();
     dcblock();
	 
     //initialize macroblocks list
     fmblock.next=NULL;	
     //main game loop
     while(1)
     {
         drawsweepbar();
	     displayupdate();
         copyOAM(); 
         dnblocks();
         gridtranslate();
         //gravity
         if(gravtmr==GRAVPER)
         {
             for(i=9; i>0; i--)
             {
                for(j=0; j<16; j++)
                {
                    if(grid[i][j]==0)
                    {
                        grid[i][j]=grid[i-1][j];
                        grid[i-1][j]=0;         
                    }
                }
             }
         gravtmr=0;
         }
        
         //movement control
         if(presstime>TTSLIDE) {pmovper=SLIDEPER;} else {pmovper=PMOVPER;} 
         if(KEY_RIGHT_PRESSED) 
         {
              if(pmovtmr>pmovper) 
              {
                   pmovtmr=0;
                   if(cby==0) {if(cbx<14) {cbpx=cbx; cbpy=cby; cbx++;}}
                   else if(cbx<14 && grid[cby-1][cbx+2]==0) {cbpx=cbx; cbpy=cby; cbx++;}
              }
              dcblock();
              presstime++;
         }
         else if(KEY_LEFT_PRESSED) 
         {
              if(pmovtmr>pmovper) 
              {
                   pmovtmr=0;
                   if(cby==0) {if(cbx>0) {cbpx=cbx; cbpy=cby; cbx--;}}
                   else if(cbx>0 && grid[cby-1][cbx-1]==0) {cbpx=cbx; cbpy=cby; cbx--;}
              }
              dcblock();
              presstime++;
         }
         else {presstime=0;}
         if(KEY_DOWN_PRESSED) {cbpx=cbx; cbpy=cby; if(cby<10 && breaker==0) {cby++; droptmr=90;} dcblock();} else {breaker=0;}
         if(droptmr>dropper) {cbpx=cbx; cbpy=cby; if(cby<10) {cby++; droptmr=90;} dcblock();}  
         if(KEY_A_PRESSED) {if (prottmr>PROTPER) {j=cb[0]; cb[0]=cb[1]; cb[1]=j; j=cb[2]; cb[2]=cb[3]; cb[3]=j; j=cb[1]; cb[1]=cb[3]; cb[3]=j; prottmr=0;} dcblock();}
         if(KEY_B_PRESSED) {if (prottmr>PROTPER) {j=cb[0]; cb[0]=cb[1]; cb[1]=j; j=cb[2]; cb[2]=cb[3]; cb[3]=j; j=cb[0]; cb[0]=cb[2]; cb[2]=j; prottmr=0;} dcblock();}

         //this puts the player controlled block in the grin and shifts player control to the next block. this is where a game over can occurr
         if(cby==10 || grid[cby][cbx]!=0 || grid[cby][cbx+1]!=0)
         {
              if(cby>1)
              {
                   cbpx=cbx;
                   cbpy=cby;
                   grid[cby-1][cbx]=cb[3];
                   grid[cby-1][cbx+1]=cb[2];
                   grid[cby-2][cbx+1]=cb[1];
                   grid[cby-2][cbx]=cb[0];
                   cbx=7;
                   cby=0;
                   for(i=0; i<4; i++) {cb[i]=nextb1[i];}
                   for(i=0; i<4; i++) {nextb1[i]=nextb2[i];}
                   for(i=0; i<4; i++) {nextb2[i]=nextb3[i];}
                   for(i=0; i<4; i++) {nextb3[i]=((rand()%2)+1);}
                   dcblock();
                   droptmr=0;    
              }
              else
              {
                  sprites[0].attribute0 = COLOR_256 | TALL | 64;
              	  sprites[0].attribute1 = SIZE_64 | 88;
	              sprites[0].attribute2 = 84;
	              copyOAM();
                  while(1)
                  {
                      if(KEY_A_PRESSED) //game reset handler
                      {
                          gamereset();
                          goto resetlabel;  
                      }
                  }
              }
              breaker=1;
         }
         
         //checks for deletes
         for(i=0; i<10; i++)   //reset stat grid (to prevent errors in certain cases)
         {
             for(j=0; j<16; j++)
             {
                 gridstat[i][j]=0;
             }
         }    
         for(i=0; i<9; i++)  //delete check
         {
             for(j=0; j<15; j++)
             {
                 if(grid[i][j]!=0 && grid[i][j+1]!=0)
                 {
                      if(grid[i][j]==grid[i+1][j] && grid[i][j]==grid[i][j+1] && grid[i][j]==grid[i+1][j+1])
                      {
                           u8 tmp=0;
                           u8 ctr=0;
                           for(ctr=i+1; ctr<10; ctr++)
                           {
                               if(grid[ctr][j]==0 || grid[ctr][j+1]==0) {tmp=1;}
                           }
                           if(tmp==0)
                           {
                           gridstat[i][j]=1;
                           gridstat[i+1][j]=1;
                           gridstat[i][j+1]=1;
                           gridstat[i+1][j+1]=1;
                           adddelete(i,j);
                           }
                      }
                 }             
             }
         }
         mbdatarebuildflag=0;      
         //sweep line movement
         if(swpbrtmr==swpbrper) {swpbrpos+=swpbrrate; swpbrtmr=0;}
         swpbrtmr++;
         if(swpbrpos>128) 
         {
              if(sweepdels>3)
              {
                  deletes+=sweepdels;
                  score+=(sweepdels*160);
                  sprites[42].attribute0 = COLOR_256 | SQUARE | 112;
             	  sprites[42].attribute1 = SIZE_16 | 24;
	              sprites[42].attribute2 = 76;
	              bsigtmr=150;
	              sweepdels=0;
                  swpbrpos=0;
              }
              else
              {
              deletes+=sweepdels;
              score+=(sweepdels*40);
              sweepdels=0;
              swpbrpos=0;
              }
         }
         //bonus signal management
         if(bsigtmr>0)
         {
              bsigtmr--;
         }
         else
         {
              sprites[42].attribute0 = COLOR_256 | SQUARE | 160;
           	  sprites[42].attribute1 = SIZE_16 | 240;
	          sprites[42].attribute2 = 76;
         }
         
         
         swpbrcoor=(swpbrpos/8); 
         if(swpbrcoor!=swpbrprevcoor)  //macroblocks deletion
         {
              macrodel();  
         }
         swpbrprevcoor=swpbrcoor;
         //increments functions counters
         gravtmr++;
         pmovtmr++;
         prottmr++;
         droptmr++;
         //level advancement timer and skin management
         lvtimer++;
         if(lvtimer==LVDUR) {loadlevel((void *)lv2pal, (void *)lv2data, 0x03ff, 0x7e20); dropper--;}
         if(lvtimer==2*LVDUR) {loadlevel((void *)lv3pal, (void *)lv3data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==3*LVDUR) {loadlevel((void *)lv4pal, (void *)lv4data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==4*LVDUR) {loadlevel((void *)lv5pal, (void *)lv5data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==5*LVDUR) {loadlevel((void *)lv6pal, (void *)lv6data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==6*LVDUR) {loadlevel((void *)lv7pal, (void *)lv7data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==7*LVDUR) {loadlevel((void *)lv8pal, (void *)lv8data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==8*LVDUR) {loadlevel((void *)lv9pal, (void *)lv9data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==9*LVDUR) {loadlevel((void *)lv10pal, (void *)lv10data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==10*LVDUR) {loadlevel((void *)lv11pal, (void *)lv11data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==11*LVDUR) {loadlevel((void *)lv12pal, (void *)lv12data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==12*LVDUR) {loadlevel((void *)lv13pal, (void *)lv13data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==13*LVDUR) {loadlevel((void *)lv14pal, (void *)lv14data, 0x03ff, 0x001f); dropper--;}
         if(lvtimer==14*LVDUR) {loadlevel((void *)lv15pal, (void *)lv15data, 0x03ff, 0x001f); dropper-=2;}
         if(lvtimer==15*LVDUR) 
         {
             lselector=(rand()%15);
             lvtimer=14*LVDUR;
             if(lselector==0) {loadlevel((void *)lv1pal, (void *)lv1data, 0x03ff, 0x7e20);}
             else if(lselector==1) {loadlevel((void *)lv2pal, (void *)lv2data, 0x03ff, 0x001f);}
             else if(lselector==2) {loadlevel((void *)lv3pal, (void *)lv3data, 0x03ff, 0x001f);}
             else if(lselector==3) {loadlevel((void *)lv4pal, (void *)lv4data, 0x03ff, 0x001f);}
             else if(lselector==4) {loadlevel((void *)lv5pal, (void *)lv5data, 0x03ff, 0x001f);}
             else if(lselector==5) {loadlevel((void *)lv6pal, (void *)lv6data, 0x03ff, 0x001f);}
             else if(lselector==6) {loadlevel((void *)lv7pal, (void *)lv7data, 0x03ff, 0x001f);}
             else if(lselector==7) {loadlevel((void *)lv8pal, (void *)lv8data, 0x03ff, 0x001f);}
             else if(lselector==8) {loadlevel((void *)lv9pal, (void *)lv9data, 0x03ff, 0x001f);}
             else if(lselector==9) {loadlevel((void *)lv10pal, (void *)lv10data, 0x03ff, 0x001f);}
             else if(lselector==10) {loadlevel((void *)lv11pal, (void *)lv11data, 0x03ff, 0x001f);}
             else if(lselector==11) {loadlevel((void *)lv12pal, (void *)lv12data, 0x03ff, 0x001f);}
             else if(lselector==12) {loadlevel((void *)lv13pal, (void *)lv13data, 0x03ff, 0x001f);}
             else if(lselector==13) {loadlevel((void *)lv14pal, (void *)lv14data, 0x03ff, 0x001f);}
             else {loadlevel((void *)lv15pal, (void *)lv15data, 0x03ff, 0x001f);}
         }
         
         //waits for vsync to begin next cycle, should stabilize program to about 60 cycles per second
         while(REG_VCOUNT !=160){}
     }
}

