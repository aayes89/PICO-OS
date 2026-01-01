#pragma once
#include "vm.h"
#include "pico/stdlib.h"
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <LittleFS.h>
#include <string.h>

typedef int32_t (*NativeFn)(int32_t *args, int argc);

typedef struct {
  const char *name;
  NativeFn fn;
  int argc;
} NativeEntry;

static inline const char *vm_get_str(int32_t v){
  return vm.string_pool[v & ~(1<<30)];
}

// ---------------- GPIO ----------------
int32_t fn_gpio_mode(int32_t *a,int c){
  pinMode(a[0], a[1]);   // 0=INPUT,1=OUTPUT,2=INPUT_PULLUP
  return 0;
}

int32_t fn_gpio_write(int32_t *a,int c){
  digitalWrite(a[0], a[1]);
  return 0;
}

int32_t fn_gpio_read(int32_t *a,int c){
  return digitalRead(a[0]);
}

// ---------------- Delay / Time --------
int32_t fn_sleep(int32_t *a,int c){
  delay(a[0]);
  return 0;
}

int32_t fn_millis(int32_t *a,int c){
  return (int32_t)millis();
}

// ---------------- ADC -----------------
int32_t fn_adc_init_pin(int32_t *a,int c){
  pinMode(a[0], INPUT);
  return 0;
}

int32_t fn_adc_read(int32_t *a,int c){
  return analogRead(a[0]);   // RP2040 = 12-bit fijo
}

// ---------------- PWM -----------------
int32_t fn_pwm_attach(int32_t *a,int c){
  pinMode(a[0], OUTPUT);
  analogWrite(a[0], 0);
  return 0;
}

int32_t fn_pwm_write(int32_t *a,int c){
  analogWrite(a[0], a[1]);
  return 0;
}

// ---------------- UART ----------------
int32_t fn_uart_begin(int32_t *a,int c){
  Serial1.begin(a[0]);
  return 0;
}

int32_t fn_uart_write(int32_t *a,int c){
  return Serial1.write((uint8_t)a[0]);
}

int32_t fn_uart_read(int32_t *a,int c){
  if(Serial1.available())
    return Serial1.read();
  return -1;
}

// ---------------- I2C -----------------
int32_t fn_i2c_begin(int32_t *a,int c){
  // SDA / SCL dependen del board mapping
  Wire.begin();
  Wire.setClock(a[2]);
  return 0;
}

int32_t fn_i2c_write(int32_t *a,int c){
  Wire.beginTransmission(a[0]);
  Wire.write(a[1]);
  Wire.write(a[2]);
  return Wire.endTransmission();
}

int32_t fn_i2c_read(int32_t *a,int c){
  Wire.beginTransmission(a[0]);
  Wire.write(a[1]);
  Wire.endTransmission(false);
  Wire.requestFrom(a[0], 1);
  if(Wire.available()) return Wire.read();
  return -1;
}

// ---------------- SPI -----------------
int32_t fn_spi_begin(int32_t *a,int c){
  SPI.begin();
  return 0;
}

int32_t fn_spi_transfer(int32_t *a,int c){
  return SPI.transfer((uint8_t)a[0]);
}

// ---------------- FS (LittleFS) -------
int32_t fn_fs_write(int32_t *a,int c){  
  const char *path = vm_get_str(a[0]);
  const char *data = vm_get_str(a[1]);
  File f = LittleFS.open(path, "w");
  if(!f) return -1;
  f.print(data);
  f.close();
  return 0;
}

int32_t fn_fs_read(int32_t *a,int c){  
  const char *path = vm_get_str(a[0]);
  File f = LittleFS.open(path, "r");
  if(!f) return -1;
  String s = f.readString();
  f.close();
  return (int32_t)s.length();
}

// ---------------- Scheduler Flag ------
volatile uint32_t sys_yield_flag = 0;

int32_t fn_yield(int32_t *a,int c){
  sys_yield_flag = 1;
  return 0;
}

// -------- Tabla Nativa -----------------
static NativeEntry native_table[] = {
  { "gpio_mode",   fn_gpio_mode,   2 },
  { "gpio_write",  fn_gpio_write,  2 },
  { "gpio_read",   fn_gpio_read,   1 },

  { "sleep",       fn_sleep,       1 },
  { "millis",      fn_millis,      0 },

  { "adc_init",    fn_adc_init_pin,1 },
  { "adc_read",    fn_adc_read,    1 },

  { "pwm_attach",  fn_pwm_attach,  1 },
  { "pwm_write",   fn_pwm_write,   2 },

  { "uart_begin",  fn_uart_begin,  1 },
  { "uart_write",  fn_uart_write,  1 },
  { "uart_read",   fn_uart_read,   0 },

  { "i2c_begin",   fn_i2c_begin,   2 },
  { "i2c_write",   fn_i2c_write,   3 },
  { "i2c_read",    fn_i2c_read,    2 },

  { "spi_begin",   fn_spi_begin,   0 },
  { "spi_xfer",    fn_spi_transfer,1 },

  { "fs_write",    fn_fs_write,    2 },
  { "fs_read",     fn_fs_read,     1 },

  { "yield",       fn_yield,       0 },
};
