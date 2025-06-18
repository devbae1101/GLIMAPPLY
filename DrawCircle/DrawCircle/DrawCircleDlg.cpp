
// DrawCircleDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "DrawCircle.h"
#include "DrawCircleDlg.h"
#include "afxdialogex.h"
#include <random>
#include <chrono>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDrawCircleDlg dialog
CDrawCircleDlg::CDrawCircleDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DRAWCIRCLE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_gdiplusToken = 0;
	m_nClickCount = 0;
	m_bCircleCalculated = false;
	m_bIsDragging = false;
	m_nDraggedPointIndex = -1;
	m_mainCircleRadius = 0.0;
	m_bIsThreadRunning = false;
}

CDrawCircleDlg::~CDrawCircleDlg()
{
	// 스레드를 안전하게 종료
	m_bIsThreadRunning = false; // 스레드에게 종료 신호 보내기
	if (m_workerThread.joinable())
	{
		m_workerThread.join(); // 스레드가 끝날 때까지 대기
	}

	// GDI+ 리소스 해제
	if (m_gdiplusToken != 0) {
		Gdiplus::GdiplusShutdown(m_gdiplusToken);
	}
}

void CDrawCircleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_POINT_RADIUS, m_strPointRadius);
	DDX_Text(pDX, IDC_EDIT_MAIN_THICKNESS, m_strMainThickness);
}

BEGIN_MESSAGE_MAP(CDrawCircleDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON_RANDOM_MOVE, &CDrawCircleDlg::OnBnClickedButtonRandomMove)
	ON_BN_CLICKED(IDC_BUTTON_RESET, &CDrawCircleDlg::OnBnClickedButtonReset)
	ON_EN_CHANGE(IDC_EDIT_POINT_RADIUS, &CDrawCircleDlg::OnEnChangeEditPointRadius)
	ON_EN_CHANGE(IDC_EDIT_MAIN_THICKNESS, &CDrawCircleDlg::OnEnChangeEditMainThickness)
END_MESSAGE_MAP()

