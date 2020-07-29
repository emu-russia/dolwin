
#pragma once

namespace Debug
{
	
	class CmdlineWindow : public CuiWindow
	{
	public:
		CmdlineWindow(RECT& rect, std::string name);
		~CmdlineWindow();

		virtual void OnDraw();
		virtual void OnKeyPress(char Ascii, int Vkey, bool shift, bool ctrl);
	};

}
