#ifndef SHARED_H
#define SHARED_H

#include <Arduino.h>
#include <RTClib.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>

// ===== 조이스틱 핀 =====
#define JOY_X  A0
#define JOY_Y  A1
#define JOY_SW 2

// ===== 공통 핀 =====
#define BUZZER 7
#define LED    8

#define IN1 9
#define IN2 10
#define IN3 11
#define IN4 12

// ===== 알람 설정값 =====
extern int  alarmHour;
extern int  alarmMin;
extern int  alarmMode;
extern bool alarmOn;

extern bool alarmConsumed;
extern uint32_t alarmConsumedMinute;

// ===== 공용 상태 변수 =====
extern bool curtainOpen;
extern bool ringing;

extern int  noteIndex;
extern unsigned long noteStart;

// 멜로디 데이터 포인터
extern int* melody;
extern int* noteSlot;
extern int  melodyLen;

// 메인에서 쓰는 공용 객체
extern RTC_DS3231 rtc;
extern LiquidCrystal_I2C lcd;
extern Stepper myStepper;

// ===== 설정 모듈 함수 =====
void initSettings();
void openSettingMenu();

// ===== 미로게임 모듈 함수 =====
bool playMazeGame();

// ===== 화면 모듈 함수 =====
void showClock(DateTime now);

// ===== 알람 로직 =====
void checkAlarm(DateTime now);

// ===== 오디오/알람 =====
void startMelody();
void soundAlarm();
void stopAlarm();

// ===== 커튼 모터 =====
void openCurtain();
void closeCurtain();

// =====RFID=====
void executeRFIDMode();

#endif