// CDrawCircleDlg message handlers
BOOL CDrawCircleDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	// GDI+ 초기화
	GdiplusStartupInput gdiplusStartupInput;
	Status status = GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	if (status != Ok)
	{
		MessageBox(_T("GDI+ 라이브러리를 시작할 수 없습니다.\n프로그램의 그래픽 기능이 동작하지 않을 수 있습니다."),
			_T("GDI+ 초기화 오류"), MB_OK | MB_ICONERROR);
	}

	UpdateData(FALSE); // 변수 값을 컨트롤에 표시

	// 고정된 크기의 캔버스 정의
	CDrawCircleDlg::InitializeCanvas();

	// 모든 상태 변수 초기화
	CDrawCircleDlg::ResetState();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDrawCircleDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDrawCircleDlg::OnPaint()
{

	CPaintDC dc(this); // 평상시 그리기를 위한 DC 생성

	// 1. 메모리 버퍼에 모든 객체를 그림
	DrawObjects();

	// 2. 완성된 버퍼 이미지를 캔버스 위치에 한 번에 복사 (더블 버퍼링)
	m_bufferImage.BitBlt(dc.GetSafeHdc(), m_canvasRect.left, m_canvasRect.top);
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDrawCircleDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CDrawCircleDlg::ResetState()
{
	m_nClickCount = 0;
	m_bCircleCalculated = false;
	m_bIsDragging = false;
	m_nDraggedPointIndex = -1;
	m_mainCircleRadius = 0;

	for (int i = 0; i < 3; i++)
	{
		m_points[i].SetPoint(0, 0);
	}

	UpdateCoordinateLabels();
	InvalidateRect(m_canvasRect, TRUE); // 캔버스 영역만 새로고침
}

bool CDrawCircleDlg::InitializeCanvas()
{
	// 고정된 크기의 캔버스 정의
	const int canvasWidth = 1280;
	const int canvasHeight = 960;

	// 캔버스가 다이얼로그의 어느 위치에서 시작할지 정의
	const CPoint canvasTopLeft(10, 10);

	// m_canvasRect 변수에 고정된 사각형 영역을 설정
	m_canvasRect.SetRect(canvasTopLeft.x,
		canvasTopLeft.y,
		canvasTopLeft.x + canvasWidth,
		canvasTopLeft.y + canvasHeight);

	// 만약을 위해 기존 이미지가 있다면 먼저 파괴
	if (!m_bufferImage.IsNull())
	{
		m_bufferImage.Destroy();
	}

	// 더블 버퍼링용 CImage 생성
	if (!m_bufferImage.Create(canvasWidth, canvasHeight, 32))
	{
		AfxMessageBox(_T("오류: 드로잉 버퍼(CImage)를 생성할 수 없습니다."));
		return false; // 생성 실패
	}

	return true; // 생성 성공}
}

void CDrawCircleDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// 캔버스 영역 밖에서 클릭했다면 아무 작업도 하지 않음
	if (!m_canvasRect.PtInRect(point))
	{
		CDialogEx::OnLButtonDown(nFlags, point);
		return;
	}

	//  DDX로 최신 값을 CString 멤버 변수에 먼저 가져옴
	UpdateData(TRUE);

	// 반지름 유효성 검사
	int pointRadius = _ttoi(m_strPointRadius);
	if (m_strPointRadius.IsEmpty() || pointRadius <= 0)
	{
		MessageBox(_T("클릭 지점 원의 반지름을 먼저 입력해주세요."), _T("입력 필요"), MB_OK | MB_ICONWARNING);
		GetDlgItem(IDC_EDIT_POINT_RADIUS)->SetFocus();
		return;
	}

	// 아직 점을 3개 다 찍지 않았을 때 -> 점 추가
	if (m_nClickCount < 3)
	{
		// 추가된 핵심 로직] 세 번째 점을 찍기 전에는 두께 값도 확인
		if (m_nClickCount == 2) // 이번 클릭이 세 번째 점이 되는 경우
		{
			if (m_strMainThickness.IsEmpty() || (float)_ttof(m_strMainThickness) <= 0)
			{
				MessageBox(_T("메인 원의 두께를 먼저 입력해주세요."), _T("입력 필요"), MB_OK | MB_ICONWARNING);
				GetDlgItem(IDC_EDIT_MAIN_THICKNESS)->SetFocus();
				return; // 두께 값이 없으면 점 추가 중단
			}
		}

		// 모든 검사를 통과했으면 점 추가
		m_points[m_nClickCount++] = point;

		if (m_nClickCount == 3)
		{
			m_bCircleCalculated = CalculateCircleFromPoints();
		}

		UpdateCoordinateLabels();
		InvalidateRect(m_canvasRect, FALSE);
	}

	// 점을 3개 다 찍은 후 -> 드래그 시작점인지 확인
	else
	{
		bool bHit = false; // 기존 점을 클릭했는지 확인하는 플래그
		for (int i = 0; i < 3; i++)
		{
			CRect hitBox(m_points[i], m_points[i]);
			hitBox.InflateRect(pointRadius, pointRadius);

			if (hitBox.PtInRect(point))
			{
				// 기존 점을 클릭했음! 드래그 상태로 전환
				m_bIsDragging = true;
				m_nDraggedPointIndex = i;
				SetCapture();
				bHit = true;
				break;
			}
		}
	}

	CDialogEx::OnLButtonDown(nFlags, point);
}

void CDrawCircleDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bIsDragging)
	{
		m_bIsDragging = false;
		m_nDraggedPointIndex = -1;
		ReleaseCapture(); // 마우스 고정 해제
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CDrawCircleDlg::DrawObjects()
{
	UpdateData(true);

	// 버퍼 이미지의 DC(Device Context)를 얻어옴
	CDC* pImageDC = CDC::FromHandle(m_bufferImage.GetDC());
	{

		// GDI+ 그래픽스 객체 생성
		Graphics graphics(pImageDC->GetSafeHdc());
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		// 버퍼 이미지의 배경을 흰색으로 깨끗하게 지움
		graphics.Clear(Gdiplus::Color(255, 255, 255, 255));


		// 1. 클릭 지점 원 그리기
		int pointRadius = _ttoi(m_strPointRadius);
		if (pointRadius > 0)
		{
			Gdiplus::SolidBrush blackBrush(Gdiplus::Color(255, 0, 0, 0));
			for (int i = 0; i < m_nClickCount; i++)
			{
				CPoint relativePt = m_points[i] - m_canvasRect.TopLeft();

				DrawCustomCircle(graphics, blackBrush, (float)relativePt.x, (float)relativePt.y, (float)pointRadius);
			}
		}

		// 2. 세 점을 지나는 정원 그리기

		float mainThickness = (float)_ttof(m_strMainThickness);
		if (mainThickness > 0)
		{
			Pen mainPen(Gdiplus::Color(255, 0, 0, 0), (float)mainThickness);

			// 다이얼로그 좌표를 캔버스(버퍼) 좌표로 변환
			PointF relativeCenter(m_mainCircleCenter.X - m_canvasRect.left, m_mainCircleCenter.Y - m_canvasRect.top);

			DrawCustomCircle(graphics, mainPen, relativeCenter.X, relativeCenter.Y, (float)m_mainCircleRadius);
		}
	}


	// 중요: 사용이 끝난 DC 핸들 반드시 해제
	m_bufferImage.ReleaseDC();
}

void CDrawCircleDlg::DrawCustomCircle(Graphics& graphics, const Pen& pen, float centerX, float centerY, float radius)
{
	if (radius <= 0) return;

	// 정밀도 설정: 원을 몇 개의 점으로 표현할지 결정
	const int steps = 200; // 더 부드러운 원을 위해 점 개수 증가
	std::vector<PointF> points = CalculateCircleVertices(centerX, centerY, radius, steps);

	graphics.DrawPolygon(&pen, points.data(), steps);
}

void CDrawCircleDlg::DrawCustomCircle(Graphics& graphics, const Brush& brush, float centerX, float centerY, float radius)
{
	if (radius <= 0) return;

	// 정밀도 설정: 원을 몇 개의 점으로 표현할지 결정
	const int steps = 100;
	std::vector<PointF> points = CalculateCircleVertices(centerX, centerY, radius, steps);

	graphics.FillPolygon(&brush, points.data(), (int)points.size());
}

std::vector<PointF> CDrawCircleDlg::CalculateCircleVertices(float centerX, float centerY, float radius, int steps)
{
	std::vector<PointF> points(steps);

	// 원의 꼭짓점 좌표 계산 (for 루프)
	for (int i = 0; i < steps; ++i)
	{
		float angle = 2.0f * 3.14159265f * i / steps;
		points[i].X = centerX + radius * cos(angle);
		points[i].Y = centerY + radius * sin(angle);
	}

	return points;
}

bool CDrawCircleDlg::CalculateCircleFromPoints()
{
	using namespace Gdiplus;
	PointF p1((float)m_points[0].x, (float)m_points[0].y);
	PointF p2((float)m_points[1].x, (float)m_points[1].y);
	PointF p3((float)m_points[2].x, (float)m_points[2].y);

	double D = 2 * (p1.X * (p2.Y - p3.Y) + p2.X * (p3.Y - p1.Y) + p3.X * (p1.Y - p2.Y));
	if (abs(D) < 1e-6) { return false; }

	double p1_sq = p1.X * p1.X + p1.Y * p1.Y;
	double p2_sq = p2.X * p2.X + p2.Y * p2.Y;
	double p3_sq = p3.X * p3.X + p3.Y * p3.Y;

	m_mainCircleCenter.X = (float)((p1_sq * (p2.Y - p3.Y) + p2_sq * (p3.Y - p1.Y) + p3_sq * (p1.Y - p2.Y)) / D);
	m_mainCircleCenter.Y = (float)((p1_sq * (p3.X - p2.X) + p2_sq * (p1.X - p3.X) + p3_sq * (p2.X - p1.X)) / D);
	m_mainCircleRadius = sqrt(pow(p1.X - m_mainCircleCenter.X, 2) + pow(p1.Y - m_mainCircleCenter.Y, 2));

	return true;
}

void CDrawCircleDlg::OnEnChangeEditPointRadius()
{
	CWnd* pWnd = GetDlgItem(IDC_EDIT_POINT_RADIUS);
	if (pWnd != nullptr && pWnd->GetSafeHwnd())
	{
		// 1. 컨트롤의 현재 값을 즉시 멤버 변수로 업데이트합니다.
		UpdateData(TRUE);

		// 2. 캔버스 영역을 다시 그리도록 요청합니다.
		InvalidateRect(m_canvasRect, FALSE);
	}
}

void CDrawCircleDlg::OnEnChangeEditMainThickness()
{
	CWnd* pWnd = GetDlgItem(IDC_EDIT_MAIN_THICKNESS);
	if (pWnd != nullptr && pWnd->GetSafeHwnd())
	{
		// 1. 컨트롤의 현재 값을 즉시 멤버 변수로 업데이트합니다.
		UpdateData(TRUE);

		// 2. 캔버스 영역을 다시 그리도록 요청합니다.
		InvalidateRect(m_canvasRect, FALSE);
	}
}

void CDrawCircleDlg::UpdateCoordinateLabels()
{
	CString str;
	if (m_nClickCount >= 1) str.Format(_T("P1: (%d, %d)"), m_points[0].x, m_points[0].y); else str = _T("P1:");
	GetDlgItem(IDC_STATIC_P1)->SetWindowText(str);

	if (m_nClickCount >= 2) str.Format(_T("P2: (%d, %d)"), m_points[1].x, m_points[1].y); else str = _T("P2:");
	GetDlgItem(IDC_STATIC_P2)->SetWindowText(str);

	if (m_nClickCount >= 3) str.Format(_T("P3: (%d, %d)"), m_points[2].x, m_points[2].y); else str = _T("P3:");
	GetDlgItem(IDC_STATIC_P3)->SetWindowText(str);
}

void CDrawCircleDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bIsDragging)
	{
		// 드래그 중인 점의 좌표를 현재 마우스 위치로 업데이트
		m_points[m_nDraggedPointIndex] = point;

		m_bCircleCalculated = CalculateCircleFromPoints();
		UpdateCoordinateLabels();
		InvalidateRect(m_canvasRect, FALSE);
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

void CDrawCircleDlg::OnBnClickedButtonReset()
{
	// TODO: Add your control notification handler code here
	ResetState();
}

void CDrawCircleDlg::OnBnClickedButtonRandomMove()
{
	// TODO: Add your control notification handler code here
	if (!m_bCircleCalculated)
	{
		MessageBox(_T("먼저 3개의 점을 찍어 원을 그려주세요."), _T("알림"));
		return;
	}

	// 이전에 실행된 스레드가 있다면 끝날 때까지 대기
	if (m_workerThread.joinable())
	{
		m_workerThread.join();
	}

	// 스레드 시작 전, 실행 플래그를 true로 설정
	m_bIsThreadRunning = true;

	// 멤버 변수에 새 스레드를 생성하여 할당
	m_workerThread = std::thread(&CDrawCircleDlg::RandomMoveThread, this);
}

void CDrawCircleDlg::RandomMoveThread()
{
	// 랜덤 숫자 생성을 위한 준비
	std::random_device rd;
	std::mt19937 gen(rd());

	// m_canvasRect에 직접 접근
	std::uniform_int_distribution<int> distX(m_canvasRect.left, m_canvasRect.right);
	std::uniform_int_distribution<int> distY(m_canvasRect.top, m_canvasRect.bottom);

	// 반복 조건에 m_bIsThreadRunning 플래그 확인 추가
	for (int i = 0; i < 10 && m_bIsThreadRunning; ++i)
	{
		// 1. 세 점의 위치를 랜덤으로 변경 (m_points에 직접 접근)
		for (int j = 0; j < 3; ++j)
		{
			m_points[j].x = distX(gen);
			m_points[j].y = distY(gen);
		}

		// 2. 원 정보 다시 계산 (멤버 함수 직접 호출)
		m_bCircleCalculated = CalculateCircleFromPoints();

		// 3. UI 업데이트 및 다시 그리기 요청
		UpdateCoordinateLabels();
		InvalidateRect(m_canvasRect, FALSE);

		// 4. 0.5초 대기 (초당 2회)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	// 스레드 작업이 끝나면 플래그를 false로 변경
	m_bIsThreadRunning = false;
}


