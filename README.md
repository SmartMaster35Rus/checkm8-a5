## A5/A5X checkm8

Arduino UNO + USB HOST SHIELD 2.0 + TFT дисплей ILI9488 320/480

## Материалы и инструменты

- Arduino Uno
- USB Host Shield
- USB кабель типа A-B для подключения Arduino к компьютеру
- TFT дисплей на базе чипа ILI9488
- Провода соединения (по необходимости)

## Сборка
1. Вставьте USB Host Shield на Arduino Uno, удостоверьтесь, что все контакты соответствуют друг другу.
2. Подключите TFT дисплей к Arduino Uno. Возможно, потребуется использование проводов для соединения дисплея с соответствующими выводами на Arduino Uno. Схема подключения может варьироваться в зависимости от модели дисплея и его интерфейса (SPI, 8-битный параллельный и т.д.).

## Программирование
1. Установите Arduino IDE. Скачайте и установите последнюю версию Arduino IDE с [официального сайта Arduino](https://www.arduino.cc/en/software).
2. Установите необходимые библиотеки. В Arduino IDE перейдите в "Скетч" -> "Подключить библиотеку". Установите библиотеки для USB Host Shield и TFT дисплея ILI9488, например, "USB Host Shield Library 2.0" и "Adafruit ILI9488" или "MCUFRIEND_kbv" (в зависимости от вашего дисплея).
3. Загрузите свой скетч на Arduino. Откройте ваш скетч в Arduino IDE, выберите "Инструменты" -> "Плата" -> "Arduino Uno", затем "Инструменты" -> "Порт" и выберите порт, к которому подключена ваша плата Arduino Uno. Нажмите кнопку "Загрузить" (стрелка вправо) для загрузки скетча на Arduino.

## Проверка работы устройства
Если все было сделано правильно, ваш TFT дисплей должен отображать информацию от USB Host Shield.

[checkm8](https://github.com/axi0mX/ipwndfu/blob/master/checkm8.py) порт для S5L8940X/S5L8942X/S5L8945X на базе Arduino и USB Host Shield на базе MAX3421E

## Сборка

Следуйте инструкциям [здесь](https://github.com/DSecurity/checkm8-arduino#building). Обратите внимание, что патч для [USB Host Library Rev. 2.0](https://github.com/felis/USB_Host_Shield_2.0) отличается.

### Установка и патчинг USB Host Library Rev. 2.0

```bash
cd путь/к/библиотекам/Arduino
git clone https://github.com/felis/USB_Host_Shield_2.0
cd USB_Host_Shield_2.0
git checkout cd87628af4a693eeafe1bf04486cf86ba01d29b8
git apply путь/к/usb_host_library.patch
```

### Выбор SoC

Перед использованием эксплоита измените эту строку в начале файла checkm8-a5.ino на целевой SoC CPID

Для получения точного SoC CPID открываем sliver.app, подклчюем устройство в DFU и кликаем внизу слева кнопку CPID #Увидим номер SoC CPID который нужно использовать в коде для checkm8 A5/A5X #

```
#define A5_8940 / #define A5_8942 / #define A5_8945
```

## S5L8940X/S5L8942X/S5L8945X-специфические заметки по эксплуатации

*[Эта статья](https://habr.com/ru/company/dsec/blog/472762/) может быть полезной для лучшего понимания.*

### 1. Обработка запроса управления `HOST2DEVICE` без обработки фазы данных

В более новых SoC этот запрос обрабатывается следующим образом. Обратите внимание, что существуют различные случаи для `request_handler_ret == 0` и `request_handler_ret > 0`:

```c
void __fastcall usb_core_handle_usb_control_receive(void *ep0_rx_buffer, __int64 is_setup, __int64 data_rcvd, bool *data_phase)
{
  ...
  // получение обработчика запроса управления интерфейсом
  request_handler = ep.registered_interfaces[(unsigned __int16)setup_request.wIndex]->request_handler;
  if ( !request_handler )
    goto error_handling;
  // вызов обработчика запроса управления интерфейсом
  // установка глобального указателя буфера
  request_handler_ret = request_handler(&g_setup_request, &ep0_data_phase_buffer);
  if ( !(g_setup_request.bmRequestType & 0x80000000) )
  {
    // HOST2DEVICE
    if ( request_handler_ret >= 1 )
    {
      // установка глобальной длины фазы данных и номера интерфейса
      ep0_data_phase_length = request_handler_ret; 
      ep0_data_phase_if_num = intf_num;
      goto continue;
    }
    if ( !request_handler_ret )
    {
      // подтверждение транзакции с zlp, не изменяя глобального состояния
      usb_core_send_zlp();
      goto continue;
    }
  ...
}
```

Но в наших целевых SoC этот запрос обрабатывается немного иначе:

```c
unsigned int __fastcall usb_core_handle_usb_control_receive(unsigned int rx_buffer, int is_setup, unsigned int data_received, int *data_phase)
{
  ...
  if ( is_setup )
  {
    v11 = memmove((unsigned int)&g_setup_packet, rx_buffer, 8);
    if ( g_setup_packet.bmRequestType & 0x60 )
    {
      if ( (g_setup_packet.bmRequestType & 0x60) != 0x20
        || (g_setup_packet.bmRequestType & 0

x1F) != 1
        || (v29 = LOBYTE(g_setup_packet.wIndex) | (HIBYTE(g_setup_packet.wIndex) << 8), v29 >= ep_size)
        || (request_handler = *(int (__fastcall **)(setup_req *, _DWORD *))(ep_array[LOBYTE(g_setup_packet.wIndex) | (HIBYTE(g_setup_packet.wIndex) << 8)]
                                                                          + 32)) == 0
        || (request_handler_ret = request_handler(&g_setup_packet, &ep0_data_phase_buffer), request_handler_ret < 0) )
      {
        // обработка ошибки
        rx_buffer = sub_2E7C(0x80, 1);
        v10 = 0;
        goto LABEL_103;
      }
      // если request_handler_ret >= 0, то длина фазы данных будет изменена в любом случае
      ep0_data_phase_length = request_handler_ret;
      ep0_data_phase_if_num = v29;
      *data_phase = 1;
      goto continue;
    }
  ...
}
```

Таким образом, если мы отправим любой запрос управления `HOST2DEVICE` без фазы данных, то `ep0_data_phase_length` будет сброшено на ноль. Из-за этого мы не можем использовать `ctrlReq(0x21,4,0)` для повторного входа в `DFU`. Но есть другой способ повторного входа в `DFU`:

1. `ctrlReq(bmRequestType = 0x21,bRequest = 1,wLength = 0x40)` с любыми данными
2. `ctrlReq(0x21,1,0)`
3. `ctrlReq(0xa1,3,1)`
4. `ctrlReq(0xa1,3,1)`
5. Сброс USB-шины

Таким образом, чтобы иметь возможность записывать в освобожденный `io_buffer`, мы выполняем шаги 1-4, затем отправляем неполный контрольный запрос `HOST2DEVICE`, чтобы установить глобальное состояние, затем вызываем сброс шины. Этот алгоритм полностью описан [здесь](https://gist.github.com/littlelailo/42c6a11d31877f98531f6d30444f59c4) [littlelailo](https://github.com/littlelailo).

Однако, если мы используем нормальную ОС с USB-стеком по умолчанию для эксплуатации, то мы не можем избежать отправки стандартных запросов устройства (например, `SET_ADDRESS`, см. [здесь](https://www.beyondlogic.org/usbnutshell/usb6.shtml) для получения более подробной информации) системой до того, как мы сможем работать с устройством. Поэтому в нашем PoC мы используем `Arduino` и `MAX3421E` для управления ранней инициализацией USB.

### 2. Обработка нулевого пакета

В более новых SoC пакеты данных обрабатываются следующим образом. Обратите внимание, что в случае обработки нулевого пакета обработка не выполняется.

```c
void __fastcall usb_core_handle_usb_control_receive(void *ep0_rx_buffer, __int64 is_setup, __int64 data_rcvd, bool *data_phase)
{
  ...
  if ( !(is_setup & 1) )
  {
    if ( !(_DWORD)data_rcvd ) // проверка на нулевой пакет
      return;
    if ( ep0_data_phase_rcvd + (unsigned int)data_rcvd <= ep0_data_phase_length )
    {
      if ( ep0_data_phase_length - ep0_data_phase_rcvd >= (unsigned int)data_rcvd )
        to_copy = (unsigned int)data_rcvd;
      else
        to_copy = ep0_data_phase_length - ep0_data_phase_rcvd;
      memmove(ep0_data_phase_buffer, ep0_rx_buffer, to_copy);// копирование полученных данных в буфер IO
      ep0_data_phase_buffer += (unsigned int)to_copy;// обновление глобального указателя буфера
      ep0_data_phase_rcvd += to_copy;           // обновление счетчика полученных данных
      *data_phase = 1;
      // остановить передачу, если получено ожидаемое количество байт
      // или получен пакет менее 0x40 байт
      if ( (_DWORD)data_rcvd == 0x40 )
		end_of_transfer = ep0_data_phase_rcvd == ep0_data_phase_length;
      else
        end_of_transfer = 1;
  ...
}
```

Но в наших целевых SoC нулевые пакеты обрабатываются так же, как и пакеты ненулевой длины.

```c
unsigned int __fastcall usb_core_handle_usb_control_receive(unsigned int rx_buffer, int is_setup, unsigned int data_received, int *data_phase)
{
  ...
  // обработка начинается здесь
  rx_buffer = ep0_data_phase_buffer;
  if ( !ep0_data_phase_buffer )
    return rx_buffer;
  if ( data_received + ep0_data_phase_rcvd > ep0_data_phase_length )
  {
    rx_buffer = sub_2E7C(128, 1);
reset_global_state:
    v10 = 0;
    ep0_data_phase_rcvd = 0;
    ep0_data_phase_length = 0;
    ep0_data_phase_buffer = 0;
    ep0_data_phase_if_num = -2;
LABEL_103:
    *v6 = v10;
    return rx_buffer;
  }
  if ( data_received >= ep0_data_phase_length - ep0_data_phase_rcvd )
    v7 = ep0_data_phase_length - ep0_data_phase_rcvd;
  else
    v7 = data_received;
  memmove(ep0_data_phase_buffer, rx_buffer_1, v7);
  end_of_transfer = data_received_1 != 0x40;  // в случае нулевого пакета `end_of_transfer` будет `true`
                                              // и глобальное состояние будет сброшено
  ep0_data_phase_buffer += v7;
  ep0_data_phase_rcvd += v7;
  rx_buffer = ep0_data_phase_rcvd;
  *v6 = 1;
  if ( rx_buffer == ep0_data_phase_length )
    end_of_transfer = 1;
  if ( end_of_transfer )
  {
    if ( (int)ep0_data_phase_if_num >= 0 && ep0_data_phase_if_num < ep_size )
    {
      v9 = *(void (**)(void))(ep_array[ep0_data_phase_if_num] + 36);
      if ( v9 )
      {
        v9();
        rx_buffer = usb_core_send_zlp();
      }
    }
    goto reset_global_state;
  }
  return rx_buffer;
}
```

Как и в предыдущем случае, если мы используем нормальную ОС с USB-стеком по умолчанию, мы не можем избежать отправки нулевых пакетов в статусной фазе стандартных управляющих запросов USB устройства.

## Важные замечания

* Не используйте кабели с встроенными USB-хабами (DCSD/Kong/Kanzi и т.д.), так как это может предотвратить распознавание устройства программой. Обычные USB-кабели подойдут нормально.
* Этот эксплойт по умолчанию понижает уровень вашего устройства, поэтому SWD-отладка будет доступна. Но это также заставляет ваше устройство использовать KBAG разработки при расшифровке Image3. Имейте в виду, если вы собираетесь использовать это для расшифровки компонентов прошивки.

## Авторы

* [ya.saxil](https://github.com/yasaxil)

## Лицензия

Этот проект лицензирован по лицензии MIT - см. файл [LICENSE](LICENSE) для получения подробной информации.
```
