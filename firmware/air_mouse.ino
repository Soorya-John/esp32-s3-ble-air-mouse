#include <Arduino.h>
#include <Wire.h>
#include <BleMouse.h>

// ============================================================
// PIN DEFINITIONS
// ============================================================

// MPU6050 / MPUU6050 clone
#define MPU_ADDR 0x68
#define SDA_PIN 8
#define SCL_PIN 9

// Mouse buttons
#define LEFT_BUTTON_PIN 10
#define RIGHT_BUTTON_PIN 11


// ============================================================
// MPU6050 REGISTERS
// ============================================================

#define PWR_MGMT_1     0x6B
#define GYRO_CONFIG    0x1B
#define ACCEL_CONFIG   0x1C
#define GYRO_XOUT_H    0x43
#define WHO_AM_I       0x75


// ============================================================
// BLE MOUSE
// ============================================================

BleMouse bleMouse(
  "Soorya Air Mouse",
  "ESP32-S3",
  100
);


// ============================================================
// GYRO SETTINGS
// ============================================================

// ±250 degrees/sec
#define GYRO_SCALE 131.0f

// Number of samples for startup calibration
#define CALIBRATION_SAMPLES 1500

float gyroOffsetX = 0;
float gyroOffsetY = 0;
float gyroOffsetZ = 0;


// ============================================================
// CURSOR SETTINGS
// ============================================================

// Cursor speed
// Lower = slower
float CURSOR_SENSITIVITY = 0.28f;

// Ignore tiny gyro movements
float GYRO_DEADZONE = 3.0f;

// Maximum cursor movement per update
int MAX_MOUSE_MOVE = 8;


// ============================================================
// SMOOTHING
// ============================================================

// Lower = smoother but more delayed
// Higher = faster but more jittery
float SMOOTHING_ALPHA = 0.12f;


// Filtered gyro values
float filteredGX = 0;
float filteredGY = 0;
float filteredGZ = 0;


// ============================================================
// BUTTON STATES
// ============================================================

bool lastLeftState = HIGH;
bool lastRightState = HIGH;


// ============================================================
// WRITE MPU REGISTER
// ============================================================

void writeMPURegister(byte reg, byte value)
{
  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);
  Wire.write(value);

  Wire.endTransmission();
}


// ============================================================
// READ MPU REGISTER
// ============================================================

byte readMPURegister(byte reg)
{
  Wire.beginTransmission(MPU_ADDR);

  Wire.write(reg);

  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, (uint8_t)1);

  if (Wire.available())
    return Wire.read();

  return 0xFF;
}


// ============================================================
// READ GYRO
// ============================================================

void readGyro(
  int16_t &gx,
  int16_t &gy,
  int16_t &gz
)
{
  Wire.beginTransmission(MPU_ADDR);

  Wire.write(GYRO_XOUT_H);

  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, (uint8_t)6);

  if (Wire.available() >= 6)
  {
    gx = ((int16_t)Wire.read() << 8) | Wire.read();
    gy = ((int16_t)Wire.read() << 8) | Wire.read();
    gz = ((int16_t)Wire.read() << 8) | Wire.read();
  }
  else
  {
    gx = 0;
    gy = 0;
    gz = 0;
  }
}


// ============================================================
// INITIALIZE MPU
// ============================================================

bool initializeMPU()
{
  Serial.println();
  Serial.println("Initializing MPU...");


  // Check I2C communication

  Wire.beginTransmission(MPU_ADDR);

  byte error = Wire.endTransmission();

  if (error != 0)
  {
    Serial.println(
      "ERROR: MPU not responding at 0x68!"
    );

    return false;
  }


  Serial.println(
    "I2C device found at 0x68"
  );


  // Read WHO_AM_I

  byte whoAmI =
    readMPURegister(WHO_AM_I);


  Serial.print("WHO_AM_I = 0x");

  if (whoAmI < 0x10)
    Serial.print("0");

  Serial.println(whoAmI, HEX);


  // Your MPUU6050 clone returns 0x70.
  // Do not reject it.

  if (whoAmI == 0x68)
  {
    Serial.println(
      "Standard MPU6050 ID detected."
    );
  }
  else
  {
    Serial.println(
      "Non-standard MPU ID detected."
    );

    Serial.println(
      "Continuing anyway."
    );
  }


  // Wake up MPU

  writeMPURegister(
    PWR_MGMT_1,
    0x00
  );

  delay(100);


  // Gyro ±250 DPS

  writeMPURegister(
    GYRO_CONFIG,
    0x00
  );


  // Accelerometer ±2G

  writeMPURegister(
    ACCEL_CONFIG,
    0x00
  );


  delay(100);


  Serial.println(
    "MPU initialization complete."
  );

  return true;
}


