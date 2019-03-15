#ifndef phone_detector_h__
#define phone_detector_h__

#include "common.h"
#include "thread_base.h"
namespace DroidPad
{

class CPhoneDetector : public CThreadBase
{
  public:
	CPhoneDetector();
	~CPhoneDetector();
	void showMessage(std::wstring &msg);

  protected:
	void OnExitThread() override;
	bool threadLoop() override;

  private:
	HWND m_hWnd;
};

}
#endif // phone_detector_h__
