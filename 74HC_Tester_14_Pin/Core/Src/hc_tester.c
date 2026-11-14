/*
 * 74HC 14-pin combinational IC identifier.
 * Assumptions: 1k series resistors on each ZIF signal, fixed 3.3V on pin 14,
 * GND on pin 7; test only known-compatible 14-pin CMOS at 3.3V.
 * All stimulus is open drain with weak internal pull-up to limit contention.
 *
 * Not a universal or safety-certified fault tester. An unknown chip can have
 * different power pins. Do NOT insert/remove while powered.
 */
#include "main.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

/* PB0 = ADC1_IN8 = powered ZIF14 side of 47-ohm resistor.
 * The resistor's upstream rail MUST match MCU VDDA for this calculation.
 * STM32F401xC factory VREFINT calibration word at 0x1FFF7A2A (3.3V).
 * No-load calibration requires EMPTY socket, held StartKey for >=2 seconds.
 * Software current checks are diagnostic; they cannot replace hardware
 * current limiting or protect against an instantaneous short.
 */
extern ADC_HandleTypeDef hadc1;
#define HC_R_MOHM 47000L       /* 47 ohms expressed in milliohms */
#define HC_MAX_CURRENT_UA 10000L  /* nominal trip threshold 10mA */
#define HC_MIN_DUT_MV 2700L
#define VREFINT_CAL_F401 (*(const uint16_t *)0x1FFF7A2AU)
static int32_t zero_drop_mv;
static int32_t last_current_ua;
static int32_t last_dut_mv;
static int32_t last_vdda_mv;
static uint8_t current_fault;

static uint8_t adc_average(uint32_t channel, uint32_t *value) {
  ADC_ChannelConfTypeDef cfg = {0};
  uint32_t sum=0;
  uint8_t i;
  cfg.Channel=channel;
  cfg.Rank=1;
  cfg.SamplingTime=ADC_SAMPLETIME_480CYCLES;
  if(HAL_ADC_ConfigChannel(&hadc1,&cfg)!=HAL_OK)return 0;
  for(i=0;i<16;i++) {
    if(HAL_ADC_Start(&hadc1)!=HAL_OK)return 0;
    if(HAL_ADC_PollForConversion(&hadc1,10)!=HAL_OK) {
      (void)HAL_ADC_Stop(&hadc1); return 0;
    }
    sum+=HAL_ADC_GetValue(&hadc1);
    if(HAL_ADC_Stop(&hadc1)!=HAL_OK)return 0;
  }
  *value=(sum+8u)/16u;
  return 1;
}
static uint8_t read_current(void) {
  uint32_t ref_raw,pin_raw;
  const uint32_t cal=VREFINT_CAL_F401;
  int32_t drop;
  if(cal<500u || cal>2500u)return 0;
  if(!adc_average(ADC_CHANNEL_VREFINT,&ref_raw) || ref_raw<500u)return 0;
  if(!adc_average(ADC_CHANNEL_8,&pin_raw))return 0;
  last_vdda_mv=(int32_t)((3300u*cal+ref_raw/2u)/ref_raw);
  if(last_vdda_mv<2800 || last_vdda_mv>3600)return 0;
  last_dut_mv=(int32_t)((pin_raw*(uint32_t)last_vdda_mv+2047u)/4095u);
  drop=last_vdda_mv-last_dut_mv-zero_drop_mv;
  last_current_ua=(drop>0)?(drop*1000000L/47000L):0L;
  return 1;
}
static uint8_t current_ok(void) {
  if(!read_current() || last_current_ua>HC_MAX_CURRENT_UA ||
      last_dut_mv<HC_MIN_DUT_MV) {
    current_fault=1; return 0;
  }
  return 1;
}

typedef struct { GPIO_TypeDef *port; uint16_t mask; } IO;
static const IO zif[15] = {
  {0,0}, {ZIF1_GPIO_Port,ZIF1_Pin}, {ZIF2_GPIO_Port,ZIF2_Pin},
  {ZIF3_GPIO_Port,ZIF3_Pin}, {ZIF4_GPIO_Port,ZIF4_Pin},
  {ZIF5_GPIO_Port,ZIF5_Pin}, {ZIF6_GPIO_Port,ZIF6_Pin},
  {0,0}, {ZIF8_GPIO_Port,ZIF8_Pin}, {ZIF9_GPIO_Port,ZIF9_Pin},
  {ZIF10_GPIO_Port,ZIF10_Pin}, {ZIF11_GPIO_Port,ZIF11_Pin},
  {ZIF12_GPIO_Port,ZIF12_Pin}, {ZIF13_GPIO_Port,ZIF13_Pin}, {0,0}
};

