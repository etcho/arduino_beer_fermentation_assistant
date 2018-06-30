//automação do processo de produção de cerveja
//
//posição na eprom
//
//#configurações
//0: temperatura máxima que o conjunto irá operar/primeira rampa
//1: temperatura mínima que o conjunto irá operar/primeira rampa
//2: número de rampas de temperatura
//3: número de dias da primeira rampa
//4: número de horas da primeira rampa
//5: temperatura máxima da segunda rampa
//6: temperatura mínima da segunda rampa
//7: número de dias da segunda rampa
//8: número de horas da segunda rampa
//9: temperatura máxima da terceira rampa
//10: temperatura mínima da terceira rampa
//11: número de dias da terceira rampa
//12: número de horas da terceira rampa
//13: temperatura máxima da quarta rampa
//14: temperatura mínima da quarta rampa
//15: número de dias da quarta rampa
//16: número de horas da quarta rampa
//17: modo (1=esfriar e 2=esquentar)
//                                                  
//#variáveis de controle
//30: qual rampa está sendo executada no momento
//31: ano em que a rampa iniciou
//32: mês em que a rampa iniciou
//33: dia em que a rampa iniciou
//34: hora em que a rampa iniciou
//35: minuto em que a rampa iniciou
//36: ano em que o ciclo de rampas iniciou
//37: mês em que o ciclo de rampas iniciou
//38: dia em que o ciclo de rampas iniciou
//39: hora em que o ciclo de rampas iniciou
//40: minuto em que o ciclo de rampas iniciou

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "RTClib.h"

#define ONE_WIRE_BUS 3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensor1;
LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS1307 RTC;

#define LAST_MENU 17
#define LAST_ACTION_MENU 4
#define LAST_PANEL 1
int rele1 = 4;
int loops = 0;
DateTime now;
char tmp_char[16];

int btn_ok = 8;
int btn_cancel = 9;
int btn_down = 10;
int btn_up = 11;
boolean btn_ok_pressed = false;
boolean btn_cancel_pressed = false;
boolean btn_down_pressed = false;
boolean btn_up_pressed = false;
boolean last_ok_pressed = false;
boolean last_cancel_pressed = false;
boolean last_down_pressed = false;
boolean last_up_pressed = false;

boolean ignore_buttons = false;
boolean menu_is_opened = false;
boolean in_edit_mode = false;
float menu_option_original_value = 0;
float menu_option_value = 0;
float menu_option_jump = 0;
float menu_option_min_value = 0;
float menu_option_max_value = 0;
int current_menu = 0;

boolean action_menu_is_opened = false;
boolean action_in_confirm_mode = false;
int current_action_menu = 0;

int current_panel = 0;

int rampa_atual = 0;
int ano_inicio_rampa = 0;
int mes_inicio_rampa = 0;
int dia_inicio_rampa = 0;
int hora_inicio_rampa = 0;
int minuto_inicio_rampa = 0;
DateTime data_inicio_rampa;
int ano_inicio_ciclo_rampas = 0;
int mes_inicio_ciclo_rampas = 0;
int dia_inicio_ciclo_rampas = 0;
int hora_inicio_ciclo_rampas = 0;
int minuto_inicio_ciclo_rampas = 0;
DateTime data_inicio_ciclo_rampas;

boolean finished = false;
float tempC = 0;
float temperatura_maxima = 0;
float temperatura_minima = 0;
float numero_rampas = 0;
float dias_rampa1 = 0;
float horas_rampa1 = 0;
float temperatura_maxima_rampa2 = 0;
float temperatura_minima_rampa2 = 0;
float dias_rampa2 = 0;
float horas_rampa2 = 0;
float temperatura_maxima_rampa3 = 0;
float temperatura_minima_rampa3 = 0;
float dias_rampa3 = 0;
float horas_rampa3 = 0;
float temperatura_maxima_rampa4 = 0;
float temperatura_minima_rampa4 = 0;
float dias_rampa4 = 0;
float horas_rampa4 = 0;
int modo = 1;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  sensors.begin();
  sensors.getAddress(sensor1, 0);
  pinMode(rele1, OUTPUT);
  pinMode(btn_ok, INPUT_PULLUP);
  pinMode(btn_cancel, INPUT_PULLUP);
  pinMode(btn_down, INPUT_PULLUP);
  pinMode(btn_up, INPUT_PULLUP);
  pinMode(13, OUTPUT);
  Wire.begin();
  RTC.begin();
  if (! RTC.isrunning()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  now = RTC.now();
  read_eprom();
  check_program();
}

