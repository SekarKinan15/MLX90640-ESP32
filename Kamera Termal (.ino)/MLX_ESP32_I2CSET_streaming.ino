
// ESP32 + MLX90640 (Melexis API) – Stream kontinu 1 frame utuh
//Copilot 15 Des 2025
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

#define MLX_ADDR 0x33

uint16_t eeData[832];
paramsMLX90640 mlxParams;
uint16_t frameData[834];

float tempEven[768]; // subpage baris genap
float tempOdd[768];  // subpage baris ganjil
float tempOut[768];

float emissivity = 0.95;   // bisa kamu naikkan ke ~0.98 untuk kulit manusia

void setup() {
  Serial.begin(921600);          // ↑ baud tinggi agar muat > 5 fps (opsional 500000 / 921600)
  delay(1500);
  Wire.begin(21, 22);
  MLX90640_I2CFreqSet(400);      // I2C 400 kHz

  if (MLX90640_DumpEE(MLX_ADDR, eeData) != 0) {
    Serial.println("EE_FAIL");
    while (1);
  }
  MLX90640_ExtractParameters(eeData, &mlxParams);

  // Mode & refresh
  MLX90640_SetInterleavedMode(MLX_ADDR);  // alternatif: SetChessMode
  MLX90640_SetRefreshRate(MLX_ADDR, 4);   // 8 Hz (0..7 -> 0.5..64 Hz). Bisa 5=16 Hz, 6=32 Hz, 7=64 Hz (cek kestabilan).
  Serial.println("READY");
}

void loop() {
  // --- Ambil subpage 0 (baris genap) ---
  int sp = -1;
  while (true) {
    if (MLX90640_GetFrameData(MLX_ADDR, frameData) >= 0) {
      sp = MLX90640_GetSubPageNumber(frameData);
      if (sp == 0) break;
    }
    delay(1);
  }
  float Ta0 = MLX90640_GetTa(frameData, &mlxParams);
  MLX90640_CalculateTo(frameData, &mlxParams, emissivity, Ta0, tempEven);

  // --- Ambil subpage 1 (baris ganjil) ---
  while (true) {
    if (MLX90640_GetFrameData(MLX_ADDR, frameData) >= 0) {
      sp = MLX90640_GetSubPageNumber(frameData);
      if (sp == 1) break;
    }
    delay(1);
  }
  float Ta1 = MLX90640_GetTa(frameData, &mlxParams);
  MLX90640_CalculateTo(frameData, &mlxParams, emissivity, Ta1, tempOdd);

  // --- Gabungkan jadi frame utuh ---
  for (int r = 0; r < 24; r++) {
    for (int c = 0; c < 32; c++) {
      int i = r * 32 + c;
      tempOut[i] = (r % 2 == 0) ? tempEven[i] : tempOdd[i];
    }
  }

  // (Opsional) koreksi piksel buruk
  // MLX90640_BadPixelsCorrection(...);

  // --- Stream satu frame utuh ke Python ---
  Serial.println("FRAME_START");
  for (int i = 0; i < 768; i++) {
    Serial.print(tempOut[i], 1); // 1 decimal -> kurangi payload; naikkan ke 2 bila perlu
    if (i < 767) Serial.print(",");
  }
  Serial.println();
  Serial.println("FRAME_END");

  // JANGAN berhenti! (tidak ada while(1);) -> langsung lanjut ambil frame berikutnya
  // (Opsional) kecilkan delay bila ingin "napas" serial; sesuaikan dengan refresh rate sensor
  // delay(1);
}
