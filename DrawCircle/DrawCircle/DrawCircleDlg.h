// DrawCircleDlg.h : header file
//

#pragma once
#include <atlimage.h> // CImage ����� ���� �߰�
#include <gdiplus.h>
#include <vector>
#include <thread>
#include <atomic>


// CDrawCircleDlg dialog
class CDrawCircleDlg : public CDialogEx
{
	// ������ �� �Ҹ���
public:
	CDrawCircleDlg(CWnd* pParent = nullptr);	// ǥ�� ������
	~CDrawCircleDlg(); // GDI+ ������ ���� �Ҹ���

	// ���̾�α� ������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DRAWCIRCLE_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ����

	// ������ �޽��� �� �Լ�
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

	// GDI+ ����
	ULONG_PTR m_gdiplusToken;

	// �׸��� ���� �� ����
	CRect m_canvasRect;
	CImage m_bufferImage;

	// UI ��Ʈ��
	CString m_strPointRadius;
	CString m_strMainThickness;

	// ���� ����
	CPoint m_points[3];
	int m_nClickCount;
	bool m_bCircleCalculated;

	// ���� �� ����
	PointF m_mainCircleCenter;
	double m_mainCircleRadius;

	// �巡�� ���� ����
	bool m_bIsDragging;
	int m_nDraggedPointIndex;

	// thread ���� ���� �߰�
	std::thread m_workerThread;
	std::atomic<bool> m_bIsThreadRunning;

	// ���� ���� ó���� ���� ���� �Լ���
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