enum { OP_AND, OP_NAND, OP_OR, OP_NOR, OP_XOR, OP_XNOR, OP_NOT, OP_BUF };
typedef struct { uint8_t inputs[8]; uint8_t output, count, op; } Gate;
typedef struct { const char *name; const Gate *gates; uint8_t count; uint8_t tristate; uint8_t oe_high; } Device;
#define G1(a,y,op)          {{a},y,1,op}
#define G2(a,b,y,op)        {{a,b},y,2,op}
#define G3(a,b,c,y,op)      {{a,b,c},y,3,op}
#define G4(a,b,c,d,y,op)    {{a,b,c,d},y,4,op}
#define G8(a,b,c,d,e,f,g,h,y,op) {{a,b,c,d,e,f,g,h},y,8,op}

static const Gate gates00[] = {G2(1,2,3,OP_NAND),G2(4,5,6,OP_NAND),
                              G2(9,10,8,OP_NAND),G2(12,13,11,OP_NAND)};
static const Gate gates02[] = {G2(2,3,1,OP_NOR),G2(5,6,4,OP_NOR),
                              G2(8,9,10,OP_NOR),G2(11,12,13,OP_NOR)};
static const Gate gates04[] = {G1(1,2,OP_NOT),G1(3,4,OP_NOT),G1(5,6,OP_NOT),
                              G1(9,8,OP_NOT),G1(11,10,OP_NOT),G1(13,12,OP_NOT)};
static const Gate gates08[] = {G2(1,2,3,OP_AND),G2(4,5,6,OP_AND),
                              G2(9,10,8,OP_AND),G2(12,13,11,OP_AND)};
static const Gate gates10[] = {G3(1,2,13,12,OP_NAND),G3(3,4,5,6,OP_NAND),
                              G3(9,10,11,8,OP_NAND)};
static const Gate gates11[] = {G3(1,2,13,12,OP_AND),G3(3,4,5,6,OP_AND),
                              G3(9,10,11,8,OP_AND)};
static const Gate gates20[] = {G4(1,2,4,5,6,OP_NAND),G4(9,10,12,13,8,OP_NAND)};
static const Gate gates21[] = {G4(1,2,4,5,6,OP_AND),G4(9,10,12,13,8,OP_AND)};
static const Gate gates27[] = {G3(1,2,13,12,OP_NOR),G3(3,4,5,6,OP_NOR),
                              G3(9,10,11,8,OP_NOR)};
static const Gate gates30[] = {G8(1,2,3,4,5,6,11,12,8,OP_NAND)};
static const Gate gates32[] = {G2(1,2,3,OP_OR),G2(4,5,6,OP_OR),
                              G2(9,10,8,OP_OR),G2(12,13,11,OP_OR)};
static const Gate gates86[] = {G2(1,2,3,OP_XOR),G2(4,5,6,OP_XOR),
                              G2(9,10,8,OP_XOR),G2(12,13,11,OP_XOR)};

/* Alternate 4000-series quad-gate layout, also used by 74HC4001/4011 etc. */
static const Gate gates4001[] = {G2(1,2,3,OP_NOR),G2(5,6,4,OP_NOR),
                                G2(8,9,10,OP_NOR),G2(12,13,11,OP_NOR)};
static const Gate gates4011[] = {G2(1,2,3,OP_NAND),G2(5,6,4,OP_NAND),
                                G2(8,9,10,OP_NAND),G2(12,13,11,OP_NAND)};
static const Gate gates4070[] = {G2(1,2,3,OP_XOR),G2(5,6,4,OP_XOR),
                                G2(8,9,10,OP_XOR),G2(12,13,11,OP_XOR)};
static const Gate gates4071[] = {G2(1,2,3,OP_OR),G2(5,6,4,OP_OR),
                                G2(8,9,10,OP_OR),G2(12,13,11,OP_OR)};
static const Gate gates4077[] = {G2(1,2,3,OP_XNOR),G2(5,6,4,OP_XNOR),
                                G2(8,9,10,OP_XNOR),G2(12,13,11,OP_XNOR)};
static const Gate gates4081[] = {G2(1,2,3,OP_AND),G2(5,6,4,OP_AND),
                                G2(8,9,10,OP_AND),G2(12,13,11,OP_AND)};

static const Gate gates125[] = {G1(2,3,OP_BUF),G1(5,6,OP_BUF),
                               G1(9,8,OP_BUF),G1(12,11,OP_BUF)};
