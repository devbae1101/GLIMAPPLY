
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
	// �����带 �����ϰ� ����
	m_bIsThreadRunning = false; // �����忡�� ���� ��ȣ ������
	if (m_workerThread.joinable())
	{
		m_workerThread.join(); // �����尡 ���� ������ ���
	}

	// GDI+ ���ҽ� ����
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
	// GDI+ �ʱ�ȭ
	GdiplusStartupInput gdiplusStartupInput;
	Status status = GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	if (status != Ok)
	{
		MessageBox(_T("GDI+ ���̺귯���� ������ �� �����ϴ�.\n���α׷��� �׷��� ����� �������� ���� �� �ֽ��ϴ�."),
			_T("GDI+ �ʱ�ȭ ����"), MB_OK | MB_ICONERROR);
	}

	UpdateData(FALSE); // ���� ���� ��Ʈ�ѿ� ǥ��

	// ������ ũ���� ĵ���� ����
	CDrawCircleDlg::InitializeCanvas();

	// ��� ���� ���� �ʱ�ȭ
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

	CPaintDC dc(this); // ���� �׸��⸦ ���� DC ����

	// 1. �޸� ���ۿ� ��� ��ü�� �׸�
	DrawObjects();

	// 2. �ϼ��� ���� �̹����� ĵ���� ��ġ�� �� ���� ���� (���� ���۸�)
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
	InvalidateRect(m_canvasRect, TRUE); // ĵ���� ������ ���ΰ�ħ
}

bool CDrawCircleDlg::InitializeCanvas()
{
	// ������ ũ���� ĵ���� ����
	const int canvasWidth = 1280;
	const int canvasHeight = 960;

	// ĵ������ ���̾�α��� ��� ��ġ���� �������� ����
	const CPoint canvasTopLeft(10, 10);

	// m_canvasRect ������ ������ �簢�� ������ ����
	m_canvasRect.SetRect(canvasTopLeft.x,
		canvasTopLeft.y,
		canvasTopLeft.x + canvasWidth,
		canvasTopLeft.y + canvasHeight);

	// ������ ���� ���� �̹����� �ִٸ� ���� �ı�
	if (!m_bufferImage.IsNull())
	{
		m_bufferImage.Destroy();
	}

	// ���� ���۸��� CImage ����
	if (!m_bufferImage.Create(canvasWidth, canvasHeight, 32))
	{
		AfxMessageBox(_T("����: ����� ����(CImage)�� ������ �� �����ϴ�."));
		return false; // ���� ����
	}

	return true; // ���� ����}
}

void CDrawCircleDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// ĵ���� ���� �ۿ��� Ŭ���ߴٸ� �ƹ� �۾��� ���� ����
	if (!m_canvasRect.PtInRect(point))
	{
		CDialogEx::OnLButtonDown(nFlags, point);
		return;
	}

	//  DDX�� �ֽ� ���� CString ��� ������ ���� ������
	UpdateData(TRUE);

	// ������ ��ȿ�� �˻�
	int pointRadius = _ttoi(m_strPointRadius);
	if (m_strPointRadius.IsEmpty() || pointRadius <= 0)
	{
		MessageBox(_T("Ŭ�� ���� ���� �������� ���� �Է����ּ���."), _T("�Է� �ʿ�"), MB_OK | MB_ICONWARNING);
		GetDlgItem(IDC_EDIT_POINT_RADIUS)->SetFocus();
		return;
	}

	// ���� ���� 3�� �� ���� �ʾ��� �� -> �� �߰�
	if (m_nClickCount < 3)
	{
		// �߰��� �ٽ� ����] �� ��° ���� ��� ������ �β� ���� Ȯ��
		if (m_nClickCount == 2) // �̹� Ŭ���� �� ��° ���� �Ǵ� ���
		{
			if (m_strMainThickness.IsEmpty() || (float)_ttof(m_strMainThickness) <= 0)
			{
				MessageBox(_T("���� ���� �β��� ���� �Է����ּ���."), _T("�Է� �ʿ�"), MB_OK | MB_ICONWARNING);
				GetDlgItem(IDC_EDIT_MAIN_THICKNESS)->SetFocus();
				return; // �β� ���� ������ �� �߰� �ߴ�
			}
		}

		// ��� �˻縦 ��������� �� �߰�
		m_points[m_nClickCount++] = point;

		if (m_nClickCount == 3)
		{
			m_bCircleCalculated = CalculateCircleFromPoints();
		}

		UpdateCoordinateLabels();
		InvalidateRect(m_canvasRect, FALSE);
	}

	// ���� 3�� �� ���� �� -> �巡�� ���������� Ȯ��
	else
	{
		bool bHit = false; // ���� ���� Ŭ���ߴ��� Ȯ���ϴ� �÷���
		for (int i = 0; i < 3; i++)
		{
			CRect hitBox(m_points[i], m_points[i]);
			hitBox.InflateRect(pointRadius, pointRadius);

			if (hitBox.PtInRect(point))
			{
				// ���� ���� Ŭ������! �巡�� ���·� ��ȯ
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
		ReleaseCapture(); // ���콺 ���� ����
	}
	CDialogEx::OnLButtonUp(nFlags, point);
}

void CDrawCircleDlg::DrawObjects()
{
	UpdateData(true);

	// ���� �̹����� DC(Device Context)�� ����
	CDC* pImageDC = CDC::FromHandle(m_bufferImage.GetDC());
	{

		// GDI+ �׷��Ƚ� ��ü ����
		Graphics graphics(pImageDC->GetSafeHdc());
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		// ���� �̹����� ����� ������� �����ϰ� ����
		graphics.Clear(Gdiplus::Color(255, 255, 255, 255));


		// 1. Ŭ�� ���� �� �׸���
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

		// 2. �� ���� ������ ���� �׸���

		float mainThickness = (float)_ttof(m_strMainThickness);
		if (mainThickness > 0)
		{
			Pen mainPen(Gdiplus::Color(255, 0, 0, 0), (float)mainThickness);

			// ���̾�α� ��ǥ�� ĵ����(����) ��ǥ�� ��ȯ
			PointF relativeCenter(m_mainCircleCenter.X - m_canvasRect.left, m_mainCircleCenter.Y - m_canvasRect.top);

			DrawCustomCircle(graphics, mainPen, relativeCenter.X, relativeCenter.Y, (float)m_mainCircleRadius);
		}
	}


	// �߿�: ����� ���� DC �ڵ� �ݵ�� ����
	m_bufferImage.ReleaseDC();
}

void CDrawCircleDlg::DrawCustomCircle(Graphics& graphics, const Pen& pen, float centerX, float centerY, float radius)
{
	if (radius <= 0) return;

	// ���е� ����: ���� �� ���� ������ ǥ������ ����
	const int steps = 200; // �� �ε巯�� ���� ���� �� ���� ����
	std::vector<PointF> points = CalculateCircleVertices(centerX, centerY, radius, steps);

	graphics.DrawPolygon(&pen, points.data(), steps);
}

void CDrawCircleDlg::DrawCustomCircle(Graphics& graphics, const Brush& brush, float centerX, float centerY, float radius)
{
	if (radius <= 0) return;

	// ���е� ����: ���� �� ���� ������ ǥ������ ����
	const int steps = 100;
	std::vector<PointF> points = CalculateCircleVertices(centerX, centerY, radius, steps);

	graphics.FillPolygon(&brush, points.data(), (int)points.size());
}

std::vector<PointF> CDrawCircleDlg::CalculateCircleVertices(float centerX, float centerY, float radius, int steps)
{
	std::vector<PointF> points(steps);

	// ���� ������ ��ǥ ��� (for ����)
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
		// 1. ��Ʈ���� ���� ���� ��� ��� ������ ������Ʈ�մϴ�.
		UpdateData(TRUE);

		// 2. ĵ���� ������ �ٽ� �׸����� ��û�մϴ�.
		InvalidateRect(m_canvasRect, FALSE);
	}
}

