# Noise Alert Program

## IoT 23-1 Final Project
### 21900411 심가현, 21900630 장유진

본 프로그램은 MQTT를 활용하여 특정 시설 내의 소음 경고 알림을 보내는 것을 목적으로 한다. <br/>
특정 시설 내의 소음이 임계값을 넘어가면 해당 시설 또는 호실 내에 있는 사람들에게 소음 경고 알림을 보낸다. <br/>
이를 위하여 publisher는 특정 시설 내의 소음을 측정하고 소음 경고 알림을 보내며, subscriber는 특정 시설 내의 위치하고 있는 구독자로 소음 경고에 대한 이벤트를 받는다. <br/>

---

### Directory Structure</br>

* **server**<br/>
ㄴ broker_recovery.c<br/>
* **admin**<br/>
ㄴ admin_logs.c<br/>
ㄴ admin_alerts.c<br/>
* **pub**<br/>
ㄴ nth_313_pub.c<br/>
* **sub**<br/>
ㄴ nth_313_sub.c<br/>

---

### Environment

* Ubuntu 22.04 LTS <br/>

---

### Needed Library
* mosquitto

---

### File Overview

* **server/broker_recovery.c**<br/>
Broker의 상태를 3초마다 체크하고 어떠한 이유로 broker와의 연결이 끊겼다면 새로운 broker를 실행시킨다.<br/>

* **admin/admin_logs.c**<br/>
broker_recovery에서 발생한 이벤트와 publisher와 subscriber 간의 데이터 송수신에 대한 모든 로그를 기록한다.<br/>

* **admin/admin_alerts.c**<br/>
소음 측정 센서의 상태 등 관리자가 긴급하게 확인해야 할 이벤트를 수신한다.<br/>

* **pub/nth_313_pub.c**<br/>
특정 위치의 소음을 측정하고 소음에 대한 이벤트를 subcriber에게 전달한다. <br/>
이때, 소음이 정상 범위(1-100)의 값일 경우 해당 위치의 subscriber에게소음에 대한 event를 전달하지만, 정상 범위가 아닌 경우 이를 리포트하기 위해 ‘admin/alerts’ 토픽에 event를 전달한다.<br/>

* **sub/nth_313_sub.c**<br/>
특정 위치의 소음 이벤트를 수신한다. <br/>

---

### How to run

1. Compile<br/>
소스코드를 컴파일하기 위해 터미널을 열어 아래의 명령어를 입력한다.<br/><br/>
`make`<br/>

2. Run<br/>
그 다음, 터미널에 아래의 명령어를 입력해 실행시킨다.<br/><br/>
`chmod +x run_program.sh`<br/>
`./run_program.bash`<br/><br/>
또는<br/><br/>
make 명령어 실행 후, make로 생성된 bin 폴더의 실행 파일들을 각각 실행한다.<br/>
단, 다음의 순서로 실행해야 한다.<br/>
```
    ./bin/broker_recovery
    ./bin/admin_logs
    ./bin/admin_alerts
    ./bin/nth_313_pub
    ./bin/nth_313_sub
```