static const uint8_t enables125[] = {1,4,10,13};
#define ARRAYLEN(a) (sizeof(a)/sizeof((a)[0]))
static const Device devices[] = {
  {"74HC00/132",gates00,ARRAYLEN(gates00),0,0},
  {"74HC02",gates02,ARRAYLEN(gates02),0,0},
  {"74HC04/14",gates04,ARRAYLEN(gates04),0,0},
  {"74HC08",gates08,ARRAYLEN(gates08),0,0},
  {"74HC10",gates10,ARRAYLEN(gates10),0,0},
  {"74HC11",gates11,ARRAYLEN(gates11),0,0},
  {"74HC20",gates20,ARRAYLEN(gates20),0,0},
  {"74HC21",gates21,ARRAYLEN(gates21),0,0},
  {"74HC27",gates27,ARRAYLEN(gates27),0,0},
  {"74HC30",gates30,ARRAYLEN(gates30),0,0},
  {"74HC32",gates32,ARRAYLEN(gates32),0,0},
  {"74HC86",gates86,ARRAYLEN(gates86),0,0},
  {"74HC4001",gates4001,ARRAYLEN(gates4001),0,0},
  {"74HC4011",gates4011,ARRAYLEN(gates4011),0,0},
  {"74HC4070",gates4070,ARRAYLEN(gates4070),0,0},
  {"74HC4071",gates4071,ARRAYLEN(gates4071),0,0},
  {"74HC4077",gates4077,ARRAYLEN(gates4077),0,0},
  {"74HC4081",gates4081,ARRAYLEN(gates4081),0,0},
  {"74HC125",gates125,ARRAYLEN(gates125),1,0},
  {"74HC126",gates125,ARRAYLEN(gates125),1,1}
};

static void pin_mode(uint8_t p, uint32_t mode, uint32_t pull) {
  GPIO_InitTypeDef s={0};
  if (!zif[p].port) return;
  s.Pin=zif[p].mask; s.Mode=mode; s.Pull=pull; s.Speed=GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(zif[p].port,&s);
}
static void all_safe(void) {
  uint8_t p;
  for(p=1;p<=13;p++) if(p!=7) pin_mode(p,GPIO_MODE_INPUT,GPIO_PULLDOWN);
}
/* Set ODR *before* switching from input to open-drain output. */
static void drive(uint8_t p, uint8_t high) {
  HAL_GPIO_WritePin(zif[p].port,zif[p].mask,high?GPIO_PIN_SET:GPIO_PIN_RESET);
  pin_mode(p,GPIO_MODE_OUTPUT_OD,GPIO_PULLUP);
}
static uint8_t sense(uint8_t p) {
  return HAL_GPIO_ReadPin(zif[p].port,zif[p].mask)==GPIO_PIN_SET;
}
static uint8_t expected(uint8_t op, uint8_t bits, uint8_t count) {
  uint8_t i,r=0;
  if(op==OP_AND || op==OP_NAND) {
    r=1; for(i=0;i<count;i++) r &= (bits>>i)&1;
  } else if(op==OP_OR || op==OP_NOR) {
    for(i=0;i<count;i++) r |= (bits>>i)&1;
  } else if(op==OP_XOR || op==OP_XNOR) {
    for(i=0;i<count;i++) r ^= (bits>>i)&1;
  } else r=bits&1;
  if(op==OP_NAND || op==OP_NOR || op==OP_XNOR || op==OP_NOT) r^=1;
  return r;
}
/* Recheck both polarities to reject floating/open output pins. */
static uint8_t output_matches(uint8_t p,uint8_t v) {
  pin_mode(p,GPIO_MODE_INPUT,GPIO_PULLUP);
  HAL_Delay(1);
  if(sense(p)!=v) return 0;
  pin_mode(p,GPIO_MODE_INPUT,GPIO_PULLDOWN);
  HAL_Delay(1);
  if(sense(p)!=v) return 0;
  return 1;
}
/* In high-impedance mode weak pull-up and pull-down should both win. */
static uint8_t output_floats(uint8_t p) {
  pin_mode(p,GPIO_MODE_INPUT,GPIO_PULLUP);
  HAL_Delay(2);
  if(!sense(p)) return 0;
  pin_mode(p,GPIO_MODE_INPUT,GPIO_PULLDOWN);
  HAL_Delay(2);
  if(sense(p)) return 0;
  return 1;
}
static uint8_t test_device(const Device *d) {
  uint8_t g,i,enable;
  uint16_t pat,n;
  for(g=0;g<d->count;g++) {
    const Gate *gate=&d->gates[g];
    enable=d->tristate?enables125[g]:0;
    n=(uint16_t)(1u<<gate->count);
    all_safe();
    if(d->tristate) {
      drive(enable,d->oe_high?0:1); /* explicitly disable before testing */
    }
    for(pat=0;pat<n;pat++) {
      for(i=0;i<gate->count;i++) drive(gate->inputs[i],(pat>>i)&1);
      if(d->tristate) drive(enable,d->oe_high?1:0);
      HAL_Delay(2);
      if(!current_ok()) { all_safe(); return 0; }
      if(!output_matches(gate->output,expected(gate->op,(uint8_t)pat,gate->count))) {
        all_safe(); return 0;
      }
      if(d->tristate) {
        drive(enable,d->oe_high?0:1); /* disable */
        HAL_Delay(2);
        if(!output_floats(gate->output)) { all_safe(); return 0; }
      }
    }
  }
  all_safe();
  return 1;
}