void loop() {
  delay(10);
  read_btn_state();
  if (loops % 100 == 0 || loops == 0) { //a cada 1 segundo
    now = RTC.now();
    check_program();
  }
  if (loops % 500 == 0 || loops == 0) { //a cada 5 segundos
    check_sensor();
  }

  //atividades do menu de ações
  if (!menu_is_opened && !ignore_buttons) {
    if (btn_cancel_pressed && action_menu_is_opened && !action_in_confirm_mode) {
      close_action_menu();
    } else if (btn_cancel_pressed && action_menu_is_opened && action_in_confirm_mode) {
      action_cancel_confirm_mode();
    } else if (btn_cancel_pressed && !action_menu_is_opened) {
      open_action_menu();
    }
    if (action_menu_is_opened && !action_in_confirm_mode) {
      if (btn_up_pressed) {
        action_menu_next_page();
      }
      if (btn_down_pressed) {
        action_menu_previous_page();
      }
      if (btn_ok_pressed) {
        action_enter_confirm_mode();
      }
    } else if (action_menu_is_opened && action_in_confirm_mode && btn_ok_pressed) {
      action_confirm();
    }
  }
  //fim das atividades do meun de ações

  //atividades do menu de opções
  if (!action_menu_is_opened && !ignore_buttons) {
    if (btn_ok_pressed && !menu_is_opened) {
      open_menu();
    } else if (btn_ok_pressed && menu_is_opened && !in_edit_mode) {
      enter_edit_mode();
    } else if (btn_ok_pressed && menu_is_opened && in_edit_mode) {
      save_edit_mode();
    }
    if (btn_cancel_pressed && menu_is_opened && !in_edit_mode) {
      close_menu();
    } else if (btn_cancel_pressed && in_edit_mode) {
      cancel_edit_mode();
    }
    if (in_edit_mode) {
      if (btn_up_pressed) {
        menu_increase_value();
      }
      if (btn_down_pressed) {
        menu_decrease_value();
      }
    } else if (menu_is_opened) {
      if (btn_up_pressed) {
        menu_next_page();
      }
      if (btn_down_pressed) {
        menu_previous_page();
      }
    }
  }
  //fim das atividades do menu de opções

  //troca de painéis
  if (!action_menu_is_opened && !menu_is_opened && !ignore_buttons) {
    if (btn_up_pressed) {
      next_panel();
    }
    if (btn_down_pressed) {
      previous_panel();
    }
  }
  //fim da troca de painéis

  if (loops % 20 == 0 && (in_edit_mode || action_in_confirm_mode)) {
    lcd.setCursor(0, 1);
    if (loops % 40 == 0) {
      lcd.print(" ");
    } else {
      lcd.print("*");
    }
  }

  if (loops % 20 == 0 || loops == 0) {
    if (!menu_is_opened && !action_menu_is_opened) {
      load_panel(current_panel);
    }
  }

  loops++;
  if (loops == 1010) {
    loops = 10;
  }
  ignore_buttons = false;
}

void read_btn_state() {
  boolean ok = digitalRead(btn_ok) == LOW;
  boolean cancel = digitalRead(btn_cancel) == LOW;
  boolean down = digitalRead(btn_down) == LOW;
  boolean up = digitalRead(btn_up) == LOW;
  if (last_ok_pressed == false) {
    btn_ok_pressed = (ok && !cancel && !down && !up);
  } else {
    btn_ok_pressed = false;
  }
  if (last_cancel_pressed == false) {
    btn_cancel_pressed = (cancel && !ok && !down && !up);
  } else {
    btn_cancel_pressed = false;
  }
  if (last_down_pressed == false) {
    btn_down_pressed = (down && !ok && !cancel && !up);
  } else {
    btn_down_pressed = false;
  }
  if (last_up_pressed == false) {
    btn_up_pressed = (up && !ok && !cancel && !down);
  } else {
    btn_up_pressed = false;
  }
  last_ok_pressed = ok;
  last_cancel_pressed = cancel;
  last_down_pressed = down;
  last_up_pressed = up;
}

