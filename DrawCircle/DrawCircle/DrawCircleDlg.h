// DrawCircleDlg.h : header file
//

#pragma once
#include <atlimage.h> // CImage 사용을 위해 추가
#include <gdiplus.h>
#include <vector>
#include <thread>
#include <atomic>


// CDrawCircleDlg dialog
class CDrawCircleDlg : public CDialogEx
{
	// 생성자 및 소멸자
public:
	CDrawCircleDlg(CWnd* pParent = nullptr);	// 표준 생성자
	~CDrawCircleDlg(); // GDI+ 정리를 위한 소멸자

	// 다이얼로그 데이터
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DRAWCIRCLE_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnEnChangeEditPointRadius();
	afx_msg void OnEnChangeEditMainThickness();
	afx_msg void OnBnClickedButtonReset();
	afx_msg void OnBnClickedButtonRandomMove();
	DECLARE_MESSAGE_MAP()

private:
	HICON m_hIcon;

	// GDI+ 관련
	ULONG_PTR m_gdiplusToken;

	// 그리기 영역 및 버퍼
	CRect m_canvasRect;
	CImage m_bufferImage;

	// UI 컨트롤
	CString m_strPointRadius;
	CString m_strMainThickness;

	// 상태 변수
	CPoint m_points[3];
	int m_nClickCount;
	bool m_bCircleCalculated;

	// 계산된 원 정보
	PointF m_mainCircleCenter;
	double m_mainCircleRadius;

	// 드래그 상태 변수
	bool m_bIsDragging;
	int m_nDraggedPointIndex;

	// thread 관리 변수 추가
	std::thread m_workerThread;
	std::atomic<bool> m_bIsThreadRunning;

	// 내부 로직 처리를 위한 헬퍼 함수들
	void ResetState();
	bool InitializeCanvas(); 
	void DrawObjects();
	void DrawCustomCircle(Graphics& graphics, const Pen& pen, float centerX, float centerY, float radius);
	void DrawCustomCircle(Graphics& graphics, const Brush& brush, float centerX, float centerY, float radius);
	std::vector<PointF> CalculateCircleVertices(float centerX, float centerY, float radius, int steps);
	bool CalculateCircleFromPoints();
	void UpdateCoordinateLabels();
	void RandomMoveThread();

};