/* Conventional PCF8574 I2C backpack: P0=RS P1=RW P2=EN P3=BL, P4..P7=data */
static uint16_t lcd_address;
static void expander(uint8_t value) {
  if(lcd_address) (void)HAL_I2C_Master_Transmit(&hi2c1,lcd_address,&value,1,30);
}
static void nibble(uint8_t n,uint8_t rs) {
  uint8_t v=(n&0xF0)|0x08|(rs?1:0);
  expander(v); expander(v|0x04); expander(v);
}
static void lcd_byte(uint8_t v,uint8_t rs) {
  nibble(v&0xF0,rs); nibble((uint8_t)(v<<4),rs);
  HAL_Delay(1);
}
static void lcd_init(void) {
  uint8_t i;
  for(i=0x20;i<=0x27;i++)
    if(HAL_I2C_IsDeviceReady(&hi2c1,(uint16_t)(i<<1),2,15)==HAL_OK) {
      lcd_address=(uint16_t)(i<<1); break;
    }
  if(!lcd_address) for(i=0x38;i<=0x3F;i++)
    if(HAL_I2C_IsDeviceReady(&hi2c1,(uint16_t)(i<<1),2,15)==HAL_OK) {
      lcd_address=(uint16_t)(i<<1); break;
    }
  if(!lcd_address)return;
  HAL_Delay(50);
  nibble(0x30,0);HAL_Delay(5);nibble(0x30,0);HAL_Delay(5);
  nibble(0x30,0);HAL_Delay(2);nibble(0x20,0);HAL_Delay(2);
  lcd_byte(0x28,0);lcd_byte(0x0C,0);lcd_byte(0x06,0);
  lcd_byte(0x01,0);HAL_Delay(3);
}
static void lcd_line(uint8_t row,const char *s) {
  uint8_t i;
  lcd_byte(row?0xC0:0x80,0);
  for(i=0;i<16;i++) {
    if(*s) lcd_byte((uint8_t)*s++,1);
    else lcd_byte(' ',1);
  }
}

void HC_Tester_Calibrate(void) {
  uint8_t i;
  int32_t sum=0;
  all_safe();
  lcd_line(0,"REMOVE IC FIRST");
  lcd_line(1,"CALIBRATING...");
  HAL_Delay(600);
  zero_drop_mv=0;
  for(i=0;i<16;i++) {
    if(!read_current() || last_dut_mv<3000) {
      zero_drop_mv=0;
      lcd_line(0,"CALIBRATION FAIL");
      lcd_line(1,"CHECK 3V3/ADC");
      return;
    }
    sum+=last_vdda_mv-last_dut_mv;
  }
  zero_drop_mv=(sum>=0)?(sum+8)/16:(sum-8)/16;
  lcd_line(0,"ZERO CAL DONE");
  lcd_line(1,"EMPTY SOCKET OK");
}

void HC_Tester_Init(void) {
  all_safe(); /* immediately release CubeMX-configured push-pull pins */
  lcd_init();
  lcd_line(0,"74HC LOGIC TESTER");
  lcd_line(1,"PRESS START");
  (void)read_current();
}
void HC_Tester_Run(void) {
  uint8_t i,matched=0,first=0;
  current_fault=0;
  all_safe();
  if(!current_ok()) {
    all_safe();
    lcd_line(0,"SUPPLY/ADC FAULT");
    lcd_line(1,"CHECK DUT POWER");
    return;
  }
  char label[17];
  lcd_line(0,"TESTING...");
  lcd_line(1,"DO NOT REMOVE");
  for(i=0;i<ARRAYLEN(devices);i++) {
    if(current_fault)break;
    if(test_device(&devices[i])) {
      if(!matched)first=i;
      matched++;
    }
  }
  all_safe();
  if(current_fault) {
    lcd_line(0,"OVERLOAD/FAULT");
    lcd_line(1,"REMOVE DUT");
  } else if(matched==1) {
    lcd_line(0,"MATCH / PASS");
    if(!read_current()) last_current_ua=0;
    snprintf(label,sizeof(label),"%.7s %ld.%02ldmA",devices[first].name,
             (long)(last_current_ua/1000L),(long)((last_current_ua%1000L)/10L));
    lcd_line(1,label);
  } else if(matched>1) {
    lcd_line(0,"MULTIPLE MATCHES");
    snprintf(label,sizeof(label),"%u POSSIBLE TYPES",(unsigned)matched);
    lcd_line(1,label);
  } else {
    lcd_line(0,"UNKNOWN / FAULT");
    lcd_line(1,"CHECK IC & POWER");
  }
}