void read_eprom() {
  temperatura_maxima = RTC.readnvram(0) / 10.0f;
  temperatura_minima = RTC.readnvram(1) / 10.0f;
  numero_rampas = RTC.readnvram(2) / 10.0f;
  dias_rampa1 = RTC.readnvram(3) / 10.0f;
  horas_rampa1 = RTC.readnvram(4) / 10.0f;
  temperatura_maxima_rampa2 = RTC.readnvram(5) / 10.0f;
  temperatura_minima_rampa2 = RTC.readnvram(6) / 10.0f;
  dias_rampa2 = RTC.readnvram(7) / 10.0f;
  horas_rampa2 = RTC.readnvram(8) / 10.0f;
  temperatura_maxima_rampa3 = RTC.readnvram(9) / 10.0f;
  temperatura_minima_rampa3 = RTC.readnvram(10) / 10.0f;
  dias_rampa3 = RTC.readnvram(11) / 10.0f;
  horas_rampa3 = RTC.readnvram(12) / 10.0f;
  temperatura_maxima_rampa4 = RTC.readnvram(13) / 10.0f;
  temperatura_minima_rampa4 = RTC.readnvram(14) / 10.0f;
  dias_rampa4 = RTC.readnvram(15) / 10.0f;
  horas_rampa4 = RTC.readnvram(16) / 10.0f;
  modo = RTC.readnvram(17) / 10.0f;

  rampa_atual = RTC.readnvram(30);
  ano_inicio_rampa = RTC.readnvram(31) + 2000;
  if (ano_inicio_rampa < 0) {
    ano_inicio_rampa = 0;
  }
  mes_inicio_rampa = RTC.readnvram(32);
  dia_inicio_rampa = RTC.readnvram(33);
  hora_inicio_rampa = RTC.readnvram(34);
  minuto_inicio_rampa = RTC.readnvram(35);
  data_inicio_rampa = DateTime (ano_inicio_rampa, mes_inicio_rampa, dia_inicio_rampa, hora_inicio_rampa, minuto_inicio_rampa, 0);
  ano_inicio_ciclo_rampas = RTC.readnvram(36) + 2000;
  if (ano_inicio_ciclo_rampas < 0) {
    ano_inicio_ciclo_rampas = 0;
  }
  mes_inicio_ciclo_rampas = RTC.readnvram(37);
  dia_inicio_ciclo_rampas = RTC.readnvram(38);
  hora_inicio_ciclo_rampas = RTC.readnvram(39);
  minuto_inicio_ciclo_rampas = RTC.readnvram(40);
  data_inicio_ciclo_rampas = DateTime (ano_inicio_ciclo_rampas, mes_inicio_ciclo_rampas, dia_inicio_ciclo_rampas, hora_inicio_ciclo_rampas, minuto_inicio_ciclo_rampas, 0);
}

void run_program_rampa(int rampa) {
  int dias_rampa_atual, horas_rampa_atual;
  DateTime datetime_tmp;
  switch (rampa) {
    case 1: {
        dias_rampa_atual = dias_rampa1;
        horas_rampa_atual = horas_rampa1;
        break;
      }
    case 2: {
        dias_rampa_atual = dias_rampa2;
        horas_rampa_atual = horas_rampa2;
        break;
      }
    case 3: {
        dias_rampa_atual = dias_rampa3;
        horas_rampa_atual = horas_rampa3;
        break;
      }
    case 4: {
        dias_rampa_atual = dias_rampa4;
        horas_rampa_atual = horas_rampa4;
        break;
      }
  }
  datetime_tmp = data_inicio_rampa + TimeSpan(dias_rampa_atual, horas_rampa_atual, 3, 0);
  if (now.unixtime() >= data_inicio_rampa.unixtime() && now.unixtime() <= datetime_tmp.unixtime()) {
    switch (rampa) {
      case 2: {
          temperatura_minima = temperatura_minima_rampa2;
          temperatura_maxima = temperatura_maxima_rampa2;
          break;
        }
      case 3: {
          temperatura_minima = temperatura_minima_rampa3;
          temperatura_maxima = temperatura_maxima_rampa3;
          break;
        }
      case 4: {
          temperatura_minima = temperatura_minima_rampa4;
          temperatura_maxima = temperatura_maxima_rampa4;
          break;
        }
    }
  } else if (numero_rampas >= rampa + 1) {
    RTC.writenvram(30, rampa + 1);
    RTC.writenvram(31, now.year() - 2000);
    RTC.writenvram(32, now.month());
    RTC.writenvram(33, now.day());
    RTC.writenvram(34, now.hour());
    RTC.writenvram(35, now.minute());
    read_eprom();
  } else {
    finished = true;
  }
}

