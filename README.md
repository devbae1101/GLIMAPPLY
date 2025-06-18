# CDrawCircleDlg 클래스 API 레퍼런스

세 점을 지나는 원을 계산하고 그리는 애플리케이션입니다.

## 1. 주요 멤버 변수

### GDI+ 및 드로잉 관련

| 타입 | 변수명 | 설명 |
| :--- | :--- | :--- |
| `ULONG_PTR` | `m_gdiplusToken` | GDI+ 라이브러리 세션 관리를 위한 토큰. `OnInitDialog`에서 초기화되고 소멸자에서 해제됨. |
| `CRect` | `m_canvasRect` | 그림이 그려질 캔버스 영역의 좌표와 크기를 저장. `OnInitDialog`에서 초기화됨. |
| `CImage` | `m_bufferImage` | 더블 버퍼링을 위한 메모리 내 비트맵 이미지. 모든 그리기 작업은 이 버퍼에 먼저 수행됨. |

### UI 데이터 바인딩 (DDX)

| 타입 | 변수명 | 설명 |
| :--- | :--- | :--- |
| `CString` | `m_strPointRadius` | '반지름' Edit Control의 텍스트와 연결된 변수. |
| `CString` | `m_strMainThickness` | '두께' Edit Control의 텍스트와 연결된 변수. |
| `int` | `m_nPrecision` | '정밀도' Edit Control의 값과 연결된 변수. |

### 애플리케이션 상태 변수

| 타입 | 변수명 | 설명 |
| :--- | :--- | :--- |
| `CPoint` | `m_points[3]` | 사용자가 클릭한 세 개의 점 좌표를 저장하는 배열. |
| `int` | `m_nClickCount` | 사용자가 클릭한 점의 개수(0~3)를 저장함. |
| `bool` | `m_bCircleCalculated` | 세 점을 지나는 원의 계산이 성공적으로 완료되었는지 여부를 나타내는 플래그. |
| `Gdiplus::PointF` | `m_mainCircleCenter` | 계산된 메인 원의 중심 좌표. |
| `double` | `m_mainCircleRadius` | 계산된 메인 원의 반지름. |
| `bool` | `m_bIsDragging` | 사용자가 점을 드래그하는 중인지 여부를 나타내는 플래그. |
| `int` | `m_nDraggedPointIndex` | 사용자가 드래그 중인 점의 인덱스(0, 1, 또는 2). |

### 스레드 관리 변수

| 타입 | 변수명 | 설명 |
| :--- | :--- | :--- |
| `std::thread` | `m_workerThread` | [랜덤 이동] 애니메이션을 실행하는 백그라운드 스레드 객체. |
| `std::atomic<bool>` | `m_bIsThreadRunning` | 스레드가 현재 실행 중인지 여부를 나타내는 플래그. |

## 2. 주요 멤버 함수

### 헬퍼 함수 (내부 로직 처리)

`private` 영역에 선언되어 클래스 내부의 핵심 로직을 처리함.

---

### `void ResetState()`
애플리케이션의 모든 상태(클릭 카운트, 계산 플래그, 점 좌표 배열 등)를 초기 값으로 되돌림. [초기화] 버튼 클릭 및 프로그램 시작 시 호출됨.

---

### `bool InitializeCanvas()`
고정된 크기의 캔버스 영역(`m_canvasRect`)을 설정하고, 더블 버퍼링에 사용될 `m_bufferImage`를 생성함. 성공 시 `true`, 실패 시 `false`를 반환함.

---

### `void DrawObjects()`
메모리 버퍼(`m_bufferImage`)에 모든 그래픽 객체(클릭 지점 원, 메인 원 등)를 그림. `OnPaint` 핸들러에 의해 호출되며 실제 렌더링을 총괄함.

---

### `void UpdateCoordinateLabels()`
`m_points` 배열의 현재 좌표 값을 읽어 다이얼로그의 P1, P2, P3 라벨(Static Text)에 표시함.

---

### `bool CalculateCircleFromPoints()`
`m_points`에 저장된 세 점의 좌표를 바탕으로 원의 중심과 반지름을 계산함.

---

### `std::vector<Gdiplus::PointF> CalculateCircleVertices(...)`
주어진 파라미터를 바탕으로 원을 근사할 다각형의 꼭짓점 좌표 목록을 계산하여 반환함.
* **파라미터**: `float centerX`, `float centerY`, `float radius`, `int steps`
* **반환값**: 계산된 `PointF` 좌표들이 담긴 `std::vector`

---

### `void RenderCustomCircle(...)` (오버로딩됨)
`CalculateCircleVertices`로부터 받은 꼭짓점 목록을 화면에 그림. 함수 오버로딩을 통해 `Pen` 또는 `Brush`를 받아 외곽선 또는 채워진 원을 그림.
* **파라미터 (외곽선용)**: `Graphics& graphics`, `const Pen& pen`, `const std::vector<PointF>& points`
* **파라미터 (채우기용)**: `Graphics& graphics`, `const Brush& brush`, `const std::vector<PointF>& points`

---
### 메시지 핸들러 함수

Windows 메시지나 컨트롤 알림에 대한 응답으로 MFC 프레임워크에 의해 자동으로 호출됨.

---

### `void OnPaint()`
`WM_PAINT` 메시지에 대한 응답. `DrawObjects()`를 호출하여 버퍼 이미지를 그린 후, 화면에 복사하는 역할을 수행함.

---

### `void OnLButtonDown(UINT nFlags, CPoint point)`
마우스 왼쪽 버튼 클릭 시 호출됨. 클릭 횟수에 따라 점을 추가하거나 드래그를 시작하는 로직을 처리함.

---

### `void OnMouseMove(UINT nFlags, CPoint point)`
마우스 이동 시 호출됨. 드래그 상태일 경우 점의 좌표를 업데이트하고 화면 갱신을 요청함.

---

### `void OnLButtonUp(UINT nFlags, CPoint point)`
마우스 왼쪽 버튼에서 손을 뗄 때 호출되며, 드래그 상태를 종료함.

---

### `void OnBnClickedButtonReset()`
[초기화] 버튼 클릭 시 호출됨. `ResetState()` 함수를 호출함.

---

### `void OnBnClickedButtonRandomMove()`
[랜덤 이동] 버튼 클릭 시 호출됨. `RandomMoveThread`를 실행할 `std::thread`를 생성함.

---

### `void OnEnChangeEdit...()`
반지름, 두께, 정밀도 Edit Control의 내용이 변경될 때마다 호출됨. `UpdateData(TRUE)`로 값을 멤버 변수에 반영하고, `InvalidateRect`를 호출하여 화면을 실시간으로 갱신함.

---

### `afx_msg LRESULT OnUpdateCoords(WPARAM, LPARAM)`
작업 스레드가 보낸 사용자 정의 메시지 `WM_UPDATE_COORDS`를 처리함. UI 스레드에서 `UpdateCoordinateLabels`를 안전하게 호출하는 역할을 함.

---
### 스레드 함수

---

### `void RandomMoveThread()`
[랜덤 이동] 애니메이션의 실제 로직. `std::thread`에 의해 별도의 스레드에서 실행되어 10회 반복하며 점의 좌표를 랜덤으로 변경하고, 메인 UI 스레드에 화면 갱신 및 라벨 업데이트를 요청함.
