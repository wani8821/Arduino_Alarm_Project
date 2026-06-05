void executeRFIDMode() {
  // 알람을 끄기 위해 카드가 태그될 때까지 여기서 대기
  bool authenticated = false;
  while (!authenticated) {
    // 부저/LED는 main.ino의 ringing 로직에서 제어 중이라면 생략 가능
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      bool match = true;
      for (byte i = 0; i < 4; i++) {
        if (rfid.uid.uidByte[i] != myUID[i]) { match = false; break; }
      }
      if (match) authenticated = true;
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }
  ringing = false; // 알람 종료
}