void check_program() {
  if (rampa_atual > 0) {
    if (rampa_atual <= numero_rampas) {
      run_program_rampa(rampa_atual);
    }
  }
}

void check_sensor() {
  sensors.requestTemperatures();
  tempC = sensors.getTempC(sensor1);
  if (finished) {
    digitalWrite(rele1, HIGH);
  } else if (modo == 1) {
    if (tempC > temperatura_maxima) {
      digitalWrite(rele1, LOW);
    } else if (tempC < temperatura_minima) {
      digitalWrite(rele1, HIGH);
    }
  } else if (modo == 2) {
    if (tempC < temperatura_minima) {
      digitalWrite(rele1, LOW);
    } else if (tempC > temperatura_maxima) {
      digitalWrite(rele1, HIGH);
    }
  }
}

void load_panel(int panel) {
  current_panel = panel;
  switch (panel) {
    case 0: {
        load_panel_0();
        break;
      }
    case 1: {
        load_panel_1();
        break;
      }
  }
}

void next_panel() {
  if (current_panel == LAST_PANEL) {
    current_panel = 0;
  } else {
    current_panel++;
  }
  load_panel(current_panel);
}

void previous_panel() {
  if (current_panel == 0) {
    current_panel = LAST_PANEL;
  } else {
    current_panel--;
  }
  load_panel(current_panel);
}

void load_panel_0() {
  lcd.setCursor(0, 0);
  lcd.print(tempC);
  if (tempC < 10) {
    lcd.setCursor(4, 0);
    lcd.print(" ");
  }
  lcd.setCursor(5, 0);
  lcd.print("      ");
  lcd.setCursor(9, 0);
  if (modo == 1){
    lcd.print("F");
  } else if (modo == 2) {
    lcd.print("Q");
  }
  lcd.setCursor(11, 0);
  if (now.hour() < 10) {
    lcd.print("0");
  }
  lcd.print(now.hour());
  lcd.print(":");
  if (now.minute() < 10) {
    lcd.print("0");
  }
  lcd.print(now.minute());

  lcd.setCursor(0, 1);
  if (finished) {
    lcd.print(" Concluido!");
  } else {
    lcd.print("[");
    dtostrf(temperatura_minima, 3, 1, tmp_char);
    lcd.print(tmp_char);
    lcd.print("~");
    dtostrf(temperatura_maxima, 3, 1, tmp_char);
    lcd.print(tmp_char);
    lcd.print("]");
    if (temperatura_minima < 10 && temperatura_maxima < 10) {
      lcd.setCursor(9, 1);
      lcd.print("  ");
    } else if (temperatura_minima < 10 || temperatura_maxima < 10) {
      lcd.setCursor(10, 1);
      lcd.print(" ");
    }
  }
  if (rampa_atual > 0) {
    lcd.setCursor(11, 1);
    lcd.print(" R");
    lcd.print(rampa_atual);
    lcd.print("/");
    lcd.print(numero_rampas);
  }
}

