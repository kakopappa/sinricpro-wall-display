#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "main.h"
#include "sinricpro_api.h"

LGFX tft;

#define screenWidth 480
#define screenHeight 320

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];

const char * ssid = "";
const char * password = "";
const char * api_key = "";

const lv_color_t green_color = lv_color_hex(0x00FF00);
const lv_color_t medium_gray = lv_color_hex(0x808080);
const lv_color_t blue_color = lv_color_hex(0x1E90FF);

SinricProAPI api(api_key);
void my_disp_flush(lv_disp_drv_t * disp,
  const lv_area_t * area, lv_color_t * color_p) {
  uint32_t w = (area -> x2 - area -> x1 + 1);
  uint32_t h = (area -> y2 - area -> y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area -> x1, area -> y1, w, h);
  tft.writePixels((lgfx::rgb565_t * ) & color_p -> full, w * h);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

void my_touch_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data) {
  uint16_t touchX, touchY;
  bool touched = tft.getTouch( & touchX, & touchY);
  if (!touched) {
    data -> state = LV_INDEV_STATE_REL;
  } else {
    data -> state = LV_INDEV_STATE_PR;
    data -> point.x = touchX;
    data -> point.y = touchY;
  }
}
 
static void close_dialog_timer_cb(lv_timer_t * timer) {
  lv_obj_t * dialog = (lv_obj_t * ) timer -> user_data;
  lv_obj_del(dialog);
  lv_timer_del(timer);
}

static void list_event_handler(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    SinricProDevice * device = (SinricProDevice * ) lv_event_get_user_data(e);
    Serial.printf("Device %s (%s) was clicked\n", device -> name, device -> id);

    // Create the "Please Wait" dialog
    lv_obj_t * dialog = lv_obj_create(lv_scr_act());
    lv_obj_set_size(dialog, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x0000FF), 0);
    lv_obj_set_style_pad_all(dialog, 20, 0);
    lv_obj_center(dialog);

    lv_obj_t * label2 = lv_label_create(dialog);
    lv_label_set_text(label2, "Please Wait...");
    lv_obj_center(label2);

    // Store dialog reference in user_data for potential future use
    //user_data->please_wait_dialog = dialog;

    // Create a timer to close the dialog after 2 seconds
    lv_timer_create(close_dialog_timer_cb, 2000, dialog);
  }
}

void show_device_list() {
  std::vector <SinricProDevice> devices = api.getDevices();
  if (!devices.empty()) {
    Serial.println("Devices found:");
    for (const auto & device: devices) {
      Serial.printf("- Name: %s, ID: %s\n", device.name.c_str(), device.id.c_str());
    }
  } else {
    Serial.println("No devices found or an error occurred.");
    return;
  }

  lv_obj_t * scr = lv_scr_act();

  // Create a container for the icon and listview
  lv_obj_t * cont = lv_obj_create(scr);
  lv_obj_set_size(cont, lv_pct(90), lv_pct(80));
  lv_obj_center(cont);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // Create and add the image
  lv_obj_t * img = lv_img_create(cont);
  lv_img_set_src(img, LV_SYMBOL_HOME);

  // Create the listview
  lv_obj_t * list = lv_list_create(cont);
  lv_obj_set_size(list, lv_pct(100), lv_pct(80));
  lv_obj_set_style_pad_top(list, 20, 0); // Add some padding at the top

  // lv_obj_t *list1 = lv_list_create(cont);
  lv_obj_set_size(list, LV_HOR_RES - 30, LV_VER_RES - 30);
  lv_obj_align(list, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t * list_btn;

  for (const auto & device: devices) {
    list_btn = lv_list_add_btn(list, NULL, device.name.c_str());
    lv_obj_add_event_cb(list_btn, list_event_handler, LV_EVENT_CLICKED, (void * ) & device);

    if (device.isOnline) {
      lv_obj_t * btn = lv_btn_create(list_btn);
      lv_obj_set_size(btn, 40, 40);
      lv_obj_align(btn, LV_ALIGN_RIGHT_MID, -10, 0);

      if (device.powerState.equalsIgnoreCase("on")) {
        lv_obj_set_style_bg_color(btn, blue_color, 0);
      } else {
        lv_obj_set_style_bg_color(btn, green_color, 0);
      }

      // Add icon to the button
      lv_obj_t * btn_label = lv_label_create(btn);
      lv_label_set_text(btn_label, LV_SYMBOL_POWER);
      lv_obj_center(btn_label);
    } else {
      lv_obj_t * label = lv_label_create(list_btn);
      lv_label_set_text(label, "Offline");
      lv_obj_align(label, LV_ALIGN_LEFT_MID, 50, 0);
    }
  }
}

void setup() {
  Serial.begin(115200);

  tft.begin();
  tft.setRotation(3);
  tft.setBrightness(255);

  lv_init();
  lv_disp_draw_buf_init( & draw_buf, buf, NULL, screenWidth * 10);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( & disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = & draw_buf;
  lv_disp_drv_register( & disp_drv);
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( & indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touch_read;
  lv_indev_drv_register( & indev_drv);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  show_device_list();
}

void loop() {
  lv_timer_handler();
  delay(5);
}