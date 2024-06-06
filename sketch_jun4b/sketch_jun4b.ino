#include <Wire.h>
#include <Firebase_Arduino_WiFiNINA.h>
#include <NewPing.h>
#include <Servo.h>

// Firebase 및 WiFi 설정 정보
#define FIREBASE_HOST "license-d68c6-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "qyQJdDEJv7Kn61As6rjle22QmTzViiHIGoGLC0ba"
#define SSID "U+Net7120"
#define SSID_PASSWORD "DD9B003487"
#define PARKING_SPOT_STATE_KEY "/parking_spot_state"
#define PARKING_SPOT_ANGLE_KEY "/parking_spot_angle/current_angle"
#define WEBCAM_ACTIVE_KEY "/webcam_active"

// 센서 및 서보 모터 설정
#define NUM_SENSORS 4
const int trigPins[NUM_SENSORS] = {2, 4, 6, 8};
const int echoPins[NUM_SENSORS] = {3, 5, 7, 9};
const int SERVO_PIN = 10;
const unsigned int MAX_DISTANCE = 200;
const int STANDARD_DISTANCE = 10;
const int NUM_READINGS = 5; // 각 센서의 평균을 계산할 읽기 횟수

Servo myservo;
FirebaseData firebaseData;
NewPing sonar[NUM_SENSORS] = {
    NewPing(trigPins[0], echoPins[0], MAX_DISTANCE),
    NewPing(trigPins[1], echoPins[1], MAX_DISTANCE),
    NewPing(trigPins[2], echoPins[2], MAX_DISTANCE),
    NewPing(trigPins[3], echoPins[3], MAX_DISTANCE)};

bool previousStates[NUM_SENSORS] = {false, false, false, false};

void setup()
{
    Serial.begin(9600); // 시리얼 통신 시작
    myservo.attach(SERVO_PIN); // 서보 모터 핀 연결
    WiFi.begin(SSID, SSID_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to WiFi");
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH, SSID, SSID_PASSWORD);
}

void updateParkingSpotState(int sensorIndex, bool isActive)
{
    String path = String(PARKING_SPOT_STATE_KEY) + "/A" + String(sensorIndex + 1); // Firebase 경로 설정
    Firebase.setBool(firebaseData, path, isActive); // 상태 업데이트
}

void updateServoAngle(int angle)
{
    Firebase.setInt(firebaseData, PARKING_SPOT_ANGLE_KEY, angle); // Firebase에 각도 업데이트
}

unsigned int getFilteredDistance(NewPing& sonar)
{
    unsigned int sum = 0;
    for (int i = 0; i < NUM_READINGS; i++)
    {
        sum += sonar.ping_cm();
        delay(50); // 각 읽기 사이의 짧은 딜레이를 추가하여 간섭을 줄임
    }
    return sum / NUM_READINGS; // 평균값 반환
}

void loop()
    for (int i = 0; i < NUM_SENSORS; i++)
    {
        unsigned int distance = getFilteredDistance(sonar[i]); // 거리 측정
        bool isActive = distance > 0 && distance <= STANDARD_DISTANCE; // 센서 활성화 여부 결정

        if (isActive != previousStates[i])
        { // 상태 변경이 있을 때만 업데이트
            previousStates[i] = isActive;

            if (i < 2 && isActive)
            { // A1 또는 A2 센서가 활성화된 경우
                myservo.write(0); // 0도로 서보모터 회전
                updateServoAngle(0);
            }
            else if (i >= 2 && isActive)
            { // A3 또는 A4 센서가 활성화된 경우
                myservo.write(165); // 165도로 서보모터 회전
                updateServoAngle(165);
            }

            delay(4000); // 1초 대기 후에 Firebase에 bool 값을 보냄
            updateParkingSpotState(i, isActive); // Firebase에 상태 업데이트
        }

        // 측정한 거리를 시리얼 모니터에 출력
        Serial.print("Sensor ");
        Serial.print(i + 1);
        Serial.print(" Distance: ");
        Serial.print(distance);
        Serial.println(" cm");
    }

    delay(100); // 센서 간 간섭을 줄이기 위해 루프 사이의 짧은 딜레이 추가
}