void load_panel_1() {
  if (finished) {
    lcd.setCursor(0, 0);
    lcd.print("Concluido!      ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  } else if (rampa_atual == 0) {
    lcd.setCursor(0, 0);
    lcd.print("Nenhuma rampa");
    lcd.print("   ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("R");
    lcd.print(rampa_atual);
    lcd.print("/");
    lcd.print((int) numero_rampas);
    int dias_rampa_atual, horas_rampa_atual;
    switch (rampa_atual) {
      case 1: {
          dias_rampa_atual = dias_rampa1;
          horas_rampa_atual = horas_rampa1;
          break;
        }
      case 2: {
          dias_rampa_atual = dias_rampa2;
          horas_rampa_atual = horas_rampa2;
          break;
        }
      case 3: {
          dias_rampa_atual = dias_rampa3;
          horas_rampa_atual = horas_rampa3;
          break;
        }
      case 4: {
          dias_rampa_atual = dias_rampa4;
          horas_rampa_atual = horas_rampa4;
          break;
        }
    }
    long unix_inicio = data_inicio_rampa.unixtime();
    long unix_previsao = unix_inicio + ((long) dias_rampa_atual * (long) 86400) + ((long) horas_rampa_atual * (long) 3600);
    DateTime datetime_tmp (unix_previsao);
    long unix_now = now.unixtime();
    long segundos_passados = unix_now - unix_inicio;
    long segundos_restantes = unix_previsao - unix_inicio - segundos_passados;
    long dias_restantes = segundos_restantes / 86400;
    segundos_restantes = segundos_restantes % 86400;
    long horas_restantes = segundos_restantes / 3600;
    segundos_restantes = segundos_restantes % 3600;
    long minutos_restantes = segundos_restantes / 60;
    if (minutos_restantes < 0){
      minutos_restantes = 0;
    }
    lcd.print("   ");
    if (dias_restantes < 10) {
      lcd.print(" ");
    }
    lcd.print(dias_restantes);
    lcd.print("d ");
    if (horas_restantes < 10) {
      lcd.print("0");
    }
    lcd.print(horas_restantes);
    lcd.print(":");
    if (minutos_restantes < 10) {
      lcd.print("0");
    }
    lcd.print(minutos_restantes);
    if (rampa_atual == 0){
      lcd.setCursor(0, 1);
      lcd.print("                ");
    } else {
      long unix_inicio_ciclo_rampas = data_inicio_ciclo_rampas.unixtime();
      segundos_passados = unix_now - unix_inicio_ciclo_rampas;
      long dias_passados = segundos_passados / 86400;
      segundos_passados = segundos_passados % 86400;
      long horas_passadas = segundos_passados / 3600;
      segundos_passados = segundos_passados % 3600;
      long minutos_passados = segundos_passados / 60;
      if (minutos_passados < 0){
        minutos_passados = 0;
      }
      lcd.setCursor(0, 1);
      lcd.print(" Total:");
      if (dias_passados < 10) {
        lcd.print(" ");
      }
      lcd.print(dias_passados);
      lcd.print("d ");
      if (horas_passadas < 10) {
        lcd.print("0");
      }
      lcd.print(horas_passadas);
      lcd.print(":");
      if (minutos_passados < 10) {
        lcd.print("0");
      }
      lcd.print(minutos_passados);
    }
  }
}

void open_menu() {
  menu_is_opened = true;
  load_menu(0);
}

void close_menu() {
  menu_is_opened = false;
  lcd.clear();
}

void menu_next_page() {
  if (current_menu >= LAST_MENU) {
    current_menu = 0;
  } else {
    current_menu++;
  }
  load_menu(current_menu);
}

void menu_previous_page() {
  if (current_menu == 0) {
    current_menu = LAST_MENU;
  } else {
    current_menu--;
  }
  load_menu(current_menu);
}

void load_menu(int menu) {
  String label = "";
  float min_value = 0;
  float max_value = 0;
  float value_jump = 0.5;
  switch (menu) {
    case 0: {
        label = "[Temp. max.]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 1: {
        label = "[Temp. min.]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 2: {
        label = "[Num. rampas]";
        min_value = 0;
        max_value = 4;
        value_jump = 1;
        break;
      }
    case 3: {
        label = "[Dias rampa 1]";
        min_value = 0;
        max_value = 100;
        value_jump = 1;
        break;
      }
    case 4: {
        label = "[Horas rampa 1]";
        min_value = 0;
        max_value = 23;
        value_jump = 1;
        break;
      }
    case 5: {
        label = "[Temp. max. R2]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 6: {
        label = "[Temp. min. R2]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 7: {
        label = "[Dias rampa 2]";
        min_value = 0;
        max_value = 100;
        value_jump = 1;
        break;
      }
    case 8: {
        label = "[Horas rampa 2]";
        min_value = 0;
        max_value = 23;
        value_jump = 1;
        break;
      }
    case 9: {
        label = "[Temp. max. R3]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 10: {
        label = "[Temp. min. R3]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 11: {
        label = "[Dias rampa 3]";
        min_value = 0;
        max_value = 100;
        value_jump = 1;
        break;
      }
    case 12: {
        label = "[Horas rampa 3]";
        min_value = 0;
        max_value = 23;
        value_jump = 1;
        break;
      }
    case 13: {
        label = "[Temp. max. R4]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 14: {
        label = "[Temp. min. R4]";
        min_value = 0;
        max_value = 100;
        value_jump = 0.5;
        break;
      }
    case 15: {
        label = "[Dias rampa 4]";
        min_value = 0;
        max_value = 100;
        value_jump = 1;
        break;
      }
    case 16: {
        label = "[Horas rampa 4]";
        min_value = 0;
        max_value = 23;
        value_jump = 1;
        break;
      }
    case 17: {
        label = "[Mod0: 1=F 2=Q]";
        min_value = 1;
        max_value = 2;
        value_jump = 1;
        break;
      }
  }
  current_menu = menu;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(label);
  float value = RTC.readnvram(current_menu) / 10.0f;
  menu_option_min_value = min_value;
  menu_option_max_value = max_value;
  if (value < menu_option_min_value || value > menu_option_max_value) {
    value = menu_option_min_value;
  }
  menu_option_value = value;
  menu_option_original_value = value;
  menu_option_jump = value_jump;
  lcd.setCursor(1, 1);
  lcd.print(menu_option_value);
}

void enter_edit_mode() {
  in_edit_mode = true;
}

void cancel_edit_mode() {
  in_edit_mode = false;
  lcd.setCursor(0, 1);
  lcd.print("       ");
  lcd.setCursor(1, 1);
  lcd.print(menu_option_original_value);
  menu_option_value = menu_option_original_value;
}

void save_edit_mode() {
  in_edit_mode = false;
  int new_value = menu_option_value * 10;
  RTC.writenvram(current_menu, new_value);
  lcd.setCursor(0, 1);
  lcd.print(" ");
  read_eprom();
}

void menu_increase_value() {
  if (menu_option_value + menu_option_jump > menu_option_max_value) {
    menu_option_value = menu_option_max_value;
  } else {
    menu_option_value += menu_option_jump;
  }
  lcd.setCursor(1, 1);
  lcd.print("      ");
  lcd.setCursor(1, 1);
  lcd.print(menu_option_value);
}

void menu_decrease_value() {
  if (menu_option_value - menu_option_jump < menu_option_min_value) {
    menu_option_value = menu_option_min_value;
  } else {
    menu_option_value -= menu_option_jump;
  }
  lcd.setCursor(1, 1);
  lcd.print("      ");
  lcd.setCursor(1, 1);
  lcd.print(menu_option_value);
}

void open_action_menu() {
  action_menu_is_opened = true;
  load_action_menu(0);
}

void close_action_menu() {
  action_menu_is_opened = false;
  lcd.clear();
  ignore_buttons = true;
}

void load_action_menu(int menu) {
  String label = "";
  float min_value = 0;
  float max_value = 0;
  float value_jump = 0.5;
  switch (menu) {
    case 0: {
        label = "[Inic. rampa 1]";
        break;
      }
    case 1: {
        label = "[Inic. rampa 2]";
        break;
      }
    case 2: {
        label = "[Inic. rampa 3]";
        break;
      }
    case 3: {
        label = "[Inic. rampa 4]";
        break;
      }
    case 4: {
        label = "[Canc. rampas]";
        break;
      }
  }
  current_action_menu = menu;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(label);
}

void action_menu_next_page() {
  if (current_action_menu >= LAST_ACTION_MENU) {
    current_action_menu = 0;
  } else {
    current_action_menu++;
  }
  load_action_menu(current_action_menu);
}

void action_menu_previous_page() {
  if (current_action_menu == 0) {
    current_action_menu = LAST_ACTION_MENU;
  } else {
    current_action_menu--;
  }
  load_action_menu(current_action_menu);
}

void action_enter_confirm_mode() {
  action_in_confirm_mode = true;
  lcd.setCursor(1, 1);
  lcd.print("Confirmar?");
}

void action_cancel_confirm_mode() {
  action_in_confirm_mode = false;
  lcd.setCursor(0, 1);
  lcd.print("           ");
}

void action_confirm() {
  action_in_confirm_mode = false;
  int rampa = 0;
  int ano = 0;
  int mes = 0;
  int dia = 0;
  int hora = 0;
  int minuto = 0;
  if (current_action_menu >= 0 && current_action_menu <= 3) {
    rampa = current_action_menu + 1;
  }
  if (rampa <= numero_rampas){
    switch (current_action_menu) {
      case 0:
      case 1:
      case 2:
      case 3: {
          ano = now.year() - 2000;
          mes = now.month();
          dia = now.day();
          hora = now.hour();
          minuto = now.minute();
          break;
        }
    }
    finished = false;
    RTC.writenvram(30, rampa);
    RTC.writenvram(31, ano);
    RTC.writenvram(32, mes);
    RTC.writenvram(33, dia);
    RTC.writenvram(34, hora);
    RTC.writenvram(35, minuto);
    RTC.writenvram(36, ano);
    RTC.writenvram(37, mes);
    RTC.writenvram(38, dia);
    RTC.writenvram(39, hora);
    RTC.writenvram(40, minuto);
    read_eprom();
  }
  close_action_menu();
}
