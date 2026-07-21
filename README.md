<div align="center">

# ⚙️ Gears of Deceit

### 달리는 증기기관차 위, 10분간의 난장판 심리전

시민은 열차를 목적지로 몰고, 마피아는 기어이 멈춰 세운다.

![Engine](https://img.shields.io/badge/Unreal_Engine_5-0E1128?style=for-the-badge&logo=unrealengine&logoColor=white)
![Language](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Multiplayer](https://img.shields.io/badge/Steam_Multiplayer-171A21?style=for-the-badge&logo=steam&logoColor=white)
![Genre](https://img.shields.io/badge/3D_Party_Action-FF4088?style=for-the-badge)

</div>

---

## 🎯 한 줄 소개

**Gears of Deceit**는 움직이는 열차 위에서 벌어지는 **3D 멀티플레이 파티 액션 게임**입니다.
어몽어스식 마피아를 뿌리로 삼되, 무거운 추리를 걷어내고 **"도착하느냐 마느냐"** 라는
단순명료한 목표 아래 *파티애니멀즈* 스타일의 정신없는 재미에 집중했습니다.

| 진영 | 목표 |
| :--- | :--- |
| 🛡️ **시민 + 보안관** | 10분 안에 열차를 목적지까지 도착시켜라 |
| 🎭 **마피아** | 사보타주로 열차를 지연시켜 도착을 막아라 |

---

## 🔁 코어 루프

```
열차 정비 · 퀘스트 수행  →  마피아의 사보타주  →  위기 해결  →  ⏱️ 목적지 도착
```

> **퀘스트 완료 인원이 많을수록 열차가 빨라진다.**
> 열차 속도 배율 `= 1.0 + (퀘스트 완료 인원 / 유효 인원)` → 전원 완료 시 **2배속** 🚂💨

---

## ✨ 핵심 특징

### 🎮 7종 미니게임 퀘스트
설거지 · 굴뚝 청소 · 압력 게이지 · 순서 기억 · 레버 물길 · 야바위 · 화물 적재
난장판 속에서 임무를 완수해 열차의 속도를 끌어올린다.
*(로직은 클라이언트 위젯에서 실행, 서버는 배정·중복·최소 소요시간만 검증)*

### 🐾 동물 스킨 시스템
메인 메뉴에서 **5종 동물 캐릭터** 선택 · SceneCapture 실시간 프리뷰 ·
스킨별 스켈레톤과 전용 몽타주까지 서버 복제로 전원 동기화.

### 🕹️ GAS 기반 역할 & 스킬
GameplayTag로 역할을 배정하면 **HUD와 스킬 셋이 자동으로 전환**.
플레이어 빙의·리스폰 상태를 실시간 감지해 캐릭터 ASC를 UI 슬롯에 연결.

### 🧗 이동 & 클라이밍
Mixamo 애니메이션을 커스텀 스켈레톤에 적용 · 사다리 등반 시스템 ·
애님 몽타주는 `NetMulticast`로 전 클라이언트 동시 재생.

### 🎨 카툰 렌더링
Custom Depth 기반 **툰 셰이딩 · 외곽선 · 상호작용 하이라이트**로 손그림 감성의 룩 완성.

### 🌐 Steam 멀티플레이
Steam OSS + AdvancedSessions로 로비 생성/참가 · 실시간 세션 매칭.

---

## 🛠️ 개발 하이라이트 (Troubleshooting)

<details>
<summary><b>🐛 호스트만 스킨이 적용 안 되는 버그</b></summary>

- **원인:** 스킨 전송을 `BeginPlay`·`OnRep_Controller`에 걸었으나, 리슨 호스트는 `BeginPlay`가 빙의 전에 실행되고 `OnRep_Controller`는 서버에서 호출되지 않아 전송 시점이 영영 오지 않음.
- **해결:** 서버 빙의 시점인 `PossessedBy`에서 전송 (중복 1회 가드).
- **배운 점:** 초기화 타이밍은 서버 / 클라 / 리슨 호스트마다 다르다.
</details>

<details>
<summary><b>🐛 로비에서 압력 100 고정 + 경고음 무한 반복</b></summary>

- **원인:** 밸브에 페이즈 체크가 없어 로비에서 미니게임 3회 실패 → 폭발 → 상태만 오염된 채 압력 틱 정지.
- **해결:** 게임 시작 시 압력 리셋 + 밸브는 `Playing` 페이즈에서만 조작 가능하도록 제한.
</details>

<details>
<summary><b>🐛 움직이는 열차 위 사다리 등반 시 반대로 밀려 떨어짐</b></summary>

- **원인:** 등반 진입 시 엔진이 `Movement Base`를 자동 해제 → `StopMovementImmediately`로 관성이 사라져 캐릭터만 정지, 열차는 계속 이동.
- **해결:** `SetBase()`로 캐릭터의 Base를 사다리에 강제 바인딩 → 엔진이 열차의 이동 벡터를 다시 더해줌.
</details>

<details>
<summary><b>🐛 소프트웨어 커서 병목 & 비대칭 커서 클릭 오차</b></summary>

- **성능:** UMG 소프트웨어 커서가 매 프레임 메인/Slate 스레드 동기화로 프리징 유발 → **하드웨어 커서**로 전면 개편.
- **조작감:** 장갑·빗자루 등 비대칭 커서의 시각적 조준점 ≠ 클릭점(0,0) → 피벗·렌더 트랜스폼 오프셋으로 정밀 보정.
</details>

<details>
<summary><b>🐛 세션 목록 안 뜸 / 모르는 방이 섞임 / 유령 방</b></summary>

- **재참여 시 방 안 뜸:** ESC Exit가 세션 정리를 우회 → `LoadMainMenu` 연결 + 검색 워치독 + 가드 리셋.
- **모르는 방 섞임:** `SteamDevAppId=480`(공용 테스트 AppID) 공유 문제 → **BUILD_ID 서명 필터**로 차단.
- **유령 방:** 강제 종료로 남은 세션 → **30분 나이 필터**로 제거.
</details>

---

## 📝 기획 회고

> 초반엔 투표 대신 총으로 겨루는 서부 마피아, 중반엔 회의 시스템을 얹었지만
> **시스템이 늘수록 오히려 복잡하고 재미가 흐려졌다.**
> 결국 추리를 덜어내고 *"도착하냐 마냐"* 의 단순한 난장판 재미로 방향을 확정.
>
> 멀티플레이는 스토리가 아니라 **게임플레이 그 자체가 재미**여야 한다는 것,
> 그리고 프로토를 빠르게 만든 뒤 **재미부터 검증**하는 것이 중요하다는 걸 배웠다.

---

## 🧰 Tech Stack

`Unreal Engine 5` · `C++` · `Gameplay Ability System (GAS)` · `Steam OSS + AdvancedSessions` · `Post Process (Toon Shading)` · `Replication / RPC`