// ============================================================
// GYRO CALIBRATION
// ============================================================

void calibrateGyro()
{
  Serial.println();
  Serial.println(
    "================================"
  );

  Serial.println(
    "GYRO CALIBRATION"
  );

  Serial.println(
    "KEEP THE MOUSE COMPLETELY STILL"
  );

  Serial.println(
    "================================"
  );


  delay(2000);


  long sumX = 0;
  long sumY = 0;
  long sumZ = 0;


  int16_t gx;
  int16_t gy;
  int16_t gz;


  for (
    int i = 0;
    i < CALIBRATION_SAMPLES;
    i++
  )
  {
    readGyro(
      gx,
      gy,
      gz
    );


    sumX += gx;
    sumY += gy;
    sumZ += gz;


    if (i % 100 == 0)
      Serial.print(".");


    delay(2);
  }


  Serial.println();


  gyroOffsetX =
    (float)sumX /
    CALIBRATION_SAMPLES;


  gyroOffsetY =
    (float)sumY /
    CALIBRATION_SAMPLES;


  gyroOffsetZ =
    (float)sumZ /
    CALIBRATION_SAMPLES;


  Serial.println();
  Serial.println(
    "Gyro calibration complete."
  );


  Serial.print(
    "X offset: "
  );

  Serial.println(
    gyroOffsetX
  );


  Serial.print(
    "Y offset: "
  );

  Serial.println(
    gyroOffsetY
  );


  Serial.print(
    "Z offset: "
  );

  Serial.println(
    gyroOffsetZ
  );


  // Reset filters

  filteredGX = 0;
  filteredGY = 0;
  filteredGZ = 0;
}


// ============================================================
// LIMIT CURSOR MOVEMENT
// ============================================================

int limitMovement(float value)
{
  if (value > MAX_MOUSE_MOVE)
    value = MAX_MOUSE_MOVE;


  if (value < -MAX_MOUSE_MOVE)
    value = -MAX_MOUSE_MOVE;


  return (int)value;
}


// ============================================================
// APPLY DEADZONE
// ============================================================

float applyDeadzone(
  float value,
  float deadzone
)
{
  if (fabs(value) < deadzone)
  {
    return 0;
  }


  if (value > 0)
  {
    return value - deadzone;
  }
  else
  {
    return value + deadzone;
  }
}


// ============================================================
// PROCESS GYRO / CURSOR
// ============================================================

