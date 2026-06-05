// ============================================================
//  설정 모듈
//  현재시간 설정 + 알람 설정을 조이스틱으로 처리
//
//  [이 모듈이 의존하는 것]  - 메인 탭에 아래가 정의돼 있어야 함
//     LiquidCrystal_I2C lcd;   RTC_DS3231 rtc;
//
//  [다른 팀원이 호출하는 함수]
//     initSettings();        // 메인 setup()에서 1번
//     openSettingMenu();     // 메인 loop()에서 버튼 눌렸을 때
//
//  [다른 파트가 읽어가는 결과물 (전역변수)]
//     alarmHour, alarmMin    // 알람 시각
//     alarmMode              // 0 = 퍼즐모드, 1 = RFID모드
//     alarmOn                // 알람 켜짐 여부 (해제 시 false 로 바꾸면 됨)
// ============================================================
#include "shared.h"

// ----- 알람 설정값 (결과물) -----
int  alarmHour = 7;
int  alarmMin  = 0;
int  alarmMode = 0;
bool alarmOn   = false;

// ----- 외부 공개 함수 -----

// 메인 setup() 에서 한 번 호출
void initSettings() {
  pinMode(JOY_SW, INPUT_PULLUP);
}

// 메인 loop() 에서 조이스틱 버튼 눌렸을 때 호출
// -> 메뉴 띄우고 시간/알람 설정 후 평시로 복귀
void openSettingMenu() {
  int  sel = 0;          // 0 = 현재시간, 1 = 알람
  bool moved = false;

  lcd.clear();
  while (true) {
    int x = analogRead(JOY_X);
    if (x > 400 && x < 600) moved = false;
    if (!moved && (x < 300 || x > 700)) { sel = !sel; moved = true; }

    lcd.setCursor(0, 0); lcd.print("Select mode:    ");
    lcd.setCursor(0, 1);
    lcd.print(sel == 0 ? "> Set Time      " : "> Set Alarm     ");

    if (digitalRead(JOY_SW) == LOW) {
      delay(300);
      if (sel == 0) doSetTime();
      else          doSetAlarm();
      return;
    }
    delay(120);
  }
}

// ----- 내부 함수 (모듈 밖에서 직접 안 부름) -----

// 조이스틱 상하 입력을 "값 증감 틱"으로 바꿔준다.
//  - 살짝 밀면 1회
//  - 계속 누르고 있으면 잠깐 뒤부터 자동 반복, 오래 누를수록 더 빠르게(가속)
// 반환: +1 증가, -1 감소, 0 변화 없음
int readStep(int y) {
  static int lastDir = 0;
  static unsigned long pressStart = 0;
  static unsigned long lastTick = 0;

  int dir = 0;
  if      (y < 300) dir = +1;
  else if (y > 700) dir = -1;

  if (dir == 0) { lastDir = 0; return 0; }   // 중립 -> 리셋

  unsigned long nowMs = millis();

  if (dir != lastDir) {                       // 막 누르기 시작 -> 즉시 1회
    lastDir = dir;
    pressStart = nowMs;
    lastTick = nowMs;
    return dir;
  }

  unsigned long held = nowMs - pressStart;
  if (held < 400) return 0;                          // 처음엔 잠깐 대기(오작동 방지)
  unsigned long interval = (held < 1500) ? 250 : 80; // 오래 누르면 가속
  if (nowMs - lastTick >= interval) {
    lastTick = nowMs;
    return dir;
  }
  return 0;
}

// 현재시간 설정 -> rtc.adjust() 로 RTC에 기록
void doSetTime() {
  DateTime now = rtc.now();
  int  h = now.hour();
  int  m = now.minute();
  int  field = 0;        // 0 = 시, 1 = 분
  bool xMoved = false;

  lcd.clear();
  lcd.blink();
  while (true) {
    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);

    // 값 증감(상하): 누르고 있으면 자동 반복
    int step = readStep(y);
    if (step != 0) joyAdjust(field == 0 ? h : m, step, field == 0 ? 24 : 60);

    // 칸 이동(좌우): 한 번 밀면 한 칸, 중앙 복귀해야 다음 입력
    if (x > 400 && x < 600) xMoved = false;
    if (!xMoved && (x < 300 || x > 700)) { field = !field; xMoved = true; }

    lcd.setCursor(0, 0); lcd.print("Set Time");
    lcd.setCursor(0, 1); printPad(h); lcd.print(":"); printPad(m); lcd.print("   ");
    lcd.setCursor(field == 0 ? 0 : 3, 1);

    if (digitalRead(JOY_SW) == LOW) {
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, 0));
      lcd.noBlink(); lcd.clear(); delay(300);
      return;
    }
    delay(40);
  }
}

// 알람 설정 -> 전역변수에 기록
void doSetAlarm() {
  int  field = 0;        // 0 = 시, 1 = 분, 2 = 모드, 3 = On/Off
  bool xMoved = false;
  bool yMoved = false;

  lcd.clear();
  lcd.blink();
  while (true) {
    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);

    // 값 변경(상하)
    if (field <= 1) {
      // 시/분: 누르고 있으면 자동 반복(빠르게 맞추기 편하게)
      int step = readStep(y);
      if (step != 0) changeAlarm(field, step);
    } else {
      // 모드 / On-Off: 한 번 밀면 한 번만 (홀드로 깜빡이지 않게)
      if (y > 400 && y < 600) yMoved = false;
      if (!yMoved) {
        if      (y < 300) { changeAlarm(field, +1); yMoved = true; }
        else if (y > 700) { changeAlarm(field, -1); yMoved = true; }
      }
    }

    // 칸 이동(좌우): 한 번 밀면 한 칸
    if (x > 400 && x < 600) xMoved = false;
    if (!xMoved) {
      if      (x < 300) { field = (field + 3) % 4; xMoved = true; }
      else if (x > 700) { field = (field + 1) % 4; xMoved = true; }
    }

    lcd.setCursor(0, 0);
    lcd.print("Set "); printPad(alarmHour); lcd.print(":"); printPad(alarmMin);
    lcd.print(alarmOn ? " ON " : " OFF");
    lcd.setCursor(0, 1);
    lcd.print("Mode:"); lcd.print(alarmMode == 0 ? "Puzzle" : "RFID  ");
    if      (field == 0) lcd.setCursor(5, 0);
    else if (field == 1) lcd.setCursor(8, 0);
    else if (field == 2) lcd.setCursor(5, 1);
    else                 lcd.setCursor(10, 0);  // On/Off 칸

    if (digitalRead(JOY_SW) == LOW) {
      // 사용자가 설정한 On/Off 상태를 그대로 저장 (강제로 켜지 않음)
      lcd.noBlink(); lcd.clear(); delay(300);
      return;
    }
    delay(40);
  }
}

// 알람 칸별 값 변경
void changeAlarm(int field, int dir) {
  if      (field == 0) joyAdjust(alarmHour, dir, 24);
  else if (field == 1) joyAdjust(alarmMin,  dir, 60);
  else if (field == 2) joyAdjust(alarmMode, dir,  2);
  else                 alarmOn = !alarmOn;   // On/Off 토글
}

// 값 v를 dir만큼 바꾸고 0~(range-1) 안에서 순환
void joyAdjust(int &v, int dir, int range) {
  v = (v + dir + range) % range;
}

// 한 자리 숫자 앞에 0 붙이기 (모듈 내부 전용)
void printPad(int v) {
  if (v < 10) lcd.print("0");
  lcd.print(v);
}