void CDrawCircleDlg::OnEnChangeEditMainThickness()
{
	CWnd* pWnd = GetDlgItem(IDC_EDIT_MAIN_THICKNESS);
	if (pWnd != nullptr && pWnd->GetSafeHwnd())
	{
		// 1. ��Ʈ���� ���� ���� ��� ��� ������ ������Ʈ�մϴ�.
		UpdateData(TRUE);

		// 2. ĵ���� ������ �ٽ� �׸����� ��û�մϴ�.
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
		// �巡�� ���� ���� ��ǥ�� ���� ���콺 ��ġ�� ������Ʈ
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
		MessageBox(_T("���� 3���� ���� ��� ���� �׷��ּ���."), _T("�˸�"));
		return;
	}

	// ������ ����� �����尡 �ִٸ� ���� ������ ���
	if (m_workerThread.joinable())
	{
		m_workerThread.join();
	}

	// ������ ���� ��, ���� �÷��׸� true�� ����
	m_bIsThreadRunning = true;

	// ��� ������ �� �����带 �����Ͽ� �Ҵ�
	m_workerThread = std::thread(&CDrawCircleDlg::RandomMoveThread, this);
}

void CDrawCircleDlg::RandomMoveThread()
{
	// ���� ���� ������ ���� �غ�
	std::random_device rd;
	std::mt19937 gen(rd());

	// m_canvasRect�� ���� ����
	std::uniform_int_distribution<int> distX(m_canvasRect.left, m_canvasRect.right);
	std::uniform_int_distribution<int> distY(m_canvasRect.top, m_canvasRect.bottom);

	// �ݺ� ���ǿ� m_bIsThreadRunning �÷��� Ȯ�� �߰�
	for (int i = 0; i < 10 && m_bIsThreadRunning; ++i)
	{
		// 1. �� ���� ��ġ�� �������� ���� (m_points�� ���� ����)
		for (int j = 0; j < 3; ++j)
		{
			m_points[j].x = distX(gen);
			m_points[j].y = distY(gen);
		}

		// 2. �� ���� �ٽ� ��� (��� �Լ� ���� ȣ��)
		m_bCircleCalculated = CalculateCircleFromPoints();

		// 3. UI ������Ʈ �� �ٽ� �׸��� ��û
		UpdateCoordinateLabels();
		InvalidateRect(m_canvasRect, FALSE);

		// 4. 0.5�� ��� (�ʴ� 2ȸ)
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}

	// ������ �۾��� ������ �÷��׸� false�� ����
	m_bIsThreadRunning = false;
}


