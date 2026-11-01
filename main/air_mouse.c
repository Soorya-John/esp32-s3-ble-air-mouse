#include <math.h>
#include <stdatomic.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_hidd.h"
#include "esp_hid_gap.h"

#define ENCODER_A 12
#define ENCODER_B 13
#define CALIBRATION_SAMPLES 1500
#define SENSITIVITY 0.28f
#define DEADZONE 3.0f
#define ALPHA 0.12f
static esp_hidd_dev_t *hid;
static atomic_bool connected;
static const uint8_t descriptor[] = {
  0x05,1,0x09,2,0xa1,1,0x09,1,0xa1,0,
  0x05,9,0x19,1,0x29,3,0x15,0,0x25,1,0x95,3,0x75,1,0x81,2,
  0x95,1,0x75,5,0x81,3,0x05,1,0x09,0x30,0x09,0x31,0x09,0x38,
  0x15,0x81,0x25,0x7f,0x75,8,0x95,3,0x81,6,0xc0,0xc0
};
static esp_hid_raw_report_map_t maps[] = {{.data=descriptor,.len=sizeof(descriptor)}};
static esp_hid_device_config_t config = {
 .device_name="Soorya Air Mouse", .manufacturer_name="ESP32-S3",
 .serial_number="001", .report_maps=maps,.report_maps_len=1
};
static void event(void *arg, esp_event_base_t base, int32_t id, void *data) {
 if (id == ESP_HIDD_START_EVENT) ESP_ERROR_CHECK(esp_hid_ble_gap_adv_start());
 if (id == ESP_HIDD_CONNECT_EVENT) atomic_store(&connected,true);
 if (id == ESP_HIDD_DISCONNECT_EVENT) {
  atomic_store(&connected,false);
  ESP_ERROR_CHECK(esp_hid_ble_gap_adv_start());
 }
}
static void write_reg(uint8_t reg,uint8_t value) {
 uint8_t b[]={reg,value};
 ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_NUM_0,0x68,b,2,pdMS_TO_TICKS(100)));
}
static esp_err_t gyro(int16_t *x,int16_t *y) {
 uint8_t reg=0x43,b[4];
 esp_err_t e=i2c_master_write_read_device(I2C_NUM_0,0x68,&reg,1,b,4,pdMS_TO_TICKS(20));
 if(e==ESP_OK) {*x=(int16_t)((b[0]<<8)|b[1]);*y=(int16_t)((b[2]<<8)|b[3]);}
 return e;
}
static int8_t movement(float f) {
 f=fabsf(f)<DEADZONE?0:f-copysignf(DEADZONE,f);
 return (int8_t)fmaxf(-8,fminf(8,f*SENSITIVITY));
}
void app_main(void) {
 esp_err_t e=nvs_flash_init();
 if(e==ESP_ERR_NVS_NO_FREE_PAGES||e==ESP_ERR_NVS_NEW_VERSION_FOUND) {
  ESP_ERROR_CHECK(nvs_flash_erase()); e=nvs_flash_init();
 }
 ESP_ERROR_CHECK(e);
 gpio_config_t pins={.pin_bit_mask=(1ULL<<10)|(1ULL<<11)|(1ULL<<ENCODER_A)|(1ULL<<ENCODER_B),
  .mode=GPIO_MODE_INPUT,.pull_up_en=GPIO_PULLUP_ENABLE};
 ESP_ERROR_CHECK(gpio_config(&pins));
 i2c_config_t bus={.mode=I2C_MODE_MASTER,.sda_io_num=8,.scl_io_num=9,
 .sda_pullup_en=1,.scl_pullup_en=1,.master.clk_speed=100000};
 ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0,&bus));
 ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0,I2C_MODE_MASTER,0,0,0));
 write_reg(0x6b,0);vTaskDelay(pdMS_TO_TICKS(100));write_reg(0x1b,0);
 ESP_LOGI("mouse","Keep still: calibrating gyro");
 vTaskDelay(pdMS_TO_TICKS(2000));
 float ox=0,oy=0;int16_t x,y;
 for(int i=0;i<CALIBRATION_SAMPLES;i++) {
  ESP_ERROR_CHECK(gyro(&x,&y));ox+=x;oy+=y;vTaskDelay(pdMS_TO_TICKS(2));
 }
 ox/=CALIBRATION_SAMPLES;oy/=CALIBRATION_SAMPLES;
 ESP_ERROR_CHECK(esp_hid_gap_init(HIDD_BLE_MODE));
 ESP_ERROR_CHECK(esp_hid_ble_gap_adv_init(ESP_HID_APPEARANCE_MOUSE,config.device_name));
 ESP_ERROR_CHECK(esp_ble_gatts_register_callback(esp_hidd_gatts_event_handler));
 ESP_ERROR_CHECK(esp_hidd_dev_init(&config,ESP_HID_TRANSPORT_BLE,event,&hid));
 ESP_LOGI("mouse","Ready: Soorya Air Mouse");
 const int8_t table[16]={0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
 int prev=(gpio_get_level(ENCODER_A)<<1)|gpio_get_level(ENCODER_B),acc=0;
 uint8_t buttons=0,candidate=0;int stable=0,tick=0;float fx=0,fy=0;
 for(;;) {
  int now=(gpio_get_level(ENCODER_A)<<1)|gpio_get_level(ENCODER_B);
  if((prev^now)==3) acc=0; else acc+=table[(prev<<2)|now];
  prev=now;int wheel=0;
  if(acc>=4){wheel=1;acc=0;}else if(acc<=-4){wheel=-1;acc=0;}
  uint8_t raw=(!gpio_get_level(10))|((!gpio_get_level(11))<<1);
  if(raw!=candidate){candidate=raw;stable=0;}else if(stable<20)stable++;
  bool changed=stable==20&&buttons!=candidate;
  if(changed)buttons=candidate;
  int8_t dx=0,dy=0;
  if(++tick>=5){tick=0;
   if(gyro(&x,&y)==ESP_OK){fx+=ALPHA*((x-ox)/131.0f-fx);fy+=ALPHA*((y-oy)/131.0f-fy);
    dx=movement(fx);dy=movement(fy);}
  }
  if(atomic_load(&connected)&&(dx||dy||wheel||changed||tick==0)) {
   uint8_t report[]={buttons,(uint8_t)dx,(uint8_t)dy,(uint8_t)wheel};
   esp_hidd_dev_input_set(hid,0,0,report,sizeof(report));
  }
  vTaskDelay(pdMS_TO_TICKS(1));
 }
}

/* GAP helper calls this after authentication; input polling already runs in app_main. */
void ble_hid_task_start_up(void) {}