void processGyro()
{
  int16_t rawGX;
  int16_t rawGY;
  int16_t rawGZ;


  readGyro(
    rawGX,
    rawGY,
    rawGZ
  );


  // Convert raw gyro values
  // to degrees per second

  float gx =
    (
      (float)rawGX -
      gyroOffsetX
    ) / GYRO_SCALE;


  float gy =
    (
      (float)rawGY -
      gyroOffsetY
    ) / GYRO_SCALE;


  float gz =
    (
      (float)rawGZ -
      gyroOffsetZ
    ) / GYRO_SCALE;


  // ==========================================================
  // LOW-PASS FILTER
  // ==========================================================

  filteredGX =
    filteredGX +
    SMOOTHING_ALPHA *
    (gx - filteredGX);


  filteredGY =
    filteredGY +
    SMOOTHING_ALPHA *
    (gy - filteredGY);


  filteredGZ =
    filteredGZ +
    SMOOTHING_ALPHA *
    (gz - filteredGZ);


  // ==========================================================
  // DEADZONE
  // ==========================================================

  filteredGX =
    applyDeadzone(
      filteredGX,
      GYRO_DEADZONE
    );


  filteredGY =
    applyDeadzone(
      filteredGY,
      GYRO_DEADZONE
    );


  // ==========================================================
  // CURSOR MAPPING
  //
  // Pitch UP    -> Cursor UP
  // Pitch DOWN  -> Cursor DOWN
  //
  // Roll RIGHT  -> Cursor RIGHT
  // Roll LEFT   -> Cursor LEFT
  // ==========================================================

  float mouseX =
    filteredGX *
    CURSOR_SENSITIVITY;


  float mouseY =
    filteredGY *
    CURSOR_SENSITIVITY;


  int moveX =
    limitMovement(
      mouseX
    );


  int moveY =
    limitMovement(
      mouseY
    );


  // ==========================================================
  // SEND CURSOR MOVEMENT
  // ==========================================================

  if (
    bleMouse.isConnected()
  )
  {
    if (
      moveX != 0 ||
      moveY != 0
    )
    {
      bleMouse.move(
        moveX,
        moveY
      );
    }
  }
}


// ============================================================
// PROCESS BUTTONS
// ============================================================

void processButtons()
{
  bool leftState =
    digitalRead(
      LEFT_BUTTON_PIN
    );


  bool rightState =
    digitalRead(
      RIGHT_BUTTON_PIN
    );


  // ==========================================================
  // LEFT CLICK
  // ==========================================================

  if (
    lastLeftState == HIGH &&
    leftState == LOW
  )
  {
    if (
      bleMouse.isConnected()
    )
    {
      bleMouse.click(
        MOUSE_LEFT
      );

      Serial.println(
        "LEFT CLICK"
      );
    }
  }


  // ==========================================================
  // RIGHT CLICK
  // ==========================================================

  if (
    lastRightState == HIGH &&
    rightState == LOW
  )
  {
    if (
      bleMouse.isConnected()
    )
    {
      bleMouse.click(
        MOUSE_RIGHT
      );

      Serial.println(
        "RIGHT CLICK"
      );
    }
  }


  lastLeftState =
    leftState;


  lastRightState =
    rightState;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(2000);


  Serial.println();
  Serial.println(
    "===================================="
  );

  Serial.println(
    "       ESP32-S3 BLE AIR MOUSE"
  );

  Serial.println(
    "===================================="
  );


  // ==========================================================
  // BUTTONS
  // ==========================================================

  pinMode(
    LEFT_BUTTON_PIN,
    INPUT_PULLUP
  );

  pinMode(
    RIGHT_BUTTON_PIN,
    INPUT_PULLUP
  );


  // ==========================================================
  // I2C
  // ==========================================================

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  Wire.setClock(100000);

  delay(100);


  // ==========================================================
  // MPU INITIALIZATION
  // ==========================================================

  if (
    !initializeMPU()
  )
  {
    Serial.println(
      "MPU initialization failed!"
    );


    while (1)
    {
      delay(1000);
    }
  }


  // ==========================================================
  // GYRO CALIBRATION
  // ==========================================================

  calibrateGyro();


  // ==========================================================
  // BLE MOUSE
  // ==========================================================

  Serial.println(
    "Starting BLE mouse..."
  );


  bleMouse.begin();


  Serial.println();
  Serial.println(
    "===================================="
  );

  Serial.println(
    "AIR MOUSE READY"
  );

  Serial.println(
    "===================================="
  );

  Serial.println(
    "Pair with: Soorya Air Mouse"
  );

  Serial.println();

  Serial.println(
    "Pitch UP   -> Cursor UP"
  );

  Serial.println(
    "Pitch DOWN -> Cursor DOWN"
  );

  Serial.println(
    "Roll RIGHT -> Cursor RIGHT"
  );

  Serial.println(
    "Roll LEFT  -> Cursor LEFT"
  );

  Serial.println();

  Serial.println(
    "Joystick scrolling DISABLED."
  );
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  processGyro();

  processButtons();

  delay(5);
}