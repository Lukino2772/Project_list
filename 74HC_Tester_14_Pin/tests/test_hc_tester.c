#include <assert.h>
#include "hc_tester.c"

GPIO_TypeDef mock_port;
I2C_HandleTypeDef hi2c1;
static uint32_t mode[15], pull[15], level[15];
static const Device *dut;
static int fault;
static char response[64];
static char screen[2][17];
static unsigned lcd_row, lcd_col;
static uint8_t lcd_hi, lcd_half, lcd_prev;

uint32_t HAL_GetTick(void) { return 0; }
uint8_t CDC_Transmit_FS(uint8_t *data,uint16_t len) {
  assert(len<sizeof(response));
  memcpy(response,data,len);
  response[len]='\0';
  return 0;
}
void HAL_GPIO_Init(GPIO_TypeDef *p, GPIO_InitTypeDef *s) {
  (void)p; mode[s->Pin]=s->Mode;pull[s->Pin]=s->Pull;
}
void HAL_GPIO_WritePin(GPIO_TypeDef *p,uint16_t n,int v) { (void)p;level[n]=(uint32_t)v; }
static int input(unsigned p) { return mode[p]==GPIO_MODE_OUTPUT_OD && !level[p]?0:(int)pull[p]; }
int HAL_GPIO_ReadPin(GPIO_TypeDef *port,uint16_t pin) {
  (void)port;
  if(dut)for(unsigned g=0;g<dut->count;g++) {
    const Gate *gate=&dut->gates[g];
    if(gate->output!=pin)continue;
    if(fault==1 && g==0)return 0; /* stuck output */
    if(fault==2 && g==0)return (int)pull[pin]; /* open output */
    if(dut->tristate) {
      int enable=input(enables125[g]);
      if(fault==4 && g==0)enable|=input(enables125[1]);
      if(enable!=dut->oe_high)return (int)pull[pin];
    }
    /* Independent Boolean evaluation of the fixture's gate description. */
    unsigned ones=0;
    for(unsigned i=0;i<gate->count;i++)ones+=(unsigned)input(gate->inputs[i]);
    int value=0;
    switch(gate->op) {
      case OP_AND: value=ones==gate->count;break;
      case OP_NAND:value=ones!=gate->count;break;
      case OP_OR:value=ones!=0;break;
      case OP_NOR:value=ones==0;break;
      case OP_XOR:value=(int)(ones%2);break;
      case OP_XNOR:value=!(ones%2);break;
      case OP_NOT:value=!ones;break;
      case OP_BUF:value=!!ones;break;
    }
    /* HC04 gate 1 incorrectly responds to gate 2's input. Isolated tests miss it. */
    if(fault==3 && g==0)value^=input(3);
    return value;
  }
  return input(pin);
}
void HAL_Delay(uint32_t ms) { (void)ms; }
int HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *h,uint16_t a,int n,int t) {
  (void)h;(void)a;(void)n;(void)t;return 1;
}
int HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h,uint16_t a,uint8_t *v,int n,int t) {
  (void)h;(void)a;(void)n;(void)t;
  if((lcd_prev&4) && !(*v&4)) {
    if(!lcd_half) {lcd_hi=*v&0xf0;lcd_half=1;}
    else {
      uint8_t byte=lcd_hi|(*v>>4);lcd_half=0;
      if(*v&1) {if(lcd_col<16)screen[lcd_row][lcd_col++]=(char)byte;}
      else if(byte==0x80 || byte==0xc0) {lcd_row=byte==0xc0;lcd_col=0;}
    }
  }
  lcd_prev=*v;return 0;
}
static void reset(void) {
  fault=0;all_safe();HC_Tester_USBReset();
}
static void released(void) {
  for(unsigned p=1;p<14;p++)if(p!=7)assert(mode[p]==GPIO_MODE_INPUT);
}
int main(void) {
  lcd_address=0x4e;
  for(unsigned d=0;d<ARRAYLEN(devices);d++) {
    reset();dut=&devices[d];assert(test_device(dut));released();
    for(fault=1;fault<=2;fault++) {assert(!test_device(dut));released();}
  }
  reset();dut=&devices[2];fault=3;assert(!test_device(dut));released();
  reset();dut=&devices[19];fault=4;assert(!test_device(dut));released();
  reset();dut=0;assert(!test_device(&devices[0]));released();
  reset();dut=&devices[2];level[StartKey_Pin]=1;
  HC_Tester_USBReceive((const uint8_t *)"RU",2);
  HC_Tester_Poll();
  assert(response[0]=='\0');
  HC_Tester_USBReceive((const uint8_t *)"N\r\nSTATUS\n",10);
  HC_Tester_Poll();
  assert(strcmp(response,"HC1,PASS,74HC04/14,1\n")==0);
  assert(strncmp(screen[0],"74HC04/14",9)==0);
  assert(level[DUT_EN_Pin]);released();

  reset();dut=0;
  HC_Tester_USBReceive((const uint8_t *)"123456789RUN\n",13);
  HC_Tester_Poll();
  assert(strcmp(response,"HC1,PASS,74HC04/14,1\n")==0);
  HC_Tester_USBReceive((const uint8_t *)"RUN\n",4);
  HC_Tester_Poll();
  assert(strcmp(response,"HC1,NO_MATCH,-,0\n")==0);
  assert(strncmp(screen[0],"NO MATCH",8)==0);
  assert(level[DUT_EN_Pin]);released();
  puts("PASS: 20 device models, gate and enable faults, USB framing, LCD results, power release.");
}
